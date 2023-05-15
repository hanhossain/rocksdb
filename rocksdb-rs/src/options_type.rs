#[cxx::bridge(namespace = "rs::options_type")]
mod ffi {
    /// The underlying "class/type" of the option. This enum is used to determine how the option
    /// should be converted to/from strings and compared.
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
}
