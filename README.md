# Malody Catch Core

Shared chart-editing core for Malody Catch tooling.

This repository was split from `Malody_catch_editor` with history preserved by
`git-filter-repo`. It is currently in the extraction stage: the build is kept
working with QtCore/QtGui while the code is migrated toward a pure C++ core and
a C ABI layer for Flutter `dart:ffi`.

## Repository Role

- Own chart data structures, edit commands, undo/redo, timing math, `.mc`
  parsing, and `.mcz` import/export.
- Exclude desktop Qt Widgets UI, rendering, audio playback, and plugin UI.
- Serve both the existing Qt desktop editor and the new Flutter Android app.

## Current Layout

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
