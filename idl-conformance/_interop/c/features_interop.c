/*
 * idl-conformance / _interop / c — per-feature XCDR2-LE golden producer.
 *
 *   <feature> ENCODE <out.bin>   -> write canonical feat::<T> as raw XCDR2-LE
 *   <feature> DECODE <in.bin>    -> decode, assert == canonical, exit 0/1
 *   -         BE                 -> decode the big-endian goldens of
 *                                   ../../proofs/endianness, assert == canonical
 *   -         XCDR1              -> built with -DFEATURES_XCDR1: decode the
 *                                   XCDR1 goldens of ../../proofs/endianness,
 *                                   little- and big-endian
 *
 * feature in { wstr, mut, mutnest, outerkey, bits, tree, arr, prim, mapenum,
 * mapprim, fixed }; values are CANONICAL.md.
 * gen_feat/features.h is `zerodds-idlc generate --lang c` output over
 * runtime/c, gen_x1/features.h the same with `--encoding xcdr1`: the bytes
 * the generated encoders write ARE the wire.
 */
#ifdef FEATURES_XCDR1
#include "gen_x1/features.h"
#define FEATURES_MAX_ALIGN 8
#else
#include "gen_feat/features.h"
#define FEATURES_MAX_ALIGN 4
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_fail = 0;
#define CHK(cond, what) do { if (!(cond)) { ++g_fail; fprintf(stderr, "  [FAIL] %s\n", what); } } while (0)

/* ---- canonical samples (CANONICAL.md) ------------------------------- */
/* A wstring is UTF-8 in C; the wire carries its UTF-16 code units.
 * café = c,a,f,é(U+00E9); 日本語🎉 = U+65E5,U+672C,U+8A9E,U+1F389. */
static const char WS_LABEL[] = "caf\xc3\xa9";
static const char WS_TEXT[]  = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xf0\x9f\x8e\x89";

