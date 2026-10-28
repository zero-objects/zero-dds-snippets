// The website's docs/rt-linux.html usage snippet, made runnable — and
// corrected. The page writes `SchedulerProfile::fifo(80)`. No release has such
// a constructor: `SchedulerProfile` is an enum with a `RealtimeFifo { priority }`
// variant, at the v1.0.0-rc.6 tag and in the current tree, and building a
// variant returns the value itself, not a `Result` to `.expect()` on.
//
// Linux only: SCHED_FIFO and sched_setaffinity are Linux system calls, so the
// crate compiles nowhere else. The snippet asks for FIFO priority 80 and pins
// to CPUs 2 and 3; FIFO needs CAP_SYS_NICE and the pinning needs two CPUs this
// thread may run on, so the demo says which requirement was not met rather
// than reporting success.
use zerodds_rt_linux::affinity::current_thread_cpus;
use zerodds_rt_linux::{pin_current_thread_to_cpus, SchedulerProfile};

fn main() {
    let profile = SchedulerProfile::RealtimeFifo { priority: 80 };
    match profile.apply_to_current_thread() {
        Ok(()) => println!("rt-linux: this thread runs SCHED_FIFO at priority 80"),
        Err(e) => {
            eprintln!("rt-linux: SCHED_FIFO refused ({e}) — CAP_SYS_NICE is required");
            std::process::exit(1);
        }
    }

    // The snippet pins to CPUs 2 and 3. A cgroup or container may not allow
    // those two; then the first two CPUs this thread may run on stand in.
    let allowed = current_thread_cpus().expect("read the affinity mask");
    let wanted: Vec<usize> = if allowed.contains(&2) && allowed.contains(&3) {
        vec![2, 3]
    } else {
        allowed.iter().copied().take(2).collect()
    };
    if wanted.len() < 2 {
        eprintln!("rt-linux: this thread may run on {} CPU(s); the snippet pins to two", allowed.len());
        std::process::exit(1);
    }
    pin_current_thread_to_cpus(&wanted).expect("affinity set");
    let now = current_thread_cpus().expect("read the affinity mask back");
    assert_eq!(now, wanted, "the kernel kept a different mask");
    println!("rt-linux: this thread is pinned to CPUs {} and {}", wanted[0], wanted[1]);
}
