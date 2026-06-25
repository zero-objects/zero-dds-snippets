# OpenDDS publisher — `@appendable` union (XCDR2)

Two OpenDDS-specific gotchas, both fixed here:

1. **Build against `opendds-secure`**, not a non-security OpenDDS build — otherwise
   the consuming compile fails with `'DDS::Security' has not been declared` from
   `Discovery.h` (the headers are security-enabled but the consumer is not):
   `cmake -DOpenDDS_DIR=<opendds-secure>/cmake ..`
2. The struct needs **`@topic`** — without it `opendds_idl` generates the struct
   but no `TrackTypeSupport`, so `dyn::TrackTypeSupport_var` does not exist. See
   [`dyn.idl`](dyn.idl).

`tao_idl` is also case-insensitive: the member `color` collides with the enum
`Color`, so the OpenDDS variant renames it to `hue` (a member rename is invisible
on the wire — the reflective decode is positional — so ZeroDDS still labels it
`color` from its own `dyn_app.idl`).

```
source <opendds-secure>/setenv.sh
cmake -B build -DOpenDDS_DIR=<opendds-secure>/cmake && cmake --build build
./build/od_pub -DCPSConfigFile rtps.ini
```
