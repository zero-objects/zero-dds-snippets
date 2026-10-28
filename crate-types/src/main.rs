// The website's docs/types.html usage snippet, made runnable.
// XTypes 1.3 section 7.6.3: two identical primitive TypeIdentifiers are assignable.
use zerodds_types::qos::TypeConsistencyEnforcement;
use zerodds_types::resolve::TypeRegistry;
use zerodds_types::type_matcher::TypeMatcher;
use zerodds_types::{PrimitiveKind, TypeIdentifier};

fn main() {
    let writer = TypeIdentifier::Primitive(PrimitiveKind::Int32);
    let reader = TypeIdentifier::Primitive(PrimitiveKind::Int32);
    let tce = TypeConsistencyEnforcement::default();
    let m = TypeMatcher::new(&tce);
    let registry = TypeRegistry::new();
    assert!(m.match_types(&writer, &reader, &registry).is_match());
    println!("types: Int32 writer and Int32 reader match");
}