static void canon_wstr(feat_WStr* v) { memset(v,0,sizeof(*v)); v->label = WS_LABEL; v->text = WS_TEXT; }
static void canon_mut(feat_Mut* v)   { memset(v,0,sizeof(*v)); v->a = 1000000; v->b = 2.5; v->c = "ok"; }
static void canon_bits(feat_Bits* v) {
    memset(v,0,sizeof(*v));
    v->perm = feat_READ | feat_EXEC;      /* 0x05 */
    v->flags.kind = 5; v->flags.prio = 20; /* 0xA5 */
}
static feat_Tree T_KIDS_ROOT[2], T_KIDS_K0[1];
static void canon_tree(feat_Tree* root) {
    memset(root,0,sizeof(*root));
    memset(T_KIDS_ROOT,0,sizeof(T_KIDS_ROOT)); memset(T_KIDS_K0,0,sizeof(T_KIDS_K0));
    T_KIDS_K0[0].value = 4;
    T_KIDS_ROOT[0].value = 2; T_KIDS_ROOT[0].kids._length = 1; T_KIDS_ROOT[0].kids._buffer = T_KIDS_K0;
    T_KIDS_ROOT[1].value = 3;
    root->value = 1; root->kids._length = 2; root->kids._buffer = T_KIDS_ROOT;
}
static void canon_arr(feat_Arr* v) {
    memset(v,0,sizeof(*v));
    int32_t g[2][3] = {{10,11,12},{13,14,15}};
    memcpy(v->grid, g, sizeof(g));
    v->shape[0].x = 1; v->shape[0].y = 2;
    v->shape[1].x = 3; v->shape[1].y = 4;
}
/* nested @mutable: MutNest{tag=9, leaf={u=100,v=1.25}, list=[{1,0.5},{2,0.25}]} */
static feat_MutLeaf MN_LIST[2];
static void canon_mutnest(feat_MutNest* v) {
    memset(v,0,sizeof(*v));
    v->tag = 9;
    v->leaf.u = 100; v->leaf.v = 1.25;
    MN_LIST[0].u = 1; MN_LIST[0].v = 0.5;
    MN_LIST[1].u = 2; MN_LIST[1].v = 0.25;
    v->list._length = 2; v->list._buffer = MN_LIST;
}
/* nested-struct @key: OuterKey{k={hi=0x01020304,lo=0x05060708}, payload=999} */
static void canon_outerkey(feat_OuterKey* v) {
    memset(v,0,sizeof(*v));
    v->k.hi = 0x01020304; v->k.lo = 0x05060708;
    v->payload = 999;
}
/* @bit_bound enum (2B) + map<long,Pt> + sequence<Sel>. h=BLUE; m={3:{11,12}}; sels=[Sel{_d=2,n=9}]. */
static Int32feat_PtMapElement ME_M[1];
static feat_Sel ME_SELS[1];
static void canon_mapenum(feat_MapEnum* v) {
    memset(v,0,sizeof(*v));
    v->h = feat_H_BLUE;
    ME_M[0].key = 3; ME_M[0].value.x = 11; ME_M[0].value.y = 12;
    v->m._length = 1; v->m._buffer = ME_M;
    memset(ME_SELS,0,sizeof(ME_SELS));
    ME_SELS[0]._d = 2; ME_SELS[0]._u.n = 9;
    v->sels._length = 1; v->sels._buffer = ME_SELS;
}
/* map<long,long> primitive-valued -> NO collection DHEADER (XCDR2 §7.4.3.5). m={7:42,8:99}. */
static Int32Int32MapElement MP_M[2];
static void canon_mapprim(feat_MapPrim* v) {
    memset(v,0,sizeof(*v));
    MP_M[0].key = 7; MP_M[0].value = 42; MP_M[1].key = 8; MP_M[1].value = 99;
    v->m._length = 2; v->m._buffer = MP_M;
}
/* fixed<P,S> as its decimal text: price=123.45 (wire 12 34 5c), qty=1234 (01 23 4c). */
static void canon_fixed(feat_Money* v) { memset(v,0,sizeof(*v)); v->price = "123.45"; v->qty = "1234"; }
static void canon_prim(feat_Prim* v) {
    memset(v,0,sizeof(*v));
    v->i8 = -128; v->u8 = 255; v->i16 = -32768; v->u16 = 65535;
    v->i32 = (int32_t)-2147483648LL; v->u32 = 4294967295u;
    v->i64 = (int64_t)0x8000000000000000ULL; v->u64 = 0xFFFFFFFFFFFFFFFFULL;
    v->f32 = 3.5f; v->f64 = -1234.5; v->b = true; v->o = 0xAB; v->ch = 'Z';
}

static uint8_t* readfile(const char* p, size_t* n) {
    FILE* f = fopen(p, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    uint8_t* b = (uint8_t*)malloc((size_t)sz);
    if (fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); fclose(f); return NULL; }
    fclose(f); *n = (size_t)sz; return b;
}

static uint8_t g_out[4096];
static uint8_t g_heap[8192];

/* XCDR2 little endian, MAXALIGN four (XTypes 1.3 §7.4.1). */
#define ENCODE(TYPE, fn, canon) do { \
        TYPE s; canon(&s); zdw_writer w; \
        zdw_writer_init(&w, g_out, sizeof(g_out), ZDW_LITTLE, 4); \
        fn(&w, &s); \
        return write_out(file, &w); \
    } while (0)

static int write_out(const char* path, const zdw_writer* w) {
    if (zdw_writer_status(w) != ZDW_OK) { fprintf(stderr, "ENCODE fail: status %d\n", (int)zdw_writer_status(w)); return 1; }
    size_t got = zdw_writer_length(w);
    FILE* f = fopen(path, "wb"); if (!f) return 1;
    fwrite(g_out, 1, got, f); fclose(f);
    printf("ENCODE OK: %zu bytes -> %s\n", got, path);
    return 0;
}

