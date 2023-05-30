use crate::ffi::{KeyVersion, ValueType};

impl KeyVersion {
    pub(crate) fn new(
        user_key: String,
        value: String,
        sequence: u64,
        value_type: ValueType,
    ) -> KeyVersion {
        KeyVersion {
            user_key,
            value,
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
            user_key: String::new(),
            value: String::new(),
            sequence: 0,
            value_type: ValueType::TypeDeletion,
        }
    }
}

pub(crate) fn new_key_version(
    user_key: String,
    value: String,
    sequence: u64,
    value_type: ValueType,
) -> KeyVersion {
    KeyVersion::new(user_key, value, sequence, value_type)
}

pub(crate) fn default_key_version() -> KeyVersion {
    KeyVersion::default()
}
