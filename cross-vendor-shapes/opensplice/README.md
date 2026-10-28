# ZeroDDS ↔ OpenSplice

Live RTPS interop on the ShapesDemo `Square` topic, **both directions**, with
a small OpenSplice Community Edition 6.9 console publisher/subscriber written
against its classic C++ API (SACPP).

```
=== A) OpenSplice pub -> ZeroDDS sub ===
ZeroDDS received from OpenSplice: 58
=== B) ZeroDDS pub -> OpenSplice sub ===
OpenSplice received from ZeroDDS: 59
RESULT: PASS (both directions; A=58 B=59)
```

Three runs on one Linux host, all with these counts.

## What is different about OpenSplice

- **No XTypes.** OpenSplice 6.9 predates XTypes: its IDL compiler `idlpp`
  takes no annotations and reads keys only through `#pragma keylist`, and its
  DDSI service writes and reads XCDR1 only. [`ShapeType.idl`](ShapeType.idl)
  here is the shared type in that form: the same four members, `color` as the
  key. Its XCDR1 bytes are those of the `@final` original, and ZeroDDS offers
  XCDR1 first, so the two match without configuration.
- **The domain comes from the configuration file.** An OpenSplice
  participant takes its domain from the file `OSPL_URI` names, not from the
  call that creates it. `run.sh` writes one for `DOMAIN` from
  `etc/config/ospl_sp_ddsi.xml` (single process, DDSI networking) and points
  `OSPL_URI` at it. OpenSplice refuses domain ids above 230.
- **Logs in the working directory.** OpenSplice writes `ospl-info.log` and
  `ospl-error.log` where it runs; `run.sh` starts both programs in its log
  directory.

## Setup

OpenSplice Community Edition 6.9 built from source
(`github.com/ADLINK-IST/opensplice`), installed under `OSPL_HOME` (default
`/opt/opensplice-6.9`), and g++. `run.sh` sources `release.com`, runs
`idlpp -S -l cpp ShapeType.idl` and builds `publisher.cpp` and
`subscriber.cpp` against `libdcpssacpp` the first time.

```sh
(cd ../zerodds && cargo build --release)
OSPL_HOME=/opt/opensplice-6.9 ./run.sh
```
