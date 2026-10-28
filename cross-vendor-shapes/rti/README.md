# ZeroDDS ↔ RTI Connext DDS

Live RTPS interop on the ShapesDemo `Square` topic, **both directions**, with the
`rtiddsgen`-generated C++11 example as the RTI peer.

```
=== A) RTI pub -> ZeroDDS sub (domain 91) ===
ZeroDDS received from RTI: 11
=== B) ZeroDDS pub -> RTI sub (domain 91) ===
RTI received from ZeroDDS: 40
RESULT: PASS (both directions; A=11 B=40)
```

(The `rtiddsgen` example publisher sends a short counter sequence with an empty
`color`; ZeroDDS decodes it correctly — that is the interop proof. The reverse
direction carries the full shape.)

## Generate the RTI peer

`rtiddsgen` (ships with Connext) emits a complete pub/sub example + makefile from
[`ShapeType.idl`](ShapeType.idl) (`@final`, type name **`ShapeType`**). Verified
against **RTI Connext 7.7 / rtiddsgen 4.7**.

```sh
export NDDSHOME=<connext-install>
$NDDSHOME/bin/rtiddsgen -language C++11 -example x64Linux4gcc8.5.0 \
    -unboundedSupport -replace ShapeType.idl
# point both endpoints at the standard ShapesDemo topic:
sed -i 's/"Example ShapeType"/"Square"/g' ShapeType_publisher.cxx ShapeType_subscriber.cxx
make -f makefile_ShapeType_x64Linux4gcc8.5.0
```

## `-unboundedSupport`

Without it `rtiddsgen` bounds the unbounded `@key string color` at 255
(`initialize_string_typecode((255L))` in the generated `ShapeType.cxx`). The
ZeroDDS writer's key is unbounded, as the IDL says, and a reader whose key
member is bounded shorter than the writer's is not assignable from it (XTypes
1.3 §7.2.4.4.8, Table 19): the RTI reader received nothing from ZeroDDS. The
other direction is assignable and was not affected. `run.sh` keeps the
generator arguments next to the build and generates the peer again when they
change.

## XCDR2 on the reader

Out of the box the RTI reader does **not** receive from the ZeroDDS writer: ZeroDDS
sends **XCDR2** and the RTI reader's representation must include it. Add both to
the `<datareader_qos>` in [`USER_QOS_PROFILES.rti.xml`](USER_QOS_PROFILES.rti.xml):

```xml
<datareader_qos>
  <representation>
    <value>
      <element>XCDR_DATA_REPRESENTATION</element>
      <element>XCDR2_DATA_REPRESENTATION</element>
    </value>
  </representation>
  ...
</datareader_qos>
```

This is exactly the same lesson as `../fastdds`: a strict XTypes reader defaults
to a single representation and must opt into the other. It is the mirror of
ZeroDDS's own Bug DR1 (where *our* reader learned to accept both).

Two details the snippet above does not show, and both are load-bearing:

* **The profile names are not free.** `rtiddsgen -example` emits a publisher and
  a subscriber that ask a `QosProvider` for
  `ShapeType_Library::Participant_Profile` and
  `ShapeType_Library::Periodic_Best_Effort_Profile` **by name**. A replacement
  file that defines its own profile instead throws before the participant
  exists — `Profile 'ShapeType_Library::Participant_Profile' not found` — and
  the run reports zero samples for a reason that has nothing to do with the
  wire. [`USER_QOS_PROFILES.rti.xml`](USER_QOS_PROFILES.rti.xml) therefore
  redefines those two profiles rather than adding a third. It is kept under that
  name because `rtiddsgen` overwrites `USER_QOS_PROFILES.xml` on every
  regeneration; `run.sh` copies it over afterwards.
* **`<transport_builtin><mask>UDPv4</mask>`.** Same-host, RTI prefers its
  shared-memory transport, which no foreign stack can read.

## Run

```sh
(cd ../zerodds && cargo build --release)
bash run.sh              # DOMAIN=91 by default
```

`run.sh` does the generation and the `sed` above by itself the first time, puts
[`USER_QOS_PROFILES.rti.xml`](USER_QOS_PROFILES.rti.xml) over the file
`rtiddsgen` overwrites, runs both directions and prints the received-sample
counts.

**Not on domain 0.** A DDS domain is a UDP port band (RTPS 2.5 §9.6.1.1), so a
stray participant elsewhere on the host is counted as this run's own result;
this host has had nineteen foreign GUID prefixes on domain 0 inside one
twenty-second window. `run.sh` checks that the band of the domain it will use is
free and aborts rather than measuring into someone else's traffic.

Measured on the Linux test host, domain 91, by `ci/run-examples.sh`: **11**
samples RTI → ZeroDDS, **40** samples ZeroDDS → RTI.

Part of **[ZeroDDS](https://zerodds.de)**.
