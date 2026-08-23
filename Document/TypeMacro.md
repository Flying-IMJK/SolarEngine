# TypeMacro 参数参考

`TypeMacro.h` 中的宏只用于声明；它们在 C++ 编译时展开为空。`SEBuildTool` 会读取宏参数，分别生成 C++ 反射代码、C++ 原生调用包装和 C# 声明。

参数以逗号分隔，顺序不限。`Reflect`、`Template` 和反射元数据参数写在宏顶层；所有 C# 绑定参数必须写在 `API(...)` 分组内。字符串值应使用双引号；字符串中的双引号使用 `\"` 转义。`Attributes` 可写成 `Attributes="Obsolete"` 或 `Attributes="[Obsolete]"`，生成器会将前者规范化为 C# 属性列表。

## 类型与枚举

| 宏 | 参数 | 作用 |
| --- | --- | --- |
| `SE_CLASS` / `SE_STRUCT` / `SE_INTERFACE` | `Reflect` | 生成 C++ 类型反射信息。 |
| | `API(...)` | 生成 C++/C# 绑定。成员仍需各自使用 `SE_PROPERTY`、`SE_FUNCTION` 或 `SE_EVENT` 标注。 |
| | `API(NoSpawn)` | 仅 `SE_CLASS` 的脚本对象类型：使用默认的不可创建入口，而不是生成实例构造入口。 |
| | `API(NoConstructor)` | 仅 `SE_CLASS`：不生成 C# 默认构造函数。 |
| | `API(Abstract / Sealed / Static)` | 仅 `SE_CLASS`：控制生成的 C# 类修饰符。`Static` 不应与 `Abstract` 或 `Sealed` 同时使用。 |
| | `API(Deprecated)` | 为生成的 C# 类、结构或接口添加 `[Obsolete]`。 |
| | `API(Name="ManagedName")` | 仅改变生成的 API/C# 类型名，不改变 C++ 原生类型名。 |
| | `API(Attributes="...")` | 添加 C# 特性，例如 `Attributes="HideInEditor"`。 |
| | `API(Tag="NativeInvokeUseName")` | 仅静态 `SE_CLASS`：原生调用目标使用 `Name` 指定的名称。 |
| | `Template` | 仅 `SE_CLASS` / `SE_STRUCT`：作为可由 `SE_TYPEDEF()` 实例化的模板绑定模式。 |
| `SE_ENUM` | `Reflect` | 生成 C++ 枚举反射信息。 |
| | `API(...)` | 生成 C++/C# 枚举绑定。 |
| | `API(Deprecated / Attributes="...")` | 分别生成 `[Obsolete]` 和指定的 C# 特性。 |

示例：

```cpp
SE_CLASS(Reflect, API(Abstract, NoSpawn, Name="NativeWindow", Attributes="HideInEditor"))
class WindowBase : public ScriptingObject
{
    // ...
};

SE_ENUM(Reflect, API(Deprecated, Attributes="Flags"))
enum class FeatureFlags : uint32 { None, FastPath };
```

## 成员

| 宏 | 参数 | 作用 |
| --- | --- | --- |
| `SE_PROPERTY` | `Reflect` | 为字段生成 C++ 属性反射。 |
| | `API(...)` | 为字段或一对 getter/setter 生成 C++/C# 属性绑定。 |
| | `API(ReadOnly)` | 对直接标注的字段省略 C# setter。 |
| | `API(Hidden)` | 不公开生成的字段或属性。 |
| | `API(Deprecated)` | 为生成的 C# 属性添加 `[Obsolete]`。 |
| | `API(Attributes="...")` | 为生成的 C# 属性添加特性。 |
| | `Category="..."` | 设置 C++ 反射属性分类。 |
| | `ToolsReadOnly` | 将反射属性标记为工具只读。 |
| | `ShowInRestrictedMode` | 将反射属性标记为受限模式可见。 |
| | `Meta="..."` / `Metadata="..."` | 附加 C++ 反射元数据；也可直接写元数据表达式。 |
| `SE_FUNCTION` | `API(...)` | 为方法生成 C++/C# 绑定。没有 `API(...)` 时不会生成方法绑定。 |
| | `API(NoProxy)` | 不生成公开的 C# 调用包装。 |
| | `API(Hidden)` | 将生成的方法设为非公开。 |
| | `API(Deprecated / Attributes="...")` | 分别添加 `[Obsolete]` 和 C# 特性。 |
| `SE_EVENT` | `API(...)` | 生成 C# 事件绑定。 |
| | `API(Attributes="...")` | 为生成的事件添加 C# 特性。 |

`SE_FUNCTION` 的静态性由 C++ 声明中的 `static` 决定，不需要额外的宏参数。

```cpp
SE_PROPERTY(Reflect, Category="Rendering", ToolsReadOnly,
            Meta="Range(0, 1), DisplayName(\"Opacity\")",
            API(Attributes="EditorOrder(20)"))
float opacity = 1.0f;

SE_FUNCTION(API(Deprecated, Attributes="EditorBrowsable(EditorBrowsableState.Never)"))
static void RebuildCache();
```

## 其他宏

| 宏 | 参数 | 作用 |
| --- | --- | --- |
| `SE_META()` | 无 | 声明反射元数据属性类型。 |
| `SE_TYPEDEF()` | 无 | 将前面使用 `Template` 标注的模板具体化为绑定类型。 |
| `SE_TYPEDEF(Alias)` | `Alias` | 保留为原生别名，不生成新的绑定类型。 |
| `API_INJECT_CODE(cpp, "...")` | `cpp` | 向生成的 C++ 绑定文件插入原始代码。 |
| `API_INJECT_CODE(csharp, "...")` | `csharp` | 向生成的 C# 绑定文件插入原始代码。 |
| `ENGINE_REFLECT_MODULE` | 无 | 声明模块类型注册入口。 |

> `MarshalAs` 和成员级 `Tag` 当前仅会被解析并保存，尚未参与代码生成；不要将它们视为可用的公开参数。
