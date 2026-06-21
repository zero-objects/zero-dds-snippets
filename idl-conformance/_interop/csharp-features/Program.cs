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
    WriteGolden("bits", BitsTypeSupport.Instance.Encode(CanonicalBits(), EndianMode.LittleEndian));
    WriteGolden("tree", TreeTypeSupport.Instance.Encode(CanonicalTree(), EndianMode.LittleEndian));
    WriteGolden("arr", ArrTypeSupport.Instance.Encode(CanonicalArr(), EndianMode.LittleEndian));
    WriteGolden("prim", PrimTypeSupport.Instance.Encode(CanonicalPrim(), EndianMode.LittleEndian));
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

var mode = args.Length >= 1 ? args[0] : "ROUNDTRIP";
switch (mode)
{
    case "ENCODE": return RunEncode();
    case "DECODE": return RunDecode();
    case "ROUNDTRIP": { var a = RunEncode(); var b = RunDecode(); return a != 0 ? a : b; }
    default:
        Console.Error.WriteLine("usage: interop-csharp-features ENCODE | DECODE | ROUNDTRIP");
        return 2;
}
