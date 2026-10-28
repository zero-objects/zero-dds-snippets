// The generated codec plus the `DdsType` impl for `../combo.idl`, the file
// the other six columns generate from.
//
// `zerodds-build` runs the same composition pipeline the `zerodds-idlc` CLI
// runs, as a library call; `dds_type_impls(true)` writes the `DdsType` impl
// beside the codec, which is the surface this harness encodes and decodes
// over. No `default_extensibility` call: `Sample` and `Reading` take the
// default every other column takes.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["../combo.idl"])
}
