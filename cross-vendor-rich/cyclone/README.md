# CycloneDDS — rich-type pub/sub (Python)

Uses the [`cyclonedds`](https://pypi.org/project/cyclonedds/) Python binding. The
type is defined three times in [`cyc_rich.py`](cyc_rich.py) — `@final` /
`@appendable` / `@mutable` decorators with a `@keylist(["id"])` — matching the
ZeroDDS topic and type names exactly.

```sh
pip install cyclonedds        # needs a CycloneDDS C library on the system
```

Run (argument 2 = `f` / `a` / `m`):

```sh
python cyc_rich_sub.py 0 m            # subscriber, @mutable
python cyc_rich_pub.py 0 m 50         # publisher, @mutable, 50-byte blob
```

## What this shows

CycloneDDS is a **hybrid** on the wire: it emits XCDR1 (plain CDR) for `@final`
but XCDR2 (D_CDR2 / PL_CDR2) for `@appendable` / `@mutable`. ZeroDDS reads both and
writes whatever the type declares — both directions decode with `integrity=OK`.
The reader is best-effort so a late join against a volatile writer never stalls.

Part of **[ZeroDDS](https://zerodds.de)**.
