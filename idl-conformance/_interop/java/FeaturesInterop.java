// idl-conformance / _interop / java — per-feature XCDR2-LE golden producer.
//
//   java FeaturesInterop ENCODE   -> write every canonical feat::<T> to
//                                    ../goldens/<feature>.java.bin
//   java FeaturesInterop DECODE   -> decode every ../goldens/<feature>.golden.bin
//                                    and compare with the canonical sample
//
// The types, codecs and TypeSupports are `zerodds-idlc generate --lang java`
// output (ZdTypes, ZdCodec, ZdTypeSupports) over runtime/java; values are
// CANONICAL.md.

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Objects;
import org.zerodds.cdr.EndianMode;

public final class FeaturesInterop {

    private static final Path DIR = Path.of(System.getProperty("goldens", "../goldens"));

    // ---- canonical samples (CANONICAL.md) ------------------------------

    static ZdTypes.feat.WStr canonicalWStr() {
        var x = new ZdTypes.feat.WStr();
        x.label = "café";      // c a f é (U+00E9)
        x.text = "日本語🎉";   // 日 本 語 🎉 (U+1F389)
        return x;
    }

    static ZdTypes.feat.Mut canonicalMut() {
        var x = new ZdTypes.feat.Mut();
        x.a = 1000000; x.b = 2.5; x.c = "ok";
        return x;
    }

    static ZdTypes.feat.MutLeaf mutLeaf(int u, double v) {
        var x = new ZdTypes.feat.MutLeaf();
        x.u = u; x.v = v;
        return x;
    }

    static ZdTypes.feat.MutNest canonicalMutNest() {
        var x = new ZdTypes.feat.MutNest();
        x.tag = 9;
        x.leaf = mutLeaf(100, 1.25);
        x.list = new ZdTypes.feat.MutLeaf[] {mutLeaf(1, 0.5), mutLeaf(2, 0.25)};
        return x;
    }

    static ZdTypes.feat.OuterKey canonicalOuterKey() {
        var k = new ZdTypes.feat.NestedKey();
        k.hi = 0x01020304; k.lo = 0x05060708;
        var x = new ZdTypes.feat.OuterKey();
        x.k = k; x.payload = 999;
        return x;
    }

    static ZdTypes.feat.Bits canonicalBits() {
        var flags = new ZdTypes.feat.Flags();
        flags.kind = 5; flags.prio = 20;
        var x = new ZdTypes.feat.Bits();
        x.perm = ZdTypes.feat.Perm.READ | ZdTypes.feat.Perm.EXEC;
        x.flags = flags;
        return x;
    }

    static ZdTypes.feat.Tree tree(int value, ZdTypes.feat.Tree... kids) {
        var x = new ZdTypes.feat.Tree();
        x.value = value; x.kids = kids;
        return x;
    }

    static ZdTypes.feat.Tree canonicalTree() {
        return tree(1, tree(2, tree(4)), tree(3));
    }

    static ZdTypes.feat.Pt pt(int x, int y) {
        var p = new ZdTypes.feat.Pt();
        p.x = x; p.y = y;
        return p;
    }

    static ZdTypes.feat.Arr canonicalArr() {
        var x = new ZdTypes.feat.Arr();
        x.grid = new int[][] {{10, 11, 12}, {13, 14, 15}};
        x.shape = new ZdTypes.feat.Pt[] {pt(1, 2), pt(3, 4)};
        return x;
    }

    static ZdTypes.feat.Prim canonicalPrim() {
        var x = new ZdTypes.feat.Prim();
        x.i8 = (byte) -128; x.u8 = 255; x.i16 = (short) -32768; x.u16 = 65535;
        x.i32 = -2147483648; x.u32 = 4294967295L;
        x.i64 = -9223372036854775808L; x.u64 = -1L; // 0xFFFFFFFFFFFFFFFF
        x.f32 = 3.5f; x.f64 = -1234.5; x.b = true; x.o = (byte) 0xAB; x.ch = 'Z';
        return x;
    }

