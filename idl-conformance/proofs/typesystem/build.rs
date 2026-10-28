// The generated codec plus the `DdsType` impl for `idl/typesystem.idl`.
//
// The corpus annotates every aggregate (`@final`, `@appendable`, `@mutable`),
// so the specification's default for an unannotated one never applies here and
// `default_extensibility` is left alone.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["idl/typesystem.idl"])
}
