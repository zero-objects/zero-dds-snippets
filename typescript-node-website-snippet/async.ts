// Verbatim snippet from website/bindings/typescript-node/index.html (async/await)
import { DataWriter, DataReader } from '@zerodds/node';

declare const writer: DataWriter<Uint8Array>;
declare const reader: DataReader<Uint8Array>;
declare const payload: Uint8Array;
declare function handle(s: Uint8Array): void;

await writer.writeAsync(payload);
const samples = await reader.takeAsync();
for await (const sample of reader.streamSamples()) {
    handle(sample);
}
