/* SPDX-License-Identifier: Apache-2.0
 * idl-conformance / _interop / c — the combo::Telemetry column.
 *
 *   interop ENCODE <out.bin>   write the canonical sample as raw XCDR2-LE
 *   interop DECODE <in.bin>    decode, assert every field == canonical
 *
 * gen_combo/combo.h is `zerodds-idlc generate --lang c` output of
 * ../combo.idl over runtime/c; values are CANONICAL.md.
 */
#include "gen_combo/combo.h"
#include <stdio.h>
#include <string.h>

#define SAMPLE_CAPACITY 4096u

static void canonical(combo_Telemetry *s, combo_Sample history[3],
                      zd_map_zd_string_long_entry counters[2]) {
    memset(s, 0, sizeof(*s));
    s->unitId = 4242;
    s->region = "eu-central-1";
    s->mode = combo_MODE_ACTIVE;
    s->batteryCurrent = 13.75;
    history[0] = (combo_Sample){1, 0.5};
    history[1] = (combo_Sample){2, 1.5};
    history[2] = (combo_Sample){3, -2.25};
    s->history._length = 3;
    s->history._buffer = history;
    s->reading._d = combo_MODE_ACTIVE;
    s->reading._u.activeRate = 60.0;
    counters[0] = (zd_map_zd_string_long_entry){"drops", 3};
    counters[1] = (zd_map_zd_string_long_entry){"packets", 100};
    s->counters._length = 2;
    s->counters._buffer = counters;
    s->_has_calibration = true;
    s->calibration = 0.001;
    for (int i = 0; i < 4; ++i) s->window[i] = (i + 1) * 10;
}

static int encode(const char *path) {
    combo_Telemetry sample;
    combo_Sample history[3];
    zd_map_zd_string_long_entry counters[2];
    canonical(&sample, history, counters);
    uint8_t bytes[SAMPLE_CAPACITY];
    zdw_writer writer;
    zdw_writer_init(&writer, bytes, sizeof bytes, ZD_ENDIANNESS, 4);
    zd_enc_combo_Telemetry(&writer, &sample);
    if (zdw_writer_status(&writer) != ZDW_OK) {
        fprintf(stderr, "ENCODE: %s\n", zdw_status_name(zdw_writer_status(&writer)));
        return 1;
    }
    const size_t length = zdw_writer_length(&writer);
    FILE *file = fopen(path, "wb");
    if (!file) { perror("ENCODE open"); return 1; }
    const size_t written = fwrite(bytes, 1, length, file);
    const int closed = fclose(file);
    if (written != length || closed != 0) {
        fprintf(stderr, "ENCODE: incomplete output\n");
        return 1;
    }
    printf("ENCODE OK: %zu bytes -> %s\n", length, path);
    return 0;
}

static int decode(const char *path) {
    /* This driver accepts one canonical sample, not unbounded input files. */
    uint8_t bytes[SAMPLE_CAPACITY];
    FILE *file = fopen(path, "rb");
    if (!file) { perror("DECODE open"); return 1; }
    const size_t length = fread(bytes, 1, sizeof bytes, file);
    const int extra = fgetc(file);
    const int read_error = ferror(file);
    const int closed = fclose(file);
    if (length == 0 || extra != EOF || read_error || closed != 0) {
        fprintf(stderr, "DECODE: empty, oversized or unreadable input\n");
        return 1;
    }
    /* Variable-length values borrow this aligned arena until checks finish. */
    _Alignas(16) uint8_t arena[SAMPLE_CAPACITY];
    zdw_reader reader;
    zdw_reader_init(&reader, bytes, length, ZD_ENDIANNESS, 4);
    zdw_reader_set_heap(&reader, arena, sizeof arena);
    combo_Telemetry got = {0};
    zd_dec_combo_Telemetry(&reader, &got, ZDW_TC_DISCARD);
    if (zdw_reader_status(&reader) != ZDW_OK || zdw_reader_position(&reader) != length) {
        fprintf(stderr, "DECODE: %s, consumed %zu/%zu bytes\n",
                zdw_status_name(zdw_reader_status(&reader)),
                zdw_reader_position(&reader), length);
        return 1;
    }
    combo_Telemetry expected;
    combo_Sample history[3];
    zd_map_zd_string_long_entry counters[2];
    canonical(&expected, history, counters);
    int ok = 1;
#define CHECK(test, label) do { if (!(test)) { fprintf(stderr, "MISMATCH %s\n", label); ok = 0; } } while (0)
    CHECK(got.unitId == expected.unitId, "unitId");
    CHECK(got.region && strcmp(got.region, expected.region) == 0, "region");
    CHECK(got.mode == expected.mode, "mode");
    CHECK(got.batteryCurrent == expected.batteryCurrent, "batteryCurrent");
    CHECK(got.history._length == expected.history._length, "history.length");
    if (got.history._length == expected.history._length) {
        for (uint32_t i = 0; i < expected.history._length; ++i) {
            CHECK(got.history._buffer[i].seq == history[i].seq, "history.seq");
            CHECK(got.history._buffer[i].value == history[i].value, "history.value");
        }
    }
    CHECK(got.reading._d == expected.reading._d, "reading.discriminator");
    if (got.reading._d == expected.reading._d)
        CHECK(got.reading._u.activeRate == expected.reading._u.activeRate, "reading.activeRate");
    CHECK(got.counters._length == expected.counters._length, "counters.length");
    if (got.counters._length == expected.counters._length) {
        for (uint32_t i = 0; i < expected.counters._length; ++i) {
            const zd_map_zd_string_long_entry entry = got.counters._buffer[i];
            CHECK(entry.key && strcmp(entry.key, counters[i].key) == 0, "counters.key");
            CHECK(entry.value == counters[i].value, "counters.value");
        }
    }
    CHECK(got._has_calibration == expected._has_calibration, "calibration.present");
    CHECK(got.calibration == expected.calibration, "calibration.value");
    for (int i = 0; i < 4; ++i) CHECK(got.window[i] == expected.window[i], "window");
#undef CHECK
    if (!ok) return 1;
    printf("DECODE OK: %zu bytes, every field equals canonical\n", length);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "ENCODE") == 0) return encode(argv[2]);
    if (argc == 3 && strcmp(argv[1], "DECODE") == 0) return decode(argv[2]);
    fprintf(stderr, "usage: %s ENCODE <output> | DECODE <input>\n", argv[0]);
    return 2;
}
