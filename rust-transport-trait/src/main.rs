// Verifies the `Transport` trait shown in website/topics/transport-reference/index.html
// matches the real zerodds_transport::Transport SPI: the snippet shows exactly
// these three methods (send / recv / local_locator). We implement the real trait
// with exactly those signatures to prove the snippet reflects the live API.
use std::sync::Arc;

use zerodds_transport::{
    Locator, ReceivedDatagram, RecvError, SendError, Transport,
};

struct NullTransport {
    loc: Locator,
}

impl Transport for NullTransport {
    fn send(&self, _dest: &Locator, _data: &[u8]) -> Result<(), SendError> {
        Ok(())
    }

    fn recv(&self) -> Result<ReceivedDatagram, RecvError> {
        Ok(ReceivedDatagram {
            source: self.loc.clone(),
            data: Arc::from(&b"x"[..]),
        })
    }

    fn local_locator(&self) -> Locator {
        self.loc.clone()
    }
}

fn main() {
    let t = NullTransport {
        loc: Locator::INVALID,
    };
    t.send(&Locator::INVALID, b"ping").expect("send");
    let dg = t.recv().expect("recv");
    println!("recv from {:?}, {} bytes", dg.source, dg.data.len());
    println!("local: {:?}", t.local_locator());
}
