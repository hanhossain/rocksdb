#[cxx::bridge(namespace = "rocksdb")]
pub mod ffi {
    unsafe extern "C++" {
        include!("rocksdb/env.h");

        fn Multiply(a: i32, b: i32) -> i32;
        fn PrintHelloWorld();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_multiply() {
        let res = ffi::Multiply(3, 4);
        assert_eq!(res, 12);
    }
}
