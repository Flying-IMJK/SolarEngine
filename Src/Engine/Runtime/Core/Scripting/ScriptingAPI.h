#pragma once

// ScriptingAPI.h
// Defines the SE_ annotation macros used to mark C++ types, methods, properties,
// fields, and enums for reflection and/or C# binding code generation.
//
// These macros are annotation-only — they expand to nothing at compile time.
// The Reflector tool parses the macro parameters to generate reflection info
// and C# binding code.
//
// The authoritative parameter reference is Document/TypeMacro.md and the comments
// beside the macro definitions in Runtime/Core/TypeSystem/TypeMacro.h.
//
// Usage:
//   // Class with reflection + binding:
//   DEFINE_CLASS(MyClass, BaseClass)
//   SE_CLASS(Reflect, API)
//   class MyClass : public BaseClass { ... };
//
//   // Enum with reflection only:
//   SE_ENUM(Reflect)
//   enum class MyEnum : uint8 { A = 0, B = 1 };
//
//   // Property with reflection + binding:
//   SE_PROPERTY(Reflect, API, Category="MyCategory")
//   int32 myProp;
//
//   // Function with binding only:
//   SE_FUNCTION(API)
//   void DoSomething(int32 value);
//
//   // Native WindowBase exposed to C# as Window:
//   SE_CLASS(API, NoSpawn, NoConstructor, Sealed, Name="Window")
//   class WindowBase : public ScriptingObject { ... };
//
//   // Inject helper code into generated binding files:
//   API_INJECT_CODE(cpp, "#include \"Runtime/Core/Platform/FileSystem.h\"");
//   API_INJECT_CODE(csharp, "using Newtonsoft.Json;");
//
// Note: The actual macro definitions are in Runtime/Core/TypeSystem/TypeMacro.h.
// This header is kept for backward compatibility and documentation purposes.
