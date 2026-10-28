// The website's docs/discovery.html usage snippet, made runnable.
//
// The snippet names `participant_data` and `incoming` without defining them.
// This builds a real ParticipantBuiltinTopicData, serializes the SPDP beacon
// datagram the snippet would put on the multicast group, and feeds that same
// datagram to SpdpReader — the "receive a peer's beacon" half of the snippet,
// with the loop closed so the run has something to check.
// DDSI-RTPS 2.5 section 8.5.3.
use zerodds_discovery::spdp::{SpdpBeacon, SpdpReader};
use zerodds_rtps::participant_data::{endpoint_flag, Duration, ParticipantBuiltinTopicData};
use zerodds_rtps::{EntityId, Guid, GuidPrefix, Locator, ProtocolVersion, VendorId};

fn participant_data() -> ParticipantBuiltinTopicData {
    ParticipantBuiltinTopicData {
        guid: Guid::new(GuidPrefix::from_bytes([0xA; 12]), EntityId::PARTICIPANT),
        protocol_version: ProtocolVersion::V2_5,
        vendor_id: VendorId::ZERODDS,
        default_unicast_locators: vec![Locator::udp_v4([127, 0, 0, 1], 7410)],
        default_multicast_locators: vec![Locator::udp_v4([239, 255, 0, 1], 7400)],
        metatraffic_unicast_locators: Vec::new(),
        metatraffic_multicast_locators: Vec::new(),
        domain_id: Some(0),
        // DDSI-RTPS 2.5 section 8.5.3.2: the empty tag is the default one.
        domain_tag: String::new(),
        builtin_endpoint_set: endpoint_flag::PARTICIPANT_ANNOUNCER
            | endpoint_flag::PARTICIPANT_DETECTOR,
        lease_duration: Duration::from_secs(100),
        user_data: Vec::new(),
        properties: Default::default(),
        identity_token: None,
        permissions_token: None,
        identity_status_token: None,
        sig_algo_info: None,
        kx_algo_info: None,
        sym_cipher_algo_info: None,
        participant_security_info: None,
    }
}

fn main() {
    // Announce ourselves — serialize an SPDP beacon datagram for multicast
    let mut beacon = SpdpBeacon::new(participant_data());
    let datagram = beacon.serialize().expect("serialize");
    // socket.send_to(&datagram, spdp_multicast_addr)?;
    println!("discovery: SPDP beacon is {} bytes", datagram.len());

    // Receive a peer's beacon -> DiscoveredParticipant
    let incoming = datagram;
    let reader = SpdpReader::new();
    let peer = reader.parse_datagram(&incoming).expect("parse_datagram");
    assert_eq!(peer.data.guid, participant_data().guid);
    println!("discovery: parsed peer GUID {:?}", peer.data.guid);
}
