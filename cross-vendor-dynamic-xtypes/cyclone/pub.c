#include "dds/dds.h"
#include "dyn.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int argc, char** argv){
  int dom = argc>1?atoi(argv[1]):0;
  dds_entity_t p = dds_create_participant(dom, NULL, NULL);
  dds_entity_t t = dds_create_topic(p, &dyn_Track_desc, "TrackTopic", NULL, NULL);
  dds_entity_t w = dds_create_writer(p, t, NULL, NULL);
  printf("[cyc-pub] dom=%d topic=TrackTopic type=dyn::Track\n", dom); fflush(stdout);
  int32_t path[3];
  for(int i=1;;i++){
    dyn_Track s; memset(&s,0,sizeof(s));
    char nm[32]; snprintf(nm,sizeof(nm),"ctrack-%d",i);
    s.id=100+i; s.name=nm; s.color=dyn_BLUE;
    path[0]=i; path[1]=i*2; path[2]=i*3;
    s.path._maximum=3; s.path._length=3; s.path._buffer=path; s.path._release=false;
    int k=i%3;
    if(k==0){ s.command._d=1; s.command._u.speed=7000+i; }
    else if(k==1){ s.command._d=2; s.command._u.angle=1.5+i; }
    else { s.command._d=0; s.command._u.flag=(uint8_t)(i&0xff); }
    dds_write(w,&s);
    printf("  -> id=%d case=%d\n",100+i,k); fflush(stdout);
    dds_sleepfor(DDS_MSECS(400));
  }
}
