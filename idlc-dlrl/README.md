# idlc-dlrl

`zerodds-idlc dlrl` turns the three DDS 1.2 §8 DLRL pragmas —
`DCPS_DATA_TYPE`, `DCPS_DATA_KEY`, `DCPS_DLRL_RELATION` — into DLRL object
and home classes. Of its four languages (C++, Java, C#, TypeScript),
TypeScript is the one whose output names no DLRL runtime type, so it
compiles as generated; the other three derive from the PSM's `ObjectRoot`
and `ObjectHome`, which ZeroDDS does not ship for them.

`trade.idl` declares two object types with their keys and a relation.
`use.ts` is application code against the generated interfaces; `run.sh`
generates, type-checks both files with `tsc --strict` and runs the result.
Each key carries the type its member has in the IDL: `venue` is a
`string`, `id` a `number`. Every other member is an attribute (DDS 1.2
§8.1.2.2, §8.1.4.3): `price` is mono-valued, read and set through the
object; `tags` (a `sequence`) and `fills` (an array) are lists and `fees`
(a `map` keyed by `string`) a map, each changed through the collection
itself. A member whose type the generator cannot map — a union, a nested
collection, a structure containing a collection — fails the run.

An attribute of an IDL enumeration or structure is typed with what `idlc
generate --lang typescript` declares for it, and the generated file
imports it from `./trade.js`; that output needs the `@zerodds/wire`
runtime, so this example keeps to basic types. The main repository
compiles both kinds together (`tools/idlc/tests/dlrl_compiles.rs`).

## Run it

```sh
ZERODDS_SRC=/path/to/zero-dds ./run.sh
```

Needs `tsc` and `node`. Output of the run this README was written from:

```
DLRL TYPESCRIPT OK: venue="" id=0 price=101.25 legs=0 tags=fx,spot filled=100 clearing=0.25
```
