/*
 * IDL conformance roundtrip — C Foundation profile.
 *
 * Drives a REAL DCPS pub->sub loopback over a single ZeroDDS participant,
 * using a typed IDL topic generated from the conformance fixture
 * tools/idlc/tests/conformance/fixtures/10_keys.idl by:
 *
 *   zerodds-idlc generate 10_keys.idl --c -o .
 *
 * The C backend is the narrow "Foundation scope": it only accepts flat
 * structs of directly-encodable members (primitives + bounded string), so
 * this example exercises the largest type that backend supports:
 *
 *   struct conf::Keyed { @key long id; @key string<16> region; double value; }
 *
 * IDL features exercised end-to-end (encode -> wire -> decode, asserted):
 *   - flat struct
 *   - @key members (composite key: long + bounded string)
 *   - bounded string member (string<16>)
 *   - long (int32) and double primitive members
 *   - XCDR2 DHEADER framing + per-type XTypes TypeObject (emitted in header)
 *
 * We publish one fully-populated sample and take it back, asserting every
 * field survived the wire.
 */
/*
 * NOTE on includes (header-packaging workaround, see README "Known
 * limitations"): the generated header `10_keys.h` pulls in BOTH the full
 * cbindgen `zerodds.h` AND the hand-written `zerodds_xcdr2.h`, and those two
 * headers re-declare the typed FFI prototypes (zerodds_writer_write_typed,
 * zerodds_reader_take_typed, zerodds_topic_create_typed, zerodds_xcdr2_*)
 * with structurally-identical-but-differently-named struct pointers
 * (`zerodds_typesupport_t*` vs `struct zerodds_ZeroDdsTypeSupport*` and
 * `struct ZeroDdsTopic*` vs `struct zerodds_ZeroDdsTopic*`), which C treats as
 * a conflicting redeclaration. We suppress the duplicate by pre-defining the
 * `zerodds.h` include guard (so only `zerodds_xcdr2.h` provides the typed
 * prototypes + inline XCDR2 helpers the generated codec needs) and then
 * forward-declare the handful of DCPS entity functions we use ourselves.
 */
#define ZERODDS_H  /* suppress generated header's #include "zerodds.h" */
#include "10_keys.h"   /* generated: conf_Keyed_t + conf_Keyed_typesupport */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* DCPS entity functions we need, declared to match zerodds.h exactly. */
struct zerodds_ZeroDdsRuntime;
struct zerodds_ZeroDdsWriter;
struct zerodds_ZeroDdsReader;
const char *zerodds_version(void);
struct zerodds_ZeroDdsRuntime *zerodds_runtime_create(uint32_t domain_id);
void zerodds_runtime_destroy(struct zerodds_ZeroDdsRuntime *runtime);
struct zerodds_ZeroDdsWriter *zerodds_writer_create_kind(
    struct zerodds_ZeroDdsRuntime *runtime, const char *topic_name,
    const char *type_name, int reliable, int is_keyed);
struct zerodds_ZeroDdsReader *zerodds_reader_create_kind(
    struct zerodds_ZeroDdsRuntime *runtime, const char *topic_name,
    const char *type_name, int reliable, int is_keyed);
int zerodds_writer_wait_for_matched(struct zerodds_ZeroDdsWriter *writer,
                                    int min_count, uint64_t timeout_ms);
int zerodds_reader_wait_for_matched(struct zerodds_ZeroDdsReader *reader,
                                    int min_count, uint64_t timeout_ms);
void zerodds_writer_destroy(struct zerodds_ZeroDdsWriter *writer);
void zerodds_reader_destroy(struct zerodds_ZeroDdsReader *reader);

static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

int main(void) {
    printf("zerodds version: %s\n", zerodds_version());

    struct zerodds_ZeroDdsRuntime *rt = zerodds_runtime_create(0);
    if (!rt) return fail("runtime_create");

    const char *topic = "ConfKeyed";
    const char *tname = conf_Keyed_type_name; /* "conf::Keyed" */

    /* Keyed writer/reader (entityKind WithKey) — the type carries @key. */
    struct zerodds_ZeroDdsWriter *w =
        zerodds_writer_create_kind(rt, topic, tname, 1 /*reliable*/, 1 /*keyed*/);
    struct zerodds_ZeroDdsReader *r =
        zerodds_reader_create_kind(rt, topic, tname, 1, 1);
    if (!w || !r) return fail("writer/reader create");

    if (zerodds_writer_wait_for_matched(w, 1, 5000) != 0)
        return fail("writer never matched");
    if (zerodds_reader_wait_for_matched(r, 1, 5000) != 0)
        return fail("reader never matched");

    /* Fully-populated sample. region is a heap-owned char* (bounded string<16>). */
    conf_Keyed_t out_sample;
    out_sample.id = 4242;
    out_sample.region = strdup("EU-CENTRAL-1");  /* 12 chars, fits string<16> */
    out_sample.value = 3.14159265358979;

    if (zerodds_writer_write_typed(w, &conf_Keyed_typesupport, &out_sample) != 0)
        return fail("write_typed");

    printf("published: id=%d region=\"%s\" value=%.14f\n",
           out_sample.id, out_sample.region, out_sample.value);

    /* Poll the reader for the sample (event would be nicer, but the runtime
     * reader take is the simple typed path). Bounded retry, short window. */
    conf_Keyed_t in_sample;
    memset(&in_sample, 0, sizeof in_sample);
    int got = -1;
    for (int i = 0; i < 200; ++i) {
        got = zerodds_reader_take_typed(r, &conf_Keyed_typesupport, &in_sample, NULL);
        if (got == 0) break;
        usleep(5000); /* 5ms */
    }
    if (got != 0) {
        free(out_sample.region);
        return fail("take_typed: no sample recovered");
    }

    printf("recovered: id=%d region=\"%s\" value=%.14f\n",
           in_sample.id, in_sample.region ? in_sample.region : "(null)",
           in_sample.value);

    /* Assert the wire preserved every field. */
    int ok = 1;
    if (in_sample.id != out_sample.id) { ok = 0; fprintf(stderr, "MISMATCH id\n"); }
    if (!in_sample.region ||
        strcmp(in_sample.region, out_sample.region) != 0) {
        ok = 0; fprintf(stderr, "MISMATCH region\n");
    }
    if (in_sample.value != out_sample.value) { ok = 0; fprintf(stderr, "MISMATCH value\n"); }

    /* Free heap fields owned by the decoded sample + our published one. */
    conf_Keyed_sample_free(&in_sample);
    free(out_sample.region);

    zerodds_reader_destroy(r);
    zerodds_writer_destroy(w);
    zerodds_runtime_destroy(rt);

    if (ok) {
        printf("ROUNDTRIP OK: recovered sample equals published sample\n");
        return 0;
    }
    return fail("roundtrip mismatch");
}
