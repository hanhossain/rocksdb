use crate::ffi::LiveFilesStorageInfoOptions;

impl LiveFilesStorageInfoOptions {
    pub fn new() -> LiveFilesStorageInfoOptions {
        LiveFilesStorageInfoOptions {
            include_checksum_info: false,
            wal_size_for_flush: 0,
        }
    }
}

pub fn new_live_files_storage_info_options() -> LiveFilesStorageInfoOptions {
    LiveFilesStorageInfoOptions::new()
}
