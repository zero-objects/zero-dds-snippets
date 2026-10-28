// The website's docs/transport-uds.html usage snippet, made runnable.
// The snippet names `local_id`, `dest` and `datagram` without defining them;
// this supplies a GUID prefix, sends a datagram to the socket's own locator
// and reads it back.
use zerodds_transport::Transport;
use zerodds_transport_uds::{UdsConfig, UdsTransport};

fn main() {
    let local_id = [0x5Au8; 16];

    // Bind a SOCK_DGRAM socket under the configured base dir
    let uds = UdsTransport::bind(local_id, UdsConfig::default()).expect("bind");
    let dest = uds.local_locator();
    println!("transport-uds: bound {dest:?}");

    let datagram = b"RTPS";
    uds.send(&dest, datagram).expect("send");
    let received = uds.recv().expect("recv");
    assert_eq!(&received.data[..], datagram);
    println!("transport-uds: {} bytes back over the unix socket", received.data.len());
}
