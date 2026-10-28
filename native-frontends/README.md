# native-frontends

Runnable **native language frontends** for ZeroDDS: in each language, the
`idlc`-generated types plus that language's endpoint SDK exchange typed samples
with a real ZeroDDS peer over UDP — no Rust runs on the frontend, so the same
code works on a big-endian or resource-constrained host.

Each language folder carries two demos:

- **ping-pong** — an idlc-typed `Ping`/`Pong` round-trip, **sync** and **async**.
  The generated type marshals to XCDR2; the endpoint SDK frames it as an XRCE
  `WRITE_DATA`; the peer decodes it with the current shared wire / `zerodds-xrce`
  codecs and replies a typed `Pong`. Proves the frontend's wire + framing + the
  sync/async delivery surface against real ZeroDDS.
- **reliable** — the reliable XRCE stream (`stream_id ≥ 128`): the app sends N
  samples through an async-decoupled reliable writer while the peer injects loss;
  the writer retransmits on `ACKNACK` until every sample arrives gap-free. Proves
  HEARTBEAT / ACKNACK / retransmit / history at the frontend.

The boundary is honest: the frontend is wire + TypeSupport + XRCE framing +
reliable delivery over a datagram. RTPS, discovery and QoS matching are the DDS
participant's job — here, the peer.

## Layout

This example currently contains only [`go/`](go/), with `pingpong/`,
`reliable/`, and a [`run.sh`](go/run.sh) that starts the peer and checks both
demos. Other language SDKs in the ZeroDDS tree are not additional runnable
examples in this directory. Referencing an SDK alone does not prevent API drift.

The Go launcher generates a fresh, unmodified codec package and links both
`endpoints/go` and `runtime/go` from the selected source checkout. Temporary
sources and run logs are retained at the path printed by the launcher.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
(for the endpoint SDK sources + the `peer` binary) and the language's toolchain.
Place this snippets repo next to a `zero-dds` checkout, or point `ZERODDS_ROOT`:

```sh
cd go && ZERODDS_ROOT=/path/to/zero-dds ./run.sh
```

Each `run.sh` builds the peer (`cargo build -p zerodds-endpoint-e2e --bin peer`)
from the tree, then runs the language app against it.

## Related

- [Website guide](https://zerodds.org/docs/examples/native-frontends.html).

- [`../native-endpoint-sdk`](../native-endpoint-sdk) — the C/Python/Java sensor
  publish/receive companion (ADR 0013).

Part of **[ZeroDDS](https://zerodds.org)** — Apache-2.0.
