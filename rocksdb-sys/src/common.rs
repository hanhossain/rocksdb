use std::ffi::{c_char, CStr};

/// Converts the char pointer to a vec with a nul terminator. If a null pointer is passed in, then
/// this returns an empty vec with no nul terminator.
pub(crate) unsafe fn char_ptr_to_bytes(data: *const c_char) -> Vec<u8> {
    if data.is_null() {
        Vec::new()
    } else {
        CStr::from_ptr(data).to_bytes_with_nul().to_vec()
    }
}

/// Converts the char pointer to a vec with a nul terminator and returns the size of the vec
/// (without nul terminator). If a null pointer is passed in, then this returns an empty vec with no
/// null terminator.
pub(crate) unsafe fn char_ptr_to_bytes_and_size(data: *const c_char) -> (Vec<u8>, usize) {
    let data = char_ptr_to_bytes(data);
    let size = if data.is_empty() { 0 } else { data.len() - 1 };
    (data, size)
}
