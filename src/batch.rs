use crate::error::Error;
use crate::ffi::rocksdb;
use autocxx::prelude::*;
use cxx::let_cxx_string;

pub struct WriteBatch {
    pub(crate) ffi_write_batch: UniquePtr<rocksdb::WriteBatch>,
}

impl WriteBatch {
    pub fn delete(&mut self, key: &str) -> Result<(), Error> {
        let_cxx_string!(key = key);
        let key = rocksdb::Slice::new2(&key).within_unique_ptr();
        let status = self
            .ffi_write_batch
            .pin_mut()
            .Delete1(&key)
            .within_unique_ptr();

        if status.ok() {
            return Ok(());
        }

        Err(Error::from(&status))
    }

    pub fn put(&mut self, key: &str, value: &str) -> Result<(), Error> {
        let_cxx_string!(key = key);
        let_cxx_string!(value = value);
        let key = rocksdb::Slice::new2(&key).within_unique_ptr();
        let value = rocksdb::Slice::new2(&value).within_unique_ptr();
        let status = self
            .ffi_write_batch
            .pin_mut()
            .Put1(&key, &value)
            .within_unique_ptr();

        if status.ok() {
            return Ok(());
        }

        Err(Error::from(&status))
    }
}

impl Default for WriteBatch {
    fn default() -> Self {
        let ffi_write_batch = rocksdb::WriteBatch::new(0, 0).within_unique_ptr();
        WriteBatch { ffi_write_batch }
    }
}
