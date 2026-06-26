#!/usr/bin/env python3
"""CycloneDDS ShapesDemo publisher (Python binding).

Publishes the canonical `ShapeType` on one of the standard ShapesDemo topics so
a ZeroDDS subscriber can receive it — a cross-vendor RTPS wire proof.

The type name (`ShapeType`) and topic name (`Square`/`Triangle`/`Circle`) match
exactly what RTI / Cyclone / Fast DDS / ZeroDDS use. The struct is the default
CycloneDDS extensibility (`@final`), which is the wire form ZeroDDS speaks too.

Requires: `pip install cyclonedds` against a CycloneDDS install (set
`CYCLONEDDS_HOME` / `LD_LIBRARY_PATH`). Linux with working IP multicast on the
chosen interface.

    python3 shapes_pub.py [topic=Square] [color=RED] [domain=0]
"""

import math
import sys
import time
from dataclasses import dataclass

from cyclonedds.domain import DomainParticipant
from cyclonedds.idl import IdlStruct
from cyclonedds.idl.annotations import keylist
from cyclonedds.idl.types import bounded_str, int32
from cyclonedds.pub import DataWriter, Publisher
from cyclonedds.topic import Topic


@keylist(["color"])
@dataclass
class ShapeType(IdlStruct, typename="ShapeType"):
    color: bounded_str[128] = ""
    x: int32 = 0
    y: int32 = 0
    shapesize: int32 = 0


def main() -> int:
    topic_name = sys.argv[1] if len(sys.argv) > 1 else "Square"
    color = sys.argv[2] if len(sys.argv) > 2 else "RED"
    domain_id = int(sys.argv[3]) if len(sys.argv) > 3 else 0

    participant = DomainParticipant(domain_id=domain_id)
    topic = Topic(participant, topic_name, ShapeType)
    writer = DataWriter(Publisher(participant), topic)

    print(f"[cyclone-pub] Topic={topic_name} Color={color} Domain={domain_id}", flush=True)

    t = 0.0
    try:
        while True:
            x = int(120 + 80 * math.sin(t))
            y = int(135 + 90 * math.cos(t * 1.3))
            writer.write(ShapeType(color=color, x=x, y=y, shapesize=30))
            print(f"  -> color={color} x={x} y={y} size=30", flush=True)
            t += 0.15
            time.sleep(0.1)
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
