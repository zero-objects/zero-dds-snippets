// The website's docs/transport-udp.html usage snippet, made runnable.
//
// The snippet names `dest` and `datagram` without defining them, and binds the
// fixed ports 7410 / 7400. This binds ephemeral ports so it does not collide
// with a running participant, then actually sends a datagram to itself and
// reads it back — the send/recv pair the snippet shows.
use std::net::Ipv4Addr;
use zerodds_transport::Transport;
use zerodds_transport_udp::UdpTransport;

fn main() {
    // Unicast bind
    let udp = UdpTransport::bind_v4(Ipv4Addr::LOCALHOST, 0).expect("unicast bind");
    let dest = udp.local_locator();
    println!("transport-udp: bound {dest:?}");

    let datagram = b"RTPS";
    udp.send(&dest, datagram).expect("send");
    let received = udp.recv().expect("recv");
    assert_eq!(&received.data[..], datagram);
    println!(
        "transport-udp: {} bytes back from {:?}",
        received.data.len(),
        received.source
    );

    // ...or join the SPDP/SEDP discovery multicast group. Binding it is the
    // whole check: a loopback-only host has no group to receive on.
    let group = Ipv4Addr::new(239, 255, 0, 1);
    match UdpTransport::bind_multicast_v4(group, 0, Ipv4Addr::UNSPECIFIED) {
        Ok(disco) => println!("transport-udp: joined {group} at {:?}", disco.local_locator()),
        Err(e) => {
            eprintln!("transport-udp: multicast join refused by this host: {e}");
            std::process::exit(1);
        }
    }
}
