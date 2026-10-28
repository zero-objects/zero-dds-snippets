# native-frontends / go

The runnable Go **native language frontend**: idlc-generated types plus the Go
endpoint SDK, talking to a real ZeroDDS peer over UDP. Two demos:

- **ping-pong** — an idlc-typed `Ping`/`Pong` round-trip, both **sync**
  (`Client.Write`/`Receive`) and **async** (`AsyncWriter`/`AsyncReader`). The
  generated `ping.go` encodes a typed `Ping` to XCDR2; the SDK frames it as an
  XRCE `WRITE_DATA`; the peer decodes it and replies a typed `Pong`.
- **reliable** — the reliable XRCE stream (`stream_id ≥ 128`): the app sends N
  samples through the async-decoupled `ReliableAsyncWriter` while the peer drops
  every 3rd datagram; the writer retransmits on `ACKNACK` until every sample is
  delivered gap-free.

## Layout

| Path | Role |
|------|------|
| `pingpong/main.go` | the ping-pong app (sync + async) |
| temporary `pingpong/generated/ping.go` | fresh idlc-generated `Ping`/`Pong`, not edited or checked in |
| `reliable/main.go` | reliable-stream sender over `ReliableAsyncWriter` |
| `run.sh` | generates the types, starts the peer, runs both demos, asserts them |
| `../ping.idl` | the authoritative IDL source (`Ping`/`Pong`) |

`run.sh` builds the current CLI and generates into its temporary evidence
directory before every run. Its generation command is equivalent to:

```sh
zerodds-idlc generate --lang go --encoding xcdr2 --out OUTPUT_DIR ../ping.idl
```

The generated `zdgen` package remains unchanged. The app calls `Zd_enc_Ping`
and `Zd_dec_Pong` with the shared `runtime/go` writer and reader. Decode status,
exact input consumption, sequence and reply text are checked. Both endpoint
writers address object `0x0015`, matching the shared Rust peer's fixture.

The endpoint SDK sources (wire-core, XRCE framing, `Client`/`AsyncReader`,
reliable stream) live in the ZeroDDS tree under `endpoints/go`; this demo
references them and `runtime/go` through local `replace` directives in its
temporary `go.mod`. A successful run demonstrates that these selected sources
work together. The launcher retains generated sources and logs and stops its
own peer on failure. Reliable delivery has a bounded completion wait.

## Build & run

The migrated example passed sync, async and reliable delivery (12/12 samples,
every third datagram dropped) on 12 September 2026. This is a Go/XRCE peer
result, not a full DDS vendor-interoperability or release certification.
On macOS the launcher uses the platform linker and ad-hoc signs its own fresh
binaries, because the installed Go 1.21.5 internal linker did not produce a
loadable executable on the test host. No system security policy is changed.

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
(for `endpoints/go` + the peer binary) and Go. Place this snippets repo next to
a `zero-dds` checkout, or point `ZERODDS_ROOT` at one:

```sh
ZERODDS_ROOT=/path/to/zero-dds ./run.sh
./run.sh 17520          # custom base port
```

Expected result:

```
== ping-pong (sync) =="
PONG seq=1 reply=pong:hello from app
PEER OK: ping-pong ...
== ping-pong (async) =="
PONG seq=1 reply=pong:hello from app
== reliable (loss recovery, drop every 3rd of 12) =="
SENT count=12
PEER OK: reliable 12/12 delivered gap-free (loss recovered via ACKNACK)
OK: ping-pong (sync+async) + reliable (go <-> Rust peer)
```

## What the frontend does

```
Ping{Seq,Msg}  --(idlc-generated Zd_enc_Ping)-->  XCDR2 body
frame          --(endpoint SDK: XRCE WRITE_DATA)-->  frame
deliver        --(transport: a UDP socket)-------->  the wire  --> ZeroDDS peer
                                                                    decode + reply Pong
```

Reliable adds the retransmit loop: `submit → history → WRITE_DATA → HEARTBEAT →
on ACKNACK retransmit → prune`, driven by a drain goroutine so the producer
never blocks on the socket.

## Reference

Companion to the **[native endpoints](https://zerodds.org)** docs. See the
sibling [`../native-endpoint-sdk`](../native-endpoint-sdk) for the C/Python/Java
sensor publish/receive demo.

Part of **[ZeroDDS](https://zerodds.org)** — Apache-2.0.
