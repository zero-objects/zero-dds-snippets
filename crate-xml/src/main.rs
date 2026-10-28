// The website's docs/xml.html usage snippet, made runnable. The snippet has
// two calls and names no document; this supplies a minimal DDS-XML 1.0
// building-block document for both of them.
use zerodds_xml::{parse_dds_xml, parse_xml_tree};

const XML_SRC: &str = r#"<?xml version="1.0" encoding="UTF-8"?>
<dds>
  <qos_library name="demo_lib">
    <qos_profile name="reliable">
      <datawriter_qos>
        <reliability>
          <kind>RELIABLE_RELIABILITY_QOS</kind>
        </reliability>
      </datawriter_qos>
    </qos_profile>
  </qos_library>
</dds>
"#;

fn main() {
    // High-level building-block loader -> DdsXml (QoS, types, domains, ...)
    let dds = parse_dds_xml(XML_SRC).expect("parse_dds_xml");
    // ...or the generic well-formedness tree -> DdsXmlDocument
    let doc = parse_xml_tree(XML_SRC).expect("parse_xml_tree");
    println!("xml: parse_dds_xml -> {dds:?}");
    println!("xml: parse_xml_tree root element = {:?}", doc.root.name);
}
