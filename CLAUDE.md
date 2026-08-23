# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SolarEngine is a modern C++17 game engine with a modular architecture and comprehensive editor. The engine follows a professional-grade architecture with clear separation between Core, Runtime, and Editor modules.

## Build System

### Prerequisites
- CMake 3.26+
- Visual Studio 2022 (Windows) or compatible C++17 compiler
- Windows SDK (for Windows builds)

### Build Configuration
The project uses CMake with the following key options:
- `USE_EDITOR` (ON by default): Build the editor
- `USE_DEVELOPMENT` (ON by default): Enable development tools
- `USE_PROFILER` (ON by default): Enable performance profiling

### Build Commands

**Using CMake Presets (Recommended):**
```bash
# Configure and build Debug
cmake --preset x64-Debug
cmake --build --preset Debug

# Configure and build Release
cmake --preset x64-Release
cmake --build --preset Release
```

**Manual CMake:**
```bash
# Debug build
cmake -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug

# Release build
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

**Build Output:**
- Debug builds output to `Debug/` directory
- Release builds output to `Release/` directory
- Main executable: `SGE.exe` (editor)

### Reflection System
The engine uses a custom reflection system for code generation:
- Reflection tool: `SE.Reflector.exe`
- Generated code goes to `_Generated/` directories
- Build dependencies: `SE.Reflector` → `PreCompile*` targets → `SE.Core` → `SE.Runtime` → `SE.Editor`

## Architecture

### Core Module (`Src/Engine/Core/`)
- **Math**: Vector, Matrix, Quaternion, bounding volumes, collision detection
- **Memory**: Custom allocators, STL allocators
- **Platform**: Abstraction layer (Win32/Windows implementations)
- **Serialization**: JSON (rapidjson), binary, file streams
- **Threading**: Task system, thread pool, job system
- **Type System**: Reflection, type descriptors, properties
- **Utilities**: Logging, exceptions, timing, encoding

### Runtime Module (`Src/Engine/Runtime/`)
- **Graphics**: GPU abstraction (Vulkan), shader compilation
- **Graph System**: Material graphs, shader graphs
- **Engine Core**: Engine context, application framework
- **Async GPU Tasks**: GPU resource management

### Editor Module (`Src/Engine/Editor/`)
- **GUI Framework**: Docking system, context menus, toolbars
- **Property Grid**: Object property editing with reflection
- **Viewports**: 3D viewport with camera controls (FPSCamera, ViewportCamera)
- **Resource Management**: Asset browser, import system (models, textures)
- **Scene Graph**: Scene hierarchy management
- **Windows**: ContentWindow, PropertiesWindow, SceneHierarchyWindow, LogWindow

### Applications (`Src/Applications/`)
- `Win32Editor`: Main editor application entry point (`wWinMain`)

## Development Workflow

### Code Style
- **Clang-format**: Custom rules with 120 column limit (see `.clang-format`)
- **EditorConfig**: VS Code/editor settings (see `.editorconfig`)
- **Naming**: Follow existing patterns in codebase
- **Includes**: Sorted with custom priority (Core > Runtime > Editor > others)

### Asset Management
- Engine assets: `Src/Engine/Assets/` (fonts, icons, models, shaders)
- Runtime assets copied to build directory during build
- Configuration: `SolarEngine.ini` in assets config directory
- Asset import system supports FBX models and various texture formats

### Dependencies
Third-party libraries in `Src/Libraries/`:
- **Graphics**: Vulkan, ShaderCompiler (glslang, SPIRV-Cross)
- **Asset Import**: Assimp (3D models)
- **UI**: ImGui (dear imgui)
- **Networking**: GameNetworkingSockets
- **Fonts**: Freetype
- **Profiling**: Tracy
- **Serialization**: rapidjson, mpack
- **Compression**: LZ4, meshoptimizer

## Key Development Patterns

### Reflection Macros
- `CLASS`, `REFLECTION_TYPE`, `REFLECTION_BODY` for type registration
- Reflection enables editor property grid integration
- Generated code in `_Generated/` directories

### Platform Abstraction
- Platform-specific code in `Core/Platform/` subdirectories
- Win32 implementation in `Core/Platform/Win32/`
- Windows-specific implementation in `Core/Platform/Windows/`
- Use platform defines: `PLATFORM_WIN32`, `PLATFORM_WINDOWS`

### Memory Management
- Custom allocators in `Core/Memory/`
- STL allocator wrapper for container compatibility
- Structured Exception Handling (SEH) on Windows

### Error Handling
- Exception system in `Core/Logging/Exceptions/`
- Logging via `LoggingSystem` with different verbosity levels
- Use `SE_ASSERT` for debug assertions

## Running the Editor

After building:
1. Navigate to build directory (`Debug/` or `Release/`)
2. Run `SGE.exe`
3. Editor loads with default configuration from `SolarEngine.ini`

The editor provides:
- 3D viewport with camera controls
- Scene hierarchy tree
- Property grid for selected objects
- Content browser for asset management
- Log window for debugging output

## Testing and Debugging

### Profiling
- Tracy profiler integrated (enabled with `USE_PROFILER`)
- CPU and GPU profiling available
- Build with `SE_PROFILER` define for profiling support

### Logging
- Log levels: Trace, Debug, Info, Warning, Error, Fatal
- Log output to console and log window
- Exception logging with stack traces

## Platform Notes

### Windows-Specific Features
- Win32 API for window management and input
- DirectX/Vulkan for graphics (Vulkan primary)
- DLL delay loading for external dependencies
- Windows clipboard and file system integration

### Cross-Platform Considerations
- Platform abstraction layer allows potential porting
- CMake configurations include Linux/macOS support
- Some third-party libraries are cross-platform

## Common Tasks

### Adding New Engine Types
1. Define class with reflection macros (`CLASS`, `REFLECTION_TYPE`)
2. Build to generate reflection code
3. Type appears in editor property grid automatically

### Adding New Asset Types
1. Implement import logic in editor resource system
2. Add to asset browser item types
3. Register with content import system

### Modifying Shaders
1. Edit HLSL/GLSL files in `Src/Engine/Shaders/`
2. Shader compiler (glslang) processes during build
3. Shader graphs in editor for material creation

### Adding Editor Windows
1. Extend `EditorWindow` base class
2. Register with window system in editor modules
3. Add to docking layout configuration