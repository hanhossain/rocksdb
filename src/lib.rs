#[cxx::bridge(namespace = "rocksdb")]
pub mod cxx_ffi {
    unsafe extern "C++" {
        include!("rocksdb/env.h");

        #[cxx_name = "Multiply"]
        fn multiply(a: i32, b: i32) -> i32;
        #[cxx_name = "PrintHelloWorld"]
        fn print_hello_world();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_multiply() {
        let res = cxx_ffi::multiply(3, 4);
        assert_eq!(res, 12);
    }
}
