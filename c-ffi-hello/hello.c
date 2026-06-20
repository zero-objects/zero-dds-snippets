#include "zerodds.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    printf("zerodds version: %s\n", zerodds_version());

    struct zerodds_ZeroDdsRuntime *rt = zerodds_runtime_create(0);
    if (!rt) return 1;

    struct zerodds_ZeroDdsWriter *w =
        zerodds_writer_create(rt, "Chatter", "RawBytes", 1 /* reliable */);
    struct zerodds_ZeroDdsReader *r =
        zerodds_reader_create(rt, "Chatter", "RawBytes", 1);

    zerodds_writer_wait_for_matched(w, 1, 5000);
    zerodds_reader_wait_for_matched(r, 1, 5000);

    const char *payload = "hello";
    zerodds_writer_write(w, (const uint8_t*)payload, strlen(payload));

    sleep(1);

    uint8_t buf[64];
    intptr_t n;
    /* Fixed-buffer convenience: copies one sample into `buf`, returns the
     * byte count. The allocate-and-free variant is `zerodds_reader_take`. */
    while ((n = zerodds_reader_take_into(r, buf, sizeof(buf))) > 0) {
        printf("got: %.*s\n", (int)n, buf);
    }

    zerodds_reader_destroy(r);
    zerodds_writer_destroy(w);
    zerodds_runtime_destroy(rt);
    return 0;
}
