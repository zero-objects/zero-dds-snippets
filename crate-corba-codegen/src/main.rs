// The website's docs/corba-codegen.html usage snippet, made runnable.
// CORBA 3.4 section 8.6.2: the RepositoryId of MyModule::MyInterface, version 1.0.
use zerodds_corba_codegen::build_repository_id;

fn main() {
    let id = build_repository_id(&["MyModule"], "MyInterface", 1, 0);
    assert_eq!(id, "IDL:MyModule/MyInterface:1.0");
    println!("corba-codegen: {id}");
}
