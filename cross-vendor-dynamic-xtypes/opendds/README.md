# OpenDDS publisher — `@appendable` union (XCDR2)

Two OpenDDS-specific gotchas, both fixed here:

1. **Build against `opendds-secure`**, not a non-security OpenDDS build — otherwise
   the consuming compile fails with `'DDS::Security' has not been declared` from
   `Discovery.h` (the headers are security-enabled but the consumer is not):
   `cmake -DOpenDDS_DIR=<opendds-secure>/cmake ..`
2. The struct needs **`@topic`** — without it `opendds_idl` generates the struct
   but no `TrackTypeSupport`, so `dyn::TrackTypeSupport_var` does not exist. See
   [`dyn.idl`](dyn.idl).

`tao_idl` is also case-insensitive: a member `color` collides with the enum
`Color`, so the member is `hue`, in `../dyn_app.idl` as well.

The key member carries `@must_understand`, which XTypes 1.3 §7.2.2.4.4.4.8
makes implicit and opendds_idl 3.34 sets only when it is written; without it
the two minimal TypeObjects hash differently and OpenDDS matches identical type
identifiers only.

`run.sh` does the build the first time and runs the publisher against
`zerodds-spy` on domain 81:

```
DDS_ROOT=<opendds> bash run.sh
```

By hand:

```
source <opendds>/setenv.sh
cmake -S . -B build/cmake -DOpenDDS_ROOT=<opendds> && cmake --build build/cmake
./build/cmake/od_pub -DCPSConfigFile rtps.ini 81
```
