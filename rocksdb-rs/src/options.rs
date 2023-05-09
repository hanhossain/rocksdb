#[cxx::bridge(namespace = "rs::options")]
mod ffi {
    enum CompactionServiceJobStatus {
        Success = 0,
        Failure = 1,
        UseLocal = 2,
    }

    enum WALRecoveryMode {
        /// Original levelDB recovery
        ///
        /// We tolerate the last record in any log to be incomplete due to a crash
        /// while writing it. Zeroed bytes from preallocation are also tolerated in the
        /// trailing data of any log.
        ///
        /// Use case: Applications for which updates, once applied, must not be rolled
        /// back even after a crash-recovery. In this recovery mode, RocksDB guarantees
        /// this as long as `WritableFile::Append()` writes are durable. In case the
        /// user needs the guarantee in more situations (e.g., when
        /// `WritableFile::Append()` writes to page cache, but the user desires this
        /// guarantee in face of power-loss crash-recovery), RocksDB offers various
        /// mechanisms to additionally invoke `WritableFile::Sync()` in order to
        /// strengthen the guarantee.
        ///
        /// This differs from `PointInTimeRecovery` in that, in case a corruption is
        /// detected during recovery, this mode will refuse to open the DB. Whereas,
        /// `PointInTimeRecovery` will stop recovery just before the corruption since
        /// that is a valid point-in-time to which to recover.
        TolerateCorruptedTailRecords = 0x00,
        /// Recover from clean shutdown
        /// We don't expect to find any corruption in the WAL
        /// Use case : This is ideal for unit tests and rare applications that
        /// can require high consistency guarantee
        AbsoluteConsistency = 0x01,
        /// Recover to point-in-time consistency (default)
        /// We stop the WAL playback on discovering WAL inconsistency
        /// Use case : Ideal for systems that have disk controller cache like
        /// hard disk, SSD without super capacitor that store related data
        PointInTimeRecovery = 0x02,
        /// Recovery after a disaster
        /// We ignore any corruption in the WAL and try to salvage as much data as
        /// possible
        /// Use case : Ideal for last ditch effort to recover data or systems that
        /// operate with low grade unrelated data
        SkipAnyCorruptedRecords = 0x03,
    }

    /// An application can issue a read request (via Get/Iterators) and specify
    /// if that read should process data that ALREADY resides on a specified cache
    /// level. For example, if an application specifies kBlockCacheTier then the
    /// Get call will process data that is already processed in the memtable or
    /// the block cache. It will not page in data from the OS cache or data that
    /// resides in storage.
    enum ReadTier {
        /// Data in memtable, block cache, OS cache or storage
        ReadAllTier = 0x0,
        /// Data in memtable or block cache
        BlockCacheTier = 0x1,
        /// Persisted data.  When WAL is disabled, this option will skip data in memtable. Note that
        /// this ReadTier currently only supports Get and MultiGet and does not support iterators.
        PersistedTier = 0x2,
        /// Data in memtable. Used for memtable-only iterators.
        MemtableTier = 0x3,
    }

    /// For level based compaction, we can configure if we want to skip/force
    /// bottommost level compaction.
    enum BottommostLevelCompaction {
        /// Skip bottommost level compaction
        Skip,
        /// Only compact bottommost level if there is a compaction filter .This is the default
        /// option.
        IfHaveCompactionFilter,
        /// Always compact bottommost level
        Force,
        /// Always compact bottommost level but in bottommost level avoid double-compacting files
        /// created in the same compaction
        ForceOptimized,
    }

    /// For manual compaction, we can configure if we want to skip/force garbage collection of blob
    /// files.
    enum BlobGarbageCollectionPolicy {
        /// Force blob file garbage collection.
        Force,
        /// Skip blob file garbage collection.
        Disable,
        /// Inherit blob file garbage collection policy from ColumnFamilyOptions.
        UseDefault,
    }

    enum TraceFilterType {
        /// Trace all the operations
        None = 0x0,
        /// Do not trace the get operations
        Get = 0x1,
        /// Do not trace the write operations
        Write = 0x2,
        /// Do not trace the `Iterator::Seek()` operations
        IteratorSeek = 0x4,
        /// Do not trace the `Iterator::SeekForPrev()` operations
        IteratorSeekForPrev = 0x8,
        /// Do not trace the `MultiGet()` operations
        MultiGet = 0x10,
    }
}
