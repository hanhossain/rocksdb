#[cxx::bridge(namespace = "rs")]
mod ffi {
    extern "Rust" {
        fn hello_world();
    }

    #[namespace = "rs::options"]
    enum CompactionServiceJobStatus {
        Success = 0,
        Failure = 1,
        UseLocal = 2,
    }
}

pub fn hello_world() {
    println!("Hello from rust!");
}
