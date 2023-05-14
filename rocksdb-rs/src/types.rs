#[cxx::bridge(namespace = "rs::types")]
mod ffi {
    enum WriteStallCondition {
        Delayed,
        Stopped,
        /// Always add new WriteStallCondition before `Normal`
        Normal,
    }
}
