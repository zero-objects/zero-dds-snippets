# SPDX-License-Identifier: Apache-2.0
#
# HAND-WRITTEN, *not* auto-generated.
#
# The Python backend of `zerodds-idlc` currently DROPS the `@optional`
# annotation: for the IDL
#
#     struct Optionals {
#       long           required;
#       @optional long maybe;
#       @optional string note;
#     };
#
# it emits `maybe: Int32` / `note: String` (required fields, no present-flag)
# instead of `Optional[Int32]` / `Optional[String]`. That is a codegen
# data-fidelity bug (see README "Known limitations").
#
# The *runtime* (`zerodds.idl.Optional`) fully supports the optional
# present-flag wire form, so this file is what a correct backend WOULD emit.
# It lets the example still exercise the @optional feature end-to-end over a
# real DCPS roundtrip, while being honest that the generator did not produce it.

from dataclasses import dataclass

from zerodds.idl import Int32, Optional, String, idl_struct


@idl_struct(typename="Optionals")
@dataclass
class Optionals:
    required: Int32
    maybe: Optional[Int32]
    note: Optional[String]
