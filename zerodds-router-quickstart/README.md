# zerodds-router quickstart — cross-domain bridge + topic rename

`zerodds-router` is ZeroDDS's first-class routing daemon (shipped in every
packaging channel as of 1.0.0-rc.4). It reads samples from input endpoints and
re-publishes them on output endpoints — bridging **DDS domains**, **renaming
topics**, and optionally **filtering** (SQL content filter) or **transforming**
(per-field rename / constant) on the way through.

This quickstart wires the simplest useful topology: forward the OMG **Shapes**
demo from **domain 0 → domain 1**, with one topic forwarded as-is and one
renamed.

```
 domain 0                      zerodds-router                    domain 1
 ────────                   (shapes-bridge config)               ────────
 Square  ──┐                                                ┌──►  Square
           ├─ route square-forward ─────────────────────────┤
 Circle  ──┘                                                └──►  CircleMirror
           └─ route circle-rename (preserve source timestamp) ─►
```

Both routes use the **type-agnostic byte path** (input and output `type_name`
match the on-the-wire type, `ShapeType`), so no IDL or type registration is
needed in the router — it forwards the CDR payload verbatim. Add a `filter` or
`transform` to a route and the router resolves the type reflectively via XTypes
(see the crate docs).

## Files

| File | What |
|------|------|
| `router.json` | The route config in JSON (`--config` default format). |
| `router.xml`  | The same config in the native `<zerodds_routing>` XML form. The parser also accepts a subset of the **RTI Routing Service** `<dds><routing_service>` schema, so an existing RTI config is a near drop-in migration. |
| `run.sh`      | One-shot demo: subscriber on domain 1, router, publisher on domain 0. |

## Validate the config (no DDS traffic)

```bash
cargo run -p zerodds-routing-service --bin zerodds-router -- \
  validate --config router.json
# config 'shapes-bridge' OK — 2 route(s):
#   square-forward: domain 0 'Square' → domain 1 'Square'
#   circle-rename: domain 0 'Circle' → domain 1 'CircleMirror'
```

## Run it (Linux)

```bash
./run.sh
```

…or by hand, in three terminals:

```bash
# 1) subscriber on the OUTPUT domain (1), watching the forwarded topic
cargo run -p zerodds-dcps --example shapes_demo_subscriber -- Square 1

# 2) the router (bridges domain 0 → domain 1)
cargo run -p zerodds-routing-service --bin zerodds-router -- run --config router.json

# 3) publisher on the INPUT domain (0)
cargo run -p zerodds-dcps --example shapes_demo_publisher -- Square BLUE 0
```

The subscriber on domain 1 prints `Square` samples even though the publisher
only ever wrote to domain 0 — they crossed the domain boundary through the
router. Publish `Circle` on domain 0 and it arrives on domain 1 as
`CircleMirror` (the `circle-rename` route).

> **Loopback note:** cross-domain discovery needs multicast loopback, which is
> reliable on Linux but flaky on macOS (see project notes). Run the live demo on
> Linux; `validate` works anywhere.

## What to try next

- Add a third route with a `filter` (`"filter": { "expression": "x > 100" }`)
  to forward only part of the stream — the router resolves `ShapeType`
  reflectively to evaluate the DDS SQL grammar.
- Point `output.domain` at a different transport/partition to bridge isolated
  segments.
- Swap `--config router.json` for `--config router.xml` to drive the same
  topology from the RTI-compatible XML.
