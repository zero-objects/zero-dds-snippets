# Status of the examples

## Last full run

`ci/run-examples.sh`, one row per call, on the Linux test host with every
vendor toolchain the manifest asks for (Cyclone DDS 11.0.1 with its Python
binding, Fast DDS 3 with fastddsgen, OpenDDS 3.34, RTI Connext 7.7):

| verdict | rows |
|---|---:|
| PASS | 111 |
| BUILT-ONLY | 7 |
| FAIL | 0 |
| UNVERIFIED | 0 |
| BLOCKED | 0 |

118 rows, 1262 s. `decoded-recorder` was UNVERIFIED in that run (no writer on
the wire); it now starts its own and passed on its own afterwards, which is the
111th PASS. The seven BUILT-ONLY rows have no program to run and say why: three
TypeScript snippets checked with `tsc`, `idl-fixed-decimal` checked with
`zerodds-idlc check`, and the three `cross-vendor-*` parents, whose legs are
rows of their own.

The twelve cross-vendor legs — shapes, rich and dynamic-xtypes against Cyclone
DDS, Fast DDS, OpenDDS and RTI Connext — all pass in both directions where the
example has two. The sections below are the history of how the examples got
here; the tables in them are not this run.

## Historical measurement

What each example is, whether it **runs** on a host that has what it needs,
and where it stands against ZeroDDS rc.7.

The distinction this file is built around: an example that compiles proves
that the API names still exist. It does not prove that a sample crossed the
wire, that a round-trip is lossless, or that a tool accepted its own
configuration. Every row below says which of the two it got.

Reproduce it with:

```sh
ci/run-examples.sh            # every example, one verdict each
ci/run-examples.sh rust-typed # or just the named ones
```

The harness refuses to start if a git-tracked directory is missing from
`ci/examples.manifest`, so a new example cannot be silently left out of the
total. An untracked directory is named at the top of the run instead.

## The measurement behind this file

| | |
|---|---|
| host | macOS 15 (Darwin 25.2.0), arm64 |
| toolchains present | rustc 1.88.0, clang, dotnet 9.0.5, JDK 21 + Maven, Node 25.2.1 + npm, Python 3.14 |
| ZeroDDS tree | the worktree at `1.0.0-rc.7`, `cargo build --workspace` green |
| prepared first | `cargo build -p zerodds-c-api --release`; `mvn -o -q install -DskipTests` in `crates/java-omgdds/java`; `npm install && npm run build` in `crates/ts-node`; `maturin build --release` in `crates/py`, wheel unpacked onto `PYTHONPATH` |
| command | `ci/run-examples.sh` |

Result: **70 ran and were checked, 0 failed, 3 built only, 5 blocked, 10 not
verified on this host** — 88 in the manifest.

The 30 added since that first run are the website's own snippets: one example
per `docs/<name>.html` page whose "Usage" block is Rust against a ZeroDDS
crate, plus `native-frontends`. 28 of the 29 `crate-*` examples ran and were
checked here; `crate-rt-linux` needs a Linux kernel and got no host (see
"Not verified"). `native-frontends` is blocked on a named codegen gap.

## Verdicts

`run` — built, executed, and its output checked.
`build` — there is no program to run; the column says what was checked instead.
`blocked` — a named defect stops it; the reason is in the row.
`unverified` — a requirement was missing on this host. Never a pass.

### Ran and checked (70)

