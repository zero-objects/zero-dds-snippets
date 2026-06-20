import init, { DomainParticipantFactory } from '@zerodds/wasm';

async function main() {
    await init();    // WASM bootstrap

    const factory = DomainParticipantFactory.instance();
    const participant = await factory.createParticipantWebSocket('ws://localhost:9001', 0);

    const topic = participant.createBytesTopic('Chatter');
    const writer = participant.createPublisher().createBytesWriter(topic);
    const reader = participant.createSubscriber().createBytesReader(topic);

    await writer.waitForMatchedSubscription(1, 5000);
    await reader.waitForMatchedPublication(1, 5000);

    writer.write(new TextEncoder().encode('hello from browser'));

    await reader.waitForData(3000);
    for (const sample of reader.take()) {
        console.log('got:', new TextDecoder().decode(sample));
    }
}

main();
