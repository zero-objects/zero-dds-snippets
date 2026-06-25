// WIRE-level XCDR2 interop goldens for the C# PSM — the FEATURE corpus
// (features.idl): WStr, Mut, Bits, Tree, Arr, Prim. Encodes the CANONICAL.md
// samples to goldens/<feature>.csharp.bin and decodes goldens/<feature>.rust.bin
// asserting every field == the canonical sample.
//
//   ENCODE  -> write every <feature>.csharp.bin
//   DECODE  -> read every <feature>.rust.bin, assert == canonical
//
// The Rust golden is the cross-PSM reference (CANONICAL.md "Convergence
// criterion"); the C# bytes must be byte-identical to it.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using ZeroDDS.Cdr;
using Omg.Types;
using feat;

const string Dir =
    "/Users/sandrakessler/projects/zerodds/zerodds-examples/idl-conformance/_interop/goldens";

// ---- canonical samples (EXACT values from CANONICAL.md) --------------------

static WStr CanonicalWStr() => new WStr
{
    Label = "café",                 // c a f é (U+00E9)
    Text = "日本語\U0001F389", // 日 本 語 🎉
};

static Mut CanonicalMut() => new Mut { A = 1000000, B = 2.5, C = "ok" };

// nested @mutable: tag=9, leaf={u=100,v=1.25}, list=[{u=1,v=0.5},{u=2,v=0.25}].
// Member 20 (leaf, nested @mutable) and 30 (list, sequence<@mutable>) ride the
// EMHEADER LengthCode-5 reuse path; member 10 (tag, long) is LC2.
static MutNest CanonicalMutNest() => new MutNest
{
    Tag = 9,
    Leaf = new MutLeaf { U = 100, V = 1.25 },
    List = new SequenceList<MutLeaf>(new[]
    {
        new MutLeaf { U = 1, V = 0.5 },
        new MutLeaf { U = 2, V = 0.25 },
    }),
};

// nested-struct @key: OuterKey{k={hi=0x01020304,lo=0x05060708}, payload=999}.
static OuterKey CanonicalOuterKey() => new OuterKey
{
    K = new NestedKey { Hi = 0x01020304, Lo = 0x05060708 },
    Payload = 999,
};

static Bits CanonicalBits()
{
    var flags = new Flags();
    flags.Kind = 5;
    flags.Prio = 20;
    return new Bits { Perm = Perm.READ | Perm.EXEC, Flags = flags };
}

static Tree CanonicalTree() => new Tree
{
    Value = 1,
    Kids = new SequenceList<Tree>(new[]
    {
        new Tree
        {
            Value = 2,
            Kids = new SequenceList<Tree>(new[]
            {
                new Tree { Value = 4, Kids = new SequenceList<Tree>() },
            }),
        },
        new Tree { Value = 3, Kids = new SequenceList<Tree>() },
    }),
};

static Arr CanonicalArr() => new Arr
{
    Grid = new[] { new[] { 10, 11, 12 }, new[] { 13, 14, 15 } },
    Shape = new[] { new Pt { X = 1, Y = 2 }, new Pt { X = 3, Y = 4 } },
};

static Prim CanonicalPrim() => new Prim
{
    I8 = -128,
    U8 = 255,
    I16 = -32768,
    U16 = 65535,
    I32 = -2147483648,
    U32 = 4294967295u,
    I64 = -9223372036854775808L,
    U64 = 18446744073709551615UL,
    F32 = 3.5f,
    F64 = -1234.5,
    B = true,
    O = 0xAB,
    Ch = 'Z',
};

// @bit_bound(16) enum + map<long,Pt> + sequence<Sel>: h=H_BLUE; m={3:{11,12}}; sels=[Sel(n=9)].
static MapEnum CanonicalMapEnum() => new MapEnum
{
    H = Hue.H_BLUE,
    M = new System.Collections.Generic.SortedDictionary<int, Pt> { [3] = new Pt { X = 11, Y = 12 } },
    Sels = new SequenceList<Sel>(new[] { new Sel { Discriminator = 2, Value = 9 } }),
};

// map<long,long> primitive-valued -> NO collection DHEADER (XCDR2 §7.4.3.5). m={7:42,8:99}.
static MapPrim CanonicalMapPrim() => new MapPrim
{
    M = new System.Collections.Generic.SortedDictionary<int, int> { [7] = 42, [8] = 99 },
};

// ---- encode -----------------------------------------------------------------

