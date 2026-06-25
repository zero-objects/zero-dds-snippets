from dataclasses import dataclass, field
from cyclonedds.idl import IdlStruct
from cyclonedds.idl.annotations import keylist, mutable, appendable, final
from cyclonedds.idl.types import sequence, uint32, float64, uint8

@final
@keylist(["id"])
@dataclass
class RichF(IdlStruct, typename="RichF"):
    id: uint32 = 0
    value: float64 = 0.0
    blob: sequence[uint8] = field(default_factory=list)

@appendable
@keylist(["id"])
@dataclass
class RichA(IdlStruct, typename="RichA"):
    id: uint32 = 0
    value: float64 = 0.0
    blob: sequence[uint8] = field(default_factory=list)

@mutable
@keylist(["id"])
@dataclass
class RichM(IdlStruct, typename="RichM"):
    id: uint32 = 0
    value: float64 = 0.0
    blob: sequence[uint8] = field(default_factory=list)

TYPES = {"f": ("RichF", RichF), "a": ("RichA", RichA), "m": ("RichM", RichM)}
