# Malody Catch Core TODO

更新日期：2026-04-29

## 近期目标

把当前双轨 core 推进到可被 Flutter Android 端实际集成：

- `malody_catch_core_pure` 承载纯 C++ 业务逻辑。
- `malody_catch_core_ffi` 暴露稳定 C ABI。
- 旧 Qt 过渡层只作为行为对照，逐步退场。

## P0 - Core 数据与编辑闭环

- [x] 建立纯 C++ `mce::Note` / `mce::Chart` / `mce::MetaData` / `mce::BpmEntry`。
- [x] 建立纯 C++ `mce::EditorSession`。
- [x] 建立最小 C ABI：session、普通音符 add/remove、snapshot、undo/redo。
- [x] 增加 rain note FFI：添加、移动、校验、snapshot。
- [x] 增加 sound note FFI：添加、校验、snapshot。
- [ ] 增加批量编辑 API：一次提交 add/remove/move，作为单个 undo step。
- [ ] 增加 BPM 编辑 API：add/update/remove BPM。
- [ ] 增加 metadata 读写 API。
- [ ] 将 `EditorSession` 的错误模型从单字符串扩展为稳定错误码 + message。
- [ ] 明确 note id 生成策略：空 id 由 core 生成；外部传入 id 时保持不变。

## P1 - 文件 IO 纯 C++ 化

- [ ] 选定并集成 JSON 库，默认使用 `nlohmann/json`。
- [ ] 新建纯 C++ `.mc` parser/writer，不依赖 Qt JSON。
- [ ] 覆盖现有 `ChartIO` 行为：BPM、normal、rain、sound、meta、兼容字段。
- [ ] 添加 `.mc` round-trip 测试：读取后保存再读取，核心字段一致。
- [ ] 选定并集成 zip 库，默认使用 `miniz`。
- [ ] 新建纯 C++ `.mcz` extract/export，不依赖 `QProcess` 或系统 `zip/unzip`。
- [ ] 覆盖 Malody 兼容导出结构：顶层 `0/`。
- [ ] 添加路径安全校验，拒绝 zip slip 和绝对路径。

## P2 - 时间与编辑规则迁移

- [ ] 从旧 `MathUtils` 迁移 beat/ms 换算到纯 C++ `mce::TimeMapper`。
- [ ] 增加分数拍号规范化工具，避免移动后分母异常漂移。
- [ ] 增加 grid snap API：时间吸附、x 边界吸附。
- [ ] 增加多选整体移动 API，保持相对时间差。
- [ ] 增加复制/粘贴 API，支持粘贴偏移和单步 undo。
- [ ] 增加 rain end beat 合法性保护。
- [ ] 增加 Hyperfruit 判定纯 C++ 版本，供移动端绘制红框。

## P3 - FFI 稳定化

- [ ] 为 FFI 增加版本查询：`mce_core_version`、`mce_ffi_abi_version`。
- [ ] 为 snapshot 增加 chart revision，移动端可判断缓存是否失效。
- [ ] 增加 note snapshot 批量读取，避免 Flutter 逐条 FFI 调用过多。
- [ ] 增加 chart summary snapshot：note count、BPM count、meta summary。
- [ ] 增加字符串内存策略文档：固定缓冲区优先，动态字符串必须配套 free。
- [ ] 增加 Android ABI 构建产物命名规范：`libmalody_catch_core_ffi.so`。
- [ ] 增加导出符号检查脚本或测试。

## P4 - 构建与仓库治理

- [ ] 将 Qt 过渡 target 与纯 C++ target 分离为 CMake option。
- [ ] 默认构建纯 C++ target，不要求 Qt。
- [ ] 增加 Android NDK toolchain 构建说明。
- [ ] 增加 CI：Windows desktop transitional test、pure C++ test、Android cross-build。
- [ ] 增加 `third_party` 依赖记录和许可证说明。
- [ ] 增加格式化规范：clang-format 或明确不自动格式化。
- [ ] 整理旧 Qt 过渡层删除计划，逐个替换 `src/model`、`src/file`、`src/controller`。

## 验收清单

- [ ] 不设置 Qt 路径时，纯 C++ core 可以配置、构建、测试。
- [ ] Android arm64-v8a 可以产出 FFI `.so`。
- [ ] Flutter 端可以 create session、添加音符、读取 snapshot、undo/redo。
- [ ] `.mc/.mcz` 行为与桌面端现有导入导出兼容。
- [ ] 所有 FFI API 对空指针和非法输入稳定返回错误，不崩溃。
