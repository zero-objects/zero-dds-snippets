// The website's docs/rpc.html usage snippet, made runnable — and corrected.
//
// The snippet as published does not compile against any release. Five points:
//
//  1. `use zerodds_rpc::{Requester, Replier, RpcQos, FnHandler}` — none of the
//     four is re-exported at the crate root, at the v1.0.0-rc.6 tag or in the
//     current tree. They live in `requester`, `replier` and `qos_profile`.
//  2. `DomainParticipant::new(0)?` — `DomainParticipant::new` is `pub(crate)`.
//     A participant comes from `DomainParticipantFactory`.
//  3. `Replier::new(..., FnHandler::new(...))` — the handler parameter is
//     `Arc<dyn ReplierHandler<TIn, TOut>>`, so the adapter has to be wrapped.
//  4. The closure must return `Result<TOut, RemoteExceptionCode>`, not `TOut`.
//  5. `MyRequest` / `MyReply` have to be DDS types; here they come from
//     `calculator.idl` through `zerodds-build`.
//
// Both endpoints sit on one offline participant and the frames are pumped
// between them in-process, which is what makes the run deterministic: an
// online pair would depend on discovery timing.
use std::sync::Arc;

use zerodds_dcps::factory::DomainParticipantFactory;
use zerodds_dcps::qos::DomainParticipantQos;
use zerodds_rpc::qos_profile::RpcQos;
use zerodds_rpc::replier::{FnHandler, Replier};
use zerodds_rpc::requester::Requester;

#[allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/calculator.rs"));
}
use generated::calculator::{MyReply, MyRequest};

fn main() {
    let participant = DomainParticipantFactory::instance()
        .create_participant_offline(0, DomainParticipantQos::default());
    let qos = RpcQos::default_basic();

    let replier = Replier::<MyRequest, MyReply>::new(
        &participant,
        "Calculator",
        &qos,
        Arc::new(FnHandler::new(|req: MyRequest| {
            Ok(MyReply { sum: req.a + req.b })
        })),
    )
    .expect("replier");

    let requester =
        Requester::<MyRequest, MyReply>::new(&participant, "Calculator", &qos).expect("requester");

    let (_id, rx) = requester
        .send_request_async(&MyRequest { a: 1, b: 2 })
        .expect("send");

    // Pump: requester -> replier, then replier -> requester.
    for frame in requester.__drain_request_writer() {
        replier.__push_request_raw(frame).expect("push request");
    }
    assert_eq!(replier.tick(), 1, "the replier must have handled one request");
    for frame in replier.__drain_reply_writer() {
        requester.__push_reply_raw(frame).expect("push reply");
    }
    requester.tick();

    let bytes = rx
        .try_recv()
        .expect("a reply was correlated to the request")
        .expect("the handler returned Ok, not a RemoteExceptionCode");
    let reply = <MyReply as zerodds_dcps::DdsType>::decode(&bytes).expect("decode MyReply");
    assert_eq!(reply.sum, 3);
    println!("rpc: Calculator(1, 2) = {}", reply.sum);
}
