#[cxx::bridge(namespace = "rs::convenience")]
mod ffi {
    /// This enum defines the RocksDB options sanity level.
    enum SanityLevel {
        /// Performs no sanity check at all.
        None = 0x01,
        /// Performs minimum check to ensure the RocksDB instance can be
        /// opened without corrupting / mis-interpreting the data.
        LooselyCompatible = 0x02,
        /// Perform exact match sanity check.
        ExactMatch = 0xFF,
    }
}
