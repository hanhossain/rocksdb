use crate::ffi::rocksdb;
use autocxx::prelude::*;
use std::pin::Pin;

pub struct Options {
    pub(crate) ffi_options: Pin<Box<rocksdb::Options>>,
}

impl Options {
    pub fn new(db_options: &DBOptions, column_family_options: &ColumnFamilyOptions) -> Options {
        let ffi_options = rocksdb::Options::new1(
            &db_options.ffi_db_options.as_ref(),
            &column_family_options.ffi_column_family_options.as_ref(),
        )
        .within_box();
        Options { ffi_options }
    }
}

impl Default for Options {
    fn default() -> Self {
        let ffi_options = rocksdb::Options::new().within_box();
        Options { ffi_options }
    }
}

pub struct DBOptions {
    pub(crate) ffi_db_options: Pin<Box<rocksdb::DBOptions>>,
}

impl DBOptions {
    pub fn set_create_if_missing(&mut self, value: bool) {
        self.ffi_db_options.as_mut().SetCreateIfMissing(value);
    }
}

impl Default for DBOptions {
    fn default() -> Self {
        let value = rocksdb::DBOptions::new().within_box();
        DBOptions {
            ffi_db_options: value,
        }
    }
}

pub struct ColumnFamilyOptions {
    pub(crate) ffi_column_family_options: Pin<Box<rocksdb::ColumnFamilyOptions>>,
}

impl Default for ColumnFamilyOptions {
    fn default() -> Self {
        let value = rocksdb::ColumnFamilyOptions::new().within_box();
        ColumnFamilyOptions {
            ffi_column_family_options: value,
        }
    }
}
