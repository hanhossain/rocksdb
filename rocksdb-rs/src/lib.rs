pub mod advanced_options;
pub mod options;

#[cxx::bridge(namespace = "rs")]
mod ffi {
    extern "Rust" {
        fn hello_world();
    }
}

pub fn hello_world() {
    println!("Hello from rust!");
}
