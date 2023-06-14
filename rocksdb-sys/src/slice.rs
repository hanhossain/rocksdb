use crate::common::{char_ptr_to_bytes, char_ptr_to_bytes_and_size};
use crate::ffi::Slice;
use std::ffi::c_char;

impl Slice {
    pub(crate) unsafe fn from_raw_with_size(d: *const c_char, n: usize) -> Slice {
        assert!(!d.is_null());
        let data = char_ptr_to_bytes(d);
        Slice { data, size: n }
    }

    pub(crate) unsafe fn from_raw(d: *const c_char) -> Slice {
        assert!(!d.is_null());
        let (data, size) = char_ptr_to_bytes_and_size(d);
        Slice { data, size }
    }

    pub(crate) fn set_size(&mut self, size: usize) {
        self.size = size;
    }

    pub(crate) fn size(&self) -> usize {
        self.size
    }

    pub(crate) unsafe fn set_data_ptr(&mut self, d: *const c_char) {
        assert!(!d.is_null());
        let data = char_ptr_to_bytes(d);
        self.data = data;
    }

    pub(crate) fn data_ptr(&self) -> *const c_char {
        self.data.as_ptr() as *const _
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
