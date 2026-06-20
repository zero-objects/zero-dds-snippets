// Runtime driver for the §async snippet methods (writeAsync / takeAsync /
// streamSamples). `async.ts` holds the verbatim website snippet for the tsc
// type-check; this file exercises the same three methods deterministically so
// the async path can be observed end-to-end (the snippet's streamSamples loop
// is infinite by design).

import { DomainParticipantFactory } from '@zerodds/node';

const participant = DomainParticipantFactory.instance().createParticipant(0);
const topic = participant.createBytesTopic('Chatter');
const writer = participant.createPublisher().createBytesWriter(topic);
const reader = participant.createSubscriber().createBytesReader(topic);
const handle = (s: Uint8Array) => console.log('got:', new TextDecoder().decode(s));

await writer.waitForMatchedSubscription(1, 5000);
await reader.waitForMatchedPublication(1, 5000);

// writeAsync: Promise-returning publish.
await writer.writeAsync(new TextEncoder().encode('async-1'));
await reader.waitForData(3000);

// takeAsync: Promise-returning drain.
const samples = await reader.takeAsync();
console.log('takeAsync count:', samples.length);
for (const s of samples) handle(s);

// streamSamples: async-iterator. Publish two, then break after consuming them.
await writer.writeAsync(new TextEncoder().encode('stream-1'));
await writer.writeAsync(new TextEncoder().encode('stream-2'));
let seen = 0;
for await (const sample of reader.streamSamples()) {
    handle(sample);
    if (++seen >= 2) break;
}

reader.destroy();
writer.destroy();
participant.destroy();
console.log('OK');
