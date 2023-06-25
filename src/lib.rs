use autocxx::prelude::*;

include_cpp! {
    #include "rocksdb/env.h"
    safety!(unsafe)
    generate!("rocksdb::Multiply")
}

#[cxx::bridge(namespace = "rocksdb")]
pub mod cxx_ffi {
    unsafe extern "C++" {
        include!("rocksdb/env.h");

        #[cxx_name = "PrintHelloWorld"]
        fn print_hello_world();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_multiply() {
        let res = ffi::rocksdb::Multiply(c_int(3), c_int(4));
        assert_eq!(res, c_int(12));
    }
}
