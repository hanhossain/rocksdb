use crate::error::Error;
use crate::ffi;
use crate::ffi::rocksdb;
use crate::options::Options;
use autocxx::WithinUniquePtr;
use cxx::UniquePtr;
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

        Err(Error::from(status.as_ref().unwrap()))
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
}