| example | language | type comes from | what the run showed |
|---|---|---|---|
| `idlc-buildrs` | Rust | `build.rs` + `zerodds-build` | 40 bytes round-tripped; `HAS_KEY`, `TYPE_NAME` and the `TypeObject` asserted |
| `zerodds-build-sample` | Rust | `build.rs` + `zerodds-build` | 36 bytes round-tripped; `#pragma keylist` and the `TypeObject` asserted |
| `rust-xcdr1-representation` | Rust | `build.rs` + `zerodds-build` | XCDR2 19 bytes vs XCDR1 23 bytes, both round-trip |
| `rust-typed` | Rust | `build.rs` + `zerodds-build` | wrote a sample; `type_object()` asserted non-`None` |
| `rust-async-showcase` | Rust | `build.rs` + `zerodds-build` | all 13 async surfaces, `ALL OK` |
| `hello-publisher` | Rust | none (bytes) | `published: Hello, DDS!` |
| `pubsub-roundtrip` | Rust | none (bytes) | 1 sample received |
| `bindings-rust-main` | Rust | none (bytes) | `got: [104, 101, 108, 108, 111]` |
| `async-pubsub` | Rust | none (bytes) | same payload, async |
| `getting-started-first-publisher` | Rust | `TypeIdentifier::Primitive` | 10 samples sent through `DcpsRuntime` |
| `observability-stderr-sink` | Rust | `TypeIdentifier::Primitive` | writer + reader created, JSON events on stderr |
| `qos-cookbook` | Rust | — | compiles and runs; it pins QoS field names and types |
| `qos-policies-full-writer` | Rust | `TypeIdentifier::Primitive` | the full policy set accepted at registration |
| `runtime-config` | Rust | — | runtime up on domain 11 with the full field list |
| `rust-audit-factory-chain` | Rust | — | audit fan-out wired on a live participant |
| `rust-audit-loggingplugin` | Rust | — | `class id = DDS:Logging:Demo` |
| `rust-audit-otlp` | Rust | — | OTLP exporter flushed |
| `rust-audit-scrubbing` | Rust | — | handshake key redacted in the record |
| `rust-audit-securitybundle-realpart` | Rust | — | bundle wired, audit NDJSON written |
| `rust-audit-syslog` | Rust | — | syslog plugin wired to 127.0.0.1:514 |
| `rust-dev-install-hello` | Rust | none (bytes) | payload round-tripped |
| `rust-dev-install-runtimeconfig` | Rust | — | config built and applied |
| `rust-transport-trait` | Rust | — | a custom transport's locator reported |
| `rust-transport-tuning` | Rust | — | all four `RuntimeConfig` recipes built |
| `security-governance-gate` | Rust | — | runtime up on domain 0 with the gate wired |
| `zerodds-router-quickstart` | config | — | `zerodds-router validate` accepted the two routes |
| `c-ffi-hello` | C | none (bytes) | `got: hello` over the plain C ABI |
| `cpp-hello` | C++ | none (bytes) | `got: hello` through the C++ wrapper |
| `csharp-quickstart` | C# | none (bytes) | `got: hello` |
| `csharp-async` | C# | none (bytes) | async demo OK |
| `csharp-typed` | C# | checked-in `Temperature.cs` | typed sample written on topic `Temp` |
| `java-quickstart-bytes` | Java | none (bytes) | `got: hello` |
| `java-typed-temperature` | Java | checked-in `sensor_msgs/` | `got: 23 / A7` |
| `java-omgpsm-pose` | Java | checked-in `Pose.java` | `Pose[id=r1, x=1.0, y=2.0, z=3.0]` |
| `java-union-codegen-illustration` | Java | checked-in `U.java` / `U8.java` | `OK: A(7) / U8.A.a()=42` |
| `python-hello-bytes` | Python | none (bytes) | `got: b'hello'` |
| `python-idl-struct` | Python | checked-in `temp.py` | `Temperature(celsius=23, sensor_id='A7', …)` |
| `python-qos-writer` | Python | — | writer created |
| `python-aio` | Python | none (bytes) | `b'async hello'` |
| `typescript-node-quickstart` | TS / Node | none (bytes) | `got: hello` |
| `typescript-node-typed` | TS / Node | checked-in `gen/` | typed sample round-tripped |
| `typescript-node-async` | TS / Node | none (bytes) | `src/run.ts` drove writeAsync / takeAsync / streamSamples |


The 28 from the website's crate pages — each one is the "Usage" snippet of
`docs/<name>.html`, made to compile and run, and the page links back to it:

