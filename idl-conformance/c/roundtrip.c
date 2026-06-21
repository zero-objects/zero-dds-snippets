/*
 * IDL conformance roundtrip — C "Foundation profile" (now widened).
 *
 * Drives a REAL DCPS pub->sub loopback over a single ZeroDDS participant,
 * using a typed IDL topic generated from this example's local fixture
 * `sensor.idl` by:
 *
 *   zerodds-idlc generate sensor.idl --c -o .
 *
 * After the real-DCPS codegen fixes, the C backend now accepts a much richer
 * keyed struct than the earlier flat-primitive-only profile. This example
 * exercises the largest type the C stack supports end-to-end:
 *
 *   module conf {
 *     enum Status { OFFLINE, NOMINAL, DEGRADED, FAULT };
 *     typedef double  Celsius;
 *     typedef Celsius TemperatureType;            // alias chain
 *     struct GeoPoint { double lat; double lon; };
 *     struct SensorReading {
 *       @key long        sensor_id;
 *       @key string<16>  site;
 *       Status           status;                  // enum member
 *       TemperatureType  temperature;             // typedef-to-double
 *       GeoPoint         location;                // nested struct
 *       long             samples[4];              // fixed array
 *       @optional double calibration;             // @optional primitive
 *       @optional string note;                    // @optional string
 *     };
 *   };
 *
 * IDL features exercised end-to-end (encode -> wire -> decode, asserted):
 *   - flat-but-rich keyed struct as a DCPS topic type
 *   - @key members (composite key: long sensor_id + bounded string<16> site)
 *   - enum member (conf::Status, encoded as int32)
 *   - typedef-to-primitive + alias chain (TemperatureType -> Celsius -> double)
 *   - nested struct member (conf::GeoPoint)
 *   - fixed-size array member (long samples[4])
 *   - @optional members WITH presence flags (calibration present, note present)
 *   - bounded string member, long (int32) and double primitive members
 *   - module nesting (module conf { ... })
 *   - XCDR2 DHEADER framing + per-type XTypes TypeObjects (emitted in header)
 *
 * We publish one fully-populated sample (including both @optional members set)
 * and take it back, asserting every field — and the optional presence flags —
 * survived the wire.
 *
 * NOTE on includes: the generated header `sensor.h` now pulls in ONLY
 * `zerodds_xcdr2.h` (the earlier self-conflicting `#include "zerodds.h"` was
 * removed by the codegen fix), so no include-guard workaround is needed. We
 * still hand-forward-declare the handful of DCPS entity functions we call,
 * since `sensor.h` deliberately carries only the typed codec surface.
 */
#include "sensor.h"   /* generated: conf_SensorReading_t + typesupport */
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
int zerodds_writer_write_typed(struct zerodds_ZeroDdsWriter *writer,
                               const zerodds_typesupport_t *ts,
                               const void *sample);
