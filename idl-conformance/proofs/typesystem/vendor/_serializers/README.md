# Vendor native serialisers

These are the programs used to produce the committed `vendor/<vendor>/*.bin`
goldens. Each one serialises the `CANONICAL.md` samples with the vendor's OWN
type support at XCDR2 and writes the bare payload (the 4-byte CDR encapsulation
header is stripped). They are committed for reproducibility; building them
requires the vendor SDK + the vendor-generated type support for `typesystem.idl`.

| file | build (after generating the vendor type support next to it) |
|------|-------------------------------------------------------------|
| `cyclonedds_serialize.c` | `idlc typesystem.idl` → `gcc serialize.c typesystem.c -I$CYCLONE/include -lddsc` |
| `fastdds_serialize.cpp`  | `fastddsgen typesystem.idl` → `g++ -std=c++17 serialize.cpp typesystemTypeObjectSupport.cxx -I$FASTDDS/include -lfastcdr -lfastdds` |
| `rti_serialize.cxx`      | `rtiddsgen -language C++11 typesystem.idl` → `g++ -std=c++11 serialize.cxx typesystem.cxx typesystemPlugin.cxx -I$NDDSHOME/include/ndds/hpp -lnddscpp2 -lnddsc -lnddscore` |

Notes:
- The Fast DDS and RTI serialisers were built against an IDL with the
  enum-discriminated union / `@bit_bound` enums removed only where the vendor
  code generator could not compile them (Fast DDS-Gen mis-generates an
  enum-switch union with a `default:`; RTI rejects non-32-bit `@bit_bound`).
  Those constructs are proven against the vendors that DO support them.
- Each serialiser writes to `/tmp/oracle-typesys/out/<case>.<vendor>.bin`; the
  files were then copied to `vendor/<vendor>/<case>.bin`.
