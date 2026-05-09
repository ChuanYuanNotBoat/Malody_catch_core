# Desktop Behavior to Core Semantics Mapping

Reference baseline: `Malody_catch_editor@f3088da`

## Editing Operations

- Add note/rain/sound:
  - Success: returns `1`, revision increments
  - Failure: returns `0`, stable error code set
- Move/remove by id:
  - Success: target updated/removed, revision increments
  - Failure: no state mutation, stable error code set
- Batch apply:
  - Success: all ops applied atomically, single undo step
  - Failure: no partial mutation (atomic reject), revision unchanged

## History Operations

- Undo:
  - Success: previous snapshot restored, revision increments
  - Failure: no mutation, recoverable error
- Redo:
  - Success: next snapshot restored, revision increments
  - Failure: no mutation, recoverable error

## Snapshot and Revision

- `mce_session_chart_revision()` changes only on successful state mutation.
- Snapshot APIs must reflect latest committed state.