int zerodds_reader_take_typed(struct zerodds_ZeroDdsReader *reader,
                              const zerodds_typesupport_t *ts,
                              void *out_sample, void *out_info);
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

    const char *topic = "ConfSensorReading";
    const char *tname = conf_SensorReading_type_name; /* "conf::SensorReading" */

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

    /* Fully-populated sample — every member set, both @optionals present. */
    conf_SensorReading_t out_sample;
    memset(&out_sample, 0, sizeof out_sample);
    out_sample.sensor_id          = 4242;
    out_sample.site               = strdup("EU-CENTRAL-1");  /* fits string<16> */
    out_sample.status             = conf_Status_DEGRADED;    /* enum */
    out_sample.temperature        = 36.625;                  /* typedef->double */
    out_sample.location.lat       = 50.110924;               /* nested struct */
    out_sample.location.lon       = 8.682127;
    out_sample.samples[0]         = 11;                       /* fixed array */
    out_sample.samples[1]         = 22;
    out_sample.samples[2]         = 33;
    out_sample.samples[3]         = 44;
    out_sample.calibration        = 0.998001;                /* @optional set */
    out_sample.calibration_present = 1;
    out_sample.note               = strdup("recalibrate");   /* @optional set */
    out_sample.note_present       = 1;

    if (zerodds_writer_write_typed(w, &conf_SensorReading_typesupport, &out_sample) != 0)
        return fail("write_typed");

    printf("published: id=%d site=\"%s\" status=%d temp=%.3f loc=(%.6f,%.6f) "
           "samples=[%d,%d,%d,%d] calib=%.6f(present=%d) note=\"%s\"(present=%d)\n",
           out_sample.sensor_id, out_sample.site, (int)out_sample.status,
           out_sample.temperature, out_sample.location.lat, out_sample.location.lon,
           out_sample.samples[0], out_sample.samples[1], out_sample.samples[2],
           out_sample.samples[3], out_sample.calibration, out_sample.calibration_present,
           out_sample.note, out_sample.note_present);

    /* Poll the reader for the sample. Bounded retry, short window. */
    conf_SensorReading_t in_sample;
    memset(&in_sample, 0, sizeof in_sample);
    int got = -1;
    for (int i = 0; i < 200; ++i) {
        got = zerodds_reader_take_typed(r, &conf_SensorReading_typesupport, &in_sample, NULL);
        if (got == 0) break;
        usleep(5000); /* 5ms */
    }
    if (got != 0) {
        free(out_sample.site);
        free(out_sample.note);
        return fail("take_typed: no sample recovered");
    }

    printf("recovered: id=%d site=\"%s\" status=%d temp=%.3f loc=(%.6f,%.6f) "
           "samples=[%d,%d,%d,%d] calib=%.6f(present=%d) note=\"%s\"(present=%d)\n",
           in_sample.sensor_id, in_sample.site ? in_sample.site : "(null)",
           (int)in_sample.status, in_sample.temperature,
           in_sample.location.lat, in_sample.location.lon,
           in_sample.samples[0], in_sample.samples[1], in_sample.samples[2],
           in_sample.samples[3], in_sample.calibration, in_sample.calibration_present,
           in_sample.note ? in_sample.note : "(null)", in_sample.note_present);

    /* Assert the wire preserved every field. */
    int ok = 1;
#define CHECK(cond, what) do { if (!(cond)) { ok = 0; fprintf(stderr, "MISMATCH %s\n", what); } } while (0)
    CHECK(in_sample.sensor_id == out_sample.sensor_id, "sensor_id (key/long)");
    CHECK(in_sample.site && strcmp(in_sample.site, out_sample.site) == 0,
          "site (key/bounded-string)");
    CHECK(in_sample.status == out_sample.status, "status (enum)");
    CHECK(in_sample.temperature == out_sample.temperature, "temperature (typedef->double)");
    CHECK(in_sample.location.lat == out_sample.location.lat, "location.lat (nested)");
    CHECK(in_sample.location.lon == out_sample.location.lon, "location.lon (nested)");
    CHECK(in_sample.samples[0] == out_sample.samples[0], "samples[0] (fixed array)");
    CHECK(in_sample.samples[1] == out_sample.samples[1], "samples[1] (fixed array)");
    CHECK(in_sample.samples[2] == out_sample.samples[2], "samples[2] (fixed array)");
    CHECK(in_sample.samples[3] == out_sample.samples[3], "samples[3] (fixed array)");
    CHECK(in_sample.calibration_present == out_sample.calibration_present,
          "calibration_present (@optional flag)");
    CHECK(in_sample.calibration == out_sample.calibration, "calibration (@optional value)");
    CHECK(in_sample.note_present == out_sample.note_present, "note_present (@optional flag)");
    CHECK(in_sample.note && out_sample.note &&
          strcmp(in_sample.note, out_sample.note) == 0, "note (@optional value)");
#undef CHECK

    /* Free heap fields owned by the decoded sample + our published one. */
    conf_SensorReading_sample_free(&in_sample);
    free(out_sample.site);
    free(out_sample.note);

    zerodds_reader_destroy(r);
    zerodds_writer_destroy(w);
    zerodds_runtime_destroy(rt);

    if (ok) {
        printf("ROUNDTRIP OK: recovered sample equals published sample "
               "(enum + typedef-double + nested struct + fixed array + "
               "@optional + composite @key all survived the wire)\n");
        return 0;
    }
    return fail("roundtrip mismatch");
}
