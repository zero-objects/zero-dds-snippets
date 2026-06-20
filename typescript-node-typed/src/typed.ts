import { DomainParticipantFactory } from '@zerodds/node';

// use it
import { Temperature, TemperatureTypeSupport } from './gen/temperature';

async function main() {
    const participant = DomainParticipantFactory.instance().createParticipant(0);
    const publisher = participant.createPublisher();

    const topic = participant.createTypedTopic('Temp', TemperatureTypeSupport);
    const writer = publisher.createTypedWriter(topic);
    await writer.write({ celsius: 23, sensor_id: 'A7' });
}

// `Temperature` is the codegen-emitted interface consumed by the typed writer.
const _example: Temperature = { celsius: 23, sensor_id: 'A7' };
void _example;

main().catch(console.error);
