#[cxx::bridge(namespace = "rs")]
mod ffi {
    extern "Rust" {
        fn hello_world();
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
}

pub fn hello_world() {
    println!("Hello from rust!");
}
