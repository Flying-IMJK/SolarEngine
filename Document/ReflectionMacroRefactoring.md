# SolarEngine 反射宏系统重构规划

## Context

SolarEngine 当前的反射宏系统存在两套并行的宏体系：`SE_***` 系列用于引擎反射，`API_***` 系列用于 C# 绑定代码生成。两者功能有大量重叠（如 `SE_ENUM` + `API_ENUM`），且命名风格不统一。本次重构将合并两套宏为统一的 `SE_***` 格式，明确区分"代码注入宏"（`DEFINE_***`）和"标注宏"（`SE_***`），通过参数（`Reflect`/`API`）区分功能。

---

## 1. 宏重命名映射

### 1.1 代码注入宏（编译时展开为 C++ 代码）

| 旧宏 | 新宏 | 说明 |
|------|------|------|
| `SE_CLASS(TypeName, BaseTypeName)` | `DEFINE_CLASS(TypeName, BaseTypeName)` | 注入 friend、s_pTypeInfo、GetTypeInfo()、GetType() |
| `SE_CLASS_DEFAULT(TypeName, BaseTypeName)` | `DEFINE_CLASS_DEFAULT(TypeName, BaseTypeName)` | 同上 + 默认构造函数 |
| `ENGINE_REFLECT_MODULE` | `ENGINE_REFLECT_MODULE` | 不变 |

### 1.2 统一标注宏（编译时为空宏，Reflector 读取参数生成代码）

| 旧宏 | 新宏 | 参数说明 |
|------|------|---------|
| `API_CLASS(...)` | `SE_CLASS(...)` | 可选参数：`Reflect`、`API(InBuild, NoSpawn, Abstract, Attributes=...)` |
| `API_STRUCT(...)` | `SE_STRUCT(...)` | 可选参数：`Reflect`、`API(Attributes=...)` |
| `API_INTERFACE(...)` | `SE_INTERFACE(...)` | 可选参数：`API(Attributes=...)` |
| `SE_ENUM(TypeName)` + `API_ENUM(...)` | `SE_ENUM(...)` | 可选参数：`Reflect`、`API(...)` |
| `SE_PROPERTY(...)` + `API_FIELD(...)` + `API_PROPERTY(...)` | `SE_PROPERTY(...)` | 可选参数：`Reflect`、`API(...)`、JSON 元数据 |
| `SE_FUNCTION`（未定义）+ `API_FUNCTION(...)` | `SE_FUNCTION(...)` | 可选参数：`Reflect`、`API(Static)` |
| `API_EVENT`（未定义） | `SE_EVENT(...)` | 可选参数：`API(...)` |
| `SE_META()` | `SE_META()` | 不变 |

### 1.3 参数语义

- `Reflect` — 生成反射信息代码（TTypeCompositeInfo / TTypeEnumInfo / 属性注册）
- `API(...)` — 生成 C# 绑定代码；绑定参数只在该分组内生效
- 其他参数（如 `InBuild`、`ReadOnly`、`Category="..."`、`Attributes="..."`）— 保持原有语义

---

## 2. 使用示例

### 类（最常见情况：仅反射）

```cpp
// 旧
class MyClass : public BaseClass {
    SE_CLASS(MyClass, BaseClass)
    // ...
};

// 新
class MyClass : public BaseClass {
    DEFINE_CLASS(MyClass, BaseClass)
    // ...
};
```

### 类（反射 + 绑定）

```cpp
// 旧
SE_CLASS(ScriptingObject, Object)
API_CLASS(InBuild)
class ScriptingObject : public Object { ... };

// 新
DEFINE_CLASS(ScriptingObject, Object)
SE_CLASS(API(InBuild))
class ScriptingObject : public Object { ... };
```

### 类（仅绑定）

```cpp
// 旧
API_CLASS(InBuild) class ScriptingObject : public Object { ... };

// 新
SE_CLASS(API(InBuild)) class ScriptingObject : public Object { ... };
```

### 枚举

```cpp
// 旧 — 仅反射
SE_ENUM(MyEnum)
enum class MyEnum : uint8 { A = 0, B = 1 };

// 新 — 仅反射
SE_ENUM(Reflect)
enum class MyEnum : uint8 { A = 0, B = 1 };

// 旧 — 反射 + 绑定
SE_ENUM(MyEnum)
API_ENUM()
enum class MyEnum : uint8 { A = 0, B = 1 };

// 新 — 反射 + 绑定
SE_ENUM(Reflect, API())
enum class MyEnum : uint8 { A = 0, B = 1 };
```

### 属性