    // @bit_bound enum + map<long,Pt> + sequence<Sel>: h=BLUE; m={3:{11,12}}; sels=[Sel{_d=2,n=9}].
    static ZdTypes.feat.MapEnum canonicalMapEnum() {
        var sel = new ZdTypes.feat.Sel();
        sel._d = 2; sel.n = 9;
        var x = new ZdTypes.feat.MapEnum();
        x.h = ZdTypes.feat.Hue.H_BLUE;
        x.m = new ZdTypes.ZdMap_long_feat_Pt();
        x.m.put(3, pt(11, 12));
        x.sels = new ZdTypes.feat.Sel[] {sel};
        return x;
    }

    // map<long,long> primitive-valued, no collection DHEADER (XCDR2 §7.4.3.5): m={7:42,8:99}.
    static ZdTypes.feat.MapPrim canonicalMapPrim() {
        var x = new ZdTypes.feat.MapPrim();
        x.m = new ZdTypes.ZdMap_long_long();
        x.m.put(7, 42);
        x.m.put(8, 99);
        return x;
    }

    // fixed<P,S> as its decimal text: price=123.45 (wire 12 34 5c), qty=1234 (01 23 4c).
    static ZdTypes.feat.Money canonicalMoney() {
        var x = new ZdTypes.feat.Money();
        x.price = new java.math.BigDecimal("123.45"); x.qty = new java.math.BigDecimal("1234");
        return x;
    }

    // ---- checks --------------------------------------------------------

    private static int failures = 0;

    static void check(boolean ok, String what) {
        if (!ok) {
            failures++;
            System.err.println("  [FAIL] " + what);
        }
    }

    static void checkWStr(ZdTypes.feat.WStr g) {
        check(Objects.equals(g.label, "café"), "wstr.label");
        check(Objects.equals(g.text, "日本語🎉"), "wstr.text");
    }

    static void checkMut(ZdTypes.feat.Mut g) {
        check(g.a == 1000000 && g.b == 2.5 && Objects.equals(g.c, "ok"), "mut");
    }

    static void checkMutNest(ZdTypes.feat.MutNest g) {
        check(g.tag == 9 && g.leaf.u == 100 && g.leaf.v == 1.25, "mutnest.tag/leaf");
        check(g.list.length == 2 && g.list[0].u == 1 && g.list[0].v == 0.5
                && g.list[1].u == 2 && g.list[1].v == 0.25, "mutnest.list");
    }

    static void checkOuterKey(ZdTypes.feat.OuterKey g) {
        check(g.k.hi == 0x01020304 && g.k.lo == 0x05060708 && g.payload == 999, "outerkey");
    }

    static void checkBits(ZdTypes.feat.Bits g) {
        check(g.perm == (ZdTypes.feat.Perm.READ | ZdTypes.feat.Perm.EXEC), "bits.perm");
        check(g.flags.kind == 5 && g.flags.prio == 20, "bits.flags");
    }

    static void checkTree(ZdTypes.feat.Tree g) {
        check(g.value == 1 && g.kids.length == 2, "tree.root");
        if (g.kids.length == 2) {
            check(g.kids[0].value == 2 && g.kids[0].kids.length == 1
                    && g.kids[0].kids[0].value == 4, "tree.k0");
            check(g.kids[1].value == 3 && g.kids[1].kids.length == 0, "tree.k1");
        }
    }

    static void checkArr(ZdTypes.feat.Arr g) {
        check(Arrays.deepEquals(g.grid, new int[][] {{10, 11, 12}, {13, 14, 15}}), "arr.grid");
        check(g.shape.length == 2 && g.shape[0].x == 1 && g.shape[0].y == 2
                && g.shape[1].x == 3 && g.shape[1].y == 4, "arr.shape");
    }

