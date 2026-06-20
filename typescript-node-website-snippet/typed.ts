// Verbatim snippet from website/bindings/typescript-node/index.html (Typed topics)
// (the import of './gen/temperature' is codegen output; this file probes the
//  participant/publisher API shape it relies on)
import { Temperature, TemperatureTypeSupport } from './gen/temperature';

declare const participant: any;
declare const publisher: any;

const topic = participant.createTypedTopic('Temp', TemperatureTypeSupport);
const writer = publisher.createTypedWriter(topic);
await writer.write({ celsius: 23, sensor_id: 'A7' } satisfies Temperature);
