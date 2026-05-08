# Malody Catch Core TODO（代码真相版 / 细化执行）

更新日期：2026-05-07
适用仓库：`Malody_catch_core`
协作约束：与 `Malody_catch_mobile` 协同推进，不修改桌面仓库。

## 状态语义

- `[x]` 完成
- `[~]` 进行中
- `[ ]` 待做

## M1 支撑 mobile 发布闭环（进行中）

| ID | 优先级 | 状态 | 任务 | 备注 |
| --- | --- | --- | --- | --- |
| COR-M1-001 | P0 | [x] | ABI4 冻结规则 | 已补 `docs/abi4_freeze_policy.md` |
| COR-M1-002 | P0 | [x] | 导出符号变更门禁 | 已补 `tools/check_ffi_symbols.ps1` + CTest 门禁 |
| COR-M1-003 | P0 | [x] | 错误码语义与映射文档 | 已补 `docs/ffi_error_code_contract.md` |
| COR-M1-004 | P0 | [x] | `arm64-v8a` 构建流程 | 已补 `docs/android_arm64_build_sop.md` |
| COR-M1-005 | P0 | [x] | 产物命名和目录规范 | 已补 `docs/android_artifact_contract.md` |
| COR-M1-006 | P0 | [x] | FFI 负向测试 | 已补空会话/空指针/非法参数等测试 |
| COR-M1-007 | P0 | [x] | mobile 对齐回归样本 | 已补 `docs/mobile_alignment_samples.md` |
| COR-M1-008 | P0 | [x] | 桌面行为 -> core 语义映射 | 已补 `docs/desktop_core_semantics_mapping.md` |
| COR-M1-009 | P1 | [x] | 批处理失败一致性测试 | 已验证失败原子性与 revision 不污染 |
| COR-M1-010 | P1 | [x] | 桌面对齐协同样本输入集 | 已落到协同样本文档 |

### M1 退出检查

- [~] `.so` 构建和同步流程可重复执行且可追溯（流程已固化，真机多轮验证待补）。
- [x] 符号与 ABI 门禁可自动发现破坏性变更。
- [x] FFI 负向场景测试通过且错误码可解释。

## M2/M3（待做）

- [ ] `.mc/.mcz` pure C++ 主流程迁移。
- [ ] 路径安全层与 zip slip 规则内建。
- [ ] 无 Qt 场景默认构建与 CI 最小矩阵收敛。

## 最小协同验收清单（执行面）

- [ ] `mce_ffi_abi_version` 与 mobile 依赖一致。
- [ ] `arm64-v8a` `.so` 可按文档从干净环境构建。
- [x] FFI 负向场景（空指针/越界/非法输入）不崩溃且错误码稳定。
- [ ] mobile 关键行为样本（add/move/batch/undo/redo）对齐通过。
- [x] 导出符号与结构布局变更有可追踪记录。

## 责任边界

- core 负责：编辑规则、模型、撤销重做、FFI 稳定契约。
- mobile 负责：文件入口、`.mcz` 工作流、音频编排、UI/交互、发布工程化。
