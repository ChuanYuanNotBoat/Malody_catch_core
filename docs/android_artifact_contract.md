# Android Artifact Contract

## File Name Contract

- Shared object name must remain:
  - `libmalody_catch_core_ffi.so`

## Directory Contract

- Mobile load path (arm64):
  - `android/app/src/main/jniLibs/arm64-v8a/`

## Traceability Metadata Contract

- Sync metadata file:
  - `libmalody_catch_core_ffi.sync.txt`
- Required fields:
  - `core_commit`
  - `ffi_abi_version`
  - `synced_at_utc`

## Integration Guardrails

- Do not change output filename without mobile script update.
- Do not change ABI number without coordinated mobile release plan.
- Run cross-repo preflight before release packaging.
