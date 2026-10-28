// The website's docs/transport-tsn.html usage snippet, made runnable.
//
// The page spelled the type `IEEE802VlanTag`. No release ever carried that
// name: it is `Ieee802VlanTag` at the v1.0.0-rc.6 tag and in the current tree,
// which is what this demo uses. IEEE 802.1Q-2018 clause 9.6: TPID 0x8100,
// PCP 6, DEI clear, VID 100.
use zerodds_transport_tsn::vlan_tag::Ieee802VlanTag;

fn main() {
    let tag = Ieee802VlanTag::new(0x8100, 6, false, 100).expect("valid 802.1Q tag");
    let wire = tag.to_wire();
    let back = Ieee802VlanTag::from_wire(&wire).expect("from_wire");
    assert_eq!(back, tag);
    println!(
        "transport-tsn: 802.1Q tag on the wire = {:02x?}, roundtripped pcp={} vid={}",
        wire,
        tag.pcp,
        tag.vid
    );
}
