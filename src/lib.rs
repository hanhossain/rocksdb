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
    generate!("rocksdb::ReadOptions")
    generate!("rocksdb::Slice")
    generate!("rocksdb::Status")
    generate!("rocksdb::WriteOptions")
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
