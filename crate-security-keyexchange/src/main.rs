// The website's docs/security-keyexchange.html usage snippet, made runnable.
// DDS-Security 1.2 section 9.3.2.3.3: both sides derive the same shared secret
// from the peer's public key. `derive_shared_secret` consumes the key pair, so
// proving both directions needs the public keys taken first.
use zerodds_security_keyexchange::KeyExchange;

fn main() {
    let alice = KeyExchange::new().expect("alice key pair");
    let bob = KeyExchange::new().expect("bob key pair");

    let alice_pub = alice.public_key().to_vec();
    let bob_pub = bob.public_key().to_vec();

    let s: Vec<u8> = alice.derive_shared_secret(&bob_pub).expect("alice -> bob");
    let s2: Vec<u8> = bob.derive_shared_secret(&alice_pub).expect("bob -> alice");

    assert_eq!(s, s2);
    println!("security-keyexchange: both sides derived the same {}-byte secret", s.len());
}
