use crate::ffi::rocksdb;
use autocxx::WithinBox;
use std::pin::Pin;

pub struct Options {
    pub(crate) ffi_options: Pin<Box<rocksdb::Options>>,
}

impl Default for Options {
    fn default() -> Self {
        let ffi_options = rocksdb::Options::new().within_box();
        Options { ffi_options }
    }
}
