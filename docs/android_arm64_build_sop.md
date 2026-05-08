# Android arm64 Build SOP

## Prerequisites

- Android NDK installed (`27.0.12077973` default in sync script)
- CMake available in PATH
- PowerShell available

## Build and Sync via mobile helper

From `Malody_catch_mobile`:

```powershell
./tools/sync_core_android_so.ps1
```

This will:

- configure and build core with Android toolchain
- produce `libmalody_catch_core_ffi.so`
- copy to mobile `jniLibs/arm64-v8a`
- emit sync metadata `.sync.txt`

## Expected Outputs

- Core build output:
  - `<core>/build_android_arm64-v8a/libmalody_catch_core_ffi.so`
- Mobile runtime output:
  - `<mobile>/android/app/src/main/jniLibs/arm64-v8a/libmalody_catch_core_ffi.so`
- Sync metadata:
  - `<mobile>/android/app/src/main/jniLibs/arm64-v8a/libmalody_catch_core_ffi.sync.txt`