| example | page | what the run showed |
|---|---|---|
| `crate-amqp-bridge` | `docs/amqp-bridge.html` | an AMQP 1.0 `long` back as `AmqpValue::Long(42)` from 2 bytes |
| `crate-amqp-endpoint` | `docs/amqp-endpoint.html` | a sender link settled; `pending_settlements` back to 0 |
| `crate-bridge-security` | `docs/bridge-security.html` | `allow_all().check(alice, Write, /topics/trade) = true` |
| `crate-coap-bridge` | `docs/coap-bridge.html` | RFC 7252 header round-trip, 4 bytes, code 0.01 |
| `crate-corba-codegen` | `docs/corba-codegen.html` | the RepositoryId `IDL:MyModule/MyInterface:1.0` |
| `crate-discovery` | `docs/discovery.html` | a 172-byte SPDP beacon serialized and parsed back to the peer GUID |
| `crate-flatdata` | `docs/flatdata.html` | a POD `Pose` through a 24-byte slot with no serialization step |
| `crate-flatdata-derive` | `docs/flatdata-derive.html` | the derive's `WIRE_SIZE = 24` and its SHA-256-derived `TYPE_HASH` |
| `crate-foundation` | `docs/foundation.html` | CRC-32C and MD5 against RFC 3720 B.4 / RFC 1321 A.5 vectors |
| `crate-grpc-bridge` | `docs/grpc-bridge.html` | gRPC length-prefixed framing, 15 bytes, flag 0 |
| `crate-hpack` | `docs/hpack.html` | RFC 7541: 3 header fields in 27 bytes, round-tripped |
| `crate-http2` | `docs/http2.html` | RFC 9113 §4.1: a 9-byte header + 8-byte PING payload |
| `crate-lint` | `docs/lint.html` | the three pragmas as a difference: 2 errors without them, 0 with |
| `crate-monitor` | `docs/monitor.html` | the Prometheus rendering, and a traceparent through 68 bytes of inline QoS |
| `crate-mqtt-bridge` | `docs/mqtt-bridge.html` | MQTT 3.1.1 §4.7.1 `+` and `#` wildcards both matched |
| `crate-rpc` | `docs/rpc.html` | `Calculator(1, 2) = 3` over a requester/replier pair |
| `crate-security-keyexchange` | `docs/security-keyexchange.html` | both sides derived the same 32-byte secret |
| `crate-sql-filter` | `docs/sql-filter.html` | DDS 1.4 Annex B filter with a positional parameter, `true` |
| `crate-time-service` | `docs/time-service.html` | OMG Time Service 1.1: now vs now+1 ms is `LessThan` |
| `crate-transport-tcp` | `docs/transport-tcp.html` | one accepted connection, 13 bytes through the ZeroDDS handshake |
| `crate-transport-tsn` | `docs/transport-tsn.html` | an 802.1Q tag on the wire as `[81, 00, c0, 64]`, pcp=6 vid=100 |
| `crate-transport-udp` | `docs/transport-udp.html` | a datagram sent to itself and read back; multicast group joined |
| `crate-transport-uds` | `docs/transport-uds.html` | 4 bytes back over the unix socket |
| `crate-types` | `docs/types.html` | XTypes 1.3 §7.6.3: two `Int32` TypeIdentifiers assignable |
| `crate-websocket-bridge` | `docs/websocket-bridge.html` | RFC 6455 §1.3's own nonce/accept pair reproduced |
| `crate-xml` | `docs/xml.html` | a DDS-XML building-block document through both entry points |
| `crate-xml-wire` | `docs/zerodds-xml-wire.html` | DDS-XML 1.0 §6.4: a sample as XML per field and back |
| `crate-zenoh-bridge` | `docs/zenoh-bridge.html` | a topic map entry and `key_expr_for_topic` with a partition |

### Built only (3) — there is no program to run

| example | what the run checked instead |
|---|---|
| `typescript-node-website-snippet` | `tsc --noEmit` over the three website snippets against the real `@zerodds/node` types |
| `typescript-wasm-vite-config` | `tsc --noEmit` over the Vite configuration |
| `idl-fixed-decimal` | `zerodds-idlc check fixed.idl` — the frontend still accepts a `fixed<P,S>` member |

### Blocked (6) — a named defect

