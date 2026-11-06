// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! GitHub #33: a union whose discriminator has values that select no
//! member, `idl/message_payload.idl` as the reporter wrote it. The derive
//! takes such values in two forms: one unit variant per value
//! (`MessagePayload`), or one `no_member` variant that holds them all
//! (`MessagePayloadSignal`).
//!
//! `cargo run` checks both forms against the IDL route (`build.rs`): the
//! same TypeIdentifier, TypeObject and dependencies, and the same octets
//! in XCDR1 and XCDR2, little and big endian, each read back by all three.
//! It prints the octets, what a peer receives for type resolution, and the
//! value the `no_member` variant refuses.

#[allow(
    dead_code,
    missing_docs,
    non_camel_case_types,
    non_snake_case,
    unused_imports
)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/message_payload.rs"));
}

use zerodds_codec::codec::Extensibility;
use zerodds_codec::{DdsType, Encapsulation};
use zerodds_dcps::{DecodeError, EncodeError};
use zerodds_types::resolve::TypeRegistry;
use zerodds_types::{TypeIdentifier, TypeInformation};

#[derive(DdsType, Debug, Clone, Copy, PartialEq)]
#[dds(name = "Vec3F32", extensibility = "final")]
struct Vec3F32 {
    x: f32,
    y: f32,
    z: f32,
}

#[derive(DdsType, Debug, Clone, Copy, PartialEq)]
#[dds(name = "MessagePayloadDiscriminator", extensibility = "final")]
enum MessagePayloadDiscriminator {
    #[dds(name = "MESSAGE_PAYLOAD_DISCRIMINATOR_POSITION", value = 0)]
    Position,
    #[dds(name = "MESSAGE_PAYLOAD_DISCRIMINATOR_VELOCITY")]
    Velocity,
    #[dds(name = "MESSAGE_PAYLOAD_DISCRIMINATOR_HEARTBEAT")]
    Heartbeat,
    #[dds(name = "MESSAGE_PAYLOAD_DISCRIMINATOR_ACKNOWLEDGEMENT")]
    Acknowledgement,
}

/// One unit variant per value that selects no member: it goes out as the
/// discriminator alone and declares no branch of the union.
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(
    name = "MessagePayload",
    extensibility = "final",
    discriminator = "MessagePayloadDiscriminator"
)]
enum MessagePayload {
    #[dds(case = MessagePayloadDiscriminator::Position, name = "position")]
    Position(Vec3F32),
    #[dds(case = MessagePayloadDiscriminator::Velocity, name = "velocity")]
    Velocity(Vec3F32),
    #[dds(case = MessagePayloadDiscriminator::Heartbeat)]
    Heartbeat,
    #[dds(case = MessagePayloadDiscriminator::Acknowledgement)]
    Acknowledgement,
}

/// One variant that holds every value no case takes. It refuses to write
/// a value a case with a member takes.
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(
    name = "MessagePayload",
    extensibility = "final",
    discriminator = "MessagePayloadDiscriminator"
)]
enum MessagePayloadSignal {
    #[dds(case = MessagePayloadDiscriminator::Position, name = "position")]
    Position(Vec3F32),
    #[dds(case = MessagePayloadDiscriminator::Velocity, name = "velocity")]
    Velocity(Vec3F32),
    #[dds(no_member)]
    Signal(MessagePayloadDiscriminator),
}

type Write<T> = fn(&T, &mut Vec<u8>) -> Result<(), EncodeError>;
type Read<T> = fn(&[u8]) -> Result<T, DecodeError>;

/// The four encodings of `zerodds_dcps::DdsType`, in the order of
/// `Encapsulation::ALL`.
fn encodings<T: zerodds_dcps::DdsType>() -> [(Write<T>, Read<T>); 4] {
    [
        (T::encode, T::decode),
        (T::encode_be, T::decode_be),
        (T::encode_xcdr1, T::decode_xcdr1),
        (T::encode_xcdr1_be, T::decode_xcdr1_be),
    ]
}

