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
}
