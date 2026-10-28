// idl-conformance / _interop / csharp-features — per-feature XCDR2-LE golden
// producer for the shared corpus (features.idl).
//
//   dotnet interop-csharp-features.dll ENCODE  -> ../goldens/<feature>.csharp.bin
//   dotnet interop-csharp-features.dll DECODE  -> decode ../goldens/<feature>.golden.bin,
//                                                compare with the canonical sample
//
// features.cs and features.TypeSupport.cs are `zerodds-idlc generate --lang
// csharp` output over runtime/csharp; values are CANONICAL.md.

using System;
using System.IO;
using System.Linq;
using ZeroDDS.Cdr;
using feat;

string dir = Environment.GetEnvironmentVariable("GOLDENS") ?? Path.Combine("..", "goldens");
int failures = 0;

void Check(bool ok, string what)
{
    if (!ok)
    {
        failures++;
        Console.Error.WriteLine($"  [FAIL] {what}");
    }
}

// ---- canonical samples (CANONICAL.md) ---------------------------------

WStr CanonicalWStr() => new WStr { Label = "café", Text = "日本語🎉" };
Mut CanonicalMut() => new Mut { A = 1000000, B = 2.5, C = "ok" };
MutLeaf Leaf(int u, double v) => new MutLeaf { U = u, V = v };
MutNest CanonicalMutNest() => new MutNest { Tag = 9, Leaf = Leaf(100, 1.25), List = new[] { Leaf(1, 0.5), Leaf(2, 0.25) } };
OuterKey CanonicalOuterKey() => new OuterKey { K = new NestedKey { Hi = 0x01020304, Lo = 0x05060708 }, Payload = 999 };
Bits CanonicalBits() => new Bits { Perm = Perm.READ | Perm.EXEC, Flags = new Flags { Kind = 5, Prio = 20 } };
Tree Node(int value, params Tree[] kids) => new Tree { Value = value, Kids = kids };
Tree CanonicalTree() => Node(1, Node(2, Node(4)), Node(3));
Pt P(int x, int y) => new Pt { X = x, Y = y };
Arr CanonicalArr() => new Arr { Grid = new[] { new[] { 10, 11, 12 }, new[] { 13, 14, 15 } }, Shape = new[] { P(1, 2), P(3, 4) } };
Prim CanonicalPrim() => new Prim
{
    I8 = -128, U8 = 255, I16 = -32768, U16 = 65535, I32 = int.MinValue, U32 = uint.MaxValue,
    I64 = long.MinValue, U64 = ulong.MaxValue, F32 = 3.5f, F64 = -1234.5, B = true, O = 0xAB, Ch = 'Z',
};
// @bit_bound enum + map<long,Pt> + sequence<Sel>: h=BLUE; m={3:{11,12}}; sels=[Sel{_d=2,n=9}].
MapEnum CanonicalMapEnum() => new MapEnum
{
    H = Hue.H_BLUE,
    M = new[] { new ZdEntry_long_feat_Pt { Key = 3, Value = P(11, 12) } },
    Sels = new[] { new Sel { _d = 2, N = 9 } },
};
// map<long,long> primitive-valued, no collection DHEADER (XCDR2 §7.4.3.5): m={7:42,8:99}.
MapPrim CanonicalMapPrim() => new MapPrim
{
    M = new[] { new ZdEntry_long_long { Key = 7, Value = 42 }, new ZdEntry_long_long { Key = 8, Value = 99 } },
};
// fixed<P,S> as its decimal text: price=123.45 (wire 12 34 5c), qty=1234 (01 23 4c).
Money CanonicalMoney() => new Money { Price = "123.45", Qty = "1234" };

// ---- drivers -------------------------------------------------------------

void Write(string feature, byte[] bytes)
{
    string path = Path.Combine(dir, $"{feature}.csharp.bin");
    File.WriteAllBytes(path, bytes);
    Console.WriteLine($"ENCODE {feature}: {bytes.Length} bytes -> {path}");
}

byte[] Golden(string feature) => File.ReadAllBytes(Path.Combine(dir, $"{feature}.golden.bin"));

const EndianMode LE = EndianMode.LittleEndian;

// BE: the big-endian goldens of ../../proofs/endianness, the Rust column's
// encodings of the same samples, for the seven features that proof covers.
string representations = Environment.GetEnvironmentVariable("REPRESENTATIONS")
    ?? Path.Combine("..", "..", "proofs", "endianness", "goldens");
string[] beFeatures = { "wstr", "mut", "bits", "tree", "arr", "prim", "mapenum" };

