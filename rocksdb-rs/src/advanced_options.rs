use crate::ffi::CompactionOptionsFIFO;

pub fn new_compaction_options_fifo() -> CompactionOptionsFIFO {
    CompactionOptionsFIFO {
        max_table_files_size: 1 * 1024 * 1024 * 1024,
        allow_compaction: false,
        age_for_warm: 0,
    }
}

pub fn new_configurable_compaction_options_fifo(
    max_table_files_size: u64,
    allow_compaction: bool,
) -> CompactionOptionsFIFO {
    let mut compaction_options_fifo = new_compaction_options_fifo();
    compaction_options_fifo.max_table_files_size = max_table_files_size;
    compaction_options_fifo.allow_compaction = allow_compaction;
    compaction_options_fifo
}
