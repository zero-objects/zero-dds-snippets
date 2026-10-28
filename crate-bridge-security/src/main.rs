// The website's docs/bridge-security.html usage snippet, made runnable.
use zerodds_bridge_security::{Acl, AclOp, AuthSubject};

fn main() {
    let subj = AuthSubject::new("alice").with_group("publishers");
    let acl = Acl::allow_all();
    let allowed = acl.check(&subj, AclOp::Write, "/topics/trade");
    println!("bridge-security: allow_all().check(alice, Write, /topics/trade) = {allowed:?}");
}
