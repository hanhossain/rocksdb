pub mod advanced_options;
pub mod math;
pub mod options;

use crate::advanced_options::{
    new_compaction_options_fifo, new_configurable_compaction_options_fifo,
};
use crate::math::*;
use crate::options::new_live_files_storage_info_options;

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

    // Value types encoded as the last component of internal keys.
    // DO NOT CHANGE THESE ENUM VALUES: they are embedded in the on-disk
    // data structures.
    // The highest bit of the value type needs to be reserved to SST tables
    // for them to do more flexible encoding.
    #[namespace = "rs::db::dbformat"]
    enum ValueType {
        kTypeDeletion = 0x0,
        kTypeValue = 0x1,
        kTypeMerge = 0x2,
        // WAL only.
        kTypeLogData = 0x3,
        // WAL only.
        kTypeColumnFamilyDeletion = 0x4,
        // WAL only.
        kTypeColumnFamilyValue = 0x5,
        // WAL only.
        kTypeColumnFamilyMerge = 0x6,
        kTypeSingleDeletion = 0x7,
        // WAL only.
        kTypeColumnFamilySingleDeletion = 0x8,
        // WAL only.
        kTypeBeginPrepareXID = 0x9,
        // WAL only.
        kTypeEndPrepareXID = 0xA,
        // WAL only.
        kTypeCommitXID = 0xB,
        // WAL only.
        kTypeRollbackXID = 0xC,
        // WAL only.
        kTypeNoop = 0xD,
        // WAL only.
        kTypeColumnFamilyRangeDeletion = 0xE,
        // meta block
        kTypeRangeDeletion = 0xF,
        // Blob DB only
        kTypeColumnFamilyBlobIndex = 0x10,
        // Blob DB only
        // When the prepared record is also persisted in db, we use a different
        // record. This is to ensure that the WAL that is generated by a WritePolicy
        // is not mistakenly read by another, which would result into data
        // inconsistency.
        kTypeBlobIndex = 0x11,
        // WAL only.
        // Similar to ValueType::kTypeBeginPersistedPrepareXID, this is to ensure that WAL
        // generated by WriteUnprepared write policy is not mistakenly read by
        // another.
        kTypeBeginPersistedPrepareXID = 0x12,
        // WAL only.
        kTypeBeginUnprepareXID = 0x13,
        kTypeDeletionWithTimestamp = 0x14,
        // WAL only
        kTypeCommitXIDAndTimestamp = 0x15,
        kTypeWideColumnEntity = 0x16,
        // WAL only
        kTypeColumnFamilyWideColumnEntity = 0x17,
        // Should be after the last valid type, only used for
        // validation
        kTypeMaxValid,
        // Not used for storing records.
        kMaxValue = 0x7F,
    }

    /// The underlying "class/type" of the option. This enum is used to determine how the option
    /// should be converted to/from strings and compared.
    #[namespace = "rs::options_type"]
    enum OptionType {
        Boolean,
        Int,
        Int32T,
        Int64T,
        UInt,
        UInt8T,
        UInt32T,
        UInt64T,
        SizeT,
        String,
        Double,
        CompactionStyle,
        CompactionPri,
        CompressionType,
        CompactionStopStyle,
        ChecksumType,
        EncodingType,
        Env,
        Enum,
        Struct,
        Vector,
        Configurable,
        Customizable,
        EncodedString,
        Temperature,
        Array,
        Unknown,
    }

    #[namespace = "rs::options_type"]
    enum OptionVerificationType {
        Normal,
        /// The option is pointer typed so we can only verify
        /// based on it's name.
        ByName,
        /// Same as kByName, but it also allows the case
        /// where one of them is a nullptr.
        ByNameAllowNull,
        /// Same as kByName, but it also allows the case
        /// where the old option is nullptr.
        ByNameAllowFromNull,
        /// The option is no longer used in rocksdb. The RocksDB
        /// OptionsParser will still accept this option if it
        /// happen to exists in some Options file.  However,
        /// the parser will not include it in serialization
        /// and verification processes.
        Deprecated,
        /// This option represents is a name/shortcut for
        /// another option and should not be written or verified
        /// independently
        Alias,
    }

    /// A set of modifier flags used to alter how an option is evaluated or
    /// processed. These flags can be combined together (e.g. kMutable | kShared).
    /// The kCompare flags can be used to control if/when options are compared.
    /// If kCompareNever is set, two related options would never be compared (always
    /// equal) If kCompareExact is set, the options will only be compared if the
    /// sanity mode
    ///                  is exact
    /// Mutable       means the option can be changed after it is prepared
    /// Shared        means the option is contained in a std::shared_ptr
    /// Unique        means the option is contained in a std::uniqued_ptr
    /// RawPointer    means the option is a raw pointer value.
    /// AllowNull     means that an option is allowed to be null for verification
    ///               purposes.
    /// DontSerialize means this option should not be serialized and included in
    ///               the string representation.
    /// DontPrepare   means do not call PrepareOptions for this pointer value.
    #[namespace = "rs::options_type"]
    enum OptionTypeFlags {
        /// No flags
        None = 0x00,
        CompareDefault = 0x0,
        // this should be SanityLevel::None
        CompareNever = 0x01,
        // this should be SanityLevel::LooselyCompatible
        CompareLoose = 0x02,
        // this should be SanityLevel::ExactMatch
        CompareExact = 0xFF,

        /// Option is mutable
        Mutable = 0x0100,
        /// The option is stored as a raw pointer
        RawPointer = 0x0200,
        /// The option is stored as a shared_ptr
        Shared = 0x0400,
        /// The option is stored as a unique_ptr
        Unique = 0x0800,
        /// The option can be null
        AllowNull = 0x1000,
        /// Don't serialize the option
        DontSerialize = 0x2000,
        /// Don't prepare or sanitize this option
        DontPrepare = 0x4000,
        /// The option serializes to a name only
        StringNameOnly = 0x8000,
    }

    /// This enum defines the RocksDB options sanity level.
    #[namespace = "rs::convenience"]
    enum SanityLevel {
        /// Performs no sanity check at all.
        None = 0x01,
        /// Performs minimum check to ensure the RocksDB instance can be
        /// opened without corrupting / mis-interpreting the data.
        LooselyCompatible = 0x02,
        /// Perform exact match sanity check.
        ExactMatch = 0xFF,
    }

    #[namespace = "rs::convenience"]
    enum Depth {
        /// Traverse nested options that are not flagged as "shallow"
        Default,
        /// Do not traverse into any nested options
        Shallow,
        /// Traverse nested options, overriding the options shallow setting
        Detailed,
    }

    #[namespace = "rs::advanced_options"]
    enum CompactionStyle {
        /// Level based compaction style
        Level = 0x0,
        /// Universal compaction style
        Universal = 0x1,
        /// FIFO compaction style
        FIFO = 0x2,
        /// Disable background compaction. Compaction jobs are submitted via CompactFiles().
        None = 0x3,
    }

    /// In Level-based compaction, it Determines which file from a level to be
    /// picked to merge to the next level. We suggest people try
    /// [`CompactionPri::MinOverlappingRatio`] first when you tune your database.
    #[namespace = "rs::advanced_options"]
    enum CompactionPri {
        /// Slightly prioritize larger files by size compensated by #deletes
        ByCompensatedSize = 0x0,
        /// First compact files whose data's latest update time is oldest.
        /// Try this if you only update some hot keys in small ranges.
        OldestLargestSeqFirst = 0x1,
        /// First compact files whose range hasn't been compacted to the next level
        /// for the longest. If your updates are random across the key space,
        /// write amplification is slightly better with this option.
        OldestSmallestSeqFirst = 0x2,
        /// First compact files whose ratio between overlapping size in next level
        /// and its size is the smallest. It in many cases can optimize write
        /// amplification.
        MinOverlappingRatio = 0x3,
        /// Keeps a cursor(s) of the successor of the file (key range) was/were
        /// compacted before, and always picks the next files (key range) in that
        /// level. The file picking process will cycle through all the files in a
        /// round-robin manner.
        RoundRobin = 0x4,
    }

    /// Temperature of a file. Used to pass to FileSystem for a different placement and/or coding.
    /// Reserve some numbers in the middle, in case we need to insert new tier there.
    #[namespace = "rs::advanced_options"]
    enum Temperature {
        Unknown = 0,
        Hot = 0x04,
        Warm = 0x08,
        Cold = 0x0C,
        LastTemperature,
    }

    /// The control option of how the cache tiers will be used. Currently rocksdb
    /// support block cache (volatile tier), secondary cache (non-volatile tier).
    /// In the future, we may add more caching layers.
    #[namespace = "rs::advanced_options"]
    enum CacheTier {
        VolatileTier = 0,
        NonVolatileBlockTier = 0x01,
    }

    /// Return status For inplace update callback
    #[namespace = "rs::advanced_options"]
    enum UpdateStatus {
        /// Nothing to update
        Failed = 0,
        /// Value updated inplace
        UpdatedInplace = 1,
        /// No inplace update. Merged value set
        Updated = 2,
    }

    #[namespace = "rs::advanced_options"]
    enum PrepopulateBlobCache {
        // Disable prepopulate blob cache
        Disable = 0x0,
        // Prepopulate blobs during flush only
        FlushOnly = 0x1,
    }

    #[namespace = "rs::advanced_options"]
    struct CompactionOptionsFIFO {
        /// Once the total sum of table files reaches this, we will delete the oldest table file.
        /// Default: 1GB
        max_table_files_size: u64,
        /// If true, try to do compaction to compact smaller files into larger ones. Minimum files
        /// to compact follows options.level0_file_num_compaction_trigger and compaction won't
        /// trigger if average compact bytes per del file is larger than options.write_buffer_size.
        /// This is to protect large files from being compacted again.
        /// Default: false;
        allow_compaction: bool,
        /// When not 0, if the data in the file is older than this threshold, RocksDB will soon move
        /// the file to warm temperature.
        age_for_warm: u64,
    }

    #[namespace = "rs::options"]
    enum CompactionServiceJobStatus {
        Success = 0,
        Failure = 1,
        UseLocal = 2,
    }

    #[namespace = "rs::options"]
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
    #[namespace = "rs::options"]
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
    #[namespace = "rs::options"]
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
    #[namespace = "rs::options"]
    enum BlobGarbageCollectionPolicy {
        /// Force blob file garbage collection.
        Force,
        /// Skip blob file garbage collection.
        Disable,
        /// Inherit blob file garbage collection policy from ColumnFamilyOptions.
        UseDefault,
    }

    #[namespace = "rs::options"]
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

    #[namespace = "rs::options"]
    struct LiveFilesStorageInfoOptions {
        /// Whether to populate FileStorageInfo::file_checksum* or leave blank
        include_checksum_info: bool,
        /// Flushes memtables if total size in bytes of live WAL files is >= this
        /// number (and DB is not read-only).
        /// Default: always force a flush without checking sizes.
        wal_size_for_flush: u64,
    }

    #[namespace = "rs::status"]
    enum Severity {
        kNoError = 0,
        kSoftError = 1,
        kHardError = 2,
        kFatalError = 3,
        kUnrecoverableError = 4,
        kMaxSeverity,
    }

    #[namespace = "rs::status"]
    enum Code {
        kOk = 0,
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5,
        kMergeInProgress = 6,
        kIncomplete = 7,
        kShutdownInProgress = 8,
        kTimedOut = 9,
        kAborted = 10,
        kBusy = 11,
        kExpired = 12,
        kTryAgain = 13,
        kCompactionTooLarge = 14,
        kColumnFamilyDropped = 15,
        kMaxCode,
    }

    #[namespace = "rs::advanced_options"]
    extern "Rust" {
        fn new_compaction_options_fifo() -> CompactionOptionsFIFO;

        fn new_configurable_compaction_options_fifo(
            max_table_files_size: u64,
            allow_compaction: bool,
        ) -> CompactionOptionsFIFO;
    }

    #[namespace = "rs::options"]
    extern "Rust" {
        #[cxx_name = "LiveFilesStorageInfoOptions_new"]
        fn new_live_files_storage_info_options() -> LiveFilesStorageInfoOptions;
    }

    #[namespace = "rs::math"]
    extern "Rust" {
        #[cxx_name = "FloorLog2"]
        fn floor_log2_i8(v: i8) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_i16(v: i16) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_i32(v: i32) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_i64(v: i64) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_u8(v: u8) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_u16(v: u16) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_u32(v: u32) -> i32;

        #[cxx_name = "FloorLog2"]
        fn floor_log2_u64(v: u64) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_i8(v: i8) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_i16(v: i16) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_i32(v: i32) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_i64(v: i64) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_u8(v: u8) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_u16(v: u16) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_u32(v: u32) -> i32;

        #[cxx_name = "CountTrailingZeroBits"]
        fn trailing_zeros_u64(v: u64) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_i8(v: i8) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_i16(v: i16) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_i32(v: i32) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_i64(v: i64) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_u8(v: u8) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_u16(v: u16) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_u32(v: u32) -> i32;

        #[cxx_name = "BitsSetToOne"]
        fn count_ones_u64(v: u64) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_i8(v: i8) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_i16(v: i16) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_i32(v: i32) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_i64(v: i64) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_u8(v: u8) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_u16(v: u16) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_u32(v: u32) -> i32;

        #[cxx_name = "BitParity"]
        fn parity_u64(v: u64) -> i32;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_i8(v: i8) -> i8;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_i16(v: i16) -> i16;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_i32(v: i32) -> i32;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_i64(v: i64) -> i64;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_u8(v: u8) -> u8;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_u16(v: u16) -> u16;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_u32(v: u32) -> u32;

        #[cxx_name = "EndianSwapValue"]
        fn swap_bytes_u64(v: u64) -> u64;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_i8(v: i8) -> i8;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_i16(v: i16) -> i16;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_i32(v: i32) -> i32;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_i64(v: i64) -> i64;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_u8(v: u8) -> u8;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_u16(v: u16) -> u16;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_u32(v: u32) -> u32;

        #[cxx_name = "ReverseBits"]
        fn reverse_bits_u64(v: u64) -> u64;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_i8(v: i8) -> i8;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_i16(v: i16) -> i16;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_i32(v: i32) -> i32;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_i64(v: i64) -> i64;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_u8(v: u8) -> u8;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_u16(v: u16) -> u16;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_u32(v: u32) -> u32;

        #[cxx_name = "DownwardInvolution"]
        fn downward_involution_u64(v: u64) -> u64;
    }
}
