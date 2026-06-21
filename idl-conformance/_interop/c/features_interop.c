/*
 * idl-conformance / _interop / c — per-feature XCDR2-LE golden producer.
 *
 *   <feature> ENCODE <out.bin>   -> write canonical feat::<T> as raw XCDR2-LE
 *   <feature> DECODE <in.bin>    -> decode, assert == canonical, exit 0/1
 *
 * feature in { wstr, mut, bits, tree, arr, prim }; values are CANONICAL.md.
 * Header-only codec — the emitted bytes ARE the wire.
 */
#include "gen_feat/features.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_fail = 0;
#define CHK(cond, what) do { if (!(cond)) { ++g_fail; fprintf(stderr, "  [FAIL] %s\n", what); } } while (0)

/* ---- canonical samples (CANONICAL.md) ------------------------------- */
/* café = c,a,f,é(U+00E9) */
static uint16_t WS_LABEL[] = { 0x0063, 0x0061, 0x0066, 0x00E9, 0 };
/* 日本語🎉 = U+65E5,U+672C,U+8A9E, then 🎉 U+1F389 -> surrogate D83C DF89 */
static uint16_t WS_TEXT[]  = { 0x65E5, 0x672C, 0x8A9E, 0xD83C, 0xDF89, 0 };

static void canon_wstr(feat_WStr_t* v) { memset(v,0,sizeof(*v)); v->label = WS_LABEL; v->text = WS_TEXT; }
static void canon_mut(feat_Mut_t* v)   { memset(v,0,sizeof(*v)); v->a = 1000000; v->b = 2.5; v->c = "ok"; }
static void canon_bits(feat_Bits_t* v) {
    memset(v,0,sizeof(*v));
    v->perm = feat_Perm_READ | feat_Perm_EXEC;                 /* 0x05 */
    v->flags = (feat_Flags_t)((20u << feat_Flags_prio_SHIFT) | (5u << feat_Flags_kind_SHIFT)); /* 0xA5 */
}
static feat_Tree_t T_GC, T_K0, T_K1, T_KIDS_ROOT[2], T_KIDS_K0[1];
static void canon_tree(feat_Tree_t* root) {
    memset(root,0,sizeof(*root));
    memset(&T_GC,0,sizeof(T_GC));  T_GC.value = 4;
    memset(&T_K0,0,sizeof(T_K0));  T_K0.value = 2;
    memset(&T_K1,0,sizeof(T_K1));  T_K1.value = 3;
    T_KIDS_K0[0] = T_GC; T_K0.kids.len = 1; T_K0.kids.elems = (struct feat_Tree_s*)T_KIDS_K0;
    T_KIDS_ROOT[0] = T_K0; T_KIDS_ROOT[1] = T_K1;
    root->value = 1; root->kids.len = 2; root->kids.elems = (struct feat_Tree_s*)T_KIDS_ROOT;
}
static void canon_arr(feat_Arr_t* v) {
    memset(v,0,sizeof(*v));
    int32_t g[2][3] = {{10,11,12},{13,14,15}};
    memcpy(v->grid, g, sizeof(g));
    v->shape[0].x = 1; v->shape[0].y = 2;
    v->shape[1].x = 3; v->shape[1].y = 4;
}
static void canon_prim(feat_Prim_t* v) {
    memset(v,0,sizeof(*v));
    v->i8 = -128; v->u8 = 255; v->i16 = -32768; v->u16 = 65535;
    v->i32 = (int32_t)-2147483648LL; v->u32 = 4294967295u;
    v->i64 = (int64_t)0x8000000000000000ULL; v->u64 = 0xFFFFFFFFFFFFFFFFULL;
    v->f32 = 3.5f; v->f64 = -1234.5; v->b = 1; v->o = 0xAB; v->ch = 'Z';
}

static uint8_t* readfile(const char* p, size_t* n) {
    FILE* f = fopen(p, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    uint8_t* b = (uint8_t*)malloc((size_t)sz);
    if (fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); fclose(f); return NULL; }
    fclose(f); *n = (size_t)sz; return b;
}

typedef int (*enc_fn)(const void*, uint8_t*, size_t, size_t*);
typedef int (*dec_fn)(const uint8_t*, size_t, void*);

