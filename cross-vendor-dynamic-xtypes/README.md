# Cross-vendor dynamic XTypes — decode any vendor's topic with no compile-time type

The other cross-vendor examples ([`../cross-vendor-rich`](../cross-vendor-rich),
[`../cross-vendor-shapes`](../cross-vendor-shapes)) prove ZeroDDS speaks the same
wire as the four big DDS stacks for a **compile-time-known** type. This one proves
the opposite direction: ZeroDDS decodes a topic published by **CycloneDDS, eProsima
Fast DDS, OpenDDS and RTI Connext** when it has **no generated Rust type at all** —
only the runtime `DynamicType` it builds from the IDL it discovered on the wire.

`zerodds-spy` follows a topic, learns each writer's real `type_name` from discovery,
builds a runtime [`DynamicType`], and runs the **reflective XCDR codec** (a
`DynamicData` ⟷ CDR walk that reuses the same hardened framing the typed codec
uses) to turn raw samples into JSON:

```
$ zerodds-spy -t TrackTopic --type-file dyn.idl --decode
zerodds-spy: attached topic=TrackTopic type=dyn::Track
[1] writer=… type=dyn::Track {"color":2,"command":{"speed":7006},"id":106,"name":"track-6","path":[6,12,18]}
```

The `--type-file` is an out-of-band schema, not a compiled type — ZeroDDS never
links a generated `Track`. (Fetching the schema from the wire via the XTypes
TypeLookup service instead of `--type-file` is the next increment.)

## The type — a struct with every aggregate shape, incl. a union

```idl
module dyn {
  enum Color { RED, GREEN, BLUE };
  @mutable union Cmd switch(long) {           // dyn.idl  — Fast DDS leg
    case 1: long    speed;
    case 2: double  angle;
    default: octet  flag;
  };
  @appendable struct Track {
    @key long       id;
    string          name;
    Color           color;                     // enum
    sequence<long>  path;                       // sequence
    Cmd             command;                     // union member
  };
};
```

Two IDL variants ship here because the vendor IDL compilers disagree on what they
accept:

- [`dyn.idl`](dyn.idl) — the **`@mutable` union**. Only Fast DDS's `fastddsgen`
  compiles it directly; CycloneDDS `idlc` rejects it (*"Mutable unions are not
  supported yet"*) and OpenDDS/RTI also balk.
- [`dyn_app.idl`](dyn_app.idl) — the same type with an **`@appendable` union**, the
  portable variant for CycloneDDS, OpenDDS and RTI. The wire differs (appendable vs
  mutable union framing), and ZeroDDS decodes both — that is the point.

## Results — all four vendors, both XCDR dialects

ZeroDDS-spy reflectively decoded each vendor's live `TrackTopic`, including **every
union branch** (`speed` / `angle` / `flag`):

| publisher | XCDR version | union | decoded sample (abridged) |
|---|---|---|---|
| **eProsima Fast DDS 3.x** | XCDR1 *and* XCDR2 | `@mutable` | `{"command":{"speed":7006},…"name":"track-6"}` |
| **CycloneDDS** | XCDR2 | `@appendable` | `{"command":{"angle":14.5},…"name":"ctrack-13"}` |
| **RTI Connext 7.7** | XCDR1 | `@appendable` | `{"command":{"flag":8},…"name":"rtrack-8"}` |
| **OpenDDS 3.34** | XCDR2 | `@appendable` | `{"command":{"speed":7015},…"name":"otrack-15"}` |

Fast DDS defaults to **XCDR1** (classic CDR / `PL_CDR1` for the mutable union) and
can be forced to **XCDR2** (`D_CDR2` + `PL_CDR2`); ZeroDDS decodes both, so the
reflective codec is exercised on every framing the four stacks emit:

- **XCDR1**, `@appendable` struct → plain (no DHEADER), `@mutable` → `PL_CDR1`
  (16-bit-PID parameter list + `PID_LIST_END`). Covered by Fast DDS-default + RTI.
- **XCDR2**, `@appendable` → DHEADER frame, `@mutable` → `PL_CDR2` (DHEADER +
  EMHEADER list). Covered by Fast DDS-forced + CycloneDDS + OpenDDS.

## What this surfaced — four reflective-codec bugs, found by diffing real wire

Self-consistency (encode-then-decode within ZeroDDS) hid all four; only a byte-diff
against a live vendor sample exposed them:

1. The IDL→`DynamicType` loader skipped `union` definitions entirely, so a struct
   with a union member fell back to a hex dump.
2. The mutable-union decoder matched the selected branch by member id — but vendors
   number union cases from 1 after the reserved discriminator id 0 (Fast DDS emits
   case id 2 for the 2nd case), while a DynamicType built from an out-of-band IDL
   numbers them from 0. Fixed by matching the single non-discriminator member **by
   position**.
3. The `@mutable` struct/union framing wrote/read the EMHEADER list **without** the
   `PL_CDR2` DHEADER that bounds it — self-consistent, but every vendor (and the
   typed codec) frames it, so a nested mutable member ran past its bounds.
4. The reflective codec implemented **only XCDR2**, so it mis-decoded the default
   wire of Fast DDS / RTI (both XCDR1). Added the XCDR1 / `PL_CDR1` path.

## Running it

Each `*/` holds the vendor publisher. Build it against that vendor's SDK, run it on
a domain, and run `zerodds-spy` against the same domain:

```
# vendor side (example: CycloneDDS)
cc -I<cyclone>/include cyclone/pub.c dyn.c -L<cyclone>/lib -lddsc -o cyc_pub && ./cyc_pub 0

# ZeroDDS side — reflective decode, no compiled type
zerodds-spy -t TrackTopic --type-file dyn_app.idl --decode
```

Per-vendor build notes (SDK paths, the IDL-compiler quirks, the RTI transport/QoS
flags) are in each subdirectory's `README.md`.

Part of **[ZeroDDS](https://zerodds.de)**.
