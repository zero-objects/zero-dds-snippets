// The website's docs/time-service.html usage snippet, made runnable.
// OMG Time Service 1.1: a Uto one millisecond later is not less than now.
use zerodds_time_service::{ComparisonType, TimeComparison, TimeService, Uto};

fn main() {
    let service = TimeService::default();
    let now = service.universal_time().unwrap();

    let later = Uto::new(now.time() + 1_000_000, 50, 0);
    let cmp = now.compare_time(ComparisonType::IntervalC, later);
    assert_ne!(cmp, TimeComparison::GreaterThan);
    println!("time-service: now vs now+1ms compares {cmp:?}");
}