static void reader_on(zdw_reader* r, const uint8_t* b, size_t n, zdw_endianness e) {
    zdw_reader_init(r, b, n, e, FEATURES_MAX_ALIGN);
    zdw_reader_set_heap(r, g_heap, sizeof(g_heap));
}
#define DECODED_OK(r) (zdw_reader_status(r) == ZDW_OK)

/* The checks of one feature against its canonical value, shared by the
 * little-endian DECODE and the big-endian BE mode. */
static void check_wstr(const feat_WStr* g) {
    CHK(g->label && !strcmp(g->label, WS_LABEL), "label"); CHK(g->text && !strcmp(g->text, WS_TEXT), "text");
}
static void check_mut(const feat_Mut* g) { CHK(g->a==1000000, "a"); CHK(g->b==2.5, "b"); CHK(g->c && !strcmp(g->c,"ok"), "c"); }
static void check_bits(const feat_Bits* g) { CHK(g->perm==(feat_READ|feat_EXEC), "perm"); CHK(g->flags.kind==5 && g->flags.prio==20, "flags"); }
static void check_tree(const feat_Tree* g) {
    CHK(g->value==1, "root.value"); CHK(g->kids._length==2, "root.kids.len");
    if (g->kids._length==2) {
        CHK(g->kids._buffer[0].value==2, "k0.value");
        CHK(g->kids._buffer[0].kids._length==1, "k0.kids.len");
        if (g->kids._buffer[0].kids._length==1) CHK(g->kids._buffer[0].kids._buffer[0].value==4, "gc.value");
        CHK(g->kids._buffer[1].value==3, "k1.value");
        CHK(g->kids._buffer[1].kids._length==0, "k1.kids.len");
    }
}
static void check_arr(const feat_Arr* g) {
    feat_Arr w; canon_arr(&w);
    for (int i=0;i<2;++i) for (int j=0;j<3;++j) CHK(g->grid[i][j]==w.grid[i][j], "grid");
    for (int i=0;i<2;++i) { CHK(g->shape[i].x==w.shape[i].x,"shape.x"); CHK(g->shape[i].y==w.shape[i].y,"shape.y"); }
}
static void check_prim(const feat_Prim* g) {
    feat_Prim w; canon_prim(&w);
    CHK(g->i8==w.i8,"i8"); CHK(g->u8==w.u8,"u8"); CHK(g->i16==w.i16,"i16"); CHK(g->u16==w.u16,"u16");
    CHK(g->i32==w.i32,"i32"); CHK(g->u32==w.u32,"u32"); CHK(g->i64==w.i64,"i64"); CHK(g->u64==w.u64,"u64");
    CHK(g->f32==w.f32,"f32"); CHK(g->f64==w.f64,"f64"); CHK(g->b==w.b,"b"); CHK(g->o==w.o,"o"); CHK(g->ch==w.ch,"ch");
}
static void check_mutnest(const feat_MutNest* g) {
    CHK(g->tag==9, "tag"); CHK(g->leaf.u==100, "leaf.u"); CHK(g->leaf.v==1.25, "leaf.v");
    CHK(g->list._length==2, "list.len");
    if (g->list._length==2) {
        CHK(g->list._buffer[0].u==1 && g->list._buffer[0].v==0.5, "list[0]");
        CHK(g->list._buffer[1].u==2 && g->list._buffer[1].v==0.25, "list[1]");
    }
}
static void check_outerkey(const feat_OuterKey* g) {
    CHK(g->k.hi==0x01020304, "k.hi"); CHK(g->k.lo==0x05060708, "k.lo"); CHK(g->payload==999, "payload");
}
static void check_mapenum(const feat_MapEnum* g) {
    CHK(g->h==feat_H_BLUE, "h"); CHK(g->m._length==1, "m.len");
    if (g->m._length==1) CHK(g->m._buffer[0].key==3 && g->m._buffer[0].value.x==11 && g->m._buffer[0].value.y==12, "m[3]");
    CHK(g->sels._length==1, "sels.len");
    if (g->sels._length==1) CHK(g->sels._buffer[0]._d==2 && g->sels._buffer[0]._u.n==9, "sels[0]");
}
static void check_mapprim(const feat_MapPrim* g) {
    CHK(g->m._length==2, "m.len");
    if (g->m._length==2) {
        CHK(g->m._buffer[0].key==7 && g->m._buffer[0].value==42, "m[7]");
        CHK(g->m._buffer[1].key==8 && g->m._buffer[1].value==99, "m[8]");
    }
}
static void check_fixed(const feat_Money* g) {
    CHK(g->price && !strcmp(g->price,"123.45"), "price"); CHK(g->qty && !strcmp(g->qty,"1234"), "qty");
}

