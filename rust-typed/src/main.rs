// Typed topics in Rust: the type comes from IDL, compiled by
// `zerodds-build` in `build.rs`, and `create_topic` / `create_datawriter`
// take it through the generated `zerodds_dcps::DdsType` impl.
//
// This replaces the `#[derive(DdsType)]` bridge the earlier releases
// carried. The derive is gone: a hand-written bridge states the type
// name and the codec twice, and its `type_object()` defaulted to `None`,
// so the endpoint announced a topic type name with no type behind it
// (XTypes 1.3 §7.6.3.2). The generated impl carries `TYPE_NAME`,
// `EXTENSIBILITY`, `HAS_KEY`, the `TypeIdentifier` and the `TypeObject`
// from the same model as the codec, so they cannot drift apart.

use zerodds_dcps::{
    DataWriterQos, DdsType, DomainParticipantFactory, DomainParticipantQos, PublisherQos, TopicQos,
};

// `temperature.idl` -> `temperature.rs` in `$OUT_DIR`. The lint allows go
// on the module as outer attributes, because `include!` splices the
// generated tokens into this file.
#[allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/temperature.rs"));
}
use generated::sensor_msgs::msg::Temperature;

fn main() -> zerodds_dcps::Result<()> {
    // A live participant: `create_datawriter` and `write` need a runtime
    // (the offline variant can only create topics, not writers).
    let factory = DomainParticipantFactory::instance();
    let p = factory.create_participant(0, DomainParticipantQos::default())?;

    let topic = p.create_topic::<Temperature>("Temp", TopicQos::default())?;
    let writer = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Temperature>(&topic, DataWriterQos::default())?;
    writer.write(&Temperature {
        celsius: 23,
        sensor_id: "A7".into(),
    })?;

    // The type name the peer matches on, and the proof the type itself is
    // announced with it: a `None` here is the failure this example exists
    // to rule out.
    assert!(
        Temperature::type_object().is_some(),
        "the generated DdsType must carry a TypeObject; without one the \
         endpoint announces a topic type name and no type (XTypes 1.3 §7.6.3.2)"
    );
    println!(
        "wrote Temperature {{ celsius: 23, sensor_id: \"A7\" }} to topic \"Temp\" (type {}, TypeObject announced)",
        Temperature::TYPE_NAME
    );
    Ok(())
}