| example | what stops it |
|---|---|
| `idl-conformance/_interop/rust-features` | `feat::Money` declares `fixed<5,2>` and `fixed<4,0>`. `zerodds-build` stops with `Codegen { detail: "NoWriter { member: \"price\", kind: \"fixed\" }" }`: `crates/lang-rules/src/encode.rs` has no `TypeKind::Fixed` arm, so the member falls through to `MemberWrite::Unsupported`. Nothing in the rc.7 crates declares a Rust decimal type either, so `canonical_fixed()` has no value type to build. The corpus's `bitmask Perm` and the `@key NestedKey` of `feat::OuterKey` hit the two defects below on top of it. |
| `idl-conformance/proofs/keyhash` | Case (d), `@key NestedKey k`. The generated `key_holder_be()` and `key_hash()` read `self.k.encode(holder)` where `holder` is an owned `zerodds_wire::Writer` and `encode` takes `&mut Writer` — `error[E0308]`, twice. Every other sink the emitter writes into is already a `&mut Writer` parameter. Cases (a)–(c) generate and the rest of the harness is migrated. |
| `idl-conformance/proofs/endianness/harness-rust` | The corpus declares `bitmask Perm`, which is generated as `pub type Perm = u32;`. `dds_type_impls(true)` then writes `impl zerodds_dcps::DdsType for feat::Perm` — a foreign trait on a primitive, `error[E0117]`, and eight `E0308` follow-ons. Without that option there is no XCDR1 codec at all (`zerodds-build` plans the second, XCDR1 pair only under it), so two of the harness's four representations cannot be produced. Either way the bitmask blocks it. |
| `cpp-typed` | rc.7 has no C++ TypeSupport generator. `crates/cpp/include/zerodds/dds.hpp` asks the traits for `type_object()`, `type_object_len()` and a four-argument `decode`; the checked-in `gen/sensor_msgs/msg/Temperature.hpp` came from the retired `zerodds-idl-cpp` and has none of them, and `zerodds-idlc generate --lang c++` emits the data type alone — no `topic_type_support` specialization at all. Five clang errors, all inside `dds.hpp` instantiating `TypedWriter`/`TypedReader`. |
| `native-frontends` | rc.7's `zerodds-idlc generate` emits data declarations only. For Go that is `type Ping struct { … }` and nothing else; `run.sh` was written against the rc.4/rc.6 CLI, whose Go backend also emitted `MarshalXCDR`, `UnmarshalXCDR*` and the XCDR2 `Writer`/`Reader`. `crates/lang-rules/src/adapter/go.rs` has no codec today, and no other rc.7 path emits a Go *library* codec — the `wireprobe-zerodds-go-gen` generators emit a probe program. Independently, `endpoints/go` grew a `uint16` stream parameter on `NewClient` and `NewAsyncWriter` that `pingpong/main.go` does not pass. |
| `native-endpoint-sdk` | The endpoint SDKs and `crates/xrce` disagree about WRITE_DATA. All three endpoints (C, Java, Python) build and send their 48-byte frame; the hub refuses it with `ValueOutOfRange { message: "WRITE_DATA: only FORMAT_DATA is structured in phase 1" }`. Two differences, both from commit `ef32a830521` landing on the crate side only: the SDKs set flags `0x03` (`E_LITTLE_ENDIAN \| FORMAT_SAMPLE`) where the crate takes `0x01` (`FORMAT_DATA`), and they put the sample straight after the submessage header where the crate reads a `BaseObjectRequest` first. |

### Not verified on this host (10)

| example | what was missing |
|---|---|
| `crate-rt-linux` | a Linux kernel. `SchedulerProfile::apply_to_current_thread` is `sched_setscheduler` and `pin_current_thread_to_cpus` is `sched_setaffinity`; on macOS the crate returns `Unsupported` and the demo says so and exits non-zero. The Linux host for it did not answer ping or SSH during this run, and the local Docker daemon was not running, so it has never been executed on a kernel that has those calls.

The nine from the first run:

| example | what was missing |
|---|---|
| `rust-security-plugin-chain` | `ZERODDS_PKI` — the six files DDS-Security 1.2 §9.3.1 Table 44 and §9.4.1.2 Table 46 name. It compiles; it was not run. |
| `typescript-wasm-quickstart` | `crates/ts-wasm` is not built here: `npm install` needs `ws` from the network, and `npm run build` then fails with `TS5011` on its own `rootDir`. |
| `typescript-wasm-website-snippet` | the same |
| `decoded-recorder` | a live writer on the domain; the harness starts none |
| `recorder-replay-workflow` | a live writer on the domain; the harness starts none |
| `cross-vendor-shapes` | Cyclone, Fast DDS, OpenDDS and RTI peers |
| `cross-vendor-rich` | the same four peers (the ZeroDDS side of it builds) |
| `cross-vendor-dynamic-xtypes` | the same four peers |
| `iceoryx-cyclone-zerocopy` | RouDi and a Cyclone built with the iceoryx PSMX plugin |

### `native-frontends` is now in the repository

It was untracked when this file was first written. It is tracked and in the
manifest now, with the verdict `blocked` and the codegen gap that stops it
named on its row — an untracked directory is verified by nobody, which is
exactly the shape of gap the manifest check exists to close.

