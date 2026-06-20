// Verbatim demo of the website snippet:
//   website/bindings/rust/index.html
//   "Typisierte Topics — DdsType / Variante A — #[derive(DdsType)]"
//
// The struct definition and the create_topic / create_datawriter / write
// lines below are copied EXACTLY from that snippet. Only the import line
// and the surrounding main()/participant scaffolding (which the snippet
// omits) were added — see the note next to `use` below.

// --- snippet imports (verbatim from the corrected website snippet) ------
// The derive macro lives in `zerodds-cdr-derive`, the trait in
// `zerodds-dcps`. They share the name `DdsType` but live in different
// namespaces (macro vs. type), so both `use`s coexist without a clash.
use zerodds_cdr_derive::DdsType; // derive macro
use zerodds_dcps::DdsType; // trait
// ------------------------------------------------------------------------

// Scaffolding the snippet omits: factory + participant + QoS imports.
use zerodds_dcps::{
    DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos, TopicQos,
};

// ===== EXACT snippet struct definition ==================================
#[derive(DdsType, Debug, Clone)]
#[dds(type_name = "sensor_msgs::msg::Temperature")]
pub struct Temperature {
    pub celsius: i32,
    pub sensor_id: String,
}
// ========================================================================

fn main() -> zerodds_dcps::Result<()> {
    // Scaffolding: build participant `p` the snippet refers to.
    // Live participant — create_datawriter/write need a runtime (the
    // offline variant can only create topics, not writers).
    let factory = DomainParticipantFactory::instance();
    let p = factory.create_participant(0, DomainParticipantQos::default())?;

    // ===== EXACT snippet usage lines ====================================
    let topic = p.create_topic::<Temperature>("Temp", TopicQos::default())?;
    let writer = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Temperature>(&topic, DataWriterQos::default())?;
    writer.write(&Temperature {
        celsius: 23,
        sensor_id: "A7".into(),
    })?;
    // ====================================================================

    println!(
        "wrote Temperature {{ celsius: 23, sensor_id: \"A7\" }} to topic \"Temp\" (type {})",
        Temperature::TYPE_NAME
    );
    Ok(())
}
