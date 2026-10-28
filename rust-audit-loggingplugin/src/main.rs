// Verifies the `LoggingPlugin` trait shown in
// website/topics/audit-reference/index.html reflects the real
// zerodds_security::logging::LoggingPlugin SPI. The website snippet shows the
// `log(&self, level: LogLevel, participant: [u8; 16], category: &str,
// message: &str)` method exactly; the live trait additionally requires
// `plugin_class_id()` and the three operations of DDS-Security 1.2 Table 46,
// `set_log_options`, `enable_logging` and `set_listener`. We implement the
// real trait with the snippet's `log` signature to prove it matches the live
// API byte-for-byte; the demo sink takes any options and keeps no listener.
use std::sync::Arc;

use zerodds_security::SecurityError;
use zerodds_security::logging::{LogLevel, LogOptions, LoggerListener, LoggingPlugin};

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

    fn set_log_options(&self, _options: LogOptions) -> Result<(), SecurityError> {
        Ok(())
    }

    fn enable_logging(&self) -> Result<(), SecurityError> {
        Ok(())
    }

    fn set_listener(&self, _listener: Option<Arc<dyn LoggerListener>>) -> Result<(), SecurityError> {
        Ok(())
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
