// Snippet from website/bindings/typescript-node/index.html (Typed topics).
// The import is the one rc.7's generator needs: it nests IDL modules as
// namespaces, where the website's text still imports `Temperature` and
// `TemperatureTypeSupport` flat. './gen/temperature' is codegen output; this
// file probes the participant/publisher API shape it relies on.
import { sensor_msgs } from './gen/temperature';

// The generator nests IDL modules as namespaces.
const { TemperatureTypeSupport } = sensor_msgs.msg;
type Temperature = sensor_msgs.msg.Temperature;

declare const participant: any;
declare const publisher: any;

const topic = participant.createTypedTopic('Temp', TemperatureTypeSupport);
const writer = publisher.createTypedWriter(topic);
await writer.write({ celsius: 23, sensor_id: 'A7' } satisfies Temperature);