## Against rc.7

rc.7 replaced the IDL handling. `crates/idl`, `crates/idl-c`, `idl-cpp`,
`idl-python`, `idl-rust`, `idl-ts`, `crates/cdr` and `crates/cdr-derive` are
gone. In their place: `crates/ast*`, `crates/idl-parse`, `crates/idl-lower`,
`crates/idl-compose`, `crates/lang-rules`, `crates/lang-render`,
`crates/wire` (the runtime a generated encoder calls), `crates/zerodds-build`
(the `build.rs` helper) and `tools/idlc`.

An example that names one of the removed crates does not build against a
current checkout. Nine did.

### Lifted to `build.rs` + `zerodds-build` (6)

Each of these now compiles its `.idl` at build time through
`zerodds_build::Config`, with `dds_type_impls(true)` where the example
creates a topic. That option is what writes the `DdsType` impl from the same
model as the codec — including `type_object()`. A hand-written bridge left it
at its default `None`, and the endpoint then announced a topic type name with
no type behind it, so a peer's TypeLookup request came back empty (XTypes 1.3
§7.6.3.2).

| example | was | now |
|---|---|---|
| `idlc-buildrs` | `Command::new("zerodds-idlc")` with the rc.4 flag spelling | `Config::new().include_dir("idl").dds_type_impls(true)` |
| `zerodds-build-sample` | `zerodds-cdr`, `strip_inner_attrs_for_include` | `zerodds-wire`; the option is gone, the generated file carries no inner attributes |
| `rust-xcdr1-representation` | `zerodds_idl::parse` + `generate_rust_module` in `cdr_only` mode | `Config` with `dds_type_impls(true)`, which is what emits the XCDR1 codec pair |
| `rust-typed` | `#[derive(DdsType)]` from `zerodds-cdr-derive` | `temperature.idl` + `Config`; `main` asserts the `TypeObject` |
| `rust-async-showcase` | `#[derive(DdsType)]`, git deps on `zerodds-cdr` | `reading.idl` + `Config`; path deps into the checkout |
| `cross-vendor-rich/zerodds` | `zerodds_idl::parse` + `generate_rust_module` | `Config` with `dds_type_impls(true)` |

### Still dead against rc.7 (4)

`cpp-typed`, `native-frontends`, `native-endpoint-sdk` and three of the eight
Rust crates under `idl-conformance` — see "Blocked" above for what stops each.

`rust-complex-types-roundtrip` is no longer among them. rc.7 **does** emit a
Rust writer for `map<>`: with the `Mode`/`mode` case collision fixed,
`zerodds-build` compiles `demo.idl` including
`map<unsigned long, Vec3> calibration`, and the example round-trips under both
representations (218/200 B with the optional present, 189/172 B without). The
`NoWriter { member: "calibration", kind: "map" }` this table used to name was
the front end refusing the file for the collision before it ever reached the
map.

### What the lift turned up

Two IDL files in this repository were never valid IDL, and rc.7's frontend
says so where the retired one did not. OMG IDL 4.2 §7.2.3: one namespace per
scope, identifiers compared without regard to case.

- `rust-complex-types-roundtrip/demo.idl` declared `enum Mode` and a member
  `Mode mode;`. The enum is now `SensorMode`.
- `zerodds-build-sample/idl/Robot.idl` declared `Pose pose;`. `Pose` now
  lives in `module geometry`, so the member names `geometry::Pose`.

Three examples had drifted from APIs that changed under them, in ways only a
run could show:

- `getting-started-first-publisher`, `observability-stderr-sink` and
  `qos-policies-full-writer` did not set `latency_budget`,
  `destination_order` or `presentation` on `UserWriterConfig` /
  `UserReaderConfig`. `UserSample` had also grown a `CoherentSetEnd` variant
  the walkthrough's `match` did not name.
- `security-governance-gate` built a `RuntimeConfig` with `security` set and
  `security_guid_prefix` unset. rc.7 refuses that at `DcpsRuntime::start`
  (DDS-Security 1.2 §9.3.3) rather than coming up with a random prefix every
  peer rejects at handshake time.
- `rust-security-plugin-chain` carried only the three `dds.sec.auth.*`
  properties. A participant that asks for security also needs the three
  `dds.sec.access.*` ones, and rc.7 refuses an incomplete list instead of
  coming up in plaintext.

