use crate::ffi::{CompactionOptionsUniversal, CompactionStopStyle};

impl CompactionOptionsUniversal {
    pub fn new() -> CompactionOptionsUniversal {
        CompactionOptionsUniversal {
            size_ratio: 1,
            min_merge_width: 2,
            max_merge_width: u32::MAX,
            max_size_amplification_percent: 200,
            compression_size_percent: -1,
            stop_style: CompactionStopStyle::kCompactionStopStyleTotalSize,
            allow_trivial_move: false,
            incremental: false,
        }
    }
}

pub fn new_compaction_options_universal() -> CompactionOptionsUniversal {
    CompactionOptionsUniversal::new()
}
