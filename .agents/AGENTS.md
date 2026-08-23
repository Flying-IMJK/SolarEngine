# SolarEngine 代理规范入口

执行本仓库任务前，必须阅读：

- [`项目规范`](../docs/项目规范.md)
- [`代码规范`](../docs/代码规范.md)

## Graphify 关系图

项目关系图输出位于 [`graphify-out`](../graphify-out/)，其中：

- [`graph.html`](../graphify-out/graph.html) 是按社区聚合的交互图；
- [`GRAPH_REPORT.md`](../graphify-out/GRAPH_REPORT.md) 是节点枢纽、异常连接和建议查询的审计报告；
- [`graph.json`](../graphify-out/graph.json) 是供 Graphify 查询与自动化使用的原始图数据。

涉及项目架构、模块依赖、调用关系、代码归属或跨模块影响分析时，优先查询现有 `graphify-out/graph.json`。仅在用户明确要求重建或增量更新时重新执行 Graphify；当前图的扫描范围不包含 `Src/Libraries`。

下列规则是所有代理必须直接遵守的约束；完整背景、构建命令和例外说明以链接文档为准。

## 范围与模块边界

- 规范适用于 `Src/Engine`、`Src/BuildTool`、`Src/Applications`。
- `Src/Libraries` 是第三方代码。不要格式化、重命名、重构或按一方规范修改它；第三方升级/补丁须保持上游格式。
- 依赖方向为 `Applications → Editor → Runtime`。Runtime 不得依赖 Editor 或 Applications；Editor 不得依赖 Applications；BuildTool 不得依赖 Runtime/Editor 实现。
- 不将 `Src/Engine/_Ignore` 作为新的生产依赖。

## 原生、托管与生成代码

- 渲染、平台、内存、反射核心、原生对象生命周期和性能关键路径由 C++ 实现。
- C# 可实现托管 Runtime 表面、Editor GUI、编辑器工作流和托管脚本/工具逻辑；不得复制或绕过 C++ 底层实现。
- C# 调用原生对象只能经 `NativeInterop`、`Object.GetUnmanagedPtr` 或等价受控 API；不得直接依赖内部原生指针字段或对象布局。
- `Src/Engine/**/_Generated` 是 BuildTool/Reflector 输出，严禁手改。应修改反射输入、生成模板或生成器后重新生成。
- 反射、ABI、绑定、Box/Unbox、封送、枚举值或原生对象指针访问方式的变更必须同时更新两端并重新生成、构建 Runtime 与 Editor。

## 格式与代码规则

- `.clang-format` 和 `.editorconfig` 是格式与命名的唯一可执行来源。仅格式化本次修改的一方文件，不执行全仓格式化。
- 修改代码文件时统一使用 CRLF 行尾；不要引入 LF 行尾混用。
- C++ 基线为 C++17、4 空格、120 列、左结合指针及仓库定义的 Allman 风格；保持现有模块的 include 分层。
- C# 使用大括号；遵循 `.editorconfig` 的 `PascalCase`、`m_`、`s_`、`camelCase` 规则，并保持 nullable 与 NativeInterop 边界明确。
- 新增 C++/C# 源文件通过所属模块的 CMake 聚合机制登记；不要创建绕过 Runtime/Editor 模块结构的临时 target。
- 不做无关的批量重命名、顺带重构或生成文件修补。

## 验证

- C++、CMake、反射或跨语言变更按风险完成相应 CMake 构建、生成和 Runtime/Editor 托管构建。
- 文档或格式配置变更至少检查链接、格式和变更范围。
- 若构建被既有基线错误阻断，必须明确记录，不能声称本次改动已完整通过验证。
- 本轮规范不新增 CI、测试框架或新的格式化工具。
