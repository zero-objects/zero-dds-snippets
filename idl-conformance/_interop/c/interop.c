/*
 * XCDR2 wire-interop driver for the C PSM — combo::Telemetry.
 *
 * Two modes:
 *   ENCODE          -> serialise the canonical sample to raw XCDR2-LE bytes,
 *                      write them to the golden path.
 *   DECODE <file>   -> read raw bytes, decode, assert EVERY field == canonical;
 *                      exit 0 on full match, nonzero (with a diff) otherwise.
 *
 * The bytes ARE the wire: we use the generated combo_Telemetry_encode/_decode
 * (XCDR2 DHEADER framing, the ZeroDDS default LE codec) directly — no live
 * pub/sub. Identical canonical values across every language => identical bytes.
 */
#include "combo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GOLDEN_PATH \
    "/path/to/zerodds/zerodds-examples/idl-conformance/_interop/goldens/c.bin"

/* Build the canonical combo::Telemetry sample — EXACT values, shared across
 * all language PSMs so the encoded bytes must match byte-for-byte. */
static void build_canonical(combo_Telemetry_t* s,
                            combo_Sample_t hist[3],
                            char* counter_keys[2], int32_t counter_vals[2]) {
    memset(s, 0, sizeof(*s));

    s->unitId         = 4242;                       /* @key long            */
    s->region         = "eu-central-1";             /* @key string<32>      */
    s->mode           = combo_Mode_MODE_ACTIVE;     /* enum idx 1           */
    s->batteryCurrent = 13.75;                      /* typedef double       */

    hist[0].seq = 1; hist[0].value = 0.5;
    hist[1].seq = 2; hist[1].value = 1.5;
    hist[2].seq = 3; hist[2].value = -2.25;
    s->history.len   = 3;
    s->history.elems = hist;

    s->reading._d              = combo_Mode_MODE_ACTIVE;   /* -> activeRate */
    s->reading._u.activeRate   = 60.0;

    /* map key-sorted: "drops" < "packets" */
    counter_keys[0] = "drops";   counter_vals[0] = 3;
    counter_keys[1] = "packets"; counter_vals[1] = 100;
    s->counters.len  = 2;
    s->counters.keys = counter_keys;
    s->counters.vals = counter_vals;

    s->calibration         = 0.001;                 /* @optional, PRESENT   */
    s->calibration_present = 1;

    s->window[0] = 10; s->window[1] = 20;
    s->window[2] = 30; s->window[3] = 40;
}

static int do_encode(const char* path) {
    combo_Telemetry_t s;
    combo_Sample_t hist[3];
    char* ckeys[2]; int32_t cvals[2];
    build_canonical(&s, hist, ckeys, cvals);

    /* size pass */
    size_t need = 0;
    if (combo_Telemetry_encode(&s, NULL, 0, &need) != -13) {
        /* -13 = "buffer too small / NULL", but out_len is set: that's the
         * sizing contract of the generated encoder. */
    }
    if (need == 0) { fprintf(stderr, "ENCODE: size pass produced 0 bytes\n"); return 1; }

    uint8_t* buf = (uint8_t*)malloc(need);
    if (!buf) { fprintf(stderr, "ENCODE: OOM\n"); return 1; }
    size_t got = 0;
    int rc = combo_Telemetry_encode(&s, buf, need, &got);
    if (rc != 0 || got != need) {
        fprintf(stderr, "ENCODE: encode failed rc=%d got=%zu need=%zu\n", rc, got, need);
        free(buf); return 1;
    }

    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "ENCODE: cannot open %s\n", path); free(buf); return 1; }
    size_t wrote = fwrite(buf, 1, got, f);
    fclose(f);
    free(buf);
    if (wrote != got) { fprintf(stderr, "ENCODE: short write\n"); return 1; }

    printf("ENCODE OK: wrote %zu bytes -> %s\n", got, path);
    return 0;
}

