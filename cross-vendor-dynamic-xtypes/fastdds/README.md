# Fast DDS publisher — the `@mutable` union leg

Fast DDS is the only one of the four whose IDL compiler accepts the `@mutable`
union in [`../dyn.idl`](../dyn.idl) directly, so it carries the mutable-union proof.

```
fastddsgen -replace dyn.idl          # generates dyn{,PubSubTypes,TypeObjectSupport}
cmake -B build -DCMAKE_PREFIX_PATH=<fastdds-install> && cmake --build build
./build/dyn_fd_pub 0                  # domain 0
```

- Fast DDS defaults to **XCDR1**. To exercise the XCDR2 path, set the writer's
  data representation: `wqos.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION)`.
  ZeroDDS decodes both (XCDR1 `PL_CDR1` and XCDR2 `PL_CDR2` union framing).
- `fastddsgen` is case-insensitive on identifiers: a member named `cmd` collides
  with the union type `Cmd`. The member is named `command` to avoid it.
