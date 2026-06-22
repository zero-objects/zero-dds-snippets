

/*
WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

This file was generated from typeobject.idl
using RTI Code Generator (rtiddsgen) version 4.7.0.
The rtiddsgen tool is part of the RTI Connext DDS distribution.
For more information, type 'rtiddsgen -help' at a command shell
or consult the Code Generator User's Manual.
*/

#ifndef typeobject_1877327936_h
#define typeobject_1877327936_h

#ifndef NDDS_STANDALONE_TYPE
#ifndef ndds_c_h
#include "ndds/ndds_c.h"
#endif
#include "cdr/cdr_typeCode.h"
#else
#include "ndds_standalone_type.h"
#endif

#ifdef __cplusplus
extern "C" {
    #endif

    extern const char *to_PlainTYPENAME;

    typedef struct to_Plain
    {

        DDS_Long a;
        DDS_Short b;

    } to_Plain ;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Plain_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Plain_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Plain_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_PlainSeq, to_Plain);

    NDDSUSERDllExport
    RTIBool to_Plain_initialize(
        to_Plain* self);

    NDDSUSERDllExport
    RTIBool to_Plain_initialize_ex(
        to_Plain* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Plain_initialize_w_params(
        to_Plain* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Plain_finalize_w_return(
        to_Plain* self);

    NDDSUSERDllExport
    void to_Plain_finalize(
        to_Plain* self);

    NDDSUSERDllExport
    void to_Plain_finalize_ex(
        to_Plain* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Plain_finalize_w_params(
        to_Plain* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Plain_finalize_optional_members(
        to_Plain* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Plain_copy(
        to_Plain* dst,
        const to_Plain* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    extern const char *to_AppendableTYPENAME;

    typedef struct to_Appendable
    {

        DDS_Long a;
        DDS_Short b;

    } to_Appendable ;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Appendable_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Appendable_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Appendable_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_AppendableSeq, to_Appendable);

    NDDSUSERDllExport
    RTIBool to_Appendable_initialize(
        to_Appendable* self);

    NDDSUSERDllExport
    RTIBool to_Appendable_initialize_ex(
        to_Appendable* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Appendable_initialize_w_params(
        to_Appendable* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Appendable_finalize_w_return(
        to_Appendable* self);

    NDDSUSERDllExport
    void to_Appendable_finalize(
        to_Appendable* self);

    NDDSUSERDllExport
    void to_Appendable_finalize_ex(
        to_Appendable* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Appendable_finalize_w_params(
        to_Appendable* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Appendable_finalize_optional_members(
        to_Appendable* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Appendable_copy(
        to_Appendable* dst,
        const to_Appendable* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    extern const char *to_MutableTYPENAME;

    typedef struct to_Mutable
    {

        DDS_Long a;
        DDS_Short b;

    } to_Mutable ;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Mutable_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Mutable_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Mutable_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_MutableSeq, to_Mutable);

    NDDSUSERDllExport
    RTIBool to_Mutable_initialize(
        to_Mutable* self);

    NDDSUSERDllExport
    RTIBool to_Mutable_initialize_ex(
        to_Mutable* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Mutable_initialize_w_params(
        to_Mutable* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Mutable_finalize_w_return(
        to_Mutable* self);

    NDDSUSERDllExport
    void to_Mutable_finalize(
        to_Mutable* self);

    NDDSUSERDllExport
    void to_Mutable_finalize_ex(
        to_Mutable* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Mutable_finalize_w_params(
        to_Mutable* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Mutable_finalize_optional_members(
        to_Mutable* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Mutable_copy(
        to_Mutable* dst,
        const to_Mutable* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    typedef enum to_Color
    {
        RED , 
        GREEN , 
        BLUE 
    } to_Color;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Color_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Color_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Color_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_ColorSeq, to_Color);

    NDDSUSERDllExport
    RTIBool to_Color_initialize(
        to_Color* self);

    NDDSUSERDllExport
    RTIBool to_Color_initialize_ex(
        to_Color* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Color_initialize_w_params(
        to_Color* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Color_finalize_w_return(
        to_Color* self);

    NDDSUSERDllExport
    void to_Color_finalize(
        to_Color* self);

    NDDSUSERDllExport
    void to_Color_finalize_ex(
        to_Color* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Color_finalize_w_params(
        to_Color* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Color_finalize_optional_members(
        to_Color* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Color_copy(
        to_Color* dst,
        const to_Color* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    typedef struct DDS_LongSeq to_LongSeq4;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_LongSeq4_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_LongSeq4_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_LongSeq4_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_LongSeq4Seq, to_LongSeq4);

    NDDSUSERDllExport
    RTIBool to_LongSeq4_initialize(
        to_LongSeq4* self);

    NDDSUSERDllExport
    RTIBool to_LongSeq4_initialize_ex(
        to_LongSeq4* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_LongSeq4_initialize_w_params(
        to_LongSeq4* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_LongSeq4_finalize_w_return(
        to_LongSeq4* self);

    NDDSUSERDllExport
    void to_LongSeq4_finalize(
        to_LongSeq4* self);

    NDDSUSERDllExport
    void to_LongSeq4_finalize_ex(
        to_LongSeq4* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_LongSeq4_finalize_w_params(
        to_LongSeq4* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_LongSeq4_finalize_optional_members(
        to_LongSeq4* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_LongSeq4_copy(
        to_LongSeq4* dst,
        const to_LongSeq4* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    extern const char *to_InnerTYPENAME;

    typedef struct to_Inner
    {

        DDS_Long x;
        DDS_Long y;

    } to_Inner ;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Inner_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Inner_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Inner_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_InnerSeq, to_Inner);

    NDDSUSERDllExport
    RTIBool to_Inner_initialize(
        to_Inner* self);

    NDDSUSERDllExport
    RTIBool to_Inner_initialize_ex(
        to_Inner* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Inner_initialize_w_params(
        to_Inner* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Inner_finalize_w_return(
        to_Inner* self);

    NDDSUSERDllExport
    void to_Inner_finalize(
        to_Inner* self);

    NDDSUSERDllExport
    void to_Inner_finalize_ex(
        to_Inner* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Inner_finalize_w_params(
        to_Inner* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Inner_finalize_optional_members(
        to_Inner* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Inner_copy(
        to_Inner* dst,
        const to_Inner* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    extern const char *to_NestedTYPENAME;

    typedef struct to_Nested
    {

        to_Inner child;
        struct DDS_LongSeq seq;
        DDS_Char * name;
        to_Color color;

    } to_Nested ;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Nested_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Nested_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Nested_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_NestedSeq, to_Nested);

    NDDSUSERDllExport
    RTIBool to_Nested_initialize(
        to_Nested* self);

    NDDSUSERDllExport
    RTIBool to_Nested_initialize_ex(
        to_Nested* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Nested_initialize_w_params(
        to_Nested* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Nested_finalize_w_return(
        to_Nested* self);

    NDDSUSERDllExport
    void to_Nested_finalize(
        to_Nested* self);

    NDDSUSERDllExport
    void to_Nested_finalize_ex(
        to_Nested* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Nested_finalize_w_params(
        to_Nested* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Nested_finalize_optional_members(
        to_Nested* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Nested_copy(
        to_Nested* dst,
        const to_Nested* src);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    extern const char *to_ChoiceTYPENAME;

    typedef struct to_Choice
    {

        DDS_Long _d;
        struct to_Choice_u 
        {

            DDS_Long as_long;
            DDS_Double as_double;
            DDS_Char * as_text;
        }_u;

    } to_Choice ;
    #if defined(NDDS_USER_DLL_EXPORT) && defined(RTI_WIN32)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __declspec(dllexport)
    #endif

    #if !defined(RTI_WIN32) && defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport __attribute__((visibility("default")))
    #endif

    #ifndef NDDS_STANDALONE_TYPE
    NDDSUSERDllExport DDS_TypeCode * to_Choice_get_typecode(void); /* Type code */
    NDDSUSERDllExport RTIXCdrTypePlugin *to_Choice_get_type_plugin_info(void);
    NDDSUSERDllExport RTIXCdrSampleAccessInfo *to_Choice_get_sample_access_info(void);
    #endif

    DDS_SEQUENCE(to_ChoiceSeq, to_Choice);

    NDDSUSERDllExport
    RTIBool to_Choice_initialize(
        to_Choice* self);

    NDDSUSERDllExport
    RTIBool to_Choice_initialize_ex(
        to_Choice* self,RTIBool allocatePointers,RTIBool allocateMemory);

    NDDSUSERDllExport
    RTIBool to_Choice_initialize_w_params(
        to_Choice* self,
        const struct DDS_TypeAllocationParams_t * allocParams);  

    NDDSUSERDllExport
    RTIBool to_Choice_finalize_w_return(
        to_Choice* self);

    NDDSUSERDllExport
    void to_Choice_finalize(
        to_Choice* self);

    NDDSUSERDllExport
    void to_Choice_finalize_ex(
        to_Choice* self,RTIBool deletePointers);

    NDDSUSERDllExport
    void to_Choice_finalize_w_params(
        to_Choice* self,
        const struct DDS_TypeDeallocationParams_t * deallocParams);

    NDDSUSERDllExport
    void to_Choice_finalize_optional_members(
        to_Choice* self, RTIBool deletePointers);  

    NDDSUSERDllExport
    RTIBool to_Choice_copy(
        to_Choice* dst,
        const to_Choice* src);

    NDDSUSERDllExport
    DDS_LongLong to_Choice_getDefaultDiscriminator(void);

    #if defined(NDDS_USER_DLL_EXPORT) || defined(NDDS_USER_SYMBOL_EXPORT)
    #undef NDDSUSERDllExport
    #define NDDSUSERDllExport
    #endif

    #ifdef __cplusplus
}
#endif

#endif /* typeobject */
