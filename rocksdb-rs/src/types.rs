#[cxx::bridge(namespace = "rs::types")]
mod ffi {
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
