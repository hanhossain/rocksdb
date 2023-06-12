use crate::ffi::Slice;
use std::ffi::{c_char, CStr};

impl Slice {
    pub(crate) unsafe fn from_raw_with_size(d: *const c_char, n: usize) -> Slice {
        assert!(!d.is_null());
        let data = CStr::from_ptr(d).to_bytes_with_nul().to_vec();
        Slice { data, size: n }
    }

    pub(crate) unsafe fn from_raw(d: *const c_char) -> Slice {
        assert!(!d.is_null());
        let data = CStr::from_ptr(d).to_bytes_with_nul().to_vec();
        // size without nul terminator
        let size = data.len() - 1;
        Slice { data, size }
    }
}

impl Default for Slice {
    fn default() -> Self {
        Slice {
            data: vec![0],
            size: 0,
        }
    }
}

pub(crate) fn default_slice() -> Slice {
    Slice::default()
}

pub(crate) unsafe fn slice_from_raw_with_size(d: *const c_char, n: usize) -> Slice {
    Slice::from_raw_with_size(d, n)
}

pub(crate) unsafe fn slice_from_raw(d: *const c_char) -> Slice {
    Slice::from_raw(d)
}
