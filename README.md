# Malody Catch Core

Shared chart-editing core for Malody Catch tooling.

This repository was split from `Malody_catch_editor` with history preserved by
`git-filter-repo`. It is currently in the extraction stage: the build is kept
working with QtCore/QtGui while the code is migrated toward a pure C++ core and
a C ABI layer for Flutter `dart:ffi`.

## Desktop Sync Baseline

- Synced desktop editor release target: `desktop main` (`2026-05-09`)
- Source commit from sibling repo `Malody_catch_editor`: `f3088da`
- This repo remains versioned independently (`mce_core_version` / `mce_ffi_abi_version`),
  while keeping compatibility tracking with desktop release updates.

## Mobile App-Layer Scope Note

- Current `.mcz` import/export workflow is implemented in
  `Malody_catch_mobile` application layer.
- Current mobile audio playback orchestration (play/pause/seek/rate and
  playhead UI sync) is implemented in `Malody_catch_mobile` application layer.
- Current mobile desktop-aligned editor interaction semantics (mode switching,
  time-division snap, grid snap/division, selection nudge/copy-paste flow) are
  implemented in `Malody_catch_mobile` application layer.
- Core C ABI remains unchanged at `mce_ffi_abi_version = 4` in this milestone.
- No `mce_*` symbol additions or signature changes were introduced.

## Desktop Parity Contract (Feature / Semantics)

- Reference desktop baseline:
  `Malody_catch_editor@f3088da` (`desktop main`, `2026-05-09`, sync scope: `2f60ae6..f3088da`)
- Core parity responsibility:
  keep editing primitives and state semantics aligned with desktop behavior
  (add/move/remove/batch, undo/redo, snapshot/revision, bpm/meta updates).
- Non-core parity responsibility:
  desktop GUI layout, panel interactions, audio runtime, and plugin workflows
  are explicitly out of scope for this repository.
- Mobile GUI/gesture parity is tracked in `Malody_catch_mobile`; this repo
  only guarantees a stable behavior contract through the C ABI.
- Ongoing parity governance tasks are tracked in `TODO.md`
  (`COR-M1-008/009/010/011`).

## Repository Role

- Own chart data structures, edit commands, undo/redo, timing math, `.mc`
  parsing, and `.mcz` import/export.
- Exclude desktop Qt Widgets UI, rendering, audio playback, and plugin UI.
- Serve both the existing Qt desktop editor and the new Flutter Android app.

## Current Layout

- `include/mce`: new pure C++ public core API.
- `src/core`: new pure C++ implementation.
- `src/model`: chart, note, BPM, and metadata types.
- `src/controller`: current editing controller, to be replaced by
  `mce::EditorSession`.
- `src/file`: `.mc` and `.mcz` IO.
- `src/utils`: timing math, diagnostics, and transitional logging helpers.
- `tests`: minimal regression tests carried over from the desktop editor.

## Build

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

FFI symbol guard report is emitted to:

- `build/ffi_symbol_report.txt` (or your chosen build directory)
- CI/local helper:
  `tools/run_ffi_symbol_guard.ps1`
- CI usage note:
  `docs/ffi_symbol_guard_ci.md`

## FFI Symbol Guard

- FFI export baseline file: `tests/ffi_symbols_abi4.txt`
- Guard script: `tools/check_ffi_symbols.ps1`
- Included in CTest as `pure_core_ffi_symbol_guard` when PowerShell is available.
- ABI freeze policy:
  `docs/abi4_freeze_policy.md`
- Error code contract:
  `docs/ffi_error_code_contract.md`
- Android arm64 build SOP:
  `docs/android_arm64_build_sop.md`
- Android artifact contract:
  `docs/android_artifact_contract.md`
- Desktop semantics mapping:
  `docs/desktop_core_semantics_mapping.md`
- Mobile alignment samples:
  `docs/mobile_alignment_samples.md`
- Manual run example:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\check_ffi_symbols.ps1 `
  -BinaryPath .\build\Release\malody_catch_core_ffi.dll `
  -BaselinePath .\tests\ffi_symbols_abi4.txt
```

## Migration Targets

- Replace Qt containers and strings with standard C++ types.
- Replace Qt JSON/file/process helpers with portable core dependencies.
- Replace `QUndoStack` with a core-owned command stack.
- Add stable C ABI functions for mobile FFI.

## Current Migration State

The repository now has two build tracks:

- `malody_catch_core_pure`: pure C++ model foundation under `include/mce` and
  `src/core`, including the initial `mce::EditorSession` edit/undo facade.
- `malody_catch_core_ffi`: shared-library C ABI facade for mobile FFI. The
  first surface covers session lifetime, normal-note edits, snapshots, errors,
  and undo/redo.
- `MalodyCatchCore`: transitional Qt-backed implementation kept alive so
  behavior can be migrated incrementally without breaking existing tests.


