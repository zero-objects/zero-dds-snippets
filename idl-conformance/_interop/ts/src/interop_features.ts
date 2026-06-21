// SPDX-License-Identifier: Apache-2.0
//
// idl-conformance / _interop / ts — WIRE-level XCDR2 interop goldens for the
// FEATURE corpus (features.idl): WStr, Mut, Bits, Tree, Arr, Prim.
//
//   ENCODE        -> writes each canonical sample's raw XCDR2-LE bytes (the
//                    exact bytes a ZeroDDS DataWriter puts on the wire) to
//                    goldens/<feature>.ts.bin.
//   DECODE <lang> -> reads goldens/<feature>.<lang>.bin back through the
//                    generated codec and asserts every field == canonical.
//
// The Rust golden is the cross-PSM reference (CANONICAL.md convergence
// criterion); this TS PSM must produce byte-identical bytes and decode the
// Rust golden.

import { writeFileSync, readFileSync } from "node:fs";

import { feat } from "../gen/30_features.js";

const {
  WStrTypeSupport,
  MutTypeSupport,
  BitsTypeSupport,
  TreeTypeSupport,
  ArrTypeSupport,
  PrimTypeSupport,
} = feat;

const GOLDEN_DIR =
  "/Users/sandrakessler/projects/zerodds/zerodds-examples/idl-conformance/_interop/goldens";

// ---------------------------------------------------------------------------
// CANONICAL SAMPLES — EXACT values from CANONICAL.md (identical across PSMs).
// ---------------------------------------------------------------------------

// feat::WStr — label="café" (c a f é/U+00E9), text="日本語🎉" (🎉 = U+1F389).
const WSTR: feat.WStr = {
  label: "café",
  text: "日本語\u{1F389}",
};

// feat::Mut — @mutable: a(@id10)=1000000, b(@id20)=2.5, c(@id30)="ok".
const MUT: feat.Mut = { a: 1_000_000, b: 2.5, c: "ok" };

// feat::Bits — perm = READ|EXEC = 0x05; flags holder = (prio=20<<3)|kind=5 = 0xA5.
const BITS: feat.Bits = {
  perm: (feat.Perm.READ | feat.Perm.EXEC) >>> 0,
  flags: { kind: 5, prio: 20 },
};

// feat::Tree — root{1, [ {2,[ {4,[]} ]}, {3,[]} ]}.
const TREE: feat.Tree = {
  value: 1,
  kids: [
    { value: 2, kids: [{ value: 4, kids: [] }] },
    { value: 3, kids: [] },
  ],
};

// feat::Arr — grid[2][3]=[[10,11,12],[13,14,15]]; shape[2]=[{1,2},{3,4}].
const ARR: feat.Arr = {
  grid: [
    [10, 11, 12],
    [13, 14, 15],
  ],
  shape: [
    { x: 1, y: 2 },
    { x: 3, y: 4 },
  ],
};

// feat::Prim — every integer at its extreme + exact floats.
const PRIM: feat.Prim = {
  i8: -128,
  u8: 255,
  i16: -32768,
  u16: 65535,
  i32: -2147483648,
  u32: 4294967295,
  i64: -9223372036854775808n,
  u64: 18446744073709551615n,
  f32: 3.5,
  f64: -1234.5,
  b: true,
  o: 0xab,
  ch: "Z",
};

// ---------------------------------------------------------------------------
// Per-feature decode comparators (readable diffs).
// ---------------------------------------------------------------------------
type Diff = string[];

function eq(diffs: Diff, label: string, a: unknown, b: unknown): void {
  const sa = typeof a === "bigint" ? a.toString() : JSON.stringify(a);
  const sb = typeof b === "bigint" ? b.toString() : JSON.stringify(b);
  if (sa !== sb) diffs.push(`${label}: got ${sa} != want ${sb}`);
}

