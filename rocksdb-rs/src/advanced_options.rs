use crate::advanced_options::ffi::CompactionOptionsFIFO;

#[cxx::bridge(namespace = "rs::advanced_options")]
mod ffi {
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
    enum CacheTier {
        VolatileTier = 0,
        NonVolatileBlockTier = 0x01,
    }

    /// Return status For inplace update callback
    enum UpdateStatus {
        /// Nothing to update
        Failed = 0,
        /// Value updated inplace
        UpdatedInplace = 1,
        /// No inplace update. Merged value set
        Updated = 2,
    }

    enum PrepopulateBlobCache {
        // Disable prepopulate blob cache
        Disable = 0x0,
        // Prepopulate blobs during flush only
        FlushOnly = 0x1,
    }

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

    extern "Rust" {
        fn new_compaction_options_fifo() -> CompactionOptionsFIFO;

        fn new_configurable_compaction_options_fifo(
            max_table_files_size: u64,
            allow_compaction: bool,
        ) -> CompactionOptionsFIFO;
    }
}

fn new_compaction_options_fifo() -> CompactionOptionsFIFO {
    CompactionOptionsFIFO {
        max_table_files_size: 1 * 1024 * 1024 * 1024,
        allow_compaction: false,
        age_for_warm: 0,
    }
}

fn new_configurable_compaction_options_fifo(
    max_table_files_size: u64,
    allow_compaction: bool,
) -> CompactionOptionsFIFO {
    let mut compaction_options_fifo = new_compaction_options_fifo();
    compaction_options_fifo.max_table_files_size = max_table_files_size;
    compaction_options_fifo.allow_compaction = allow_compaction;
    compaction_options_fifo
}