static int do_encode(const char* path, enc_fn enc, const void* s) {
    size_t need = 0;
    enc(s, NULL, 0, &need);                 /* sizing pass */
    if (need == 0) { fprintf(stderr, "ENCODE: 0 bytes\n"); return 1; }
    uint8_t* buf = (uint8_t*)malloc(need);
    size_t got = 0;
    if (enc(s, buf, need, &got) != 0 || got != need) { fprintf(stderr, "ENCODE fail\n"); free(buf); return 1; }
    FILE* f = fopen(path, "wb"); if (!f) { free(buf); return 1; }
    fwrite(buf, 1, got, f); fclose(f); free(buf);
    printf("ENCODE OK: %zu bytes -> %s\n", got, path);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: <feature> ENCODE|DECODE <file>\n"); return 2; }
    const char* feat = argv[1];
    const char* mode = argv[2];
    const char* file = argc > 3 ? argv[3] : NULL;

    feat_WStr_t ws; feat_Mut_t mu; feat_Bits_t bi; feat_Tree_t tr; feat_Arr_t ar; feat_Prim_t pr;

    if (strcmp(mode, "ENCODE") == 0) {
        if (!strcmp(feat,"wstr")) { canon_wstr(&ws); return do_encode(file, feat_WStr_encode, &ws); }
        if (!strcmp(feat,"mut"))  { canon_mut(&mu);  return do_encode(file, feat_Mut_encode,  &mu); }
        if (!strcmp(feat,"bits")) { canon_bits(&bi); return do_encode(file, feat_Bits_encode, &bi); }
        if (!strcmp(feat,"tree")) { canon_tree(&tr); return do_encode(file, feat_Tree_encode, &tr); }
        if (!strcmp(feat,"arr"))  { canon_arr(&ar);  return do_encode(file, feat_Arr_encode,  &ar); }
        if (!strcmp(feat,"prim")) { canon_prim(&pr); return do_encode(file, feat_Prim_encode, &pr); }
        fprintf(stderr, "unknown feature\n"); return 2;
    } else if (strcmp(mode, "DECODE") == 0) {
        size_t n = 0; uint8_t* buf = readfile(file, &n);
        if (!buf) { fprintf(stderr, "missing/empty %s\n", file); return 2; }
        if (!strcmp(feat,"wstr")) {
            feat_WStr_t g; memset(&g,0,sizeof(g)); feat_WStr_t w; canon_wstr(&w);
            if (feat_WStr_decode(buf, n, &g) != 0) { fprintf(stderr,"decode fail\n"); return 1; }
            size_t li=0; for (; w.label[li]; ++li) CHK(g.label && g.label[li]==w.label[li], "label[]");
            CHK(g.label && g.label[li]==0, "label.len");
            size_t ti=0; for (; w.text[ti]; ++ti) CHK(g.text && g.text[ti]==w.text[ti], "text[]");
            CHK(g.text && g.text[ti]==0, "text.len");
        } else if (!strcmp(feat,"mut")) {
            feat_Mut_t g; memset(&g,0,sizeof(g)); feat_Mut_t w; canon_mut(&w);
            if (feat_Mut_decode(buf, n, &g) != 0) { fprintf(stderr,"decode fail\n"); return 1; }
            CHK(g.a==w.a, "a"); CHK(g.b==w.b, "b"); CHK(g.c && !strcmp(g.c,w.c), "c");
        } else if (!strcmp(feat,"bits")) {
            feat_Bits_t g; memset(&g,0,sizeof(g)); feat_Bits_t w; canon_bits(&w);
            if (feat_Bits_decode(buf, n, &g) != 0) { fprintf(stderr,"decode fail\n"); return 1; }
            CHK(g.perm==w.perm, "perm"); CHK(g.flags==w.flags, "flags");
        } else if (!strcmp(feat,"tree")) {
            feat_Tree_t g; memset(&g,0,sizeof(g)); feat_Tree_t w; canon_tree(&w);
            if (feat_Tree_decode(buf, n, &g) != 0) { fprintf(stderr,"decode fail\n"); return 1; }
            CHK(g.value==1, "root.value"); CHK(g.kids.len==2, "root.kids.len");
            if (g.kids.len==2) {
                CHK(g.kids.elems[0].value==2, "k0.value");
                CHK(g.kids.elems[0].kids.len==1, "k0.kids.len");
                if (g.kids.elems[0].kids.len==1) CHK(g.kids.elems[0].kids.elems[0].value==4, "gc.value");
                CHK(g.kids.elems[1].value==3, "k1.value");
                CHK(g.kids.elems[1].kids.len==0, "k1.kids.len");
            }
        } else if (!strcmp(feat,"arr")) {
            feat_Arr_t g; memset(&g,0,sizeof(g)); feat_Arr_t w; canon_arr(&w);
            if (feat_Arr_decode(buf, n, &g) != 0) { fprintf(stderr,"decode fail\n"); return 1; }
            for (int i=0;i<2;++i) for (int j=0;j<3;++j) CHK(g.grid[i][j]==w.grid[i][j], "grid");
            for (int i=0;i<2;++i) { CHK(g.shape[i].x==w.shape[i].x,"shape.x"); CHK(g.shape[i].y==w.shape[i].y,"shape.y"); }
        } else if (!strcmp(feat,"prim")) {
            feat_Prim_t g; memset(&g,0,sizeof(g)); feat_Prim_t w; canon_prim(&w);
            if (feat_Prim_decode(buf, n, &g) != 0) { fprintf(stderr,"decode fail\n"); return 1; }
            CHK(g.i8==w.i8,"i8"); CHK(g.u8==w.u8,"u8"); CHK(g.i16==w.i16,"i16"); CHK(g.u16==w.u16,"u16");
            CHK(g.i32==w.i32,"i32"); CHK(g.u32==w.u32,"u32"); CHK(g.i64==w.i64,"i64"); CHK(g.u64==w.u64,"u64");
            CHK(g.f32==w.f32,"f32"); CHK(g.f64==w.f64,"f64"); CHK(g.b==w.b,"b"); CHK(g.o==w.o,"o"); CHK(g.ch==w.ch,"ch");
        } else { fprintf(stderr, "unknown feature\n"); free(buf); return 2; }
        free(buf);
        if (g_fail == 0) { printf("DECODE OK: %s\n", feat); return 0; }
        printf("DECODE FAILED: %d mismatch(es)\n", g_fail); return 1;
    }
    fprintf(stderr, "bad args\n"); return 2;
}