static void WriteGolden(string name, byte[] bytes)
{
    Directory.CreateDirectory(Dir);
    var path = Path.Combine(Dir, $"{name}.csharp.bin");
    File.WriteAllBytes(path, bytes);
    Console.WriteLine($"ENCODE {name}: {bytes.Length} bytes -> {path}");
    Console.WriteLine("  hex: " + string.Concat(bytes.Select(b => b.ToString("x2"))));
}

static int RunEncode()
{
    WriteGolden("wstr", WStrTypeSupport.Instance.Encode(CanonicalWStr(), EndianMode.LittleEndian));
    WriteGolden("mut", MutTypeSupport.Instance.Encode(CanonicalMut(), EndianMode.LittleEndian));
    WriteGolden("mutnest", MutNestTypeSupport.Instance.Encode(CanonicalMutNest(), EndianMode.LittleEndian));
    WriteGolden("outerkey", OuterKeyTypeSupport.Instance.Encode(CanonicalOuterKey(), EndianMode.LittleEndian));
    WriteGolden("bits", BitsTypeSupport.Instance.Encode(CanonicalBits(), EndianMode.LittleEndian));
    WriteGolden("tree", TreeTypeSupport.Instance.Encode(CanonicalTree(), EndianMode.LittleEndian));
    WriteGolden("arr", ArrTypeSupport.Instance.Encode(CanonicalArr(), EndianMode.LittleEndian));
    WriteGolden("prim", PrimTypeSupport.Instance.Encode(CanonicalPrim(), EndianMode.LittleEndian));
    WriteGolden("mapenum", MapEnumTypeSupport.Instance.Encode(CanonicalMapEnum(), EndianMode.LittleEndian));
    WriteGolden("mapprim", MapPrimTypeSupport.Instance.Encode(CanonicalMapPrim(), EndianMode.LittleEndian));
    return 0;
}

// ---- decode the RUST golden, assert == canonical ----------------------------

