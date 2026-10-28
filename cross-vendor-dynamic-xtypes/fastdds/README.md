# Fast DDS publisher — the `@mutable` union leg

Fast DDS is the only one of the four whose IDL compiler accepts the `@mutable`
union in [`../dyn.idl`](../dyn.idl) directly, so it carries the mutable-union proof.

`run.sh` generates the type support with `fastddsgen`, builds the publisher
into `build/` the first time and runs it against `zerodds-spy --dialect fastdds`
on domain 80:

```
bash run.sh            # FASTDDS_HOME, FASTDDSGEN and DOMAIN override
```

- Fast DDS defaults to **XCDR1**. To exercise the XCDR2 path, set the writer's
  data representation: `wqos.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION)`.
  ZeroDDS decodes both (XCDR1 `PL_CDR1` and XCDR2 `PL_CDR2` union framing).
- `fastddsgen` is case-insensitive on identifiers: a member named `cmd` collides
  with the union type `Cmd`. The member is named `command` to avoid it.
