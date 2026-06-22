#include "dds/dds.h"
#include "dds/cdr/dds_cdrstream.h"
#include "dds/ddsrt/md5.h"
#include "keyhash_full.h"
#include <stdio.h>
#include <string.h>

static void keyhash_of(const char *label, const dds_topic_descriptor_t *td, const void *sample)
{
  struct dds_cdrstream_desc desc;
  memset(&desc, 0, sizeof desc);
  dds_cdrstream_desc_from_topic_desc(&desc, td);

  /* 1. serialize full sample, native (LE) XCDR2 */
  dds_ostream_t os; dds_ostream_init(&os, &dds_cdrstream_default_allocator, 0, DDSI_RTPS_CDR_ENC_VERSION_2);
  dds_stream_write(&os, &dds_cdrstream_default_allocator, (const char*)sample, desc.ops.ops);

  /* 2. read that back as istream, extract BE key */
  dds_istream_t is; dds_istream_init(&is, os.m_index, os.m_buffer, DDSI_RTPS_CDR_ENC_VERSION_2);
  dds_ostreamBE_t kos; dds_ostreamBE_init(&kos, &dds_cdrstream_default_allocator, 0, DDSI_RTPS_CDR_ENC_VERSION_2);
  bool ok = dds_stream_extract_keyBE_from_data(&is, &kos, &dds_cdrstream_default_allocator, &desc);

  uint32_t n = kos.x.m_index; unsigned char *buf = kos.x.m_buffer;
  unsigned char key16[16]; memset(key16,0,16); memcpy(key16, buf, n<16?n:16);
  unsigned char md5o[16]; ddsrt_md5_state_t st; ddsrt_md5_init(&st);
  ddsrt_md5_append(&st,(const ddsrt_md5_byte_t*)buf,n); ddsrt_md5_finish(&st,md5o);

  printf("%s ok=%d key_len=%u\n", label, ok, n);
  printf("  raw_be   = "); for(uint32_t i=0;i<n;i++)printf("%02x",buf[i]); printf("\n");
  printf("  padded16 = "); for(int i=0;i<16;i++)printf("%02x",key16[i]); printf("\n");
  printf("  md5(raw) = "); for(int i=0;i<16;i++)printf("%02x",md5o[i]); printf("\n");

  dds_ostream_fini(&os,&dds_cdrstream_default_allocator);
  dds_istream_fini(&is);
  dds_ostreamBE_fini(&kos,&dds_cdrstream_default_allocator);
  dds_cdrstream_desc_fini(&desc,&dds_cdrstream_default_allocator);
}

int main(void){
  keyhash_SingleKey a={.id=0x11223344,.payload=999};
  keyhash_BigKey b={.name=(char*)"sensor-array-north-wing",.region=7,.payload=999};
  keyhash_MixedKeys c={.a=0xAA,.b=0x1234,.c=0x55667788,.d=(char)0x5A,.payload=999};
  keyhash_NestedKey nk={.hi=0x01020304,.lo=0x05060708};
  keyhash_OuterKey d={.k=nk,.payload=999};
  keyhash_of("(a) SingleKey",&keyhash_SingleKey_desc,&a);
  keyhash_of("(b) BigKey   ",&keyhash_BigKey_desc,&b);
  keyhash_of("(c) MixedKeys",&keyhash_MixedKeys_desc,&c);
  keyhash_of("(d) OuterKey ",&keyhash_OuterKey_desc,&d);
  return 0;
}
