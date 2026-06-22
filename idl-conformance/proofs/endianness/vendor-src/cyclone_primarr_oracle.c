#include "cyc/primarr.h"
#include "dds/cdr/dds_cdrstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static void *mymalloc(size_t n){return malloc(n);}
static void *myrealloc(void*p,size_t n){return realloc(p,n);}
static void myfree(void*p){free(p);}
static struct dds_cdrstream_allocator A={mymalloc,myrealloc,myfree};
static void dumpit(const char*feat,const char*tag,int be,uint32_t ver,const void*s,const uint32_t*ops){
  char path[256]; snprintf(path,sizeof path,"out/%s.%s.cyclone.bin",feat,tag);
  uint32_t idx; const char*buf;
  if(be){dds_ostreamBE_t os;dds_ostreamBE_init(&os,&A,0,ver);(void)dds_stream_writeBE(&os,&A,(const char*)s,ops);idx=os.x.m_index;buf=(const char*)os.x.m_buffer;FILE*f=fopen(path,"wb");fwrite(buf,1,idx,f);fclose(f);dds_ostreamBE_fini(&os,&A);}
  else {dds_ostreamLE_t os;dds_ostreamLE_init(&os,&A,0,ver);(void)dds_stream_writeLE(&os,&A,(const char*)s,ops);idx=os.x.m_index;buf=(const char*)os.x.m_buffer;FILE*f=fopen(path,"wb");fwrite(buf,1,idx,f);fclose(f);dds_ostreamLE_fini(&os,&A);}
  printf("CYCLONE %-5s %-8s len=%u\n",feat,tag,idx);
}
static void all(const char*feat,const void*s,const uint32_t*ops){
  dumpit(feat,"xcdr2-le",0,2,s,ops); dumpit(feat,"xcdr2-be",1,2,s,ops);
  dumpit(feat,"xcdr1-le",0,1,s,ops); dumpit(feat,"xcdr1-be",1,1,s,ops);
}
int main(void){
  feat_Arr a; int32_t g[2][3]={{10,11,12},{13,14,15}}; memcpy(a.grid,g,sizeof g);
  a.shape[0].x=1;a.shape[0].y=2;a.shape[1].x=3;a.shape[1].y=4;
  all("arr",&a,feat_Arr_desc.m_ops);
  feat_Prim p; p.i8=-128;p.u8=255;p.i16=-32768;p.u16=65535;p.i32=-2147483648;p.u32=4294967295u;
  p.i64=INT64_MIN;p.u64=UINT64_MAX;p.f32=3.5f;p.f64=-1234.5;p.b=1;p.o=0xAB;p.ch=0x5A;
  all("prim",&p,feat_Prim_desc.m_ops);
  return 0;
}
