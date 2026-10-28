// The website's docs/amqp-endpoint.html usage snippet, made runnable.
// A sender link grants credit, delivers, and settles; the settlement count
// must return to zero.
use zerodds_amqp_endpoint::{
    DispositionState, LinkRole, LinkSession, NoopDispositionMapper, SettlementMode,
};

fn main() {
    let mut link = LinkSession::new(
        "outbound-1".into(),
        0,
        LinkRole::Sender,
        SettlementMode::Unsettled,
    );
    link.grant_credit(2);
    link.deliver().expect("ok");

    let mapper = NoopDispositionMapper;
    link.settle_with_mapper(&mapper, [0u8; 16], DispositionState::Accepted);
    assert_eq!(link.pending_settlements, 0);
    println!("amqp-endpoint: link settled, pending_settlements = {}", link.pending_settlements);
}
