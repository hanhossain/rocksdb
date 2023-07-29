use crate::error::Error;
use crate::ffi;
use crate::ffi::rocksdb;
use crate::ffi::rocksdb::Slice;
use crate::options::{Options, ReadOptions, WriteOptions};
use autocxx::WithinUniquePtr;
use cxx::{let_cxx_string, UniquePtr};
use std::path::Path;

pub struct DB {
    ffi_db: UniquePtr<rocksdb::DB>,
}

impl DB {
    pub fn open<P: AsRef<Path>>(options: &Options, path: P) -> Result<DB, Error> {
        let db_path = ffi::make_string(path.as_ref().to_str().unwrap());
        let mut db_result = rocksdb::DB_Open(&options.ffi_options, &db_path).within_unique_ptr();
        let status = db_result.pin_mut().get_status().within_unique_ptr();
        if status.ok() {
            let db = db_result.pin_mut().get_db();
            return Ok(DB { ffi_db: db });
        }

        Err(Error::from(&status))
    }

    pub fn put(
        &mut self,
        write_options: &WriteOptions,
        key: &str,
        value: &str,
    ) -> Result<(), Error> {
        let_cxx_string!(k = key);
        let_cxx_string!(v = value);
        let k = Slice::new2(&k).within_unique_ptr();
        let v = Slice::new2(&v).within_unique_ptr();
        let status = self
            .ffi_db
            .pin_mut()
            .Put2(&write_options.ffi_write_options, &k, &v)
            .within_unique_ptr();

        if status.ok() {
            return Ok(());
        }

        Err(Error::from(&status))
    }

    pub fn get(&mut self, read_options: &ReadOptions, key: &str) -> Result<String, Error> {
        let_cxx_string!(k = key);
        let k = Slice::new2(&k).within_unique_ptr();
        let value = ffi::make_string("");
        let string_ptr = value.into_raw();
        let status = unsafe {
            self.ffi_db
                .pin_mut()
                .Get2(&read_options.ffi_read_options, &k, string_ptr)
                .within_unique_ptr()
        };

        let mut string = unsafe { UniquePtr::from_raw(string_ptr) };

        if status.ok() {
            return Ok(string.pin_mut().to_str().unwrap().to_string());
        }

        Err(Error::from(&status))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::error::Code;
    use crate::test_common::new_temp_path;

    #[test]
    fn open_without_creating() {
        let options = Options::default();
        let path = new_temp_path().unwrap();
        let result = DB::open(&options, &path);

        let expected = Error {
            code: Code::InvalidArgument,
            subcode: None,
            state: Some(format!(
                "{}/CURRENT: does not exist (create_if_missing is false)",
                path.display()
            )),
        };

        assert_eq!(result.err().unwrap(), expected);
    }

    #[test]
    fn open_with_options_override_without_creating() {
        let mut options = Options::default();
        options.as_db_options().set_create_if_missing(false);

        let path = new_temp_path().unwrap();
        let result = DB::open(&options, &path);

        let expected = Error {
            code: Code::InvalidArgument,
            subcode: None,
            state: Some(format!(
                "{}/CURRENT: does not exist (create_if_missing is false)",
                path.display()
            )),
        };

        assert_eq!(result.err().unwrap(), expected);
    }

    #[test]
    fn open_create() {
        let mut options = Options::default();
        options.as_db_options().set_create_if_missing(true);

        let path = new_temp_path().unwrap();
        let _db = DB::open(&options, &path).unwrap();
    }

    #[test]
    fn put_and_get() {
        let mut options = Options::default();
        options.as_db_options().set_create_if_missing(true);

        let path = new_temp_path().unwrap();
        let mut db = DB::open(&options, &path).unwrap();

        let write_options = WriteOptions::default();
        db.put(&write_options, "key1", "value1").unwrap();

        let read_options = ReadOptions::default();
        let value = db.get(&read_options, "key1").unwrap();
        assert_eq!(value, "value1");
    }
}
