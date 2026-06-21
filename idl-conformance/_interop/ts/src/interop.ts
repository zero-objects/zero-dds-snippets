// SPDX-License-Identifier: Apache-2.0
//
// idl-conformance / _interop / ts — WIRE-level XCDR2 interop golden.
//
// ENCODE        -> write the canonical combo::Telemetry sample's raw XCDR2-LE
//                  bytes (the exact bytes DDS carries) to the shared golden file.
// DECODE <file> -> read raw bytes, reconstruct the sample via the generated
//                  codec, assert EVERY field == canonical. exit 0 on match,
//                  nonzero with a printed diff otherwise.
//
// The bytes ARE the wire: this uses the generated TelemetryTypeSupport
// encode/decode directly (no live pub/sub), so any language's golden of the
// same canonical sample must be byte-identical.

import { writeFileSync, readFileSync } from "node:fs";

import { combo } from "../gen/20_mixed_combo.js";

const { TelemetryTypeSupport, Mode } = combo;
type Telemetry = combo.Telemetry;

const GOLDEN_DEFAULT =
  "/Users/sandrakessler/projects/zerodds/zerodds-examples/idl-conformance/_interop/goldens/ts.bin";

// ---------------------------------------------------------------------------
// CANONICAL SAMPLE — identical across every language PSM.
// ---------------------------------------------------------------------------
const CANONICAL: Telemetry = {
  unitId: 4242,
  region: "eu-central-1",
  mode: Mode.MODE_ACTIVE,
  batteryCurrent: 13.75,
  history: [
    { seq: 1, value: 0.5 },
    { seq: 2, value: 1.5 },
    { seq: 3, value: -2.25 },
  ],
  reading: { discriminator: Mode.MODE_ACTIVE, activeRate: 60.0 },
  // key-sorted: "drops" < "packets"
  counters: new Map<string, number>([
    ["drops", 3],
    ["packets", 100],
  ]),
  calibration: 0.001,
  window: [10, 20, 30, 40],
};

// ---------------------------------------------------------------------------
// Field-by-field comparison with readable diffs.
// ---------------------------------------------------------------------------
function diffFields(got: Telemetry, want: Telemetry): string[] {
  const diffs: string[] = [];
  const eq = (label: string, a: unknown, b: unknown) => {
    if (a !== b) diffs.push(`${label}: got ${JSON.stringify(a)} != want ${JSON.stringify(b)}`);
  };

  eq("unitId", got.unitId, want.unitId);
  eq("region", got.region, want.region);
  eq("mode", got.mode, want.mode);
  eq("batteryCurrent", got.batteryCurrent, want.batteryCurrent);

  // history (sequence<Sample>)
  if (got.history.length !== want.history.length) {
    diffs.push(`history.length: got ${got.history.length} != want ${want.history.length}`);
  } else {
    for (let i = 0; i < want.history.length; i++) {
      eq(`history[${i}].seq`, got.history[i].seq, want.history[i].seq);
      eq(`history[${i}].value`, got.history[i].value, want.history[i].value);
    }
  }

  // reading (union)
  eq("reading.discriminator", got.reading.discriminator, want.reading.discriminator);
  eq(
    "reading.activeRate",
    (got.reading as { activeRate?: number }).activeRate,
    (want.reading as { activeRate?: number }).activeRate,
  );

  // counters (map<string,long>)
  if (got.counters.size !== want.counters.size) {
    diffs.push(`counters.size: got ${got.counters.size} != want ${want.counters.size}`);
  } else {
    for (const [k, v] of want.counters) {
      eq(`counters[${k}]`, got.counters.get(k), v);
    }
  }

  // calibration (@optional double)
  eq("calibration.present", got.calibration !== undefined, want.calibration !== undefined);
  eq("calibration", got.calibration, want.calibration);

  // window (long[4])
  if (got.window.length !== want.window.length) {
    diffs.push(`window.length: got ${got.window.length} != want ${want.window.length}`);
  } else {
    for (let i = 0; i < want.window.length; i++) {
      eq(`window[${i}]`, got.window[i], want.window[i]);
    }
  }

  return diffs;
}

function toHex(b: Uint8Array): string {
  return Array.from(b, (x) => x.toString(16).padStart(2, "0")).join("");
}

// ---------------------------------------------------------------------------
function encode(path: string): void {
  const bytes = TelemetryTypeSupport.encode(CANONICAL, "le");
  writeFileSync(path, bytes);
  console.log(`ENCODE -> ${path}`);
  console.log(`  size: ${bytes.length} bytes`);
  console.log(`  hex : ${toHex(bytes)}`);
}

function decode(path: string): void {
  const buf = readFileSync(path);
  const bytes = new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
  const got = TelemetryTypeSupport.decode(bytes);
  const diffs = diffFields(got, CANONICAL);
  if (diffs.length > 0) {
    console.error(`DECODE MISMATCH on ${path}:`);
    for (const d of diffs) console.error(`  - ${d}`);
    process.exit(1);
  }
  console.log(`DECODE OK <- ${path}`);
  console.log("  every field == canonical combo::Telemetry");
}

// ---------------------------------------------------------------------------
const mode = process.argv[2];
if (mode === "ENCODE") {
  encode(process.argv[3] ?? GOLDEN_DEFAULT);
} else if (mode === "DECODE") {
  const p = process.argv[3];
  if (!p) {
    console.error("usage: interop.ts DECODE <file>");
    process.exit(2);
  }
  decode(p);
} else {
  console.error("usage: interop.ts ENCODE [file] | DECODE <file>");
  process.exit(2);
}
