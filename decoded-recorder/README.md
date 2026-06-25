# Decoded recorder — capture, decode to JSON/SQLite, and replay any topic

ZeroDDS ships three CLI tools that share one reflective decode core: `zerodds-spy`
(live monitor), `zerodds-record` (capture) and `zerodds-replay` (re-publish). With
an out-of-band IDL they turn raw DDS samples into **typed JSON or a per-topic SQLite
table** — and they need **no compiled type**: the decoder builds a runtime
`DynamicType` from the IDL and walks the wire with the same hardened XCDR codec the
typed bindings use. Point them at a topic whose writer you discovered (a Fast DDS /
CycloneDDS / RTI / OpenDDS publisher, or another ZeroDDS app) and read its data.

```idl
// sensor.idl — the schema handed to --type-file (not a compiled type)
module telemetry {
  enum Unit { CELSIUS, KELVIN, FAHRENHEIT };
  @appendable struct Reading {
    @key long       sensor_id;
    string          name;
    Unit            unit;
    double          value;
    sequence<long>  history;
  };
};
```

## 1. Live typed monitor — `zerodds-spy --decode`

```
$ zerodds-spy -t Readings --type-file sensor.idl --decode
zerodds-spy: attached topic=Readings type=telemetry::Reading
[1] writer=… type=telemetry::Reading {"sensor_id":7,"name":"intake","unit":0,"value":21.5,"history":[20,21,22]}
```

Without `--type-file` the spy still **follows the type**: it learns each writer's
real `type_name` from discovery and attaches a raw reader, printing a hex snippet
until you supply the matching IDL. That is how it decodes a foreign vendor's topic
(see [`../cross-vendor-dynamic-xtypes`](../cross-vendor-dynamic-xtypes)).

## 2. Capture — `zerodds-record`

Raw, byte-exact, no schema needed:

```
$ zerodds-record record -t Readings -o capture.zddsrec
```

Decoded, while recording — typed NDJSON and/or a per-topic SQLite table:

```
$ zerodds-record record -t Readings --decode --type-file sensor.idl \
      --out-json readings.ndjson --out-sqlite readings.db
```

- `readings.ndjson` — one decoded sample per line.
- `readings.db` — a table per topic, **one column per (flattened) field**; a
  `sequence<Reading>` becomes a child table joined on `sample_id`. Query it with
  plain SQL.

The `.zddsrec` always preserves the on-the-wire **byte order and XCDR version**, so
a big-endian or XCDR1 capture replays exactly as it arrived.

## 3. Replay — `zerodds-replay`

All three recorded forms re-publish byte-exact:

```
$ zerodds-replay replay capture.zddsrec --inject          # native .zddsrec
$ zerodds-replay replay readings.ndjson --inject          # decoded NDJSON
$ zerodds-replay replay readings.db --inject              # decoded SQLite
$ zerodds-replay replay capture.zddsrec --time-scale 60   # 1 h trace in 1 min
```

`--inject` re-publishes onto the live bus (`--inject-domain N` to retarget);
without it the frames print to stdout for inspection. `--topic NAME` filters.

## Why it matters

The recorder is **type-agnostic**: a single binary records, decodes and replays
*any* topic — yours or a third-party vendor's — given only its IDL, with no
per-type code generation or rebuild. The decode core, the SQLite/JSON sinks and the
replay sources are the same ones exercised in
[`../cross-vendor-dynamic-xtypes`](../cross-vendor-dynamic-xtypes).

Part of **[ZeroDDS](https://zerodds.de)**.
