# iceoryx-cyclone-zerocopy

Bidirectional **same-host zero-copy** interop between **ZeroDDS** and
**Cyclone DDS** over the **iceoryx** C++ (POSH) shared-memory transport — no
serialization and no UDP loopback on the hot path, the sample is published once
into shared memory and read in place by the other middleware.

```
  READ : Cyclone publisher  ──iceoryx SHM──▶ ZeroDDS reader   (READER OK)
  WRITE: ZeroDDS writer     ──iceoryx SHM──▶ Cyclone subscriber (CYCLONE OK)
```

Both ZeroDDS ends use the `zerodds-iceoryx-cyclone` crate, which talks to the
iceoryx daemon (`iox-roudi`) through `libiceoryx_binding_c` — the exact same
shared-memory service Cyclone's `psmx_iox` plugin uses, so the two stacks share
one iceoryx service and exchange chunks directly.

## How it works

- A ZeroDDS sample is written into an iceoryx chunk laid out as Cyclone expects
  it: `[iox ChunkHeader][dds_psmx_metadata_t user header][CDR payload]`. The
  user header carries the writer's real RTPS GUID and the
  `sample_state`/`cdr_identifier` Cyclone needs to accept it.
- For the WRITE direction Cyclone is configured with
  `ALLOW_NONDISCOVERED_WRITERS` so it accepts the ZeroDDS writer (which is not
  an RTPS-discovered DDS writer). That flag makes Cyclone's *own* publisher
  iox-ineligible, so the two directions use two configs
  (`cyclone_read.xml` / `cyclone_write.xml`) — see the comments in each.
- Do **not** add the legacy `<SharedMemory><Enable>` element to the Cyclone
  config: it spawns a shadow default PSMX that overrides `INSTANCE_NAME`.

## Requirements (Linux only)

- iceoryx POSH stack + `iox-roudi` on `PATH`, `libiceoryx_binding_c.so`.
- Cyclone DDS with the iox PSMX plugin (`libpsmx_iox.so`) — set `CYCLONE` to the
  install prefix (default `/opt/cyclone`).
- A checked-out [zero-dds](https://github.com/zero-objects/zero-dds) source tree
  — set `ZERODDS` to it (the script builds `crates/iceoryx-cyclone` with
  `--features posh`).

## Run

```sh
CYCLONE=/opt/cyclone ZERODDS=/path/to/zero-dds ./run.sh
```

Expected tail:

```
=== READ: Cyclone publisher -> ZeroDDS reader ===
  READ PASS
=== WRITE: ZeroDDS writer -> Cyclone subscriber ===
  WRITE PASS
PROOF: bidirectional ZeroDDS <-> Cyclone iceoryx SHM — PASS
```

> Verified live on a Linux host (iceoryx POSH + iox-roudi + Cyclone DDS): both
> directions `PASS`. This demo requires the shared-memory stack, so it does not
> run on macOS/Windows; the `zerodds-iceoryx-cyclone` crate still *builds*
> everywhere (the `posh` feature is what links the iceoryx FFI).

## Files

| File | Role |
|------|------|
| `sample.idl` | the topic type, shared by both stacks |
| `cyclone_pub.c` / `cyclone_sub.c` | the Cyclone peer (idlc-generated `sample.c`) |
| `cyclone_read.xml` / `cyclone_write.xml` | Cyclone iox PSMX config per direction |
| `roudi.toml` | iceoryx daemon shared-memory pool config |
| `run.sh` | builds both peers, starts roudi, runs READ + WRITE |

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
The ZeroDDS reader/writer bins live in the `zerodds-iceoryx-cyclone` crate.
