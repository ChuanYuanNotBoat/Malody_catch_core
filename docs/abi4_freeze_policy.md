# ABI4 Freeze Policy

## Scope

- Applies to exported C ABI symbols `mce_*`
- Current ABI: `mce_ffi_abi_version = 4`

## Breaking Change Definition

Any of the following is breaking and requires ABI bump + mobile upgrade path:

- Remove or rename exported `mce_*` symbol
- Change function signature/argument meaning
- Change struct field order/type/size consumed by FFI
- Change success/failure contract without migration note

## Review Checklist

- Symbol diff report checked (`pure_core_ffi_symbol_guard`)
- `tests/ffi_symbols_abi4.txt` compatibility reviewed
- FFI tests pass, including negative cases
- Mobile sync path impact reviewed

## Exception Process

- Document reason and blast radius in PR
- Require explicit maintainers approval
- Add mobile-side compatibility plan and rollback path
