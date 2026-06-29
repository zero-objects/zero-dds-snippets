# Recorder → replay workflow — capture, transcode, replay from every format

A production capture/replay loop built only from the rc.4 CLI tools, end to end:

1. a **publisher** writes `ShapeType` samples (the OMG ShapesDemo type — any
   vendor's ShapesDemo works as the source too),
2. **`zerodds-record --decode`** captures them to **three** artifacts at once —
   the binary `.zddsrec`, a typed **SQLite** table, and a typed **NDJSON** file
   — following the writer's type from `shapes.idl` with *no compiled type*,
3. **`zerodds-replay`** re-publishes from **each** of the three formats,
4. a **subscriber** reads the replayed stream back and confirms the round-trip.

This is the difference from the [`decoded-recorder`](../decoded-recorder) demo:
that one shows the decode *outputs*; this one shows the full
**capture → transcode → replay** operations loop and that all three persisted
forms replay byte-for-byte the same samples.

## Files

| File | What |
|------|------|
| `shapes.idl` | Out-of-band schema handed to `--decode --type-file` (mirrors `zerodds_dcps::interop::ShapeType`). |
| `run.sh` | One-shot: publisher → record(3 formats) → replay(×3) → subscriber check. |

## Run it (Linux)

```bash
./run.sh
```

…or by hand:

```bash
# 1) capture 6 s of Square into .zddsrec + typed SQLite + typed NDJSON
zerodds-record record -t Square -d 0 --duration 6s -o cap.zddsrec \
  --decode --type-file shapes.idl --map Square=ShapeType \
  --out-sqlite cap.db --out-json cap.ndjson &
# 2) feed it (any ShapesDemo publisher on domain 0 works)
cargo run -p zerodds-dcps --example shapes_demo_publisher -- Square BLUE 0

# 3) re-inject from each persisted form onto a fresh domain (1)
zerodds-replay replay cap.zddsrec --inject --inject-domain 1   # from binary
zerodds-replay replay cap.ndjson  --inject --inject-domain 1   # from NDJSON
zerodds-replay replay cap.db      --inject --inject-domain 1   # from SQLite

# 4) watch the replayed stream
cargo run -p zerodds-dcps --example shapes_demo_subscriber -- Square 1
```

`zerodds-replay` auto-detects the format by extension
(`.zddsrec | .ndjson/.json | .db`) and re-stamps the original `raw_cdr` onto the
wire, so a sample captured once can be replayed from whichever artifact a
downstream tool kept.

> **Loopback note:** live discovery needs multicast loopback — reliable on
> Linux, flaky on macOS (see project notes). The `--decode` parse + the three
> sinks are exercised headless in CI (`tools/record` live decode test).

## What to try next

- Add `--filter "x > 100"` to the `record` step — the recorder resolves
  `ShapeType` reflectively and only keeps matching samples in all three sinks.
- Point step 2 at a **CycloneDDS / Fast DDS / RTI / OpenDDS** ShapesDemo and
  capture a *foreign* vendor's stream with the same command — the reflective
  codec follows their wire type.