function diffWStr(g: feat.WStr, w: feat.WStr): Diff {
  const d: Diff = [];
  eq(d, "label", g.label, w.label);
  eq(d, "text", g.text, w.text);
  return d;
}
function diffMut(g: feat.Mut, w: feat.Mut): Diff {
  const d: Diff = [];
  eq(d, "a", g.a, w.a);
  eq(d, "b", g.b, w.b);
  eq(d, "c", g.c, w.c);
  return d;
}
function diffBits(g: feat.Bits, w: feat.Bits): Diff {
  const d: Diff = [];
  eq(d, "perm", g.perm, w.perm);
  eq(d, "flags.kind", g.flags.kind, w.flags.kind);
  eq(d, "flags.prio", g.flags.prio, w.flags.prio);
  return d;
}
function diffTree(g: feat.Tree, w: feat.Tree, path = "root"): Diff {
  const d: Diff = [];
  eq(d, `${path}.value`, g.value, w.value);
  if (g.kids.length !== w.kids.length) {
    d.push(`${path}.kids.length: got ${g.kids.length} != want ${w.kids.length}`);
  } else {
    for (let i = 0; i < w.kids.length; i++) {
      d.push(...diffTree(g.kids[i], w.kids[i], `${path}.kids[${i}]`));
    }
  }
  return d;
}
function diffArr(g: feat.Arr, w: feat.Arr): Diff {
  const d: Diff = [];
  for (let i = 0; i < 2; i++)
    for (let j = 0; j < 3; j++) eq(d, `grid[${i}][${j}]`, g.grid[i][j], w.grid[i][j]);
  for (let i = 0; i < 2; i++) {
    eq(d, `shape[${i}].x`, g.shape[i].x, w.shape[i].x);
    eq(d, `shape[${i}].y`, g.shape[i].y, w.shape[i].y);
  }
  return d;
}
function diffPrim(g: feat.Prim, w: feat.Prim): Diff {
  const d: Diff = [];
  for (const k of Object.keys(w) as Array<keyof feat.Prim>) eq(d, k, g[k], w[k]);
  return d;
}

// ---------------------------------------------------------------------------
interface Feature<T> {
  name: string;
  sample: T;
  encode(s: T): Uint8Array;
  decode(b: Uint8Array): T;
  diff(g: T, w: T): Diff;
}

const FEATURES: Array<Feature<any>> = [
  {
    name: "wstr",
    sample: WSTR,
    encode: (s) => WStrTypeSupport.encode(s, "le"),
    decode: (b) => WStrTypeSupport.decode(b),
    diff: diffWStr,
  },
  {
    name: "mut",
    sample: MUT,
    encode: (s) => MutTypeSupport.encode(s, "le"),
    decode: (b) => MutTypeSupport.decode(b),
    diff: diffMut,
  },
  {
    name: "bits",
    sample: BITS,
    encode: (s) => BitsTypeSupport.encode(s, "le"),
    decode: (b) => BitsTypeSupport.decode(b),
    diff: diffBits,
  },
  {
    name: "tree",
    sample: TREE,
    encode: (s) => TreeTypeSupport.encode(s, "le"),
    decode: (b) => TreeTypeSupport.decode(b),
    diff: (g, w) => diffTree(g, w),
  },
  {
    name: "arr",
    sample: ARR,
    encode: (s) => ArrTypeSupport.encode(s, "le"),
    decode: (b) => ArrTypeSupport.decode(b),
    diff: diffArr,
  },
  {
    name: "prim",
    sample: PRIM,
    encode: (s) => PrimTypeSupport.encode(s, "le"),
    decode: (b) => PrimTypeSupport.decode(b),
    diff: diffPrim,
  },
];

function toHex(b: Uint8Array): string {
  return Array.from(b, (x) => x.toString(16).padStart(2, "0")).join("");
}

function encodeAll(): void {
  for (const f of FEATURES) {
    const bytes = f.encode(f.sample);
    const path = `${GOLDEN_DIR}/${f.name}.ts.bin`;
    writeFileSync(path, bytes);
    console.log(`ENCODE ${f.name}: ${bytes.length} bytes -> ${path}`);
    console.log(`  hex: ${toHex(bytes)}`);
  }
}

function decodeAll(lang: string): void {
  let ok = true;
  for (const f of FEATURES) {
    const path = `${GOLDEN_DIR}/${f.name}.${lang}.bin`;
    const buf = readFileSync(path);
    const bytes = new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
    const got = f.decode(bytes);
    const diffs = f.diff(got, f.sample);
    if (diffs.length > 0) {
      ok = false;
      console.error(`DECODE MISMATCH ${f.name} <- ${path}:`);
      for (const dd of diffs) console.error(`  - ${dd}`);
    } else {
      console.log(`DECODE OK ${f.name} <- ${path} (every field == canonical)`);
    }
  }
  if (!ok) process.exit(1);
}

// ---------------------------------------------------------------------------
const mode = process.argv[2];
if (mode === "ENCODE") {
  encodeAll();
} else if (mode === "DECODE") {
  decodeAll(process.argv[3] ?? "rust");
} else {
  console.error("usage: interop_features.ts ENCODE | DECODE <lang>");
  process.exit(2);
}
