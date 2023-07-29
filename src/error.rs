use crate::ffi::rocksdb::{Status, Status_Code, Status_SubCode};
use std::ffi::CStr;
use std::fmt::{Display, Formatter};

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Code {
    NotFound,
    Corruption,
    NotSupported,
    InvalidArgument,
    IOError,
    MergeInProgress,
    Incomplete,
    ShutdownInProgress,
    TimedOut,
    Aborted,
    Busy,
    Expired,
    TryAgain,
    CompactionTooLarge,
    ColumnFamilyDropped,
}

impl From<Status_Code> for Code {
    fn from(value: Status_Code) -> Self {
        match value {
            Status_Code::kOk => panic!("Expected failure but found Status_Code::kOk"),
            Status_Code::kNotFound => Code::NotFound,
            Status_Code::kCorruption => Code::Corruption,
            Status_Code::kNotSupported => Code::NotSupported,
            Status_Code::kInvalidArgument => Code::InvalidArgument,
            Status_Code::kIOError => Code::IOError,
            Status_Code::kMergeInProgress => Code::MergeInProgress,
            Status_Code::kIncomplete => Code::Incomplete,
            Status_Code::kShutdownInProgress => Code::ShutdownInProgress,
            Status_Code::kTimedOut => Code::TimedOut,
            Status_Code::kAborted => Code::Aborted,
            Status_Code::kBusy => Code::Busy,
            Status_Code::kExpired => Code::Expired,
            Status_Code::kTryAgain => Code::TryAgain,
            Status_Code::kCompactionTooLarge => Code::CompactionTooLarge,
            Status_Code::kColumnFamilyDropped => Code::ColumnFamilyDropped,
            _ => unreachable!(),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum SubCode {
    MutexTimeout,
    LockTimeout,
    LockLimit,
    NoSpace,
    Deadlock,
    StaleFile,
    MemoryLimit,
    SpaceLimit,
    PathNotFound,
    MergeOperandsInsufficientCapacity,
    ManualCompactionPaused,
    Overwritten,
    TxnNotPrepared,
    IOFenced,
    MergeOperatorFailed,
}

impl From<Status_SubCode> for Option<SubCode> {
    fn from(value: Status_SubCode) -> Self {
        let subcode = match value {
            Status_SubCode::kNone => {
                return None;
            }
            Status_SubCode::kMutexTimeout => SubCode::MutexTimeout,
            Status_SubCode::kLockTimeout => SubCode::LockTimeout,
            Status_SubCode::kLockLimit => SubCode::LockLimit,
            Status_SubCode::kNoSpace => SubCode::NoSpace,
            Status_SubCode::kDeadlock => SubCode::Deadlock,
            Status_SubCode::kStaleFile => SubCode::StaleFile,
            Status_SubCode::kMemoryLimit => SubCode::MemoryLimit,
            Status_SubCode::kSpaceLimit => SubCode::SpaceLimit,
            Status_SubCode::kPathNotFound => SubCode::PathNotFound,
            Status_SubCode::KMergeOperandsInsufficientCapacity => {
                SubCode::MergeOperandsInsufficientCapacity
            }
            Status_SubCode::kManualCompactionPaused => SubCode::ManualCompactionPaused,
            Status_SubCode::kOverwritten => SubCode::Overwritten,
            Status_SubCode::kTxnNotPrepared => SubCode::TxnNotPrepared,
            Status_SubCode::kIOFenced => SubCode::IOFenced,
            Status_SubCode::kMergeOperatorFailed => SubCode::MergeOperatorFailed,
            _ => unreachable!(),
        };
        Some(subcode)
    }
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct Error {
    pub code: Code,
    pub subcode: Option<SubCode>,
    pub state: Option<String>,
}

impl From<&Status> for Error {
    fn from(value: &Status) -> Self {
        let state = value.getState();
        let state = if state.is_null() {
            None
        } else {
            // SAFETY: The message will always be null terminated and we checked that it's not null.
            Some(unsafe { CStr::from_ptr(state).to_string_lossy().to_string() })
        };
        Error {
            code: value.code().into(),
            subcode: value.subcode().into(),
            state,
        }
    }
}

impl From<&cxx::UniquePtr<Status>> for Error {
    fn from(value: &cxx::UniquePtr<Status>) -> Self {
        Error::from(value.as_ref().unwrap())
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let code = match self.code {
            Code::NotFound => "NotFound: ",
            Code::Corruption => "Corruption: ",
            Code::NotSupported => "Not implemented: ",
            Code::InvalidArgument => "Invalid argument: ",
            Code::IOError => "IO error: ",
            Code::MergeInProgress => "Merge in progress: ",
            Code::Incomplete => "Result incomplete: ",
            Code::ShutdownInProgress => "Shutdown in progress: ",
            Code::TimedOut => "Operation timed out: ",
            Code::Aborted => "Operation aborted: ",
            Code::Busy => "Resource busy: ",
            Code::Expired => "Operation expired: ",
            Code::TryAgain => "Operation failed. Try again.: ",
            Code::CompactionTooLarge => "Compaction too large: ",
            Code::ColumnFamilyDropped => "Column family dropped: ",
        };

        f.write_str(code)?;

        if let Some(subcode) = self.subcode {
            let msg = match subcode {
                SubCode::MutexTimeout => "Timeout Acquiring Mutex",
                SubCode::LockTimeout => "Timeout waiting to lock key",
                SubCode::LockLimit => "Failed to acquire lock due to max_num_locks limit",
                SubCode::NoSpace => "No space left on device",
                SubCode::Deadlock => "Deadlock",
                SubCode::StaleFile => "Stale file handle",
                SubCode::MemoryLimit => "Memory limit reached",
                SubCode::SpaceLimit => "Space limit reached",
                SubCode::PathNotFound => "No such file or directory",
                SubCode::MergeOperandsInsufficientCapacity => {
                    "Insufficient capacity for merge operands"
                }
                SubCode::ManualCompactionPaused => "Manual compaction paused",
                SubCode::Overwritten => " (overwritten)",
                SubCode::TxnNotPrepared => "Txn not prepared",
                SubCode::IOFenced => "IO fenced off",
                SubCode::MergeOperatorFailed => "Merge operator failed",
            };

            f.write_str(msg)?;
        }

        if let Some(state) = &self.state {
            f.write_str(state)?;
        }

        Ok(())
    }
}
