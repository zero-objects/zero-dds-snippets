// The topic type comes from `temperature.idl`, not from a hand-written
// bridge. `dds_type_impls(true)` writes the `zerodds_dcps::DdsType` impl
// beside the codec, from the same model, so the type name, the
// extensibility, the key, the `TypeIdentifier` and the `TypeObject`
// cannot disagree with the bytes on the wire.
//
// A hand-written bridge left `type_object()` at its default `None`, and
// the endpoint then announced a topic type name and no type at all
// (XTypes 1.3 §7.6.3.2) — a peer that asked TypeLookup for the type got
// nothing back.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir(".")
        .dds_type_impls(true)
        .compile(&["temperature.idl"])
}
