// The generated codec plus the `DdsType` impl for `MapEnum.idl` —
// `create_topic::<MapEnum>` takes the `DdsType`, which is what
// `dds_type_impls(true)` writes.
//
// `default_extensibility(Final)`: `feat::Pt` carries no annotation, and the
// specification's default would put a DHEADER in front of it that the other
// PSMs of this proof do not write.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .default_extensibility(zerodds_build::DefaultExt::Final)
        .dds_type_impls(true)
        .compile(&["MapEnum.idl"])
}
