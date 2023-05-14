#[cxx::bridge(namespace = "rs::types")]
mod ffi {
    enum BlobFileCreationReason {
        Flush,
        Compaction,
        Recovery,
    }

    /// The types of files RocksDB uses in a DB directory. (Available for
    /// advanced options.)
    #[repr(i32)]
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

    enum WriteStallCondition {
        Delayed,
        Stopped,
        // Always add new WriteStallCondition before `Normal`
        Normal,
    }
}
