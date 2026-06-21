// SPDX-License-Identifier: Apache-2.0
//
// idl-conformance / typescript — real DCPS pub -> sub loopback roundtrip.
//
// Builds a fully-populated `combo::Telemetry` sample (the largest subset of the
// upstream conformance combo that the idl-ts XCDR2 backend can round-trip —
// typedef + nested struct + sequence-of-struct + bounded string + map +
// @optional + @key), publishes it over a REAL ZeroDDS DomainParticipant +
// Publisher/DataWriter, reads it back through a Subscriber/DataReader on the
// loopback transport, and asserts the recovered sample is deep-equal to what
// was published. Nothing here is in-memory encode/decode: the bytes traverse
// the native RTPS stack via the @zerodds/node FFI binding.

import assert from "node:assert/strict";

import { DomainParticipantFactory } from "@zerodds/node";
import { combo } from "../gen/telemetry_combo.js";

const { TelemetryTypeSupport } = combo;
type Telemetry = combo.Telemetry;

// The fully-populated sample. Every supported combo feature is non-trivial:
//   - @key long unitId / @key string<32> region   (keyed, bounded string)
//   - typedef double batteryCurrent
//   - sequence<Sample> history                     (sequence of nested struct)
//   - map<string,long> counters
//   - @optional double calibration                 (present)
const SENT: Telemetry = {
  unitId: 4711,
  region: "eu-central-1",
  batteryCurrent: 12.75,
  history: [
    { seq: 1, value: 3.14159 },
    { seq: 2, value: 2.71828 },
    { seq: 3, value: -1.5 },
  ],
  counters: new Map<string, number>([
    ["packets", 9001],
    ["drops", 2],
  ]),
  calibration: 0.0009765625,
};

// Deep-equal that understands Map (node assert.deepEqual already does, but we
// normalise the readonly map to a plain object for a readable diff on failure).
function normalise(t: Telemetry): unknown {
  return {
    ...t,
    counters: Object.fromEntries(t.counters),
  };
}

async function main(): Promise<void> {
  const participant = DomainParticipantFactory.instance().createParticipant(0);

  const topic = participant.createTypedTopic("ComboTelemetry", TelemetryTypeSupport);
  const writer = participant.createPublisher().createTypedWriter(topic);
  const reader = participant.createSubscriber().createTypedReader(topic);

  // Wait for the writer<->reader match on the loopback before publishing.
  await writer.waitForMatchedSubscription(1, 5000);
  await reader.waitForMatchedPublication(1, 5000);

  writer.write(SENT);

  const ready = await reader.waitForData(3000);
  assert.equal(ready, true, "reader.waitForData timed out — sample never arrived");

  const received = reader.take();
  assert.equal(received.length, 1, `expected exactly 1 sample, got ${received.length}`);

  const got = received[0] as Telemetry;

  // The load-bearing assertion: the recovered sample survived the wire intact.
  assert.deepEqual(
    normalise(got),
    normalise(SENT),
    "roundtrip mismatch: recovered sample != published sample",
  );

  console.log("PUBLISHED:", JSON.stringify(normalise(SENT)));
  console.log("RECEIVED :", JSON.stringify(normalise(got)));
  console.log("ROUNDTRIP OK — combo::Telemetry survived the DCPS wire intact.");

  participant.destroy();
}

main().catch((e) => {
  console.error("ROUNDTRIP FAILED:", e);
  process.exit(1);
});