```cpp
// 旧 — 仅反射
SE_PROPERTY(Category="MyCategory")
int32 myProp;

// 新 — 仅反射
SE_PROPERTY(Reflect, Category="MyCategory")
int32 myProp;

// 旧 — 反射 + 绑定
SE_PROPERTY(Category="MyCategory")
API_FIELD()
int32 myProp;

// 新 — 反射 + 绑定
SE_PROPERTY(Reflect, Category="MyCategory", API())
int32 myProp;

// 旧 — 仅绑定
API_FIELD(ReadOnly)
int32 myProp;

// 新 — 仅绑定
SE_PROPERTY(API(ReadOnly))
int32 myProp;
```

### 函数

```cpp
// 旧 — 仅绑定
API_FUNCTION()
void DoSomething(int32 value);

// 新 — 仅绑定
SE_FUNCTION(API())
void DoSomething(int32 value);
```

---

## 3. 实施步骤

### Phase 1: 添加新宏定义（不破坏现有代码）

**文件:** `Src/Engine/Core/TypeSystem/IType.h`

1. 在现有 `SE_CLASS` / `SE_CLASS_DEFAULT` 下方，添加 `DEFINE_CLASS` / `DEFINE_CLASS_DEFAULT`（内容与旧宏完全相同）
2. 定义 `SE_FUNCTION(...)` 和 `SE_EVENT(...)` 为空宏
3. 此时编译不受影响，新旧宏共存

### Phase 2: 原子性宏重命名 + 全量迁移（核心变更）

此步骤必须在一个提交中完成，因为 `SE_CLASS` 和 `SE_ENUM` 的语义会改变。

**2.1 修改 IType.h 宏定义:**
- 将 `SE_CLASS(TypeName, BaseTypeName)` 改名为 `DEFINE_CLASS(TypeName, BaseTypeName)`
- 将 `SE_CLASS_DEFAULT(TypeName, BaseTypeName)` 改名为 `DEFINE_CLASS_DEFAULT(TypeName, BaseTypeName)`
- 重新定义 `SE_CLASS(...)` 为空标注宏（替代旧 API_CLASS）
- 重新定义 `SE_ENUM(...)` 为空可变参数标注宏（替代旧 SE_ENUM + API_ENUM）
- `SE_PROPERTY(...)` 已是空宏，语义扩展（增加 Reflect/API 参数解析）
- 定义 `SE_STRUCT(...)` / `SE_INTERFACE(...)` 空宏
- 删除底部 `API_ENUM` ~ `API_FIELD` 定义（IType.h 221-227 行）

**2.2 修改 ScriptingAPI.h:**
- 删除旧 `API_CLASS` / `API_STRUCT` / `API_FUNCTION` / `API_PROPERTY` / `API_FIELD` / `API_ENUM` 定义
- 更新文档注释为新的 SE_ 宏用法

**2.3 批量替换引擎头文件中的宏使用:**

| 替换规则 | 预估数量 |
|---------|---------|
| `SE_CLASS(` → `DEFINE_CLASS(` | ~72 处 |
| `SE_CLASS_DEFAULT(` → `DEFINE_CLASS_DEFAULT(` | ~78 处 |
| `SE_ENUM(TypeName)` → `SE_ENUM(Reflect)` | ~30 处 |
| `SE_PROPERTY(...)` → `SE_PROPERTY(Reflect, ...)` | ~20 处 |
| `API_CLASS(InBuild)` → `SE_CLASS(API(InBuild))` | ~4 处 |
| `API_FUNCTION()` → `SE_FUNCTION(API())` | ~2 处 |
| `API_FIELD(...)` → `SE_PROPERTY(API(...))` | ~1 处活跃 |

注：大量 `API_FIELD` 以注释形式存在，可分批迁移，不影响编译。

### Phase 3: Reflector 工具更新

**3.1 更新 ReflectionMacroType 枚举 (`ReflectorSettingsAndUtils.h`):**

```cpp
enum class ReflectionMacroType
{
    DefineClass = 0,        // DEFINE_CLASS
    DefineClassDefault = 1, // DEFINE_CLASS_DEFAULT
    ReflectModule = 2,      // ENGINE_REFLECT_MODULE
    ReflectMeta = 3,        // SE_META

    SEClass = 4,            // SE_CLASS(Reflect, API(...))
    SEStruct = 5,           // SE_STRUCT(...)
    SEInterface = 6,        // SE_INTERFACE(...)
    SEEnum = 7,             // SE_ENUM(Reflect, API(...))
    SEProperty = 8,         // SE_PROPERTY(Reflect, API(...))
    SEFunc = 9,             // SE_FUNCTION(API(...))
    SEEvent = 10,           // SE_EVENT(API(...))

    NumMacros,
    Unknown = NumMacros,
};
```