    static void checkPrim(ZdTypes.feat.Prim g) {
        var w = canonicalPrim();
        check(g.i8 == w.i8 && g.u8 == w.u8 && g.i16 == w.i16 && g.u16 == w.u16, "prim.8/16");
        check(g.i32 == w.i32 && g.u32 == w.u32 && g.i64 == w.i64 && g.u64 == w.u64, "prim.32/64");
        check(g.f32 == w.f32 && g.f64 == w.f64 && g.b == w.b && g.o == w.o && g.ch == w.ch, "prim.rest");
    }

    static void checkMapEnum(ZdTypes.feat.MapEnum g) {
        check(g.h == ZdTypes.feat.Hue.H_BLUE, "mapenum.h");
        var entries = new java.util.ArrayList<>(g.m.entrySet());
        check(entries.size() == 1 && entries.get(0).getKey() == 3
                && entries.get(0).getValue().x == 11 && entries.get(0).getValue().y == 12, "mapenum.m");
        check(g.sels.length == 1 && g.sels[0]._d == 2 && g.sels[0].n == 9, "mapenum.sels");
    }

    static void checkMapPrim(ZdTypes.feat.MapPrim g) {
        var entries = new java.util.ArrayList<>(g.m.entrySet());
        check(entries.size() == 2 && entries.get(0).getKey() == 7 && entries.get(0).getValue() == 42
                && entries.get(1).getKey() == 8 && entries.get(1).getValue() == 99, "mapprim.m");
    }

    static void checkMoney(ZdTypes.feat.Money g) {
        check(Objects.equals(g.price, new java.math.BigDecimal("123.45"))
                && Objects.equals(g.qty, new java.math.BigDecimal("1234")), "fixed");
    }

    // ---- drivers -------------------------------------------------------

    static final EndianMode LE = EndianMode.LITTLE_ENDIAN;

    static void encodeTo(String feature, byte[] bytes) throws Exception {
        Path out = DIR.resolve(feature + ".java.bin");
        Files.write(out, bytes);
        System.out.println("ENCODE " + feature + ": " + bytes.length + " bytes -> " + out);
    }

    static byte[] golden(String feature) throws Exception {
        return Files.readAllBytes(DIR.resolve(feature + ".golden.bin"));
    }

    static void encodeAll() throws Exception {
        encodeTo("wstr", ZdTypeSupports.Support_feat_WStr.INSTANCE.encode(canonicalWStr(), LE, 2));
        encodeTo("mut", ZdTypeSupports.Support_feat_Mut.INSTANCE.encode(canonicalMut(), LE, 2));
        encodeTo("mutnest", ZdTypeSupports.Support_feat_MutNest.INSTANCE.encode(canonicalMutNest(), LE, 2));
        encodeTo("outerkey", ZdTypeSupports.Support_feat_OuterKey.INSTANCE.encode(canonicalOuterKey(), LE, 2));
        encodeTo("bits", ZdTypeSupports.Support_feat_Bits.INSTANCE.encode(canonicalBits(), LE, 2));
        encodeTo("tree", ZdTypeSupports.Support_feat_Tree.INSTANCE.encode(canonicalTree(), LE, 2));
        encodeTo("arr", ZdTypeSupports.Support_feat_Arr.INSTANCE.encode(canonicalArr(), LE, 2));
        encodeTo("prim", ZdTypeSupports.Support_feat_Prim.INSTANCE.encode(canonicalPrim(), LE, 2));
        encodeTo("mapenum", ZdTypeSupports.Support_feat_MapEnum.INSTANCE.encode(canonicalMapEnum(), LE, 2));
        encodeTo("mapprim", ZdTypeSupports.Support_feat_MapPrim.INSTANCE.encode(canonicalMapPrim(), LE, 2));
        encodeTo("fixed", ZdTypeSupports.Support_feat_Money.INSTANCE.encode(canonicalMoney(), LE, 2));
    }

