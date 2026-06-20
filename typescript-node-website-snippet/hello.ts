// Verbatim snippet from website/bindings/typescript-node/index.html (hello.ts)
import { DomainParticipantFactory } from '@zerodds/node';

async function main() {
    const factory = DomainParticipantFactory.instance();
    const participant = factory.createParticipant(0);

    const topic = participant.createBytesTopic('Chatter');
    const publisher = participant.createPublisher();
    const subscriber = participant.createSubscriber();

    const writer = publisher.createBytesWriter(topic);
    const reader = subscriber.createBytesReader(topic);

    await writer.waitForMatchedSubscription(1, 5000);
    await reader.waitForMatchedPublication(1, 5000);

    writer.write(new TextEncoder().encode('hello'));
    await reader.waitForData(3000);

    for (const sample of reader.take()) {
        console.log('got:', new TextDecoder().decode(sample));
    }
}

main().catch(console.error);
