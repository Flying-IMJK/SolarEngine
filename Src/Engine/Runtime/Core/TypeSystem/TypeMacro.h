#pragma once

namespace SE
{
	// Flag a class as the module class for that project
	#define ENGINE_REFLECT_MODULE           \
	        public:                         \
	        static void RegisterTypes();    \
	        static void UnregisterTypes();


	// SE_META
	// class nameAttribute : public TypeMetaAttribute
	// {
	//
	// }
	#define SE_META()

	// Flag this enum for reflection and/or C# binding
	// Parameters: Reflect, API
	// SE_ENUM(Reflect)             - reflection only
	// SE_ENUM(API)                 - binding only
	// SE_ENUM(Reflect, API)        - reflection + binding
	#define SE_ENUM(...)

	// Flag this class/struct/interface for reflection and/or C# binding
	// Parameters: Reflect, API, NoSpawn, NoConstructor, Abstract, Sealed, Static, Name=..., Attributes=..., Tag=...
	// Name="..." changes the generated API/C# type name while preserving the native C++ type name.
	// Tag="NativeInvokeUseName" makes static binding calls use the API name as the native C++ call target.
	// [API] binding [NoSpawn] [Abstract]
	// [Reflect] reflection
	#define SE_CLASS(...)
	#define SE_STRUCT(...)
	#define SE_INTERFACE(...)

	// Flag a typedef/using declaration for binding generation.
	// Parameters: Alias
	// SE_TYPEDEF()       - instantiate the referenced template type for bindings
	// SE_TYPEDEF(Alias)  - pure alias, do not introduce a new generated type
	#define SE_TYPEDEF(...)

	// Inject raw code into generated binding files.
	// API_INJECT_CODE(cpp, "#include \"Runtime/Core/Platform/FileSystem.h\"")
	// API_INJECT_CODE(csharp, "using Newtonsoft.Json;")
	#define API_INJECT_CODE(...)

	// Flag this member variable for reflection and/or C# binding
	// Parameters: Reflect, API, plus JSON metadata
	// SE_PROPERTY(Reflect, Category="MyCategory")     - reflection only
	// SE_PROPERTY(API, ReadOnly)                       - binding only
	// SE_PROPERTY(Reflect, API, Category="MyCategory") - reflection + binding
	#define SE_PROPERTY(...)

	// Flag this method for reflection and/or C# binding
	// Parameters: Reflect, API, Static
	// SE_FUNCTION(API)             - binding only
	// SE_FUNCTION(Reflect, API)    - reflection + binding
	#define SE_FUNCTION(...)

	// Flag this event for C# binding
	// Parameters: API
	#define SE_EVENT(...)
}
