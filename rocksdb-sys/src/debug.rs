use crate::ffi::{KeyVersion, ValueType};
use cxx::CxxString;
use std::ffi::{c_char, CStr};

impl KeyVersion {
    pub(crate) fn new<T: Into<Vec<u8>>>(
        user_key: T,
        value: T,
        sequence: u64,
        value_type: ValueType,
    ) -> KeyVersion {
        KeyVersion {
            user_key: user_key.into(),
            value: value.into(),
            sequence,
            value_type,
        }
    }

    pub(crate) fn get_type_name(&self) -> String {
        match self.value_type {
            ValueType::TypeDeletion => String::from("TypeDeletion"),
            ValueType::TypeValue => String::from("TypeValue"),
            ValueType::TypeMerge => String::from("TypeMerge"),
            ValueType::TypeLogData => String::from("TypeLogData"),
            ValueType::TypeColumnFamilyDeletion => String::from("TypeColumnFamilyDeletion"),
            ValueType::TypeColumnFamilyValue => String::from("TypeColumnFamilyValue"),
            ValueType::TypeColumnFamilyMerge => String::from("TypeColumnFamilyMerge"),
            ValueType::TypeSingleDeletion => String::from("TypeSingleDeletion"),
            ValueType::TypeColumnFamilySingleDeletion => {
                String::from("TypeColumnFamilySingleDeletion")
            }
            ValueType::TypeBeginPrepareXID => String::from("TypeBeginPrepareXID"),
            ValueType::TypeEndPrepareXID => String::from("TypeEndPrepareXID"),
            ValueType::TypeCommitXID => String::from("TypeCommitXID"),
            ValueType::TypeRollbackXID => String::from("TypeRollbackXID"),
            ValueType::TypeNoop => String::from("TypeNoop"),
            ValueType::TypeColumnFamilyRangeDeletion => {
                String::from("TypeColumnFamilyRangeDeletion")
            }
            ValueType::TypeRangeDeletion => String::from("TypeRangeDeletion"),
            ValueType::TypeColumnFamilyBlobIndex => String::from("TypeColumnFamilyBlobIndex"),
            ValueType::TypeBlobIndex => String::from("TypeBlobIndex"),
            ValueType::TypeBeginPersistedPrepareXID => String::from("TypeBeginPersistedPrepareXID"),
            ValueType::TypeBeginUnprepareXID => String::from("TypeBeginUnprepareXID"),
            ValueType::TypeDeletionWithTimestamp => String::from("TypeDeletionWithTimestamp"),
            ValueType::TypeCommitXIDAndTimestamp => String::from("TypeCommitXIDAndTimestamp"),
            ValueType::TypeWideColumnEntity => String::from("TypeWideColumnEntity"),
            ValueType::TypeColumnFamilyWideColumnEntity => {
                String::from("TypeColumnFamilyWideColumnEntity")
            }
            ValueType::TypeMaxValid => String::from("TypeMaxValid"),
            ValueType::MaxValue => String::from("MaxValue"),
            _ => String::from("Invalid"),
        }
    }
}

impl Default for KeyVersion {
    fn default() -> Self {
        KeyVersion {
            user_key: Vec::new(),
            value: Vec::new(),
            sequence: 0,
            value_type: ValueType::TypeDeletion,
        }
    }
}

pub(crate) unsafe fn new_key_version(
    user_key: *const c_char,
    value: *const c_char,
    sequence: u64,
    value_type: ValueType,
) -> KeyVersion {
    assert!(!user_key.is_null());
    assert!(!value.is_null());

    let user_key = CStr::from_ptr(user_key);
    let value = CStr::from_ptr(value);

    KeyVersion::new(
        user_key.to_bytes_with_nul().to_vec(),
        value.to_bytes_with_nul().to_vec(),
        sequence,
        value_type,
    )
}

pub(crate) fn new_key_version_from_cstrings(
    user_key: &CxxString,
    value: &CxxString,
    sequence: u64,
    value_type: ValueType,
) -> KeyVersion {
    unsafe {
        // SAFETY: CxxString will always produce a valid *const c_char ptr
        new_key_version(
            user_key.as_ptr() as *const c_char,
            value.as_ptr() as *const c_char,
            sequence,
            value_type,
        )
    }
}

pub(crate) fn default_key_version() -> KeyVersion {
    KeyVersion::default()
}
