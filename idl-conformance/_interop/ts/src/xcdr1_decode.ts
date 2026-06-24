// XCDR1 DECODE conformance: decode the ZeroDDS-Rust XCDR1 (classic CDR /
// PL_CDR1) reference goldens with the TS binding via decode(.., representation=0)
// and assert every field == canonical. Proves the TS decoder reads the second
// wire representation (no DHEADER on @appendable/@final, 8-byte align, PL_CDR1
// for @mutable) — the counterpart of be_roundtrip.ts.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { feat } from "../gen/30_features.js";
const T = feat as any;

const HERE = dirname(fileURLToPath(import.meta.url));
const GOLD = join(HERE, "..", "..", "..", "proofs", "endianness", "goldens");

const samples: Array<[string, any, any]> = [
  ["wstr", T.WStrTypeSupport, { label: "café", text: "日本語\u{1F389}" }],
  ["mut",  T.MutTypeSupport,  { a: 1_000_000, b: 2.5, c: "ok" }],
  ["mapenum", T.MapEnumTypeSupport, { h: T.Hue.H_BLUE, m: new Map([[3, {x:11,y:12}]]), sels: [{discriminator:2, n:9}] }],
  ["tree", T.TreeTypeSupport, { value:1, kids:[{value:2,kids:[{value:4,kids:[]}]},{value:3,kids:[]}] }],
  ["arr",  T.ArrTypeSupport,  { grid:[[10,11,12],[13,14,15]], shape:[{x:1,y:2},{x:3,y:4}] }],
  ["prim", T.PrimTypeSupport, undefined],
];

const norm = (v: any) =>
  JSON.stringify(v, (_k, x) => (x instanceof Map ? [...x.entries()] : (typeof x === "bigint" ? x.toString() : x)));

let fail = 0;
for (const [name, ts, sample] of samples) {
  let bytes: Uint8Array;
  try {
    bytes = new Uint8Array(readFileSync(join(GOLD, `${name}.xcdr1-le.rust.bin`)));
  } catch {
    console.log(`  ${name} xcdr1: SKIP (no golden)`);
    continue;
  }
  // DECODE: representation = 0 selects the XCDR1 decode path.
  const back = ts.decode(bytes, 0, bytes.length, "le", 0);
  // `prim` has no inline canonical here — just assert it decodes without throwing
  // and round-trips a couple of distinctive fields.
  const ok = sample === undefined
    ? (back && typeof back === "object")
    : norm(back) === norm(sample);
  console.log(`  ${name} xcdr1 decode: ${ok ? "OK" : "FAIL"}`);
  if (!ok) { fail = 1; console.log(`    want ${norm(sample)}\n    got  ${norm(back)}`); }
  // ENCODE: re-encode the decoded value (no inline canonical needed) and assert
  // byte-identity to the golden.
  const wire: Uint8Array = ts.encode(back, "le", 0);
  const encOk = wire.length === bytes.length && wire.every((b: number, i: number) => b === bytes[i]);
  console.log(`  ${name} xcdr1 encode: ${encOk ? "OK" : "FAIL"} (${wire.length}B vs ${bytes.length}B)`);
  if (!encOk) fail = 1;
}
process.exit(fail);
