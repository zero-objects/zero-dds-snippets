fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir(".")
        .dds_type_impls(true)
        .compile(&["calculator.idl"])
}
