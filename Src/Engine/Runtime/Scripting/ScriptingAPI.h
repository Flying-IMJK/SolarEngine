#pragma once

// ScriptingAPI.h
// Defines the SE_ annotation macros used to mark C++ types, methods, properties,
// fields, and enums for reflection and/or C# binding code generation.
//
// These macros are annotation-only — they expand to nothing at compile time.
// The Reflector tool parses the macro parameters to generate reflection info
// and C# binding code.
//
// Parameters:
//   Reflect  — generate C++ reflection info (TTypeCompositeInfo/TTypeEnumInfo)
//   API      — generate C# binding code
//
// Usage:
//   // Class with reflection + binding:
//   DEFINE_CLASS(MyClass, BaseClass)
//   SE_CLASS(Reflect, API, InBuild)
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
// Note: The actual macro definitions are in Engine/Core/TypeSystem/IType.h.
// This header is kept for backward compatibility and documentation purposes.