/// Writes one sample in all four encodings through the unit variant form,
/// the `no_member` form and the IDL route, and has each read what the
/// others wrote.
fn one_sample(
    unit: &MessagePayload,
    signal: &MessagePayloadSignal,
    idl: &generated::MessagePayload,
) -> Result<(), Box<dyn std::error::Error>> {
    println!("{unit:?} / {signal:?}");
    let forms = encodings::<MessagePayload>()
        .into_iter()
        .zip(encodings::<MessagePayloadSignal>())
        .zip(encodings::<generated::MessagePayload>());
    for (
        encapsulation,
        (((write_unit, read_unit), (write_signal, read_signal)), (write_idl, read_idl)),
    ) in Encapsulation::ALL.into_iter().zip(forms)
    {
        let (mut a, mut b, mut c) = (Vec::new(), Vec::new(), Vec::new());
        write_unit(unit, &mut a)?;
        write_signal(signal, &mut b)?;
        write_idl(idl, &mut c)?;
        if a != c || b != c {
            return Err(
                format!("{encapsulation:?}: the three forms write different octets").into(),
            );
        }
        if read_unit(&c)? != *unit || read_signal(&c)? != *signal || read_idl(&a)? != *idl {
            return Err(format!("{encapsulation:?}: a form reads another value").into());
        }
        let header = encapsulation.identifier(Extensibility::Final);
        println!(
            "  {encapsulation:?}: header {} 00 00 | body {}",
            hex(&header),
            hex(&c)
        );
    }
    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    print!("exported IDL:\n{}", MessagePayload::idl()?);

    // One type: both forms and the IDL route.
    let id = <generated::MessagePayload as zerodds_dcps::DdsType>::type_identifier();
    let object = <generated::MessagePayload as zerodds_dcps::DdsType>::type_object();
    let closure = |mut objects: Vec<zerodds_types::type_object::TypeObject>| {
        objects.sort_by_key(|o| o.to_bytes_le().unwrap_or_default());
        objects
    };
    let dependencies =
        closure(<generated::MessagePayload as zerodds_dcps::DdsType>::type_object_dependencies());
    if <MessagePayload as zerodds_dcps::DdsType>::type_identifier() != id
        || <MessagePayloadSignal as zerodds_dcps::DdsType>::type_identifier() != id
        || <MessagePayload as zerodds_dcps::DdsType>::type_object() != object
        || <MessagePayloadSignal as zerodds_dcps::DdsType>::type_object() != object
        || closure(<MessagePayload as zerodds_dcps::DdsType>::type_object_dependencies())
            != dependencies
        || closure(<MessagePayloadSignal as zerodds_dcps::DdsType>::type_object_dependencies())
            != dependencies
    {
        return Err("the forms and the IDL route are not one type".into());
    }
    println!("both forms and the IDL route: one TypeIdentifier, TypeObject and dependencies");

    println!("\nsamples:");
    let v = |x, y, z| (Vec3F32 { x, y, z }, generated::Vec3F32 { x, y, z });
    let (ours, theirs) = v(1.0, 2.0, 3.0);
    one_sample(
        &MessagePayload::Position(ours),
        &MessagePayloadSignal::Position(ours),
        &generated::MessagePayload::Position(theirs),
    )?;
    let (ours, theirs) = v(0.5, -0.25, 9.75);
    one_sample(
        &MessagePayload::Velocity(ours),
        &MessagePayloadSignal::Velocity(ours),
        &generated::MessagePayload::Velocity(theirs),
    )?;
    use generated::MessagePayloadDiscriminator as D;
    one_sample(
        &MessagePayload::Heartbeat,
        &MessagePayloadSignal::Signal(MessagePayloadDiscriminator::Heartbeat),
        &generated::MessagePayload::_NoMember(D::MESSAGE_PAYLOAD_DISCRIMINATOR_HEARTBEAT),
    )?;
    one_sample(
        &MessagePayload::Acknowledgement,
        &MessagePayloadSignal::Signal(MessagePayloadDiscriminator::Acknowledgement),
        &generated::MessagePayload::_NoMember(D::MESSAGE_PAYLOAD_DISCRIMINATOR_ACKNOWLEDGEMENT),
    )?;

    // A value a case with a member takes is no value without a member.
    let position = MessagePayloadSignal::Signal(MessagePayloadDiscriminator::Position);
    match zerodds_codec::encode(&position, Encapsulation::Xcdr2Le, &mut Vec::new()) {
        Ok(()) => return Err("Signal(Position) was written".into()),
        Err(refused) => println!("\nrefused: {position:?}: {refused}"),
    }

    // What a peer receives: PID_TYPE_INFORMATION in the endpoint's
    // announcement, and the TypeObjects a TypeLookup request returns.
    let mut registry = TypeRegistry::new();
    let mut objects = <MessagePayload as zerodds_dcps::DdsType>::type_object_dependencies();
    objects.extend(object);
    for object in objects {
        registry
            .insert_object(object)
            .map_err(|e| format!("{e:?}"))?;
    }
    registry.project_completes_to_minimal();
    let TypeIdentifier::EquivalenceHashComplete(hash) = &id else {
        return Err("no complete TypeIdentifier".into());
    };
    let complete = registry.get_complete(hash).ok_or("not registered")?.clone();
    let info =
        TypeInformation::from_registry(&complete, &registry).map_err(|e| format!("{e:?}"))?;
    let octets = info.to_bytes_le().map_err(|e| format!("{e:?}"))?;
    println!("\ntype information, {} octets:", octets.len());
    for (kind, with) in [("minimal", &info.minimal), ("complete", &info.complete)] {
        let root = &with.typeid_with_size;
        println!(
            "  {kind} {} ({} octets)",
            identifier(&root.type_id),
            root.typeobject_serialized_size
        );
        for dependency in &with.dependent_typeids {
            println!(
                "    depends on {} ({} octets)",
                identifier(&dependency.type_id),
                dependency.typeobject_serialized_size
            );
        }
    }
    Ok(())
}

fn identifier(id: &TypeIdentifier) -> String {
    match id {
        TypeIdentifier::EquivalenceHashMinimal(h) => format!("f1 {}", hex(&h.0)),
        TypeIdentifier::EquivalenceHashComplete(h) => format!("f2 {}", hex(&h.0)),
        other => format!("{other:?}"),
    }
}

fn hex(bytes: &[u8]) -> String {
    bytes
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect::<Vec<_>>()
        .join(" ")
}
