pub mod db;
pub mod error;
pub mod options;

use autocxx::prelude::*;

include_cpp! {
    #include "rocksdb/db.h"
    #include "rocksdb/options.h"
    #include "rocksdb/status.h"
    safety!(unsafe)
    generate!("rocksdb::DB")
    generate!("rocksdb::DB_Open")
    generate!("rocksdb::DBResult")
    generate!("rocksdb::DBOptions")
    generate!("rocksdb::ColumnFamilyOptions")
    generate!("rocksdb::Options")
    generate!("rocksdb::Status")
}

#[cxx::bridge(namespace = "rocksdb_test")]
pub mod cxx_ffi {
    unsafe extern "C++" {
        include!("rocksdb/env.h");

        #[cxx_name = "PrintHelloWorld"]
        fn print_hello_world();
    }
}

#[cfg(test)]
pub(crate) mod test_common {
    use std::fs;
    use std::io;
    use std::path::PathBuf;
    use uuid::Uuid;

    pub(crate) fn new_temp_path() -> io::Result<PathBuf> {
        let mut dir = std::env::temp_dir();
        dir.push("rocksdb-rs-tests");
        fs::create_dir_all(&dir)?;
        dir.push(Uuid::new_v4().to_string());
        Ok(dir)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn db_open_create() {
        let path = test_common::new_temp_path().unwrap();
        let mut db_options = ffi::rocksdb::DBOptions::new().within_unique_ptr();
        db_options.pin_mut().SetCreateIfMissing(true);
        moveit! {
            let column_family_options = ffi::rocksdb::ColumnFamilyOptions::new();
            let options = ffi::rocksdb::Options::new1(&db_options, &column_family_options);
        }
        let db_path = ffi::make_string(path.to_str().unwrap());
        let mut res = ffi::rocksdb::DB_Open(&options, &db_path).within_unique_ptr();
        moveit! { let status = res.pin_mut().get_status(); }
        println!("status: {}", status.ToString().to_string());
        assert!(status.ok());
    }
}
