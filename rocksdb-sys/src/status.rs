use crate::ffi::{Code, Severity, Status, SubCode};
use std::ffi::{c_char, CStr};

impl Status {
    pub(crate) fn new_with_code_subcode_retryable_data_loss_scope(
        code: Code,
        subcode: SubCode,
        retryable: bool,
        data_loss: bool,
        scope: u8,
    ) -> Status {
        Status {
            code,
            subcode,
            sev: Severity::NoError,
            retryable,
            data_loss,
            scope,
            state: Vec::new(),
        }
    }

    pub(crate) fn new_with_code(code: Code) -> Status {
        Status::new_with_code_and_subcode(code, SubCode::None)
    }

    pub(crate) fn new_with_code_and_subcode(code: Code, subcode: SubCode) -> Status {
        Status {
            code,
            subcode,
            sev: Severity::NoError,
            retryable: false,
            data_loss: false,
            scope: 0,
            state: Vec::new(),
        }
    }

    pub(crate) fn permit_unchecked_error(&self) {
        // leaving as a no-op to denote which methods we can discard the result in once more is
        // moved to rust
    }

    pub(crate) fn must_check(&self) {
        // leaving as a no-op to denote which methods we need to check the result for once more is
        // moved to rust
    }

    pub(crate) fn code(&self) -> Code {
        self.code
    }

    pub(crate) fn subcode(&self) -> SubCode {
        self.subcode
    }

    pub(crate) fn severity(&self) -> Severity {
        self.sev
    }

    pub(crate) fn get_state(&self) -> *const c_char {
        self.state.as_ptr() as *const c_char
    }

    /// Returns true iff the status indicates success.
    pub(crate) fn ok(&self) -> bool {
        self.code == Code::Ok
    }

    /// Returns true iff the status indicates success *with* something overwritten
    pub(crate) fn is_ok_overwritten(&self) -> bool {
        self.code == Code::Ok && self.subcode == SubCode::Overwritten
    }

    /// Returns true iff the status indicates a NotFound error.
    pub(crate) fn is_not_found(&self) -> bool {
        self.code == Code::NotFound
    }

    /// Returns true iff the status indicates a Corruption error.
    pub(crate) fn is_corruption(&self) -> bool {
        self.code == Code::Corruption
    }

    /// Returns true iff the status indicates a NotSupported error.
    pub(crate) fn is_not_supported(&self) -> bool {
        self.code == Code::NotSupported
    }

    /// Returns true iff the status indicates a NotSupported error.
    pub(crate) fn is_invalid_argument(&self) -> bool {
        self.code == Code::InvalidArgument
    }

    /// Returns true iff the status indicates an IOError.
    pub(crate) fn is_io_error(&self) -> bool {
        self.code == Code::IOError
    }

    /// Returns true iff the status indicates an MergeInProgress.
    pub(crate) fn is_merge_in_progress(&self) -> bool {
        self.code == Code::MergeInProgress
    }

    /// Returns true iff the status indicates an Incomplete.
    pub(crate) fn is_incomplete(&self) -> bool {
        self.code == Code::Incomplete
    }

    /// Returns true iff the status indicates Shutdown in progress
    pub(crate) fn is_shutdown_in_progress(&self) -> bool {
        self.code == Code::ShutdownInProgress
    }

    pub(crate) fn is_timed_out(&self) -> bool {
        self.code == Code::TimedOut
    }

    pub(crate) fn is_aborted(&self) -> bool {
        self.code == Code::Aborted
    }

    /// Returns true iff the status indicates that a resource is Busy and temporarily could not be
    /// acquired.
    pub(crate) fn is_busy(&self) -> bool {
        self.code == Code::Busy
    }

    pub(crate) fn is_deadlock(&self) -> bool {
        self.code == Code::Busy && self.subcode == SubCode::Deadlock
    }

    /// Returns true iff the status indicated that the operation has Expired.
    pub(crate) fn is_expired(&self) -> bool {
        self.code == Code::Expired
    }

    /// Returns true iff the status indicates a TryAgain error. This usually means that the
    /// operation failed, but may succeed if re-attempted.
    pub(crate) fn is_try_again(&self) -> bool {
        self.code == Code::TryAgain
    }

    /// Returns true iff the status indicates the proposed compaction is too large
    pub(crate) fn is_compaction_too_large(&self) -> bool {
        self.code == Code::CompactionTooLarge
    }