#define DECODE(TYPE, fn, check) do { \
        TYPE g; memset(&g,0,sizeof(g)); zdw_reader r; reader_on(&r, buf, n, e); \
        fn(&r, &g, ZDW_TC_DISCARD); \
        if (!DECODED_OK(&r)) { fprintf(stderr,"decode fail: status %d at %zu\n", (int)zdw_reader_status(&r), zdw_reader_error_offset(&r)); ++g_fail; } \
        else check(&g); \
    } while (0)

static int decode_one(const char* feat, const uint8_t* buf, size_t n, zdw_endianness e) {
    if (!strcmp(feat,"wstr")) DECODE(feat_WStr, zd_dec_feat_WStr, check_wstr);
    else if (!strcmp(feat,"mut")) DECODE(feat_Mut, zd_dec_feat_Mut, check_mut);
    else if (!strcmp(feat,"mutnest")) DECODE(feat_MutNest, zd_dec_feat_MutNest, check_mutnest);
    else if (!strcmp(feat,"outerkey")) DECODE(feat_OuterKey, zd_dec_feat_OuterKey, check_outerkey);
    else if (!strcmp(feat,"bits")) DECODE(feat_Bits, zd_dec_feat_Bits, check_bits);
    else if (!strcmp(feat,"tree")) DECODE(feat_Tree, zd_dec_feat_Tree, check_tree);
    else if (!strcmp(feat,"arr")) DECODE(feat_Arr, zd_dec_feat_Arr, check_arr);
    else if (!strcmp(feat,"prim")) DECODE(feat_Prim, zd_dec_feat_Prim, check_prim);
    else if (!strcmp(feat,"mapenum")) DECODE(feat_MapEnum, zd_dec_feat_MapEnum, check_mapenum);
    else if (!strcmp(feat,"mapprim")) DECODE(feat_MapPrim, zd_dec_feat_MapPrim, check_mapprim);
    else if (!strcmp(feat,"fixed")) DECODE(feat_Money, zd_dec_feat_Money, check_fixed);
    else { fprintf(stderr, "unknown feature %s\n", feat); return 2; }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: <feature> ENCODE|DECODE <file> | - BE\n"); return 2; }
    const char* feat = argv[1];
    const char* mode = argv[2];
    const char* file = argc > 3 ? argv[3] : NULL;

    if (strcmp(mode, "ENCODE") == 0) {
        if (!file) return 2;
        if (!strcmp(feat,"wstr")) ENCODE(feat_WStr, zd_enc_feat_WStr, canon_wstr);
        if (!strcmp(feat,"mut"))  ENCODE(feat_Mut, zd_enc_feat_Mut, canon_mut);
        if (!strcmp(feat,"mutnest")) ENCODE(feat_MutNest, zd_enc_feat_MutNest, canon_mutnest);
        if (!strcmp(feat,"outerkey")) ENCODE(feat_OuterKey, zd_enc_feat_OuterKey, canon_outerkey);
        if (!strcmp(feat,"bits")) ENCODE(feat_Bits, zd_enc_feat_Bits, canon_bits);
        if (!strcmp(feat,"tree")) ENCODE(feat_Tree, zd_enc_feat_Tree, canon_tree);
        if (!strcmp(feat,"arr"))  ENCODE(feat_Arr, zd_enc_feat_Arr, canon_arr);
        if (!strcmp(feat,"prim")) ENCODE(feat_Prim, zd_enc_feat_Prim, canon_prim);
        if (!strcmp(feat,"mapenum")) ENCODE(feat_MapEnum, zd_enc_feat_MapEnum, canon_mapenum);
        if (!strcmp(feat,"mapprim")) ENCODE(feat_MapPrim, zd_enc_feat_MapPrim, canon_mapprim);
        if (!strcmp(feat,"fixed")) ENCODE(feat_Money, zd_enc_feat_Money, canon_fixed);
        fprintf(stderr, "unknown feature\n"); return 2;
    } else if (strcmp(mode, "DECODE") == 0) {
        size_t n = 0; uint8_t* buf = file ? readfile(file, &n) : NULL;
        if (!buf) { fprintf(stderr, "missing/empty %s\n", file ? file : "(none)"); return 2; }
        int rc = decode_one(feat, buf, n, ZDW_LITTLE);
        free(buf);
        if (rc) return rc;
        if (g_fail == 0) { printf("DECODE OK: %s\n", feat); return 0; }
        printf("DECODE FAILED: %d mismatch(es)\n", g_fail); return 1;
    } else if (strcmp(mode, "BE") == 0) {
        /* Big-endian DECODE of the ZeroDDS-Rust reference big-endian wire
         * (proofs/endianness BE goldens — an independent encoder), asserting
         * the fields equal the canonical sample. */
        static const char* BE_FEATS[] = { "wstr", "mut", "bits", "tree", "arr", "prim", "mapenum" };
        for (size_t i = 0; i < sizeof(BE_FEATS)/sizeof(BE_FEATS[0]); ++i) {
            char p[256];
            snprintf(p, sizeof(p), "../../proofs/endianness/goldens/%s.xcdr2-be.rust.bin", BE_FEATS[i]);
            size_t n = 0; uint8_t* buf = readfile(p, &n);
            if (!buf) { printf("  %s be: MISSING %s\n", BE_FEATS[i], p); ++g_fail; continue; }
            int before = g_fail;
            decode_one(BE_FEATS[i], buf, n, ZDW_BIG);
            printf("  %s be: %s\n", BE_FEATS[i], g_fail == before ? "OK" : "FAIL");
            free(buf);
        }
        if (g_fail == 0) { printf("BE/LE roundtrip PASS\n"); return 0; }
        printf("BE FAILED: %d mismatch(es)\n", g_fail); return 1;
    } else if (strcmp(mode, "XCDR1") == 0) {
        /* XCDR1 DECODE of the same Rust reference goldens, both byte orders. */
        static const char* X1_FEATS[] = { "wstr", "mut", "bits", "tree", "arr", "prim", "mapenum" };
        static const char* X1_TAGS[] = { "xcdr1-le", "xcdr1-be" };
        for (size_t i = 0; i < sizeof(X1_FEATS)/sizeof(X1_FEATS[0]); ++i) {
            for (size_t t = 0; t < 2; ++t) {
                char p[256];
                snprintf(p, sizeof(p), "../../proofs/endianness/goldens/%s.%s.rust.bin", X1_FEATS[i], X1_TAGS[t]);
                size_t n = 0; uint8_t* buf = readfile(p, &n);
                if (!buf) { printf("  %s %s: MISSING %s\n", X1_FEATS[i], X1_TAGS[t], p); ++g_fail; continue; }
                int before = g_fail;
                decode_one(X1_FEATS[i], buf, n, t == 1 ? ZDW_BIG : ZDW_LITTLE);
                printf("  %s %s: %s\n", X1_FEATS[i], X1_TAGS[t], g_fail == before ? "OK" : "FAIL");
                free(buf);
            }
        }
        if (g_fail == 0) { printf("XCDR1 PASS\n"); return 0; }
        printf("XCDR1 FAILED: %d mismatch(es)\n", g_fail); return 1;
    }
    fprintf(stderr, "bad args\n"); return 2;
}
