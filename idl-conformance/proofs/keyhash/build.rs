// The generated codec plus the `DdsType` impl for `idl/keyhash_cases.idl` —
// the KeyHolder and the KeyHash come out of that impl, which is what this
// proof reads.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["idl/keyhash_cases.idl"])
}