**3.2 更新 g_macroNames[] (`ReflectorSettingsAndUtils.cpp`):**

```cpp
static Char const* g_macroNames[] =
{
    SE_TEXT("DEFINE_CLASS"),          // DefineClass
    SE_TEXT("DEFINE_CLASS_DEFAULT"),  // DefineClassDefault
    SE_TEXT("ENGINE_REFLECT_MODULE"), // ReflectModule
    SE_TEXT("SE_META"),               // ReflectMeta

    SE_TEXT("SE_CLASS"),              // SEClass
    SE_TEXT("SE_STRUCT"),             // SEStruct
    SE_TEXT("SE_INTERFACE"),          // SEInterface
    SE_TEXT("SE_ENUM"),               // SEEnum
    SE_TEXT("SE_PROPERTY"),           // SEProperty
    SE_TEXT("SE_FUNCTION"),               // SEFunc
    SE_TEXT("SE_EVENT"),              // SEEvent
};
```

**3.3 更新 VisitMacro() (`ClangVisitors_Macro.cpp`):**
- 从 `StartsWith` 匹配改为精确 `==` 匹配（避免 SE_ENUM 匹配到 SE_EVENT）
- 按 `GetReflectionMacroText()` 进行分发

**3.4 扩展 ReflectionMacro 结构体 (`ClangParserContext.h`):**

```cpp
struct ReflectionMacro
{
    // ... 现有字段 ...
    bool hasReflect = false;   // 包含 "Reflect" 参数
    bool hasAPI = false;       // 包含 "API(...)" 分组
    String macroMetadata;      // 顶层反射元数据，不包含 API(...) 内参数
};
```

**3.5 实现参数解析:**
- 在 `ReflectionMacro` 构造函数中，对所有标注宏类型解析参数
- 实现 `SplitRespectingBrackets()` 辅助函数，处理 `Attributes="EditorOrder(0), DefaultValue(1.0f)"` 等嵌套括号内的逗号
- 提取 `Reflect` 标志和 `API(...)` 分组；API 内参数单独保存，其余反射元数据存入 `macroMetadata`

**3.6 更新 AddFoundReflectionMacro() (`ClangParserContext.cpp`):**
- Property 级别宏：`SEProperty`、`SEFunc`、`SEEvent` → `m_propertyReflectionMacros`
- Type 级别宏：其余所有 → `m_typeReflectionMacros`

**3.7 更新 FindReflectionMacroForType():**
- 匹配 `DefineClass`、`DefineClassDefault`、`ReflectModule`（替代旧 `ReflectType`、`ReflectModule`）

**3.8 更新 FindBindingMacroForType():**
- 匹配 `SEClass`、`SEStruct`、`SEInterface`（替代旧 `APIClass`、`APIStruct`、`APIInterface`）

**3.9 更新 FindReflectionMacroForEnum():**
- 匹配 `SEEnum` 且 `hasReflect == true`

**3.10 更新 FindBindingMacroForEnum():**
- 匹配 `SEEnum` 且 `hasAPI == true`

**3.11 更新 FindReflectionMacroForProperty():**
- 匹配 `SEProperty` 且 `hasReflect == true`

**3.12 更新 FindBindingMacroForMember():**
- 按 `SEProperty`/`SEFunc`/`SEEvent` 匹配，且 `hasAPI == true`

**3.13 更新 VisitStructure() (`ClangVisitors_Structure.cpp`):**
- `FindReflectionMacroForType` 返回的宏，检查 `IsReflectedTypeMacro()` 需更新为检查 `DefineClass`/`DefineClassDefault`

**3.14 更新 VisitEnum() (`ClangVisitors_Enum.cpp`):**
- 单个 `SE_ENUM(Reflect, API())` 宏需同时满足反射和绑定查找
- 当 `FindReflectionMacroForEnum` 找到的宏也包含 `hasAPI` 时，直接设置绑定信息，无需再次查找
- 当 `FindReflectionMacroForEnum` 找到的宏不含 `hasAPI` 时，调用 `FindBindingMacroForEnum` 查找单独的 `SE_ENUM(API())` 标注

**3.15 更新 VisitStructureContents() (`ClangVisitors_Structure.cpp`):**
- 单个 `SE_PROPERTY(Reflect, ..., API(...))` 宏需同时满足属性反射和绑定查找
- 当 `FindReflectionMacroForProperty` 找到的宏也包含 `hasAPI` 时，直接使用该宏的 API 参数提取绑定信息
- 当该宏不含 `hasAPI` 时，调用 `FindBindingMacroForMember` 查找单独的 `SE_PROPERTY(API(...))` 标注