The documents these three are copied from were corrected with them:
`documentation/01-getting-started/first-publisher.md`,
`documentation/03-configuration/qos-policies.md` and
`documentation/03-configuration/security.md`, plus
`documentation/04-idl/idlc-handbook.md` §6, which still showed the retired
CLI invocation as the way to compile IDL in a `build.rs`.

## Which release each example builds against

The four examples that take their dependencies from crates.io were pinned to
`1.0.0-rc.3`; they are now on **`1.0.0-rc.6`**, the newest release published
(`async-pubsub`, `bindings-rust-main`, `hello-publisher`, `pubsub-roundtrip`).
The CI's path-dep rewrite moved from `1.0.0-rc.4` to the same `1.0.0-rc.6`.

## What waits on the rc.7 crates.io cut

`crates/wire` and `crates/zerodds-build` did not exist at the `v1.0.0-rc.6`
tag, so no version of `zerodds-wire` or `zerodds-build` is on crates.io. The
six lifted examples therefore build against a checkout and not against the
published crates, and the `crates-io` CI lane names them one by one as out of
lane (a warning per example, never a quiet skip). The `run` lane builds and
runs them against the source, so they are not unmeasured.

When rc.7 is published:

1. Delete the six names from `UNPUBLISHED_DEP_EXAMPLES` in
   `.github/workflows/ci.yml` and set `PUBLISHED=1.0.0-rc.7`.
2. Move the four crates.io-pinned examples from `1.0.0-rc.6` to
   `1.0.0-rc.7` (`Cargo.toml` and the sentence in each README).
3. Re-check the blocked examples. `cpp-typed` needs a C++ TypeSupport
   generator. The three `idl-conformance` crates need three fixes in the Rust
   column of `crates/lang-rules`: a `TypeKind::Fixed` writer (plus a Rust
   decimal type to write), a bitmask that `dds_type_impls` can implement
   `DdsType` for without an orphan-rule violation, and a `key_holder_be()` that
   hands its owned `Writer` to a nested member's `encode` by mutable
   reference.

## To do on a Linux host, or with peers on the wire

None of this needs new preparation — `ci/run-examples.sh` runs it as it
stands once the requirement is there.

1. **The four cross-vendor examples and `iceoryx-cyclone-zerocopy`** need
   Cyclone, Fast DDS, OpenDDS, RTI and RouDi. macOS loopback is not a
   substitute: these measure encapsulation ids and shared-memory hand-off
   against real peers.
2. **`decoded-recorder` and `recorder-replay-workflow`** need a live writer
   on the domain — a ShapesDemo publisher is enough.
3. **`rust-security-plugin-chain`** needs `ZERODDS_PKI` pointing at the six
   DDS-Security files. It has no way to be verified without them, and it says
   which are missing rather than starting an unsecured participant.
4. **`typescript-wasm-quickstart` and `typescript-wasm-website-snippet`**
   need `crates/ts-wasm` built, which needs network for `ws` and a fix for
   the `TS5011` `rootDir` error its own `tsconfig.json` produces with
   TypeScript 5.
5. **`native-endpoint-sdk`** needs the WRITE_DATA framing reconciled between
   `endpoints/*` and `crates/xrce` first; a different host will not change
   the result.

## The languages with no example at all

`endpoints/` carries endpoint SDKs for 22 languages: ada, ada-83, ada-native,
c, cpp, csharp, d, elixir, fsharp, go, java, julia, kotlin, lua, nim, node,
ocaml, python, rust, swift, typescript, zig.

Exactly one example in this repository exercises any of them —
`native-endpoint-sdk`, which drives **c**, **java** and **python**. The
examples for cpp, csharp, rust and typescript go through the language
*bindings* (the C ABI, JNI, PyO3, koffi), not the endpoint SDKs.

That leaves **19 endpoint SDKs with no runnable example**: ada, ada-83,
ada-native, cpp, csharp, d, elixir, fsharp, go, julia, kotlin, lua, nim,
node, ocaml, rust, swift, typescript, zig. On this host every one of them
except ada, ada-83 and d has a toolchain available, so the gap is not a
hardware problem.

`native-frontends` holds a Go frontend for exactly this. It is tracked now,
and blocked: see its row under "Blocked".

## Back-links to the website

