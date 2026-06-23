// WIRE-level XCDR2 interop harness for the FEATURE corpus (features.idl):
// WStr, Mut, Bits, Tree, Arr, Prim.
//
// Two modes per feature:
//   ENCODE          -> write the CANONICAL.md sample's raw XCDR2-LE bytes to
//                      _interop/goldens/<feature>.java.bin
//   DECODE <file>   -> read raw bytes, reconstruct, assert EVERY field equals
//                      the canonical sample; exit nonzero with a diff otherwise.
//
// The bytes produced by <Feature>TypeSupport.encode(sample, LITTLE_ENDIAN) ARE
// the exact bytes DDS carries on the wire — no live pub/sub needed. The rust
// golden (<feature>.rust.bin) is the cross-PSM reference; we must produce a
// byte-identical encoding AND decode the rust golden back to the canonical.

import feat.Arr;
import feat.Bits;
import feat.Flags;
import feat.Mut;
import feat.MutLeaf;
import feat.MutNest;
import feat.NestedKey;
import feat.OuterKey;
import feat.Perm;
import feat.Prim;
import feat.Pt;
import feat.Tree;
import feat.WStr;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public final class FeaturesInterop {

    private static final String DIR =
        "/Users/sandrakessler/projects/zerodds/zerodds-examples/idl-conformance/_interop/goldens";

    // ---- canonical samples (EXACT values from CANONICAL.md) ----

    static WStr canonicalWStr() {
        WStr x = new WStr();
        x.setLabel("café");                 // c a f é (U+00E9)
        x.setText("日本語🎉"); // 日 本 語 🎉 (U+1F389)
        return x;
    }

    static Mut canonicalMut() {
        Mut x = new Mut();
        x.setA(1000000);
        x.setB(2.5);
        x.setC("ok");
        return x;
    }

    static MutLeaf mutLeaf(int u, double v) {
        MutLeaf x = new MutLeaf();
        x.setU(u);
        x.setV(v);
        return x;
    }

    static MutNest canonicalMutNest() {
        MutNest x = new MutNest();
        x.setTag(9);
        x.setLeaf(mutLeaf(100, 1.25));
        List<MutLeaf> list = new ArrayList<>();
        list.add(mutLeaf(1, 0.5));
        list.add(mutLeaf(2, 0.25));
        x.setList(list);
        return x;
    }

    static OuterKey canonicalOuterKey() {
        OuterKey x = new OuterKey();
        NestedKey k = new NestedKey();
        k.setHi(0x01020304);
        k.setLo(0x05060708);
        x.setK(k);
        x.setPayload(999);
        return x;
    }

    static Bits canonicalBits() {
        Bits x = new Bits();
        Perm perm = new Perm();
        perm.set(Perm.Flag.READ);
        perm.set(Perm.Flag.EXEC);
        x.setPerm(perm);
        Flags flags = new Flags();
        flags.setKind(5);
        flags.setPrio(20);
        x.setFlags(flags);
        return x;
    }

    static Tree canonicalTree() {
        Tree leaf = new Tree();
        leaf.setValue(4);
        leaf.setKids(new ArrayList<>());

        Tree k0 = new Tree();
        k0.setValue(2);
        List<Tree> k0kids = new ArrayList<>();
        k0kids.add(leaf);
        k0.setKids(k0kids);

        Tree k1 = new Tree();
        k1.setValue(3);
        k1.setKids(new ArrayList<>());

        Tree root = new Tree();
        root.setValue(1);
        List<Tree> kids = new ArrayList<>();
        kids.add(k0);
        kids.add(k1);
        root.setKids(kids);
        return root;
    }

    static Arr canonicalArr() {
        Arr x = new Arr();
        x.setGrid(new int[][] {{10, 11, 12}, {13, 14, 15}});
        Pt p0 = new Pt(); p0.setX(1); p0.setY(2);
        Pt p1 = new Pt(); p1.setX(3); p1.setY(4);
        x.setShape(new Pt[] {p0, p1});
        return x;
    }

    static Prim canonicalPrim() {
        Prim x = new Prim();
        x.setI8((byte) -128);
        x.setU8((short) 255);
        x.setI16((short) -32768);
        x.setU16(65535);
        x.setI32(-2147483648);
        x.setU32(4294967295L);
        x.setI64(-9223372036854775808L);
        x.setU64(-1L);                 // 0xFFFFFFFFFFFFFFFF (two's complement)
        x.setF32(3.5f);
        x.setF64(-1234.5);
        x.setB(true);
        x.setO((byte) 0xAB);
        x.setCh('Z');
        return x;
    }

    // ---- driver ----

    private static int failures = 0;

    static void eq(String what, Object expected, Object actual) {
        boolean ok = (expected == null) ? actual == null : expected.equals(actual);
        if (!ok) {
            failures++;
            System.out.println("  DIFF " + what + ": expected=" + expected + " actual=" + actual);
        }
    }

    public static void main(String[] args) throws Exception {
        String mode = args.length > 0 ? args[0] : "ROUNDTRIP";
        boolean ok = true;
        switch (mode) {
            case "ENCODE" -> ok = encodeAll();
            case "DECODE" -> ok = decodeAll();
            default -> ok = encodeAll() & decodeAll();
        }
        System.exit(ok ? 0 : 1);
    }

    static boolean encodeAll() throws Exception {
        boolean ok = true;
        ok &= enc("wstr", feat.WStrTypeSupport.INSTANCE.encode(canonicalWStr(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("mut", feat.MutTypeSupport.INSTANCE.encode(canonicalMut(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("mutnest", feat.MutNestTypeSupport.INSTANCE.encode(canonicalMutNest(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("outerkey", feat.OuterKeyTypeSupport.INSTANCE.encode(canonicalOuterKey(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("bits", feat.BitsTypeSupport.INSTANCE.encode(canonicalBits(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("tree", feat.TreeTypeSupport.INSTANCE.encode(canonicalTree(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("arr", feat.ArrTypeSupport.INSTANCE.encode(canonicalArr(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("prim", feat.PrimTypeSupport.INSTANCE.encode(canonicalPrim(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        ok &= enc("mapenum", feat.MapEnumTypeSupport.INSTANCE.encode(canonicalMapEnum(),
            org.zerodds.cdr.EndianMode.LITTLE_ENDIAN));
        return ok;
    }

    static feat.MapEnum canonicalMapEnum() {
        feat.MapEnum v = new feat.MapEnum();
        v.setH(feat.Hue.H_BLUE);
        java.util.Map<Integer, feat.Pt> m = new java.util.TreeMap<>();
        feat.Pt p = new feat.Pt(); p.setX(11); p.setY(12);
        m.put(3, p);
        v.setM(m);
        v.setSels(java.util.List.of(new feat.Sel.N(9)));
        return v;
    }

    static boolean enc(String name, byte[] bytes) throws Exception {
        Path out = Path.of(DIR, name + ".java.bin");
        Files.createDirectories(out.getParent());
        Files.write(out, bytes);
        System.out.println("ENCODE " + name + ": " + bytes.length + " bytes -> " + out);
        return true;
    }

    static boolean decodeAll() throws Exception {
        boolean ok = true;
        ok &= decWStr();
        ok &= decMut();
        ok &= decMutNest();
        ok &= decOuterKey();
        ok &= decBits();
        ok &= decTree();
        ok &= decArr();
        ok &= decPrim();
        return ok;
    }

    static byte[] golden(String name) throws Exception {
        return Files.readAllBytes(Path.of(DIR, name + ".rust.bin"));
    }

    static boolean decWStr() throws Exception {
        failures = 0;
        WStr r = feat.WStrTypeSupport.INSTANCE.decode(golden("wstr"));
        WStr c = canonicalWStr();
        eq("wstr.label", c.getLabel(), r.getLabel());
        eq("wstr.text", c.getText(), r.getText());
        return report("wstr");
    }

    static boolean decMut() throws Exception {
        failures = 0;
        Mut r = feat.MutTypeSupport.INSTANCE.decode(golden("mut"));
        Mut c = canonicalMut();
        eq("mut.a", c.getA(), r.getA());
        eq("mut.b", c.getB(), r.getB());
        eq("mut.c", c.getC(), r.getC());
        return report("mut");
    }

    static boolean decMutNest() throws Exception {
        failures = 0;
        MutNest r = feat.MutNestTypeSupport.INSTANCE.decode(golden("mutnest"));
        eq("mutnest.tag", 9, r.getTag());
        eq("mutnest.leaf.u", 100, r.getLeaf().getU());
        eq("mutnest.leaf.v", 1.25, r.getLeaf().getV());
        eq("mutnest.list.size", 2, r.getList().size());
        eq("mutnest.list[0].u", 1, r.getList().get(0).getU());
        eq("mutnest.list[0].v", 0.5, r.getList().get(0).getV());
        eq("mutnest.list[1].u", 2, r.getList().get(1).getU());
        eq("mutnest.list[1].v", 0.25, r.getList().get(1).getV());
        return report("mutnest");
    }

    static boolean decOuterKey() throws Exception {
        failures = 0;
        OuterKey r = feat.OuterKeyTypeSupport.INSTANCE.decode(golden("outerkey"));
        eq("outerkey.k.hi", 0x01020304, r.getK().getHi());
        eq("outerkey.k.lo", 0x05060708, r.getK().getLo());
        eq("outerkey.payload", 999, r.getPayload());
        return report("outerkey");
    }

    static boolean decBits() throws Exception {
        failures = 0;
        Bits r = feat.BitsTypeSupport.INSTANCE.decode(golden("bits"));
        eq("bits.perm.READ", Boolean.TRUE, r.getPerm().isSet(Perm.Flag.READ));
        eq("bits.perm.WRITE", Boolean.FALSE, r.getPerm().isSet(Perm.Flag.WRITE));
        eq("bits.perm.EXEC", Boolean.TRUE, r.getPerm().isSet(Perm.Flag.EXEC));
        eq("bits.flags.kind", 5, r.getFlags().getKind());
        eq("bits.flags.prio", 20, r.getFlags().getPrio());
        return report("bits");
    }

    static boolean decTree() throws Exception {
        failures = 0;
        Tree r = feat.TreeTypeSupport.INSTANCE.decode(golden("tree"));
        eq("tree.value", 1, r.getValue());
        eq("tree.kids.size", 2, r.getKids().size());
        Tree k0 = r.getKids().get(0);
        eq("tree.k0.value", 2, k0.getValue());
        eq("tree.k0.kids.size", 1, k0.getKids().size());
        eq("tree.k0.k0.value", 4, k0.getKids().get(0).getValue());
        eq("tree.k0.k0.kids.size", 0, k0.getKids().get(0).getKids().size());
        Tree k1 = r.getKids().get(1);
        eq("tree.k1.value", 3, k1.getValue());
        eq("tree.k1.kids.size", 0, k1.getKids().size());
        return report("tree");
    }

    static boolean decArr() throws Exception {
        failures = 0;
        Arr r = feat.ArrTypeSupport.INSTANCE.decode(golden("arr"));
        int[][] g = r.getGrid();
        int[][] eg = {{10, 11, 12}, {13, 14, 15}};
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 3; j++)
                eq("arr.grid[" + i + "][" + j + "]", eg[i][j], g[i][j]);
        Pt[] s = r.getShape();
        eq("arr.shape[0].x", 1, s[0].getX());
        eq("arr.shape[0].y", 2, s[0].getY());
        eq("arr.shape[1].x", 3, s[1].getX());
        eq("arr.shape[1].y", 4, s[1].getY());
        return report("arr");
    }

    static boolean decPrim() throws Exception {
        failures = 0;
        Prim r = feat.PrimTypeSupport.INSTANCE.decode(golden("prim"));
        Prim c = canonicalPrim();
        eq("prim.i8", c.getI8(), r.getI8());
        eq("prim.u8", c.getU8(), r.getU8());
        eq("prim.i16", c.getI16(), r.getI16());
        eq("prim.u16", c.getU16(), r.getU16());
        eq("prim.i32", c.getI32(), r.getI32());
        eq("prim.u32", c.getU32(), r.getU32());
        eq("prim.i64", c.getI64(), r.getI64());
        eq("prim.u64", c.getU64(), r.getU64());
        eq("prim.f32", c.getF32(), r.getF32());
        eq("prim.f64", c.getF64(), r.getF64());
        eq("prim.b", c.getB(), r.getB());
        eq("prim.o", c.getO(), r.getO());
        eq("prim.ch", c.getCh(), r.getCh());
        return report("prim");
    }

    static boolean report(String name) {
        if (failures == 0) {
            System.out.println("DECODE " + name + " OK");
            return true;
        }
        System.out.println("DECODE " + name + " FAILED (" + failures + " diffs)");
        return false;
    }
}
