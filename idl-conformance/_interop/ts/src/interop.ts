// SPDX-License-Identifier: Apache-2.0
//
// idl-conformance / _interop / ts — the combo::Telemetry column.
//
//   tsx src/interop.ts ENCODE <out.bin>   write the canonical sample as raw XCDR2-LE
//   tsx src/interop.ts DECODE <in.bin>    decode, assert every field == canonical
//
// gen/combo.ts is `zerodds-idlc generate --lang typescript` output of
// ../combo.idl over runtime/typescript (@zerodds/wire); values are
// CANONICAL.md.

import { readFileSync, writeFileSync } from "node:fs";

import { combo } from "../gen/combo.js";

const { TelemetryTypeSupport, Mode } = combo;

const CANONICAL: combo.Telemetry = {
  unitId: 4242, // @key long
  region: "eu-central-1", // @key string<32>
  mode: Mode.MODE_ACTIVE, // enum, index 1
  batteryCurrent: 13.75, // typedef double
  history: [
    { seq: 1, value: 0.5 },
    { seq: 2, value: 1.5 },
    { seq: 3, value: -2.25 },
  ],
  reading: { discriminator: Mode.MODE_ACTIVE, activeRate: 60.0 },
  // map<string,long>, key-sorted: "drops" < "packets"
  counters: new Map<string, number>([
    ["drops", 3],
    ["packets", 100],
  ]),
  calibration: 0.001, // @optional double, present
  window: [10, 20, 30, 40], // long[4]
};

function diffFields(got: combo.Telemetry, want: combo.Telemetry): string[] {
  const diffs: string[] = [];
  const eq = (label: string, a: unknown, b: unknown) => {
    const sa = JSON.stringify(a instanceof Map ? [...a] : a);
    const sb = JSON.stringify(b instanceof Map ? [...b] : b);
    if (sa !== sb) diffs.push(`${label}: got ${sa} != want ${sb}`);
  };
  eq("unitId", got.unitId, want.unitId);
  eq("region", got.region, want.region);
  eq("mode", got.mode, want.mode);
  eq("batteryCurrent", got.batteryCurrent, want.batteryCurrent);
  eq("history", got.history, want.history);
  eq("reading", got.reading, want.reading);
  eq("counters", got.counters, want.counters);
  eq("calibration", got.calibration, want.calibration);
  eq("window", got.window, want.window);
  return diffs;
}

const [mode, path] = process.argv.slice(2);
if (!path || (mode !== "ENCODE" && mode !== "DECODE")) {
  console.error("usage: interop.ts ENCODE <out.bin> | DECODE <in.bin>");
  process.exit(2);
}
if (mode === "ENCODE") {
  const bytes = TelemetryTypeSupport.encode(CANONICAL, "le");
  writeFileSync(path, bytes);
  console.log(`ENCODE OK: ${bytes.length} bytes -> ${path}`);
} else {
  const buf = readFileSync(path);
  const got = TelemetryTypeSupport.decode(new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength));
  const diffs = diffFields(got, CANONICAL);
  if (diffs.length > 0) {
    for (const d of diffs) console.error(`  [FAIL] ${d}`);
    console.log(`DECODE FAILED: ${diffs.length} mismatch(es)`);
    process.exit(1);
  }
  console.log("DECODE OK: every field equals canonical");
}
