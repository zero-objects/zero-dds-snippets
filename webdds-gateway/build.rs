// `idl/Sensor.idl` into `$OUT_DIR/Sensor.rs`, with the `DdsType` impls a
// DCPS topic needs.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .dds_type_impls(true)
        .compile(&["idl/Sensor.idl"])
}
