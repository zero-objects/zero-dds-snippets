import { DomainParticipantFactory } from '@zerodds/node';

const participant = DomainParticipantFactory.instance().createParticipant(0);
const topic = participant.createBytesTopic('Chatter');
const writer = participant.createPublisher().createBytesWriter(topic);
const reader = participant.createSubscriber().createBytesReader(topic);
const payload = new TextEncoder().encode('hello');
const handle = (s: Uint8Array) => console.log('got:', new TextDecoder().decode(s));

await writer.writeAsync(payload);
const samples = await reader.takeAsync();
for await (const sample of reader.streamSamples()) {
    handle(sample);
}

void samples;
