#pragma once

namespace SE
{
	// Flag a class as the module class for that project
	#define ENGINE_REFLECT_MODULE           \
	        public:                         \
	        static void RegisterTypes();    \
	        static void UnregisterTypes();

	struct Reflect
    {};

	struct API
    {};

	struct NoSpawn
    {};

	struct NoConstructor
    {};
	
	struct Abstract
    {};
	
	struct Sealed
    {};

	struct Static
    {};

	struct Attributes
    {};

	struct Name
    {
	public:
		Name(const char* name) {}
	};

	struct Tag
    {};
	
	struct Prop
    {};

	struct ReadOnly
    {};

	struct Template
    {};

	struct InBuild
    {
    public:
		InBuild(const char* mapType) {}
	};
	

	// Declares a reflection metadata attribute type.
	// Usage: SE_META() class MyAttribute : public TypeMetaAttribute { ... };
	#define SE_META()

	// Annotation macros expand to nothing in C++. BuildTool parses their comma-separated
	// parameters. String values may contain commas and escaped quotes.
	// See Document/TypeMacro.md for the complete parameter reference and examples.

	// Enum annotation. Parameters: Reflect, API(Deprecated, Attributes="...").
	// Attributes accepts C# attribute content with or without the outer [ and ].
	// Example: SE_ENUM(Reflect, API(Attributes="Flags"))
	#define SE_ENUM(...)

	// Class/struct/interface annotation.
	// Parameters: Reflect, Template,
	// API(NoSpawn, NoConstructor, Abstract, Sealed, Static, Deprecated,
	//     Name="...", Attributes="...", Tag="NativeInvokeUseName").
	// Name changes only the generated API/C# name; Tag makes static native calls use
	// that API name as their C++ target. Template is used with SE_TYPEDEF() specializations.
	#define SE_CLASS(...)
	#define SE_STRUCT(...)
	#define SE_INTERFACE(...)

	// Template typedef/using annotation. SE_TYPEDEF() instantiates a binding type;
	// SE_TYPEDEF(Alias) leaves the declaration as a native-only alias.
	#define SE_TYPEDEF(...)

	// Inject raw code into generated binding files.
	// API_INJECT_CODE(cpp, "#include \"Runtime/Core/Platform/FileSystem.h\"")
	// API_INJECT_CODE(csharp, "using Newtonsoft.Json;")
	#define SE_INJECT_CODE(...)

	// Field or getter/setter property annotation.
	// Parameters: Reflect, Category="...", ToolsReadOnly, ShowInRestrictedMode,
	// Meta="..." or Metadata="...", API(ReadOnly, Deprecated, Attributes="...").
	// Bare metadata expressions are also accepted, e.g. SE_PROPERTY(Reflect, Range(0, 1)).
	// ReadOnly, Deprecated and Attributes apply to generated C# bindings.
	// Category, ToolsReadOnly, ShowInRestrictedMode and Meta apply to C++ reflection.
	#define SE_FIELD(...)


	// Method annotation. Parameters: API(NoProxy, Deprecated, Propertie, Attributes="...").
	// Static binding is inferred from the C++ method declaration.
	#define SE_FUNCTION(...)

	// Event annotation. Parameters: API(Attributes="...").
	#define SE_EVENT(...)
}
