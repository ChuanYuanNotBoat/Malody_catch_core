# Malody Catch Core

Shared chart-editing core for Malody Catch tooling.

This repository was split from `Malody_catch_editor` with history preserved by
`git-filter-repo`. It is currently in the extraction stage: the build is kept
working with QtCore/QtGui while the code is migrated toward a pure C++ core and
a C ABI layer for Flutter `dart:ffi`.

## Desktop Sync Baseline

- Synced desktop editor release target: `desktop main` (`2026-05-05`)
- Source commit from sibling repo `Malody_catch_editor`: `2f60ae6`
- This repo remains versioned independently (`mce_core_version` / `mce_ffi_abi_version`),
  while keeping compatibility tracking with desktop release updates.

## Mobile App-Layer Scope Note

- Current `.mcz` import/export workflow is implemented in
  `Malody_catch_mobile` application layer.
- Current mobile audio playback orchestration (play/pause/seek/rate and
  playhead UI sync) is implemented in `Malody_catch_mobile` application layer.
- Core C ABI remains unchanged at `mce_ffi_abi_version = 4` in this milestone.
- No `mce_*` symbol additions or signature changes were introduced.

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
