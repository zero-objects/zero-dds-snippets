# CycloneDDS publisher — `@appendable` union (XCDR2)

CycloneDDS `idlc` rejects the `@mutable` union (*"Mutable unions are not supported
yet"*), so this leg uses the portable [`../dyn_app.idl`](../dyn_app.idl) (appendable
union). CycloneDDS emits XCDR2 for the appendable struct.

```
idlc dyn_app.idl                      # -> dyn.c / dyn.h  (rename dyn_app.idl -> dyn.idl first)
cc -I<cyclone>/include pub.c dyn.c -L<cyclone>/lib -lddsc -o cyc_pub
./cyc_pub 0
```

[`pub.c`](pub.c) uses the Cyclone C API (`dds_create_writer` + `dds_write`); the
union is the generated `dyn_Cmd { int32_t _d; union { … } _u; }`.
