# FFI Error Code Contract

## Goal

Provide stable code-level semantics so mobile can render deterministic UX.

## Core Rules

- Every failed FFI call must set a stable error code.
- `mce_session_last_error_code()` is the source of truth for failure class.
- `mce_session_last_error()` is human-readable and non-contractual text.

## Code Classes

- `MCE_ERROR_NONE`
  - Success path or no active error.
- `MCE_ERROR_INVALID_SESSION`
  - Null/dangling session usage.
  - Mobile action: force close current edit context and ask reload.
- `MCE_ERROR_INVALID_ARGUMENT`
  - Null pointer, out-of-range index, invalid beat/params.
  - Mobile action: keep session alive, show recoverable prompt.
- `MCE_ERROR_OPERATION_FAILED`
  - Operation rejected by rule/state conflict.
  - Mobile action: keep session alive, show operation-specific hint.
- `MCE_ERROR_IO`
  - IO interaction failure surfaced through core APIs (when applicable).
  - Mobile action: show retry/select-another-path guidance.

## Recovery Classes

- Recoverable:
  - `INVALID_ARGUMENT`, `OPERATION_FAILED`, most `IO`
- Non-recoverable in current session:
  - `INVALID_SESSION`

## Validation Gate

- Negative tests in `tests/ffi_tests.cpp` must assert both:
  - return value contract
  - error code stability
