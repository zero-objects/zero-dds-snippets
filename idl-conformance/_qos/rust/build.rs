// The generated codec plus the `DdsType` impl for `idl/reading.idl`.
//
// `default_extensibility(Final)`: `qos::Reading` carries no extensibility
// annotation and XTypes 1.3 §7.3.1.2.1.8 would make it appendable, which puts
// a DHEADER in front of a sample the other PSMs of this corpus write without
// one.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .default_extensibility(zerodds_build::DefaultExt::Final)
        .dds_type_impls(true)
        .compile(&["idl/reading.idl"])
}
