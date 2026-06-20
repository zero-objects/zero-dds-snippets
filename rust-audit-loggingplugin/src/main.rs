// Verifies the `LoggingPlugin` trait shown in
// website/topics/audit-reference/index.html reflects the real
// zerodds_security::logging::LoggingPlugin SPI. The website snippet shows the
// `log(&self, level: LogLevel, participant: [u8; 16], category: &str,
// message: &str)` method exactly; the live trait additionally requires
// `plugin_class_id()`. We implement the real trait with the snippet's `log`
// signature to prove it matches the live API byte-for-byte.
use zerodds_security::logging::{LogLevel, LoggingPlugin};

struct DemoSink;

impl LoggingPlugin for DemoSink {
    // <-- exactly the signature shown on the website -->
    fn log(&self, level: LogLevel, participant: [u8; 16], category: &str, message: &str) {
        println!(
            "[{:?}] participant={:02x?} category={} message={}",
            level, participant, category, message
        );
    }

    fn plugin_class_id(&self) -> &str {
        "DDS:Logging:Demo"
    }
}

fn main() {
    let sink = DemoSink;
    sink.log(
        LogLevel::Warning,
        [1u8; 16],
        "auth.handshake.failed",
        "cert chain rejected",
    );
    println!("class id = {}", sink.plugin_class_id());
}
