use std::ffi::{c_char, CStr};

/// Converts the char pointer to a vec with a nul terminator.
pub(crate) unsafe fn char_ptr_to_bytes(data: *const c_char) -> Vec<u8> {
    CStr::from_ptr(data).to_bytes_with_nul().to_vec()
}

/// Converts the char pointer to a vec with a nul terminator and returns the size of the vec
/// (without nul terminator)
pub(crate) unsafe fn char_ptr_to_bytes_and_size(data: *const c_char) -> (Vec<u8>, usize) {
    let data = char_ptr_to_bytes(data);
    let size = data.len() - 1;
    (data, size)
}