static int RunDecode()
{
    var fails = new List<string>();

    void Check(string label, bool ok)
    {
        Console.WriteLine($"  {(ok ? "OK  " : "FAIL")} {label}");
        if (!ok) fails.Add(label);
    }

    byte[] Read(string name) => File.ReadAllBytes(Path.Combine(Dir, $"{name}.rust.bin"));

    // wstr
    {
        var got = WStrTypeSupport.Instance.Decode(Read("wstr"));
        var want = CanonicalWStr();
        Check($"wstr.label ({got.Label})", got.Label == want.Label);
        Check($"wstr.text ({got.Text})", got.Text == want.Text);
    }
    // mut
    {
        var got = MutTypeSupport.Instance.Decode(Read("mut"));
        var want = CanonicalMut();
        Check($"mut.a ({got.A})", got.A == want.A);
        Check($"mut.b ({got.B})", got.B == want.B);
        Check($"mut.c ({got.C})", got.C == want.C);
    }
    // mutnest (nested @mutable + sequence<@mutable> via LC5 EMHEADER reuse)
    {
        var got = MutNestTypeSupport.Instance.Decode(Read("mutnest"));
        var want = CanonicalMutNest();
        Check($"mutnest.tag ({got.Tag})", got.Tag == want.Tag);
        Check($"mutnest.leaf.u ({got.Leaf.U})", got.Leaf.U == want.Leaf.U);
        Check($"mutnest.leaf.v ({got.Leaf.V})", got.Leaf.V == want.Leaf.V);
        var list = got.List.ToList();
        Check($"mutnest.list.count ({list.Count})", list.Count == 2);
        if (list.Count == 2)
        {
            Check($"mutnest.list[0].u ({list[0].U})", list[0].U == 1);
            Check($"mutnest.list[0].v ({list[0].V})", list[0].V == 0.5);
            Check($"mutnest.list[1].u ({list[1].U})", list[1].U == 2);
            Check($"mutnest.list[1].v ({list[1].V})", list[1].V == 0.25);
        }
    }
    // outerkey (nested-struct @key)
    {
        var got = OuterKeyTypeSupport.Instance.Decode(Read("outerkey"));
        var want = CanonicalOuterKey();
        Check($"outerkey.k.hi ({got.K.Hi:x})", got.K.Hi == want.K.Hi);
        Check($"outerkey.k.lo ({got.K.Lo:x})", got.K.Lo == want.K.Lo);
        Check($"outerkey.payload ({got.Payload})", got.Payload == want.Payload);
    }
    // bits
    {
        var got = BitsTypeSupport.Instance.Decode(Read("bits"));
        var want = CanonicalBits();
        Check($"bits.perm ({(uint)got.Perm:x})", got.Perm == want.Perm);
        Check($"bits.flags.kind ({got.Flags.Kind})", got.Flags.Kind == want.Flags.Kind);
        Check($"bits.flags.prio ({got.Flags.Prio})", got.Flags.Prio == want.Flags.Prio);
        Check($"bits.flags.holder ({got.Flags.Value:x})", got.Flags.Value == want.Flags.Value);
    }
    // tree
    {
        var got = TreeTypeSupport.Instance.Decode(Read("tree"));
        Check($"tree.value ({got.Value})", got.Value == 1);
        var kids = got.Kids.ToList();
        Check($"tree.kids.count ({kids.Count})", kids.Count == 2);
        if (kids.Count == 2)
        {
            Check($"tree.kid0.value ({kids[0].Value})", kids[0].Value == 2);
            var k0 = kids[0].Kids.ToList();
            Check($"tree.kid0.kids.count ({k0.Count})", k0.Count == 1);
            if (k0.Count == 1)
            {
                Check($"tree.kid0.kid0.value ({k0[0].Value})", k0[0].Value == 4);
                Check($"tree.kid0.kid0.kids.empty", k0[0].Kids.ToList().Count == 0);
            }
            Check($"tree.kid1.value ({kids[1].Value})", kids[1].Value == 3);
            Check($"tree.kid1.kids.empty", kids[1].Kids.ToList().Count == 0);
        }
    }
    // arr
    {
        var got = ArrTypeSupport.Instance.Decode(Read("arr"));
        var want = CanonicalArr();
        bool gridOk = got.Grid.Length == 2;
        for (int i = 0; gridOk && i < 2; i++)
            for (int j = 0; j < 3; j++)
                gridOk &= got.Grid[i][j] == want.Grid[i][j];
        Check("arr.grid", gridOk);
        bool shapeOk = got.Shape.Length == 2
            && got.Shape[0].X == 1 && got.Shape[0].Y == 2
            && got.Shape[1].X == 3 && got.Shape[1].Y == 4;
        Check("arr.shape", shapeOk);
    }
    // prim
    {
        var got = PrimTypeSupport.Instance.Decode(Read("prim"));
        var w = CanonicalPrim();
        Check($"prim.i8 ({got.I8})", got.I8 == w.I8);
        Check($"prim.u8 ({got.U8})", got.U8 == w.U8);
        Check($"prim.i16 ({got.I16})", got.I16 == w.I16);
        Check($"prim.u16 ({got.U16})", got.U16 == w.U16);
        Check($"prim.i32 ({got.I32})", got.I32 == w.I32);
        Check($"prim.u32 ({got.U32})", got.U32 == w.U32);
        Check($"prim.i64 ({got.I64})", got.I64 == w.I64);
        Check($"prim.u64 ({got.U64})", got.U64 == w.U64);
        Check($"prim.f32 ({got.F32})", got.F32 == w.F32);
        Check($"prim.f64 ({got.F64})", got.F64 == w.F64);
        Check($"prim.b ({got.B})", got.B == w.B);
        Check($"prim.o ({got.O:x})", got.O == w.O);
        Check($"prim.ch ({got.Ch})", got.Ch == w.Ch);
    }

    if (fails.Count == 0)
    {
        Console.WriteLine("\nDECODE PASS — every feature golden decodes to its canonical sample.");
        return 0;
    }
    Console.Error.WriteLine($"\nDECODE FAIL — {fails.Count}: {string.Join(", ", fails)}");
    return 1;
}