static int do_decode(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "DECODE: cannot open %s\n", path); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fprintf(stderr, "DECODE: empty file\n"); fclose(f); return 1; }
    uint8_t* buf = (uint8_t*)malloc((size_t)sz);
    if (!buf) { fclose(f); return 1; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "DECODE: short read\n"); free(buf); fclose(f); return 1;
    }
    fclose(f);

    combo_Telemetry_t in;
    memset(&in, 0, sizeof(in));
    int rc = combo_Telemetry_decode(buf, (size_t)sz, &in);
    free(buf);
    if (rc != 0) { fprintf(stderr, "DECODE: decode failed rc=%d\n", rc); return 1; }

    /* canonical reference */
    combo_Telemetry_t ref;
    combo_Sample_t rhist[3];
    char* rkeys[2]; int32_t rvals[2];
    build_canonical(&ref, rhist, rkeys, rvals);

    int ok = 1;
#define CHECK(cond, what) do { if (!(cond)) { ok = 0; fprintf(stderr, "MISMATCH %s\n", what); } } while (0)
    CHECK(in.unitId == ref.unitId, "unitId (@key long)");
    CHECK(in.region && strcmp(in.region, ref.region) == 0, "region (@key string<32>)");
    CHECK(in.mode == ref.mode, "mode (enum)");
    CHECK(in.batteryCurrent == ref.batteryCurrent, "batteryCurrent (typedef double)");
    CHECK(in.history.len == ref.history.len, "history.len (sequence)");
    if (in.history.len == ref.history.len) {
        for (uint32_t i = 0; i < ref.history.len; ++i) {
            char w[48];
            snprintf(w, sizeof w, "history[%u].seq", i);
            CHECK(in.history.elems[i].seq == ref.history.elems[i].seq, w);
            snprintf(w, sizeof w, "history[%u].value", i);
            CHECK(in.history.elems[i].value == ref.history.elems[i].value, w);
        }
    }
    CHECK(in.reading._d == ref.reading._d, "reading._d (union discriminator)");
    CHECK(in.reading._u.activeRate == ref.reading._u.activeRate, "reading.activeRate (union double)");
    CHECK(in.counters.len == ref.counters.len, "counters.len (map)");
    if (in.counters.len == ref.counters.len) {
        for (uint32_t i = 0; i < ref.counters.len; ++i) {
            char w[48];
            snprintf(w, sizeof w, "counters[%u].key", i);
            CHECK(in.counters.keys[i] && strcmp(in.counters.keys[i], ref.counters.keys[i]) == 0, w);
            snprintf(w, sizeof w, "counters[%u].val", i);
            CHECK(in.counters.vals[i] == ref.counters.vals[i], w);
        }
    }
    CHECK(in.calibration_present == ref.calibration_present, "calibration_present (@optional flag)");
    CHECK(in.calibration == ref.calibration, "calibration (@optional double)");
    for (int i = 0; i < 4; ++i) {
        char w[32];
        snprintf(w, sizeof w, "window[%d]", i);
        CHECK(in.window[i] == ref.window[i], w);
    }
#undef CHECK

    combo_Telemetry_sample_free(&in);

    if (ok) {
        printf("DECODE OK: every field == canonical "
               "(key long+string, enum, typedef-double, sequence<struct>, "
               "union, map, @optional, long[4] all survived the wire)\n");
        return 0;
    }
    fprintf(stderr, "DECODE FAIL: field mismatch\n");
    return 1;
}

int main(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "ENCODE") == 0) {
        return do_encode(argc >= 3 ? argv[2] : GOLDEN_PATH);
    }
    if (argc >= 2 && strcmp(argv[1], "DECODE") == 0) {
        if (argc < 3) { fprintf(stderr, "usage: %s DECODE <file>\n", argv[0]); return 2; }
        return do_decode(argv[2]);
    }
    fprintf(stderr, "usage: %s ENCODE [out] | DECODE <file>\n", argv[0]);
    return 2;
}
