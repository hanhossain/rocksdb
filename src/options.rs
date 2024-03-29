use crate::ffi::rocksdb;
use autocxx::prelude::*;
use std::pin::Pin;

pub struct Options {
    pub(crate) ffi_options: Pin<Box<rocksdb::Options>>,
}

impl Options {
    pub fn as_db_options(&mut self) -> DBOptionsRef<'_> {
        let db_options = self.ffi_options.as_mut().AsDBOptions();
        DBOptionsRef {
            ffi_db_options: db_options,
        }
    }

    pub fn as_column_family_options(&mut self) -> ColumnFamilyOptionsRef<'_> {
        let cf_options = self.ffi_options.as_mut().AsColumnFamilyOptions();
        ColumnFamilyOptionsRef {
            ffi_column_family_options: cf_options,
        }
    }
}

impl Default for Options {
    fn default() -> Self {
        let ffi_options = rocksdb::Options::new().within_box();
        Options { ffi_options }
    }
}

pub struct DBOptionsRef<'a> {
    pub(crate) ffi_db_options: Pin<&'a mut rocksdb::DBOptions>,
}

impl<'a> DBOptionsRef<'a> {
    pub fn set_create_if_missing(&mut self, value: bool) {
        self.ffi_db_options.as_mut().SetCreateIfMissing(value);
    }

    pub fn increase_parallelism(&mut self, total_threads: i32) {
        self.ffi_db_options
            .as_mut()
            .IncreaseParallelism(c_int(total_threads));
    }
}

pub struct ColumnFamilyOptionsRef<'a> {
    pub(crate) ffi_column_family_options: Pin<&'a mut rocksdb::ColumnFamilyOptions>,
}

impl<'a> ColumnFamilyOptionsRef<'a> {
    pub fn optimize_level_style_compaction(&mut self, memtable_memory_budget: u64) {
        self.ffi_column_family_options
            .as_mut()
            .OptimizeLevelStyleCompaction(memtable_memory_budget);
    }
}

pub struct WriteOptions {
    pub(crate) ffi_write_options: Pin<Box<rocksdb::WriteOptions>>,
}

impl Default for WriteOptions {
    fn default() -> Self {
        let value = rocksdb::WriteOptions::new().within_box();
        WriteOptions {
            ffi_write_options: value,
        }
    }
}

pub struct ReadOptions {
    pub(crate) ffi_read_options: Pin<Box<rocksdb::ReadOptions>>,
}

impl Default for ReadOptions {
    fn default() -> Self {
        let value = rocksdb::ReadOptions::new().within_box();
        ReadOptions {
            ffi_read_options: value,
        }
    }
}