// Big-endian end-to-end: encode each canonical in BOTH byte orders and decode
// it back through the new Decode(bytes, endian) overload, asserting equality.
// Proves the C# binding decodes big-endian, not just little-endian.
static int RunBe()
{
    var fails = new List<string>();
    void Rt<T>(string name, IDdsTopicType<T> ts, T v, Func<T, T, bool> eq)
    {
        foreach (var e in new[] { EndianMode.LittleEndian, EndianMode.BigEndian })
        {
            var bytes = ts.Encode(v, e);
            var back = ts.Decode(bytes, e);
            bool ok = eq(v, back);
            Console.WriteLine($"  {name} {(e == EndianMode.LittleEndian ? "le" : "be")}: {(ok ? "OK" : "FAIL")}");
            if (!ok) fails.Add($"{name}/{e}");
        }
    }
    Rt("wstr", WStrTypeSupport.Instance, CanonicalWStr(), (a, b) => a.Label == b.Label && a.Text == b.Text);
    Rt("mut", MutTypeSupport.Instance, CanonicalMut(), (a, b) => a.A == b.A && a.B == b.B && a.C == b.C);
    Rt("mapenum", MapEnumTypeSupport.Instance, CanonicalMapEnum(),
        (a, b) => a.H == b.H && a.M.Count == b.M.Count && a.Sels.Count() == b.Sels.Count());
    Rt("arr", ArrTypeSupport.Instance, CanonicalArr(),
        (a, b) => a.Shape[0].X == b.Shape[0].X && a.Grid[1][2] == b.Grid[1][2]);
    if (fails.Count == 0) { Console.WriteLine("BE/LE roundtrip PASS"); return 0; }
    Console.Error.WriteLine($"BE roundtrip FAIL: {string.Join(", ", fails)}");
    return 1;
}

// XCDR1 DECODE: read the Rust XCDR1 (classic CDR / PL_CDR1) reference goldens
// and decode them through Decode(bytes, endian, representation=0), asserting
// every field == canonical. Proves the C# binding reads the second wire
// representation (no DHEADER on @appendable/@final, 8-byte align, PL_CDR1 for
// @mutable).
static int RunXcdr1()
{
    string dir = Path.Combine(Dir, "..", "..", "proofs", "endianness", "goldens");
    var fails = new List<string>();
    void Dec<T>(string name, IDdsTopicType<T> ts, T v, Func<T, T, bool> eq)
    {
        string path = Path.Combine(dir, $"{name}.xcdr1-le.rust.bin");
        if (!File.Exists(path)) { Console.WriteLine($"  {name} xcdr1: SKIP"); return; }
        var golden = File.ReadAllBytes(path);
        // DECODE.
        var back = ts.Decode(golden, EndianMode.LittleEndian, 0);
        bool ok = eq(v, back);
        Console.WriteLine($"  {name} xcdr1 decode: {(ok ? "OK" : "FAIL")}");
        if (!ok) fails.Add(name + " decode");
        // ENCODE: must be byte-identical to the golden.
        var wire = ts.Encode(v, EndianMode.LittleEndian, 0);
        bool encOk = wire.AsSpan().SequenceEqual(golden);
        Console.WriteLine($"  {name} xcdr1 encode: {(encOk ? "OK" : "FAIL")} ({wire.Length}B vs {golden.Length}B)");
        if (!encOk) fails.Add(name + " encode");
    }
    Dec("wstr", WStrTypeSupport.Instance, CanonicalWStr(), (a, b) => a.Label == b.Label && a.Text == b.Text);
    Dec("mut", MutTypeSupport.Instance, CanonicalMut(), (a, b) => a.A == b.A && a.B == b.B && a.C == b.C);
    Dec("mapenum", MapEnumTypeSupport.Instance, CanonicalMapEnum(),
        (a, b) => a.H == b.H && a.M.Count == b.M.Count && a.Sels.Count() == b.Sels.Count());
    Dec("tree", TreeTypeSupport.Instance, CanonicalTree(), (a, b) => a.Value == b.Value && a.Kids.Count == b.Kids.Count);
    Dec("arr", ArrTypeSupport.Instance, CanonicalArr(),
        (a, b) => a.Shape[0].X == b.Shape[0].X && a.Grid[1][2] == b.Grid[1][2]);
    Dec("prim", PrimTypeSupport.Instance, CanonicalPrim(), (a, b) => a.I64 == b.I64 && a.F64 == b.F64);
    if (fails.Count == 0) { Console.WriteLine("XCDR1 DECODE PASS"); return 0; }
    Console.Error.WriteLine($"XCDR1 DECODE FAIL: {string.Join(", ", fails)}");
    return 1;
}

var mode = args.Length >= 1 ? args[0] : "ROUNDTRIP";
switch (mode)
{
    case "ENCODE": return RunEncode();
    case "DECODE": return RunDecode();
    case "BE": return RunBe();
    case "XCDR1": return RunXcdr1();
    case "ROUNDTRIP": { var a = RunEncode(); var b = RunDecode(); return a != 0 ? a : b; }
    default:
        Console.Error.WriteLine("usage: interop-csharp-features ENCODE | DECODE | BE | XCDR1 | ROUNDTRIP");
        return 2;
}
