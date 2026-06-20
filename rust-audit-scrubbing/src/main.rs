// Demo for the website page:
//   website/topics/audit-logging/index.html — "6 · Sensitive-Data-Scrub",
//   "Scrub-Wrapper-Pattern" snippet.
//
// The published snippet (HTML-unescaped) defines a wrapper logger that scrubs
// secrets before delegating to an inner LoggingPlugin. The struct, the `log`
// method, and the `scrub_secrets` fn are reproduced below VERBATIM.
//
// The wrapper implements both `LoggingPlugin` methods: `log` (the scrubbing
// body, verbatim) and `plugin_class_id`, forwarded to the wrapped inner plugin
// (the natural behaviour for a transparent wrapper). The demo wraps a real
// StderrLoggingPlugin and logs a message containing a 64-char hex key to show
// it being redacted.
use zerodds_security::logging::{LogLevel, LoggingPlugin};
use zerodds_security_logging::StderrLoggingPlugin;

// ===== BEGIN verbatim website snippet =====

pub struct ScrubbingLogger<P: LoggingPlugin> {
    inner: P,
}

impl<P: LoggingPlugin> LoggingPlugin for ScrubbingLogger<P> {
    fn log(&self, level: LogLevel, participant: [u8; 16], category: &str, message: &str) {
        // Replace bekannte Geheimnis-Patterns.
        let scrubbed = scrub_secrets(message);
        self.inner.log(level, participant, category, &scrubbed);
    }

    // Required by the `LoggingPlugin` trait; forward to the wrapped plugin.
    fn plugin_class_id(&self) -> &str {
        self.inner.plugin_class_id()
    }
}

fn scrub_secrets(msg: &str) -> String {
    use std::sync::OnceLock;
    use regex::Regex;
    static RE_HEX: OnceLock<Regex> = OnceLock::new();
    let re_hex = RE_HEX.get_or_init(|| {
        // 64+ hex chars = wahrscheinlich Key/Nonce/Hash
        Regex::new(r"\b[0-9a-fA-F]{64,}\b").unwrap()
    });
    re_hex.replace_all(msg, "[REDACTED-HEX]").into_owned()
}

// ===== END verbatim website snippet =====

fn main() {
    // The snippet defines `ScrubbingLogger { inner }` with a private field and
    // no constructor; build it with a struct literal in-module (the demo lives
    // in the same module as the snippet, so the private field is reachable).
    let logger = ScrubbingLogger {
        inner: StderrLoggingPlugin::with_level(LogLevel::Notice),
    };

    let participant = [0u8; 16];

    // A 64-char hex string standing in for a leaked symmetric key.
    let secret = "a".repeat(64);
    let leaky = format!("handshake key was {secret} for peer");

    eprintln!("--- raw message (NOT logged): {leaky}");
    eprintln!("--- logging through ScrubbingLogger (stderr below should be redacted):");
    logger.log(LogLevel::Warning, participant, "auth.handshake", &leaky);

    // Also show a benign message passes through unchanged.
    logger.log(
        LogLevel::Notice,
        participant,
        "auth.cert",
        "cert subject CN=publisher-1 OU=mission fingerprint=ab12",
    );

    // Prove the scrub at the value level too.
    let scrubbed = scrub_secrets(&leaky);
    assert!(scrubbed.contains("[REDACTED-HEX]"));
    assert!(!scrubbed.contains(&secret));
    println!("scrub verified: {scrubbed}");
}
