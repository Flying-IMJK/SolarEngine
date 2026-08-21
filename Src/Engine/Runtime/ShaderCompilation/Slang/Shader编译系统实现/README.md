# SolarEngine Slang Shader 编译系统实施指南

本目录同时保留两套文档：

- HTML：审查版，用于浏览、跳转和讨论。
- Markdown：实施版，用于编码时快速读取，内容更精练。

系统设计契约仍以 `../Shader编译系统设计/` 为准。本实施版只说明模块拆分、调用顺序、边界、不变量和验收。

## 阅读顺序

1. [依赖与工程.md](./依赖与工程.md)
2. [编译器实现.md](./编译器实现.md)
3. [反射与SLC2.md](./反射与SLC2.md)
4. [运行时与Vulkan.md](./运行时与Vulkan.md)
5. [测试与交付.md](./测试与交付.md)

## 总边界

编译部分必须独立运行、独立测试、独立验收。运行时只消费编译产物，不能成为编译功能可用的前置条件。

```text
Compile-only:
  Slang Source
    -> Program/Variant/Target plan
    -> linked component
    -> target code + ShaderReflectionIR
    -> SLC2 JSON
    -> SLC2Reader + Validator
    -> semantic manifest + byte determinism

Runtime/GPU:
  SLC2 JSON
    -> Program/Target/Variant selection
    -> runtime reflection
    -> name binding
    -> Vulkan layout/descriptors
    -> Draw/Dispatch
```

## 编译侧负责

- `ShaderCompileRequest` 校验和规范化。
- Slang global session / request session。
- Program 发现、Stage 校验、EntryPoint 检查。
- Variant 规划、默认补全、规范化和去重。
- Target 策略、`TargetKey`、目标代码生成。
- `SlangReflectionBuilder` 生成 `ShaderReflectionIR`。
- `ShaderReflectionValidator` 强校验。
- `SLC2Writer` 写 UTF-8 JSON。
- 通过共享无后端 `SLC2Reader` self-read。
- 返回 `ShaderCompileResult { Status, CompileMessage, SLC2Data }`。

编译侧不依赖：

- `ShaderProgramInstance`
- GPU 设备
- Vulkan PipelineLayout / DescriptorSet
- 运行时资源对象
- Draw / Dispatch

## 运行时侧负责

- 严格读取 SLC2。
- 选择 Program / Target / Variant。
- 从 IR 重建 `ShaderProgramReflection`。
- 解析 `a.b[2].c` 名称路径。
- 保存 uniform bytes 和逻辑资源绑定状态。
- 提交前验证所有资源槽已绑定非空且类型兼容。
- 从物理 descriptor 记录创建 Vulkan Pipeline Layout。
- 写 descriptor 并提交 Draw/Dispatch。

运行时不得：

- 调用 Slang。
- 从 SPIR-V 二次反射。
- 修复或猜测不完整 SLC2。
- 参与 Program 发现、Variant 规划、Target codegen 或 SLC2 写入。

## 共享无后端层

这些类型和算法可被编译器和运行时共同使用，但不能依赖 Slang 或 Vulkan：

- `ShaderReflectionTypes`
- `ShaderReflectionIR`
- `ShaderReflectionValidator`
- `SLC2Reader`
- `Variant Normalize`
- `PipelineLayoutFingerprint`

## 阶段划分

```text
P0  Slang 依赖与骨架
P1  最小 Compute 编译
P2  Program / Variant / Target
P3  ReflectionIR + SLC2
P4  Runtime Reflection + Name Binding
P5  Vulkan Compute
P6  Vulkan Graphics
P7  完整已设计能力
P8  调用点迁移与清理
```

P0-P3 是编译独立闭环，不创建 GPU 设备、不创建 `ShaderProgramInstance`。P4 从已通过 self-read 的 SLC2 开始。P5 之后才接入 Vulkan。

## 非目标

当前实施不处理：

- 旧 HLSL 资产兼容或迁移。
- Legacy shader 自动 fallback。
- Cook/发布流程。
- `DependencyIndex` 和精确增量编译。
- 默认资源库和 fallback。
- Runtime lifecycle 优化。
- Push Constant。
- 无界 descriptor。
- 运行时 interface specialization。
