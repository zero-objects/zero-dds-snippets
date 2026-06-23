#include <open62541/types.h>
#include <open62541/pubsub.h>
#include <string.h>
#include <stdio.h>

static void dump(const char* label, UA_NetworkMessage* nm){
  UA_ByteString out=UA_BYTESTRING_NULL;
  UA_StatusCode rc=UA_NetworkMessage_encodeBinary(nm,&out,NULL);
  if(rc!=UA_STATUSCODE_GOOD){printf("OPEN62541_UADP %s ERR 0x%08x\n",label,rc);return;}
  printf("OPEN62541_UADP %s %zu ",label,out.length);
  for(size_t i=0;i<out.length;i++) printf("%02x",out.data[i]);
  printf("\n"); UA_ByteString_clear(&out);
}
static void base(UA_NetworkMessage* nm){
  memset(nm,0,sizeof(*nm));
  nm->version=1; nm->networkMessageType=UA_NETWORKMESSAGE_DATASET;
  nm->publisherIdEnabled=true;
  nm->publisherId.idType=UA_PUBLISHERIDTYPE_UINT16; nm->publisherId.id.uint16=42;
  nm->groupHeaderEnabled=true;
  nm->groupHeader.writerGroupIdEnabled=true; nm->groupHeader.writerGroupId=7;
  nm->groupHeader.sequenceNumberEnabled=true; nm->groupHeader.sequenceNumber=99;
  nm->payloadHeaderEnabled=true;
}
static void dsm_init(UA_DataSetMessage* d){
  memset(d,0,sizeof(*d));
  d->header.dataSetMessageValid=true;
  d->header.fieldEncoding=UA_FIELDENCODING_VARIANT;
  d->header.dataSetMessageType=UA_DATASETMESSAGE_DATAKEYFRAME;
}
int main(void){
  // V1
  { UA_NetworkMessage nm; base(&nm); nm.messageCount=1; nm.dataSetWriterIds[0]=1;
    UA_DataSetMessage d; dsm_init(&d); d.fieldCount=2;
    UA_DataValue f[2]; UA_UInt32 a=0x11223344u,b=0x55667788u;
    UA_DataValue_init(&f[0]); f[0].hasValue=true; UA_Variant_setScalar(&f[0].value,&a,&UA_TYPES[UA_TYPES_UINT32]);
    UA_DataValue_init(&f[1]); f[1].hasValue=true; UA_Variant_setScalar(&f[1].value,&b,&UA_TYPES[UA_TYPES_UINT32]);
    d.data.keyFrameFields=f; nm.payload.dataSetMessages=&d; dump("V1",&nm); }
  // V2: Byte pubid, Boolean+Double+Int16
  { UA_NetworkMessage nm; base(&nm); nm.publisherId.idType=UA_PUBLISHERIDTYPE_BYTE; nm.publisherId.id.byte=9;
    nm.messageCount=1; nm.dataSetWriterIds[0]=1;
    UA_DataSetMessage d; dsm_init(&d); d.fieldCount=3;
    UA_DataValue f[3]; UA_Boolean bo=true; UA_Double db=1.5; UA_Int16 i16=-7;
    UA_DataValue_init(&f[0]); f[0].hasValue=true; UA_Variant_setScalar(&f[0].value,&bo,&UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_DataValue_init(&f[1]); f[1].hasValue=true; UA_Variant_setScalar(&f[1].value,&db,&UA_TYPES[UA_TYPES_DOUBLE]);
    UA_DataValue_init(&f[2]); f[2].hasValue=true; UA_Variant_setScalar(&f[2].value,&i16,&UA_TYPES[UA_TYPES_INT16]);
    d.data.keyFrameFields=f; nm.payload.dataSetMessages=&d; dump("V2",&nm); }
  // V3: String
  { UA_NetworkMessage nm; base(&nm); nm.messageCount=1; nm.dataSetWriterIds[0]=1;
    UA_DataSetMessage d; dsm_init(&d); d.fieldCount=1;
    UA_DataValue f[1]; UA_String s=UA_STRING("hi");
    UA_DataValue_init(&f[0]); f[0].hasValue=true; UA_Variant_setScalar(&f[0].value,&s,&UA_TYPES[UA_TYPES_STRING]);
    d.data.keyFrameFields=f; nm.payload.dataSetMessages=&d; dump("V3",&nm); }
  // V4: two messages
  { UA_NetworkMessage nm; base(&nm); nm.messageCount=2; nm.dataSetWriterIds[0]=1; nm.dataSetWriterIds[1]=2;
    UA_DataSetMessage d[2]; UA_UInt32 a=0x11223344u; UA_Int32 n=-5;
    dsm_init(&d[0]); d[0].fieldCount=1; UA_DataValue f0[1];
    UA_DataValue_init(&f0[0]); f0[0].hasValue=true; UA_Variant_setScalar(&f0[0].value,&a,&UA_TYPES[UA_TYPES_UINT32]); d[0].data.keyFrameFields=f0;
    dsm_init(&d[1]); d[1].fieldCount=1; UA_DataValue f1[1];
    UA_DataValue_init(&f1[0]); f1[0].hasValue=true; UA_Variant_setScalar(&f1[0].value,&n,&UA_TYPES[UA_TYPES_INT32]); d[1].data.keyFrameFields=f1;
    nm.payload.dataSetMessages=d; dump("V4",&nm); }
  // V5: DataSetMessage sequence number
  { UA_NetworkMessage nm; base(&nm); nm.messageCount=1; nm.dataSetWriterIds[0]=1;
    UA_DataSetMessage d; dsm_init(&d);
    d.header.dataSetMessageSequenceNrEnabled=true; d.header.dataSetMessageSequenceNr=5;
    d.fieldCount=1; UA_DataValue f[1]; UA_UInt32 a=0xDEADBEEFu;
    UA_DataValue_init(&f[0]); f[0].hasValue=true; UA_Variant_setScalar(&f[0].value,&a,&UA_TYPES[UA_TYPES_UINT32]);
    d.data.keyFrameFields=f; nm.payload.dataSetMessages=&d; dump("V5",&nm); }
  return 0;
}
