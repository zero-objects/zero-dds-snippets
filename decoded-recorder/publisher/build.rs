// The publisher's `telemetry::Reading` is generated from the same
// `../sensor.idl` the tools are handed with --type-file.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("..")
        .dds_type_impls(true)
        .compile(&["../sensor.idl"])
}
