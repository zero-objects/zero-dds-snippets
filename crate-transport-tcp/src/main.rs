// The website's docs/transport-tcp.html usage snippet, made runnable.
//
// The snippet is two lines: bind port 7400, then `accept_one()`. Both are
// kept, with two changes a run forces:
//
//  1. Port 7400 is the well-known DDS discovery port and is often already
//     taken on a test host, so this binds an ephemeral port and prints it.
//  2. `accept_one()` does not accept a bare TCP connection — it performs the
//     16-byte ZeroDDS handshake and then reads frames until the peer closes.
//     A plain `TcpStream::connect` leaves it waiting and it fails with
//     `Handshake(Io(WouldBlock))` after its 2 s budget. The peer here is a
//     second `TcpTransport` sending through the `Transport` trait, which is
//     what speaks that handshake.
use std::net::Ipv4Addr;
use zerodds_transport::Transport;
use zerodds_transport_tcp::TcpTransport;

fn main() {
    let listener = TcpTransport::bind_v4(Ipv4Addr::LOCALHOST, 0).expect("bind listener");
    let listen_at = listener.local_locator();
    println!(
        "transport-tcp: listening on 127.0.0.1:{}",
        listen_at.port
    );

    let peer = std::thread::spawn(move || {
        let sender = TcpTransport::bind_v4(Ipv4Addr::LOCALHOST, 0).expect("bind sender");
        sender.send(&listen_at, b"RTPS-over-TCP").expect("send");
    });

    listener.accept_one().expect("accept one inbound connection");
    peer.join().expect("peer thread");

    let got = listener.try_recv().expect("one frame in the inbound queue");
    println!(
        "transport-tcp: accepted one connection, {} bytes from {:?}",
        got.data.len(),
        got.source.kind
    );
    assert_eq!(&got.data[..], b"RTPS-over-TCP");
    println!("transport-tcp: payload roundtripped");
}
