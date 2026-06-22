#include "cyc/wstr_only.h"
#include "dds/cdr/dds_cdrstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

extern const dds_topic_descriptor_t feat_WStr_desc;

static void *mymalloc(size_t n){return malloc(n);}
static void *myrealloc(void*p,size_t n){return realloc(p,n);}
static void myfree(void*p){free(p);}

static void dump(const char*tag,int be,uint32_t ver,feat_WStr*s){
  int rc; (void)rc; struct dds_cdrstream_allocator alloc={mymalloc,myrealloc,myfree};
  char path[256];
  if(be){
    dds_ostreamBE_t os; dds_ostreamBE_init(&os,&alloc,0,ver);
    dds_stream_writeBE(&os,&alloc,(const char*)s,feat_WStr_desc.m_ops);
    snprintf(path,sizeof path,"out/wstr.%s.cyclone.bin",tag);
    FILE*f=fopen(path,"wb"); fwrite(os.x.m_buffer,1,os.x.m_index,f); fclose(f);
    printf("CYCLONE wstr %-8s len=%u\n",tag,os.x.m_index);
    dds_ostreamBE_fini(&os,&alloc);
  } else {
    dds_ostreamLE_t os; dds_ostreamLE_init(&os,&alloc,0,ver);
    dds_stream_writeLE(&os,&alloc,(const char*)s,feat_WStr_desc.m_ops);
    snprintf(path,sizeof path,"out/wstr.%s.cyclone.bin",tag);
    FILE*f=fopen(path,"wb"); fwrite(os.x.m_buffer,1,os.x.m_index,f); fclose(f);
    printf("CYCLONE wstr %-8s len=%u\n",tag,os.x.m_index);
    dds_ostreamLE_fini(&os,&alloc);
  }
}

int main(void){
  feat_WStr s;
  static wchar_t lbl[]={0x63,0x61,0x66,0xE9,0}; // café
  static wchar_t txt[]={0x65E5,0x672C,0x8A9E,0x1F389,0}; // 日本語🎉
  s.label=lbl; s.text=txt;
  dump("xcdr2-le",0,DDSI_RTPS_CDR_ENC_VERSION_2,&s);
  dump("xcdr2-be",1,DDSI_RTPS_CDR_ENC_VERSION_2,&s);
  dump("xcdr1-le",0,DDSI_RTPS_CDR_ENC_VERSION_1,&s);
  dump("xcdr1-be",1,DDSI_RTPS_CDR_ENC_VERSION_1,&s);
  return 0;
}
