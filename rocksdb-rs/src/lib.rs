pub mod advanced_options;
pub mod convenience;
pub mod options;
pub mod options_type;
pub mod types;

#[cxx::bridge(namespace = "rs")]
mod ffi {
    extern "Rust" {
        fn hello_world();
    }
}

pub fn hello_world() {
    println!("Hello from rust!");
}
