# ZeroDDS ↔ OpenSplice — the @final cell

OpenSplice Community Edition 6.9 has no XTypes and no extensibility kinds:
`idlpp` takes no annotations, reads keys only through `#pragma keylist`, and
the product writes XCDR1 only. Of the three variants of this example only
`RichF` has a counterpart there, so this leg measures the `@final` cell, in
both directions. [`Rich.idl`](Rich.idl) is `RichF` in the form `idlpp`
reads; its XCDR1 bytes are those of the `@final` original.

```
=== A/f) OpenSplice pub -> ZeroDDS sub (domain 73) ===
ZeroDDS received from OpenSplice (@f): 46  (corrupt: 0)
  <- id=1 value=3.142 blob=50 integrity=OK
=== B/f) ZeroDDS pub -> OpenSplice sub (domain 74) ===
OpenSplice received from ZeroDDS (@f): 66  (corrupt: 0)
  <- id=1 value=3.14159 blob=50 integrity=OK
RESULT: PASS (the @final cell, both directions)
```

Four runs on one Linux host, all passed. Each side checks every sample it
takes: `blob[k] = k % 251`, and on the OpenSplice side also `value = π · id`.

An OpenSplice participant takes its domain from the configuration file
`OSPL_URI` names; `run.sh` writes one per domain from
`etc/config/ospl_sp_ddsi.xml`. OpenSplice refuses domain ids above 230.

## Run it

```sh
OSPL_HOME=/opt/opensplice-6.9 ./run.sh
```

Needs OpenSplice 6.9 under `OSPL_HOME`, g++ and cargo; `run.sh` builds the
ZeroDDS side and, the first time, the OpenSplice peer.
