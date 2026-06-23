import { feat } from "../gen/30_features.js";
const T = feat as any;

// canonical samples (subset spanning the tricky constructs: wstring, @mutable,
// map-of-struct, seq-of-union, @bit_bound enum, recursion, arrays).
const samples: Array<[string, any, any]> = [
  ["wstr", T.WStrTypeSupport, { label: "café", text: "日本語\u{1F389}" }],
  ["mut",  T.MutTypeSupport,  { a: 1_000_000, b: 2.5, c: "ok" }],
  ["mapenum", T.MapEnumTypeSupport, { h: T.Hue.H_BLUE, m: new Map([[3, {x:11,y:12}]]), sels: [{discriminator:2, n:9}] }],
  ["tree", T.TreeTypeSupport, { value:1, kids:[{value:2,kids:[{value:4,kids:[]}]},{value:3,kids:[]}] }],
  ["arr",  T.ArrTypeSupport,  { grid:[[10,11,12],[13,14,15]], shape:[{x:1,y:2},{x:3,y:4}] }],
];

let fail = 0;
for (const [name, ts, sample] of samples) {
  for (const endian of ["le", "be"] as const) {
    const bytes = ts.encode(sample, endian);
    const back = ts.decode(bytes, 0, bytes.length, endian);
    const ok = JSON.stringify(back, (_k,v)=> v instanceof Map ? [...v.entries()] : (typeof v==="bigint"?v.toString():v))
             === JSON.stringify(sample, (_k,v)=> v instanceof Map ? [...v.entries()] : (typeof v==="bigint"?v.toString():v));
    console.log(`  ${name} ${endian}: ${ok ? "OK" : "FAIL"}`);
    if (!ok) { fail=1; console.log(`    want ${JSON.stringify(sample)}\n    got  ${JSON.stringify(back)}`); }
  }
}
process.exit(fail);