    static void decodeAll() throws Exception {
        checkWStr(ZdTypeSupports.Support_feat_WStr.INSTANCE.decode(golden("wstr")));
        checkMut(ZdTypeSupports.Support_feat_Mut.INSTANCE.decode(golden("mut")));
        checkMutNest(ZdTypeSupports.Support_feat_MutNest.INSTANCE.decode(golden("mutnest")));
        checkOuterKey(ZdTypeSupports.Support_feat_OuterKey.INSTANCE.decode(golden("outerkey")));
        checkBits(ZdTypeSupports.Support_feat_Bits.INSTANCE.decode(golden("bits")));
        checkTree(ZdTypeSupports.Support_feat_Tree.INSTANCE.decode(golden("tree")));
        checkArr(ZdTypeSupports.Support_feat_Arr.INSTANCE.decode(golden("arr")));
        checkPrim(ZdTypeSupports.Support_feat_Prim.INSTANCE.decode(golden("prim")));
        checkMapEnum(ZdTypeSupports.Support_feat_MapEnum.INSTANCE.decode(golden("mapenum")));
        checkMapPrim(ZdTypeSupports.Support_feat_MapPrim.INSTANCE.decode(golden("mapprim")));
        checkMoney(ZdTypeSupports.Support_feat_Money.INSTANCE.decode(golden("fixed")));
    }

    // BE: the big-endian goldens of ../../proofs/endianness, the Rust column's
    // encodings of the same samples, for the seven features that proof covers.
    static final Path REPRESENTATIONS =
            Path.of(System.getProperty("representations", "../../proofs/endianness/goldens"));

    static byte[] bigEndian(String feature) throws Exception {
        return Files.readAllBytes(REPRESENTATIONS.resolve(feature + ".xcdr2-be.rust.bin"));
    }

    static void decodeBigEndian() throws Exception {
        final EndianMode BE = EndianMode.BIG_ENDIAN;
        byte[] b;
        b = bigEndian("wstr");
        checkWStr(ZdTypeSupports.Support_feat_WStr.INSTANCE.decode(b, 0, b.length, BE, 2));
        b = bigEndian("mut");
        checkMut(ZdTypeSupports.Support_feat_Mut.INSTANCE.decode(b, 0, b.length, BE, 2));
        b = bigEndian("bits");
        checkBits(ZdTypeSupports.Support_feat_Bits.INSTANCE.decode(b, 0, b.length, BE, 2));
        b = bigEndian("tree");
        checkTree(ZdTypeSupports.Support_feat_Tree.INSTANCE.decode(b, 0, b.length, BE, 2));
        b = bigEndian("arr");
        checkArr(ZdTypeSupports.Support_feat_Arr.INSTANCE.decode(b, 0, b.length, BE, 2));
        b = bigEndian("prim");
        checkPrim(ZdTypeSupports.Support_feat_Prim.INSTANCE.decode(b, 0, b.length, BE, 2));
        b = bigEndian("mapenum");
        checkMapEnum(ZdTypeSupports.Support_feat_MapEnum.INSTANCE.decode(b, 0, b.length, BE, 2));
    }

    public static void main(String[] args) throws Exception {
        String mode = args.length > 0 ? args[0] : "ROUNDTRIP";
        if (mode.equals("BE")) {
            decodeBigEndian();
            if (failures != 0) {
                System.out.println("BE FAILED: " + failures + " mismatch(es)");
                System.exit(1);
            }
            System.out.println("BE OK: every big-endian golden decodes to the canonical sample");
            return;
        }
        if (mode.equals("ENCODE") || mode.equals("ROUNDTRIP")) {
            encodeAll();
        }
        if (mode.equals("DECODE") || mode.equals("ROUNDTRIP")) {
            decodeAll();
            if (failures != 0) {
                System.out.println("DECODE FAILED: " + failures + " mismatch(es)");
                System.exit(1);
            }
            System.out.println("DECODE OK: every golden decodes to the canonical sample");
        }
    }
}
