/* Native CycloneDDS XCDR2-LE serializer for the wave-2 type-system corpus.
 * Uses the public dds_cdrstream API to emit the bare XCDR2 payload (no RTPS
 * encapsulation header) for each canonical sample. */
#include "typesystem.h"
#include "dds/cdr/dds_cdrstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct dds_cdrstream_allocator alloc = { malloc, realloc, free };

static void emit(const char *name, const dds_topic_descriptor_t *td, const void *sample)
{
    struct dds_cdrstream_desc desc;
    memset(&desc, 0, sizeof(desc));
    dds_cdrstream_desc_from_topic_desc(&desc, td);

    dds_ostreamLE_t os;
    dds_ostreamLE_init(&os, &alloc, 0, DDSI_RTPS_CDR_ENC_VERSION_2);
    bool ok = dds_stream_write_sampleLE(&os, &alloc, sample, &desc);
    if (!ok) { fprintf(stderr, "%s: write failed\n", name); }

    uint32_t len = os.x.m_index;
    char path[256];
    snprintf(path, sizeof(path), "/tmp/oracle-typesys/out/%s.cyclone.bin", name);
    FILE *f = fopen(path, "wb");
    fwrite(os.x.m_buffer, 1, len, f);
    fclose(f);

    printf("%-20s %u bytes:", name, len);
    for (uint32_t i = 0; i < len; i++) printf(" %02x", os.x.m_buffer[i]);
    printf("\n");

    dds_ostreamLE_fini(&os, &alloc);
    dds_cdrstream_desc_fini(&desc, &alloc);
}

int main(void)
{
    /* enum_holder */
    {
        ts_EnumHolder h = { ts_BLUE, ts_T_C, ts_MID_D };
        emit("enum_holder", &ts_EnumHolder_desc, &h);
    }
    /* union_long_default: note="hi" -> discriminator must NOT be 1 or 2 */
    {
        ts_LongUnion u;
        memset(&u, 0, sizeof(u));
        u._d = 0;                 /* a value that selects the default arm */
        u._u.note = "hi";
        emit("union_long_default", &ts_LongUnion_desc, &u);
    }
    /* union_long_struct: where={7,8}, case 2 */
    {
        ts_LongUnion u;
        memset(&u, 0, sizeof(u));
        u._d = 2;
        u._u.where.x = 7;
        u._u.where.y = 8;
        emit("union_long_struct", &ts_LongUnion_desc, &u);
    }
    /* union_enum_default: fallback=42, default arm (disc BLUE) */
    {
        ts_EnumUnion u;
        memset(&u, 0, sizeof(u));
        u._d = ts_BLUE;
        u._u.fallback = 42;
        emit("union_enum_default", &ts_EnumUnion_desc, &u);
    }
    /* opt_holder: id=1, maybe_num=77, maybe_str=NULL, maybe_obj={5,6} */
    {
        int32_t num = 77;
        ts_Inner inner = { 5, 6 };
        ts_OptHolder h;
        memset(&h, 0, sizeof(h));
        h.id = 1;
        h.maybe_num = &num;
        h.maybe_str = NULL;
        h.maybe_obj = &inner;
        emit("opt_holder", &ts_OptHolder_desc, &h);
    }
    /* mut_outer: tag=9, leaf={100,1.25}, list=[{1,0.5},{2,0.25}] */
    {
        ts_MutLeaf list_buf[2] = { { 1, 0.5 }, { 2, 0.25 } };
        ts_MutOuter o;
        memset(&o, 0, sizeof(o));
        o.tag = 9;
        o.leaf.u = 100;
        o.leaf.v = 1.25;
        o.list._maximum = 2;
        o.list._length = 2;
        o.list._buffer = list_buf;
        o.list._release = false;
        emit("mut_outer", &ts_MutOuter_desc, &o);
    }
    return 0;
}
