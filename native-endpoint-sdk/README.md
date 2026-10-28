# native-endpoint-sdk

The runnable companion to the **native endpoint SDKs** (ADR 0013): conservative,
no-Rust endpoints in **C89 / Python / Java** that talk to a ZeroDDS/XRCE hub
over UDP — in both directions. Each endpoint encodes/decodes a `SensorReading`
with the wire-core, frames it as an XRCE
`WRITE_DATA` message, and hands the frame to a transport through the frame-hook
(here a UDP socket; another target fills the same hook with its own transport).
The hub side is the real `zerodds-xrce`, so the endpoint frames are
checked against genuine Rust XRCE over a socket. No Rust runs on the endpoint.
This example uses little-endian XCDR2 and does not certify execution on a
big-endian host or compare independent byte dumps.

## Layout

| Path | Role |
|------|------|
| `c/publish.c`, `c/receive.c` | C89 endpoint: POSIX-UDP publish / receive |
| `python/publish.py`, `python/receive.py` | pure-Python endpoint |
| `java/PublishUdp.java`, `java/ReceiveUdp.java` | pure-Java endpoint |
| `run.sh` | builds the hub + endpoints, runs both directions, asserts all six exchanges |

The endpoint SDK sources (the wire-core, the XRCE/serial framing, the sample
codec) live in the ZeroDDS source tree under `endpoints/`; this demo references
them directly. Build and run the example against the intended source revision
to check that they still agree.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
(for the endpoint SDK sources + the hub binary). Place this snippets repo next
to a `zero-dds` checkout, or point `ZERODDS_ROOT` at one:

```sh
ZERODDS_ROOT=/path/to/zero-dds ./run.sh
./run.sh 12345          # custom base port
```

`run.sh` builds the hub (`zerodds-xrce-agent-demo`) and the C + Java endpoints,
then:

1. **Publish** — the C, Python and Java endpoints each send a `WRITE_DATA`
   frame to the hub; the hub decodes each `SensorReading` (`AGENT OK` ×3).
2. **Receive** — the hub pushes a `DATA` sample to each endpoint; the endpoint
   unwraps and decodes it (`RECEIVER OK` ×3).

Every field is checked against the expected sample. Socket reads have a
ten-second deadline and subprocess failures fail the run. Each invocation keeps
its logs and built endpoints in a fresh temporary directory printed on exit.
Cleanup targets only the receiver/hub processes started by that invocation.

Expected result:

```
OK: publish 3/3 + receive 3/3 (C/Python/Java <-> Rust hub)
```

## What each endpoint does

```
encode sample  --(wire-core: wire-fixed or reflective)-->  XCDR body
frame          --(zdw_xrce_write_frame / language equiv.)--> XRCE WRITE_DATA
deliver        --(frame-hook: here a UDP socket)---------->  the wire
```

The reverse for receive: `receive → unwrap DATA → decode`. Using a different
transport is just a different frame-hook fill — see
`endpoints/c/src/zerodds_endpoint.c` `zdw_serial_frame` for the Annex-C HDLC
(byte-stuffing + CRC-16) serial framing.

## License

Apache-2.0, same as ZeroDDS.

## Reference

- Explained on the website: [native-endpoints](https://zerodds.org/docs/native-endpoints.html)
- This example in the repository: [zero-dds-snippets/native-endpoint-sdk](https://github.com/zero-objects/zero-dds-snippets/tree/main/native-endpoint-sdk)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