    /// Returns true iff the status indicates Column Family Dropped
    pub(crate) fn is_column_family_dropped(&self) -> bool {
        self.code == Code::CompactionTooLarge
    }

    /// Returns true iff the status indicates a NoSpace error. This is caused by an I/O error
    /// returning the specific "out of space" error condition. Stricto sensu, an NoSpace error is an
    /// I/O error with a specific subcode, enabling users to take the appropriate action if needed
    pub(crate) fn is_no_space(&self) -> bool {
        self.code == Code::IOError && self.subcode == SubCode::NoSpace
    }

    /// Returns true iff the status indicates a memory limit error. There may be cases where we
    /// limit the memory used in certain operations (eg. the size of a write batch) in order to
    /// avoid out of memory exceptions.
    pub(crate) fn is_memory_limit(&self) -> bool {
        self.code == Code::Aborted && self.subcode == SubCode::MemoryLimit
    }

    /// Returns true iff the status indicates a PathNotFound error. This is caused by an I/O error
    /// returning the specific "no such file or directory" error condition. A PathNotFound error is
    /// an I/O error with a specific subcode, enabling users to take appropriate action if necessary
    pub(crate) fn is_path_not_found(&self) -> bool {
        (self.code == Code::IOError || self.code == Code::NotFound)
            && self.subcode == SubCode::PathNotFound
    }

    /// Returns true iff the status indicates manual compaction paused. This is caused by a call to
    /// PauseManualCompaction
    pub(crate) fn is_manual_compaction_paused(&self) -> bool {
        self.code == Code::Incomplete && self.subcode == SubCode::ManualCompactionPaused
    }

    /// Returns true iff the status indicates a TxnNotPrepared error.
    pub(crate) fn is_txn_not_prepared(&self) -> bool {
        self.code == Code::InvalidArgument && self.subcode == SubCode::TxnNotPrepared
    }

    // Returns true iff the status indicates a IOFenced error.
    pub(crate) fn is_io_fenced(&self) -> bool {
        self.code == Code::IOError && self.subcode == SubCode::IOFenced
    }

    pub(crate) fn to_string(&self) -> String {
        let mut result = match self.code {
            Code::Ok => "OK",
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
            _ => todo!(),
        }
        .to_string();

        result.push_str(match self.subcode {
            SubCode::None => "",
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
            _ => todo!(),
        });

        if !self.state.is_empty() {
            if self.subcode != SubCode::None {
                result.push_str(": ");
            }

            // The state contains the null terminator, strip it out before appending
            result.push_str(&*String::from_utf8_lossy(
                &self.state[..self.state.len() - 1],
            ));
        }

        result
    }

    /// # Safety
    ///
    /// This has the same safety constraints as [`CStr::from_ptr`]
    pub(crate) unsafe fn set_state_unsafe(&mut self, state_: *const c_char) {
        let state = if state_.is_null() {
            vec![0]
        } else {
            CStr::from_ptr(state_).to_bytes_with_nul().to_vec()
        };

        self.state = state;
    }
}

impl Default for Status {
    fn default() -> Self {
        Status {
            code: Code::Ok,
            subcode: SubCode::None,
            sev: Severity::NoError,
            retryable: false,
            data_loss: false,
            scope: 0,
            state: Vec::new(),
        }
    }
}

pub(crate) fn new_status_with_code_subcode_retryable_data_loss_scope(
    code: Code,
    subcode: SubCode,
    retryable: bool,
    data_loss: bool,
    scope: u8,
) -> Status {
    Status::new_with_code_subcode_retryable_data_loss_scope(
        code, subcode, retryable, data_loss, scope,
    )
}

pub(crate) fn default_status() -> Status {
    Status::default()
}

pub(crate) fn new_status_with_code(code: Code) -> Status {
    Status::new_with_code(code)
}

pub(crate) fn new_status_with_code_and_subcode(code: Code, subcode: SubCode) -> Status {
    Status::new_with_code_and_subcode(code, subcode)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn  status_to_string() {
        let mut status = Status::new_with_code(Code::TryAgain);
        unsafe { status.set_state_unsafe(b"Oops I did it again\0".as_ptr().cast())}
        assert_eq!(status.to_string(), "Operation failed. Try again.: Oops I did it again");
    }
}