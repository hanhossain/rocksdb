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
}
