/* RTI Connext DDS TypeObject / TypeIdentifier extractor.
 *
 * For each canonical type it takes the generated DDS_TypeCode, builds the
 * XTypes-1.3 V2 TypeObject via DDS_TypeObjectV2_create_from_typecode (which
 * also returns the 14-byte complete + minimal EquivalenceHashes), serializes
 * the V2 TypeObject and writes:
 *   <type>.complete.bin / <type>.complete.hash
 *   <type>.minimal.hash   (RTI's create_from_typecode yields the COMPLETE
 *                          serialized TypeObject; the minimal serialized bytes
 *                          are not separately exposed by this entry point, so
 *                          only the minimal HASH is recorded here.)
 *
 * Build (codepit):
 *   export NDDSHOME=/opt/rti.com/rti_connext_dds-7.7.0
 *   gcc -DRTI_UNIX -DRTI_LINUX -DRTI_64BIT rti_extract.c \
 *       rti_out/typeobject.c rti_out/typeobjectPlugin.c rti_out/typeobjectSupport.c \
 *       -I rti_out -I $NDDSHOME/include -I $NDDSHOME/include/ndds \
 *       -L $NDDSHOME/lib/x64Linux4gcc7.3.0 \
 *       -lnddsc -lnddscore -ldl -lm -lpthread -lrt -o rti_extract
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ndds/ndds_c.h"
#include "ndds/dds_c/dds_c_typeobject.h"
#include "typeobject.h"

static char g_dir[512];

static void write_file(const char* name, const void* data, unsigned int n)
{
    char path[700];
    snprintf(path, sizeof(path), "%s/%s", g_dir, name);
    FILE* f = fopen(path, "wb");
    if (f) { fwrite(data, 1, n, f); fclose(f); }
}

static void write_hash(const char* name, const DDS_TypeObjectV2EquivalenceHash* h)
{
    char hx[29];
    static const char* X = "0123456789abcdef";
    for (int i = 0; i < 14; ++i) {
        unsigned char b = (*h)[i];
        hx[i*2]   = X[b >> 4];
        hx[i*2+1] = X[b & 0xf];
    }
    hx[28] = 0;
    write_file(name, hx, 28);
}

static void dump(const char* case_name, DDS_TypeCode* tc)
{
    if (!tc) { fprintf(stderr, "%s: NULL typecode\n", case_name); return; }

    DDS_TypeObjectV2EquivalenceHash completeHash, minimalHash;
    struct DDS_TypeObjectV2* to =
        DDS_TypeObjectV2_create_from_typecode(tc, &completeHash, &minimalHash);
    if (!to) { fprintf(stderr, "%s: create_from_typecode FAILED\n", case_name); return; }

    char* buf = NULL;
    unsigned int sz = 0;
    if (DDS_TypeObjectV2_serialize(to, &buf, &sz) != DDS_RETCODE_OK) {
        fprintf(stderr, "%s: serialize FAILED\n", case_name);
        DDS_TypeObjectV2_delete(to);
        return;
    }

    char cbin[64], chash[64], mhash[64];
    snprintf(cbin,  sizeof(cbin),  "%s.complete.bin",  case_name);
    snprintf(chash, sizeof(chash), "%s.complete.hash", case_name);
    snprintf(mhash, sizeof(mhash), "%s.minimal.hash",  case_name);
    write_file(cbin, buf, sz);
    write_hash(chash, &completeHash);
    write_hash(mhash, &minimalHash);

    static const char* X = "0123456789abcdef";
    char hx[29];
    for (int i = 0; i < 14; ++i) { unsigned char b=completeHash[i]; hx[i*2]=X[b>>4]; hx[i*2+1]=X[b&0xf]; }
    hx[28]=0;
    char mh[29];
    for (int i = 0; i < 14; ++i) { unsigned char b=minimalHash[i]; mh[i*2]=X[b>>4]; mh[i*2+1]=X[b&0xf]; }
    mh[28]=0;
    printf("%-14s COMPLETE ser_len=%u cmp_hash=%s min_hash=%s\n",
           case_name, sz, hx, mh);

    DDS_TypeObjectV2_delete(to);
}

int main(int argc, char** argv)
{
    strncpy(g_dir, (argc > 1) ? argv[1] : ".", sizeof(g_dir) - 1);

    /* Initialize the RTI runtime (allocators, type-object factory) before
     * touching the TypeObject V2 API; create_from_typecode segfaults if the
     * factory singleton has not been created. */
    DDS_DomainParticipantFactory* f = DDS_DomainParticipantFactory_get_instance();
    if (!f) { fprintf(stderr, "factory init FAILED\n"); return 1; }

    dump("plain",           to_Plain_get_typecode());
    dump("appendable",      to_Appendable_get_typecode());
    dump("mutable",         to_Mutable_get_typecode());
    dump("color_enum",      to_Color_get_typecode());
    dump("long_seq4_alias", to_LongSeq4_get_typecode());
    dump("inner",           to_Inner_get_typecode());
    dump("nested",          to_Nested_get_typecode());
    dump("choice_union",    to_Choice_get_typecode());
    return 0;
}
