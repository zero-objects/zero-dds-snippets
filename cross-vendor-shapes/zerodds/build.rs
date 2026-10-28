// The ZeroDDS side of the ShapesDemo proof is generated from the same
// `../ShapeType.idl` every peer here is built from: the four-member @final
// type the interactive ShapesDemo applications speak. The `shapes_demo_*`
// examples of the main repository speak the other `ShapeType`, the
// five-member @appendable one of the OMG dds-rtps suite; the two share a name
// and not a wire form (PLAIN_CDR2 against DELIMITED_CDR in XCDR2), so a peer
// that writes XCDR2 is a decode error to a reader of the other.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("..")
        .dds_type_impls(true)
        .compile(&["../ShapeType.idl"])
}
