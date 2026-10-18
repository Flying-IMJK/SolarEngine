# SolarEngine 代理规范入口

## 工作流程

- 处理请求先进行只读分析并提出具体实施方案，等待用户审核和授权后实施。
- 代码编译前另行取得用户授权；未获授权的生成、编译或运行验证明确标记为未执行。

## 按任务读取规则

- 涉及架构、模块依赖或项目验证时读取[项目规范](docs/项目规范.md)。
- 修改 C++ 时直接读取 `.clang-format` 和 `.clang-tidy`；修改 C# 时直接读取 `.editorconfig`。涉及语义、互操作或规则例外时读取[代码规范](docs/代码规范.md)，不另写一套格式与命名规则。

## Graphify 关系图

项目关系图输出位于 [`graphify-out`](graphify-out/)，其中：

- [`graph.html`](graphify-out/graph.html) 是按社区聚合的交互图；
- [`GRAPH_REPORT.md`](graphify-out/GRAPH_REPORT.md) 是节点枢纽、异常连接和建议查询的审计报告；
- [`graph.json`](graphify-out/graph.json) 是供 Graphify 查询与自动化使用的原始图数据。

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
- 反射、ABI、绑定、Box/Unbox、封送、枚举值或原生对象指针访问方式的变更应同时覆盖两端；获得相应授权后重新生成并构建 Runtime 与 Editor。

## 代码修改

- 功能实现覆盖完整预期流程，减少游离函数；按任务需要放宽安全边界处理。
- 代码实现保持中等频率的注释，说明非直观逻辑。
- 仅格式化本次实际修改的一方文件；代码文件使用 CRLF 行尾，不引入 LF 混用。
- 新增 C++/C# 源文件通过所属模块的 CMake 聚合机制登记，不绕过 Runtime/Editor 模块结构。
- 修改范围限于当前任务，保留无关的未提交改动。

## 验证

- 根据变更风险提出生成、静态检查、构建和运行验证；取得各步骤所需授权后执行并分别报告结果。
- 文档或格式配置变更检查链接、配置解析和变更范围。
- 构建若被既有基线错误阻断，明确记录原因，不声称本次改动已通过完整验证。
