pub mod advanced_options;
pub mod convenience;
pub mod db;
pub mod options;
pub mod options_type;

#[cxx::bridge]
mod ffi {
    #[namespace = "rs::types"]
    enum TableFileCreationReason {
        Flush,
        Compaction,
        Recovery,
        Misc,
    }

    #[namespace = "rs::types"]
    enum BlobFileCreationReason {
        Flush,
        Compaction,
        Recovery,
    }

    /// The types of files RocksDB uses in a DB directory. (Available for
    /// advanced options.)
    #[repr(i32)]
    #[namespace = "rs::types"]
    enum FileType {
        WalFile,
        DBLockFile,
        TableFile,
        DescriptorFile,
        CurrentFile,
        TempFile,
        /// Either the current one, or an old one
        InfoLogFile,
        MetaDatabase,
        IdentityFile,
        OptionsFile,
        BlobFile,
    }

    /// User-oriented representation of internal key types.
    /// Ordering of this enum entries should not change.
    #[namespace = "rs::types"]
    enum EntryType {
        Put,
        Delete,
        SingleDelete,
        Merge,
        RangeDeletion,
        BlobIndex,
        DeleteWithTimestamp,
        WideColumnEntity,
        Other,
    }

    #[namespace = "rs::types"]
    enum WriteStallCause {
        // Beginning of CF-scope write stall causes
        // Always keep `MemtableLimit` as the first stat in this section
        MemtableLimit,
        L0FileCountLimit,
        PendingCompactionBytes,
        CFScopeWriteStallCauseEnumMax,
        // End of CF-scope write stall causes

        // Beginning of DB-scope write stall causes
        //
        // Always keep `WriteBufferManagerLimit` as the first stat in this section
        WriteBufferManagerLimit,
        DBScopeWriteStallCauseEnumMax,
        // End of DB-scope write stall causes

        // Always add new WriteStallCause before `None`
        None,
    }

    #[namespace = "rs::types"]
    enum WriteStallCondition {
        Delayed,
        Stopped,
        // Always add new WriteStallCondition before `Normal`
        Normal,
    }
}

pub fn hello_world() {
    println!("Hello from rust!");
}
