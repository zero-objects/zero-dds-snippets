#!/usr/bin/env python3
"""CycloneDDS ShapesDemo subscriber (Python binding).

Receives `ShapeType` samples on a standard ShapesDemo topic and prints them —
the receiving end of the cross-vendor proof (e.g. against a ZeroDDS publisher).

CycloneDDS 11.x API notes (differ from older releases) baked in below:
  * key is declared with the `@keylist([...])` decorator (not `key[T]`),
  * `reader.take(N=100)` takes a capital `N`,
  * do NOT add `from __future__ import annotations` — it breaks idl type
    resolution.

    python3 shapes_sub.py [topic=Square] [domain=0]
"""

import sys
import time
from dataclasses import dataclass

from cyclonedds.domain import DomainParticipant
from cyclonedds.idl import IdlStruct
from cyclonedds.idl.annotations import keylist
from cyclonedds.idl.types import bounded_str, int32
from cyclonedds.sub import DataReader, Subscriber
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
    domain_id = int(sys.argv[2]) if len(sys.argv) > 2 else 0

    participant = DomainParticipant(domain_id=domain_id)
    topic = Topic(participant, topic_name, ShapeType)
    reader = DataReader(Subscriber(participant), topic)

    print(f"[cyclone-sub] Topic={topic_name} Domain={domain_id}", flush=True)

    try:
        while True:
            for s in reader.take(N=100):
                print(f"  <- color={s.color:>8} x={s.x:4} y={s.y:4} size={s.shapesize}", flush=True)
            time.sleep(0.05)
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
