// The website's docs/zerodds-xml-wire.html usage snippet, made runnable.
// DDS-XML 1.0 section 6.4: a sample encoded as an XML element per field and
// decoded back to the same map.
use std::collections::BTreeMap;
use zerodds_xml_wire::codec::{decode_xml, encode_to_xml, FieldValue};

fn main() {
    let mut fields = BTreeMap::new();
    fields.insert("celsius".to_string(), FieldValue::Float(21.5));

    // Sample -> XML (DDS-XML 1.0 section 6.4 wire form)
    let xml = encode_to_xml("Temperature", &fields).expect("encode");

    // ...and back
    let (type_name, decoded) = decode_xml(&xml).expect("decode");
    assert_eq!(type_name, "Temperature");
    assert_eq!(decoded, fields);
    println!("xml-wire: {xml}");
    println!("xml-wire: decoded type {type_name}, {} field(s), roundtripped", decoded.len());
}
