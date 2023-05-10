use crate::advanced_options::ffi::{CompactionOptionsFIFO, CompressionOptions};

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

    /// Compression options for different compression algorithms like Zlib
    struct CompressionOptions {
        // ==> BEGIN options that can be set by deprecated configuration syntax, <==
        // ==> e.g. compression_opts=5:6:7:8:9:10:true:11:false                  <==
        // ==> Please use compression_opts={level=6;strategy=7;} form instead.   <==
        /// zlib only: windowBits parameter. See https://www.zlib.net/manual.html
        window_bits: i32,

        /// Compression "level" applicable to zstd, zlib, LZ4. Except for
        /// kDefaultCompressionLevel (see above), the meaning of each value depends
        /// on the compression algorithm.
        level: i32,

        /// zlib only: strategy parameter. See https://www.zlib.net/manual.html
        strategy: i32,

        /// Maximum size of dictionaries used to prime the compression library.
        /// Enabling dictionary can improve compression ratios when there are
        /// repetitions across data blocks.
        ///
        /// The dictionary is created by sampling the SST file data. If
        /// `zstd_max_train_bytes` is nonzero, the samples are passed through zstd's
        /// dictionary generator (see comments for option `use_zstd_dict_trainer` for
        /// detail on dictionary generator). If `zstd_max_train_bytes` is zero, the
        /// random samples are used directly as the dictionary.
        ///
        /// When compression dictionary is disabled, we compress and write each block
        /// before buffering data for the next one. When compression dictionary is
        /// enabled, we buffer SST file data in-memory so we can sample it, as data
        /// can only be compressed and written after the dictionary has been finalized.
        ///
        /// The amount of data buffered can be limited by `max_dict_buffer_bytes`. This
        /// buffered memory is charged to the block cache when there is a block cache.
        /// If block cache insertion fails with `Status::MemoryLimit` (i.e., it is
        /// full), we finalize the dictionary with whatever data we have and then stop
        /// buffering.
        max_dict_bytes: u32,

        /// Maximum size of training data passed to zstd's dictionary trainer. Using
        /// zstd's dictionary trainer can achieve even better compression ratio
        /// improvements than using `max_dict_bytes` alone.
        ///
        /// The training data will be used to generate a dictionary of max_dict_bytes.
        zstd_max_train_bytes: u32,

        /// Number of threads for parallel compression.
        /// Parallel compression is enabled only if threads > 1.
        /// THE FEATURE IS STILL EXPERIMENTAL
        ///
        /// This option is valid only when BlockBasedTable is used.
        ///
        /// When parallel compression is enabled, SST size file sizes might be
        /// more inflated compared to the target size, because more data of unknown
        /// compressed size is in flight when compression is parallelized. To be
        /// reasonably accurate, this inflation is also estimated by using historical
        /// compression ratio and current bytes inflight.
        parallel_threads: u32,

        /// When the compression options are set by the user, it will be set to "true".
        /// For bottommost_compression_opts, to enable it, user must set enabled=true.
        /// Otherwise, bottommost compression will use compression_opts as default
        /// compression options.
        ///
        /// For compression_opts, if compression_opts.enabled=false, it is still
        /// used as compression options for compression process.
        enabled: bool,

        /// Limit on data buffering when gathering samples to build a dictionary. Zero
        /// means no limit. When dictionary is disabled (`max_dict_bytes == 0`),
        /// enabling this limit (`max_dict_buffer_bytes != 0`) has no effect.
        ///
        /// In compaction, the buffering is limited to the target file size (see
        /// `target_file_size_base` and `target_file_size_multiplier`) even if this
        /// setting permits more buffering. Since we cannot determine where the file
        /// should be cut until data blocks are compressed with dictionary, buffering
        /// more than the target file size could lead to selecting samples that belong
        /// to a later output SST.
        ///
        /// Limiting too strictly may harm dictionary effectiveness since it forces
        /// RocksDB to pick samples from the initial portion of the output SST, which
        /// may not be representative of the whole file. Configuring this limit below
        /// `zstd_max_train_bytes` (when enabled) can restrict how many samples we can
        /// pass to the dictionary trainer. Configuring it below `max_dict_bytes` can
        /// restrict the size of the final dictionary.
        max_dict_buffer_bytes: u64,

        /// Use zstd trainer to generate dictionaries. When this option is set to true,
        /// zstd_max_train_bytes of training data sampled from max_dict_buffer_bytes
        /// buffered data will be passed to zstd dictionary trainer to generate a
        /// dictionary of size max_dict_bytes.
        ///
        /// When this option is false, zstd's API ZDICT_finalizeDictionary() will be
        /// called to generate dictionaries. zstd_max_train_bytes of training sampled
        /// data will be passed to this API. Using this API should save CPU time on
        /// dictionary training, but the compression ratio may not be as good as using
        /// a dictionary trainer.
        use_zstd_dict_trainer: bool,

        // ===> END options that can be set by deprecated configuration syntax <===
        // ===> Use compression_opts={level=6;strategy=7;} form for below opts <===
        /// Essentially specifies a minimum acceptable compression ratio. A block is
        /// stored uncompressed if the compressed block does not achieve this ratio,
        /// because the downstream cost of decompression is not considered worth such
        /// a small savings (if any).
        /// However, the ratio is specified in a way that is efficient for checking.
        /// An integer from 1 to 1024 indicates the maximum allowable compressed bytes
        /// per 1KB of input, so the minimum acceptable ratio is 1024.0 / this value.
        /// For example, for a minimum ratio of 1.5:1, set to 683. See SetMinRatio().
        /// Default: abandon use of compression for a specific block or entry if
        /// compressed by less than 12.5% (minimum ratio of 1.143:1).
        max_compressed_bytes_per_kb: i32,
    }

    extern "Rust" {
        fn new_compaction_options_fifo() -> CompactionOptionsFIFO;

        fn new_configurable_compaction_options_fifo(
            max_table_files_size: u64,
            allow_compaction: bool,
        ) -> CompactionOptionsFIFO;

        #[cxx_name = "SetMinRatio"]
        fn set_min_ratio(self: &mut CompressionOptions, min_ratio: f64);

        fn new_compression_options() -> CompressionOptions;
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

impl CompressionOptions {
    /// A convenience function for setting max_compressed_bytes_per_kb based on a
    /// minimum acceptable compression ratio (uncompressed size over compressed
    /// size).
    fn set_min_ratio(&mut self, min_ratio: f64) {
        self.max_compressed_bytes_per_kb = (1024. / min_ratio + 0.5) as i32;
    }
}

fn new_compression_options() -> CompressionOptions {
    CompressionOptions {
        window_bits: -14,
        level: 32767,
        strategy: 0,
        max_dict_bytes: 0,
        zstd_max_train_bytes: 0,
        parallel_threads: 1,
        enabled: false,
        max_dict_buffer_bytes: 0,
        use_zstd_dict_trainer: true,
        max_compressed_bytes_per_kb: 1024 * 7 / 8,
    }
}
