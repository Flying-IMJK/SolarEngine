# SolarEngine

SolarEngine 是一个以 C++17 为核心、结合 .NET 8 托管代码的游戏引擎项目。仓库包含运行时、编辑器、Shader 与资源、Win32 编辑器入口，以及用于反射和绑定生成的 SEBuilder。

## 项目结构

| 路径 | 内容 |
| --- | --- |
| `Src/Engine/Runtime` | 原生运行时及托管 Runtime，包括平台、渲染、资源等模块 |
| `Src/Engine/Editor` | 原生及托管编辑器 |
| `Src/Engine/Shaders`、`Src/Engine/Assets` | 引擎 Shader 与资源 |
| `Src/BuildTool` | 反射、绑定及代码生成工具 |
| `Src/Applications/Win32EditorApp` | Windows 编辑器程序入口 |
| `Src/Libraries` | 第三方依赖 |

## 构建要求

仓库提供的 CMake 预设面向 Windows x64，使用以下工具：

- Visual Studio 2022，安装 C++ 开发工具及 Windows SDK
- CMake 3.26 或更高版本
- .NET 8 SDK

## 构建与运行

在仓库根目录执行：

```powershell
cmake --preset x64-Debug
cmake --build --preset Debug --config Debug
.\Debug\SE.exe
```

构建 Release 版本时，改用：

```powershell
cmake --preset x64-Release
cmake --build --preset Release --config Release
.\Release\SE.exe
```

也可以运行 `GenerateSln.bat Debug` 或 `GenerateSln.bat Release`，在 `Sln` 目录生成 Visual Studio 解决方案。

## 开发文档

- [项目规范](docs/项目规范.md)：模块边界、构建流程和反射生成约定
- [代码规范](docs/代码规范.md)：C++、C# 与 CMake 的代码约定

`Src/Engine` 中的 `_Generated` 目录由构建工具生成；需要调整生成内容时，请修改生成输入或生成器，勿直接编辑生成文件。

## 许可证

本项目采用 [MIT License](LICENSE)。
