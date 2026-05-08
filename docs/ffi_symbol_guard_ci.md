# FFI Symbol Guard (CI)

## Goal

Detect accidental `mce_*` exported symbol changes before release.

## Run locally

```powershell
./tools/run_ffi_symbol_guard.ps1 -BuildDir build_codex -Config Debug
```

Generate a copy for CI artifact collection:

```powershell
./tools/run_ffi_symbol_guard.ps1 -BuildDir build_codex -Config Debug -ArtifactDir artifacts
```

## Output

- Test gate: `pure_core_ffi_symbol_guard`
- Report file: `<build_dir>/ffi_symbol_report.txt`
- Optional copied artifact: `<artifact_dir>/ffi_symbol_report.txt`