Every README ends with the same `## Reference` block: the page that explains
the example, the example's own URL in this repository, and the project line.
66 of the 88 have a page; the rest say so rather than leaving the reader to
guess.

The 29 `crate-*` examples close the loop in both directions: each README
names its `docs/<name>.html`, and each of those 29 pages carries a
"Runnable example" / "Lauffähiges Beispiel" section — one per language block,
so the link is there in EN and in DE — pointing at the directory.

Two things measured while writing them:

- The website links examples as `zero-dds-snippets/tree/master/<name>` in 35
  places and `…/tree/main/<name>` in 2. The branch here is **`main`**; every
  link written into this repository uses it.
- The READMEs used `zerodds.de`, the site's own pages use `zerodds.org`
  (218 occurrences against 4 in the ZeroDDS tree). All back-links are now
  `zerodds.org`.
- `java-omgpsm-pose` linked `/docs/java-omgdds/`; the file is
  `docs/java-omgdds.html`, which is what the back-link now names.

## What making the website's snippets run turned up

A snippet on a page is not checked by anything. Five did not compile against
**any** release — not rc.6, which the pages describe, and not the current
tree. Each was corrected on the page (EN and DE) as well as here:

| page | what was wrong |
|---|---|
| `docs/rpc.html` | Five things in one block. `Requester`, `Replier`, `RpcQos` and `FnHandler` are not re-exported at the crate root (they live in `requester`, `replier`, `qos_profile`); `DomainParticipant::new` is `pub(crate)`, so a participant comes from `DomainParticipantFactory`; the handler parameter is `Arc<dyn ReplierHandler<TIn, TOut>>`, so `FnHandler::new(…)` has to be wrapped; and the closure returns `Result<TOut, RemoteExceptionCode>`, not `TOut`. |
| `docs/rt-linux.html` | `SchedulerProfile::fifo(80).expect(…)`. `SchedulerProfile` is an enum with a `RealtimeFifo { priority }` variant and no `fifo` constructor, and building a variant yields the value, not a `Result`. |
| `docs/transport-tsn.html` | The type was spelled `IEEE802VlanTag` in six places. It is `Ieee802VlanTag`. |
| `docs/zenoh-bridge.html` | `TopicMap::add` takes one `TopicMapEntry`, not a (topic, key) pair; `key_expr_for_topic` takes `(prefix, partition, topic)`, not `(topic, Option<prefix>)`; and the asserted result could not occur, because `sanitize` replaces only the Zenoh wildcards `* ? [ ] $ #` — a `.` in a DDS topic name stays a `.`. |
| `idl/index.html` | The build.rs block called `zerodds_idlc::build()`. `tools/idlc` is a `[[bin]]`-only crate with no `src/lib.rs`, at the rc.6 tag and now — a build script cannot link `zerodds_idlc`, let alone call `build()` on it. The helper is `zerodds_build::Config`, which is what `idlc-buildrs` in this repository compiles and runs; the page links it now. |

Two defects in the crates themselves, both found by trying to write the caller
the page describes:

- `zerodds-zenoh-bridge` did not re-export `TopicMapEntry`, `ZenohReliability`
  or `ZenohCongestion` from its crate root. `TopicMap::add` is public and takes
  `TopicMapEntry`, and `dds_qos_to_zenoh` returns the other two, so all three
  were types no caller outside the crate could name. Fixed in the ZeroDDS tree.
- `idlc generate` wrote a `//` line-comment header onto **every** language's
  output, including Python, Elixir, Julia, Nim, Ada, Ada 83, Lua and OCaml,
  where `//` is not a comment — line 1 of those files does not parse, and
  `generate` still reported "0 error(s)". Go and F# additionally got no
  `package` / `module` clause, which is what makes a file a translation unit
  in either language. Both fixed in the ZeroDDS tree, with a test per
  language over the delimiters and the clause.

Of the 72 site pages that carry a program-code block, 68 now link a runnable
example. The four that do not: `docs/rmw-zerodds-shim.html` (an ASCII layer
diagram and shell, no program), `bindings/index.html` (one dependency-install
line per language), and `idl/constructs/constant.html` plus the IDL blocks of
`idl/index.html` (IDL declarations, not a program).

Six more snippets name variables they never define (`participant_data`,
`incoming`, `dest`, `datagram`, `local_id`, and the document `xml` parses).
Those are legitimate as page prose — the example supplies them and says so in
its header comment.