string mode = args.Length > 0 ? args[0] : "ENCODE";
if (mode == "ENCODE")
{
    Write("wstr", WStrTypeSupport.Instance.Encode(CanonicalWStr(), LE));
    Write("mut", MutTypeSupport.Instance.Encode(CanonicalMut(), LE));
    Write("mutnest", MutNestTypeSupport.Instance.Encode(CanonicalMutNest(), LE));
    Write("outerkey", OuterKeyTypeSupport.Instance.Encode(CanonicalOuterKey(), LE));
    Write("bits", BitsTypeSupport.Instance.Encode(CanonicalBits(), LE));
    Write("tree", TreeTypeSupport.Instance.Encode(CanonicalTree(), LE));
    Write("arr", ArrTypeSupport.Instance.Encode(CanonicalArr(), LE));
    Write("prim", PrimTypeSupport.Instance.Encode(CanonicalPrim(), LE));
    Write("mapenum", MapEnumTypeSupport.Instance.Encode(CanonicalMapEnum(), LE));
    Write("mapprim", MapPrimTypeSupport.Instance.Encode(CanonicalMapPrim(), LE));
    Write("fixed", MoneyTypeSupport.Instance.Encode(CanonicalMoney(), LE));
    return 0;
}
if (mode != "DECODE" && mode != "BE")
{
    Console.Error.WriteLine("usage: ENCODE | DECODE | BE");
    return 2;
}
bool big = mode == "BE";
EndianMode endian = big ? EndianMode.BigEndian : LE;
bool Wants(string feature) => !big || beFeatures.Contains(feature);
byte[] Source(string feature) => big
    ? File.ReadAllBytes(Path.Combine(representations, $"{feature}.xcdr2-be.rust.bin"))
    : Golden(feature);

if (Wants("wstr"))
{
    var g = WStrTypeSupport.Instance.Decode(Source("wstr"), endian);
    Check(g.Label == "café" && g.Text == "日本語🎉", "wstr");
}
if (Wants("mut"))
{
    var g = MutTypeSupport.Instance.Decode(Source("mut"), endian);
    Check(g.A == 1000000 && g.B == 2.5 && g.C == "ok", "mut");
}
if (Wants("mutnest"))
{
    var g = MutNestTypeSupport.Instance.Decode(Source("mutnest"), endian);
    Check(g.Tag == 9 && g.Leaf.U == 100 && g.Leaf.V == 1.25, "mutnest.tag/leaf");
    Check(g.List.Length == 2 && g.List[0].U == 1 && g.List[0].V == 0.5 && g.List[1].U == 2 && g.List[1].V == 0.25, "mutnest.list");
}
if (Wants("outerkey"))
{
    var g = OuterKeyTypeSupport.Instance.Decode(Source("outerkey"), endian);
    Check(g.K.Hi == 0x01020304 && g.K.Lo == 0x05060708 && g.Payload == 999, "outerkey");
}
if (Wants("bits"))
{
    var g = BitsTypeSupport.Instance.Decode(Source("bits"), endian);
    Check(g.Perm == (Perm.READ | Perm.EXEC), "bits.perm");
    Check(g.Flags.Kind == 5 && g.Flags.Prio == 20, "bits.flags");
}
if (Wants("tree"))
{
    var g = TreeTypeSupport.Instance.Decode(Source("tree"), endian);
    Check(g.Value == 1 && g.Kids.Length == 2, "tree.root");
    if (g.Kids.Length == 2)
    {
        Check(g.Kids[0].Value == 2 && g.Kids[0].Kids.Length == 1 && g.Kids[0].Kids[0].Value == 4, "tree.k0");
        Check(g.Kids[1].Value == 3 && g.Kids[1].Kids.Length == 0, "tree.k1");
    }
}
if (Wants("arr"))
{
    var g = ArrTypeSupport.Instance.Decode(Source("arr"), endian);
    Check(g.Grid.SelectMany(row => row).SequenceEqual(new[] { 10, 11, 12, 13, 14, 15 }), "arr.grid");
    Check(g.Shape.Length == 2 && g.Shape[0].X == 1 && g.Shape[0].Y == 2 && g.Shape[1].X == 3 && g.Shape[1].Y == 4, "arr.shape");
}
if (Wants("prim"))
{
    var g = PrimTypeSupport.Instance.Decode(Source("prim"), endian);
    var w = CanonicalPrim();
    Check(g.I8 == w.I8 && g.U8 == w.U8 && g.I16 == w.I16 && g.U16 == w.U16, "prim.8/16");
    Check(g.I32 == w.I32 && g.U32 == w.U32 && g.I64 == w.I64 && g.U64 == w.U64, "prim.32/64");
    Check(g.F32 == w.F32 && g.F64 == w.F64 && g.B == w.B && g.O == w.O && g.Ch == w.Ch, "prim.rest");
}
if (Wants("mapenum"))
{
    var g = MapEnumTypeSupport.Instance.Decode(Source("mapenum"), endian);
    Check(g.H == Hue.H_BLUE, "mapenum.h");
    Check(g.M.Length == 1 && g.M[0].Key == 3 && g.M[0].Value.X == 11 && g.M[0].Value.Y == 12, "mapenum.m");
    Check(g.Sels.Length == 1 && g.Sels[0]._d == 2 && g.Sels[0].N == 9, "mapenum.sels");
}
if (Wants("mapprim"))
{
    var g = MapPrimTypeSupport.Instance.Decode(Source("mapprim"), endian);
    Check(g.M.Length == 2 && g.M[0].Key == 7 && g.M[0].Value == 42 && g.M[1].Key == 8 && g.M[1].Value == 99, "mapprim.m");
}
if (Wants("fixed"))
{
    var g = MoneyTypeSupport.Instance.Decode(Source("fixed"), endian);
    Check(g.Price == "123.45" && g.Qty == "1234", "fixed");
}

if (failures != 0)
{
    Console.WriteLine($"{mode} FAILED: {failures} mismatch(es)");
    return 1;
}
Console.WriteLine(big
    ? "BE OK: every big-endian golden decodes to the canonical sample"
    : "DECODE OK: every golden decodes to the canonical sample");
return 0;
