# ZeroDDS ↔ RTI Connext DDS

Live RTPS interop on the ShapesDemo `Square` topic, **both directions**, with the
`rtiddsgen`-generated C++11 example as the RTI peer.

```
A) RTI pub -> ZeroDDS sub:   ZeroDDS received: 5    <- x=3 y=3 size=3 (rtiddsgen's counter sample)
B) ZeroDDS pub -> RTI sub:   RTI received: 80       [color: BLUE, x: 120, y: 225, shapesize: 30]
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
$NDDSHOME/bin/rtiddsgen -language C++11 -example x64Linux4gcc8.5.0 -replace ShapeType.idl
# point both endpoints at the standard ShapesDemo topic:
sed -i 's/"Example ShapeType"/"Square"/g' ShapeType_publisher.cxx ShapeType_subscriber.cxx
make -f makefile_ShapeType_x64Linux4gcc8.5.0
```

## The one thing to configure — XCDR2 on the reader

Out of the box the RTI reader does **not** receive from the ZeroDDS writer: ZeroDDS
sends **XCDR2** and the RTI reader's representation must include it. Add both to
the `<datareader_qos>` in `USER_QOS_PROFILES.xml`:

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

## Run

```sh
cargo build -p zerodds-dcps --example shapes_demo_publisher --example shapes_demo_subscriber
export LD_LIBRARY_PATH=$NDDSHOME/lib/x64Linux4gcc8.5.0

# A) RTI pub -> ZeroDDS sub
./../../../target/debug/examples/shapes_demo_subscriber Square 0 &
./objs/x64Linux4gcc8.5.0/ShapeType_publisher -d 0

# B) ZeroDDS pub -> RTI sub
./objs/x64Linux4gcc8.5.0/ShapeType_subscriber -d 0 &
./../../../target/debug/examples/shapes_demo_publisher Square BLUE 0
```

Part of **[ZeroDDS](https://zerodds.de)**.