### Phase 4: 代码生成器更新

**4.1 CodeGenerator_CPP_Type.cpp:**
- `IsReflectedTypeMacro()` 检查更新为 `DefineClass`/`DefineClassDefault`
- 元数据提取按用途拆分为 `topLevelArguments`、`apiArguments` 与 `macroMetadata`

**4.2 CodeGenerator_BindingsCpp.cpp / CodeGenerator_BindingsCSharp.cpp:**
- 绑定参数（`InBuild`、`ReadOnly`、`Attributes=...`）从 `macroMetadata` 提取，逻辑不变
- 验证字段名变更不影响序列化/反序列化

### Phase 5: 清理

1. 删除 IType.h 底部旧的 `API_***` 定义
2. 删除 ScriptingAPI.h 中旧定义
3. 迁移注释中的 `API_FIELD(...)` 为 `SE_PROPERTY(API(...))`
4. 更新 IType.h 中的文档注释

---

## 4. 关键文件清单

| 文件路径 | 修改内容 |
|---------|---------|
| `Src/Engine/Core/TypeSystem/IType.h` | 宏定义重命名、新增、删除 |
| `Src/Engine/Runtime/Scripting/ScriptingAPI.h` | 删除旧 API_ 定义，更新注释 |
| `Src/Reflector/Code/ReflectorSettingsAndUtils.h` | 新枚举定义 |
| `Src/Reflector/Code/ReflectorSettingsAndUtils.cpp` | g_macroNames[] 更新 |
| `Src/Reflector/Code/Clang/ClangVisitors_Macro.cpp` | VisitMacro() 改为精确匹配 |
| `Src/Reflector/Code/Clang/ClangParserContext.h` | ReflectionMacro 结构体扩展 |
| `Src/Reflector/Code/Clang/ClangParserContext.cpp` | 所有 Find* 方法更新 + 参数解析 |
| `Src/Reflector/Code/Clang/ClangVisitors_Structure.cpp` | VisitStructure/VisitStructureContents |
| `Src/Reflector/Code/Clang/ClangVisitors_Enum.cpp` | VisitEnum |
| `Src/Reflector/Code/CodeGenerator_CPP_Type.cpp` | IsReflectedTypeMacro 检查 |
| `Src/Reflector/Code/CodeGenerator_BindingsCpp.cpp` | macroMetadata 提取 |
| `Src/Reflector/Code/CodeGenerator_BindingsCSharp.cpp` | macroMetadata 提取 |
| `Src/Engine/**/*.h`（约 150+ 文件） | 宏使用批量替换 |

---

## 5. 风险与缓解

| 风险 | 缓解措施 |
|------|---------|
| `SE_CLASS` 重命名期间名称冲突 | Phase 2 必须原子提交：重命名 DEFINE_CLASS + 重定义 SE_CLASS + 全量替换同时完成 |
| `SE_ENUM`/`SE_EVENT` 前缀冲突 | VisitMacro 改为精确 `==` 匹配，不再用 StartsWith |
| 参数解析：嵌套括号内逗号 | 实现 `SplitRespectingBrackets()` 处理 `Attributes="EditorOrder(0), DefaultValue(1.0f)"` |
| 单宏双用途（Reflect+API）查找冲突 | 找到反射宏后直接检查 `hasAPI`，避免二次查找已被移除的宏 |
| 增量迁移期间 Reflector 兼容性 | 过渡期在 g_macroNames 中保留旧宏名，全部迁移后再删除 |
| DEFINE_CLASS_DEFAULT 之前未被 Reflector 识别 | 新增 DefineClassDefault 枚举值，与 DefineClass 等价处理 |

---

## 6. 验证方案

1. **编译验证**：每个 Phase 完成后全量编译，确认无编译错误
2. **生成代码对比**：重构前快照 `_Generated/` 目录，重构后重新运行 Reflector，diff 验证输出一致
3. **参数解析测试**：对 `SplitRespectingBrackets` 和参数解析编写独立测试用例
4. **全宏变体集成测试**：创建测试头文件覆盖所有宏组合（Reflect only、API only、Reflect+API）
## 7. 设计决策记录

| 决策 | 选择 | 原因 |
|------|------|------|
| DEFINE_CLASS 位置 | 类体内（注入 friend/virtual） | 保持代码注入功能不变，SE_CLASS 放在类声明上方 |
| SE_ENUM 默认行为 | 必须显式标注 Reflect | 与 API 参数对称，语义清晰，无隐式行为 |
| API_FIELD + API_PROPERTY 合并 | 统一为 SE_PROPERTY | Reflector 已能通过上下文（FieldDecl vs CXXMethod）自动区分，无需两个宏 |
