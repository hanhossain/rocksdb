#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn hello_world();
    }
}

pub fn hello_world() {
    println!("Hello from rust!");
}
