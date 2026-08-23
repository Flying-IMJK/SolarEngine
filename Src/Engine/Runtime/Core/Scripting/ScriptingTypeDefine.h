#pragma once


namespace SE
{
    #define SCRIPTING_TYPE_MIN(type) \
    public: \
    friend class type##Internal; \
    static struct ScriptingTypeInitializer TypeInitializer; \


    /// <summary>
    /// Helper define used to declare required components for native structures that have managed type.
    /// </summary>
    #define SCRIPTING_TYPE_STRUCTURE(type) \
    public: \
    friend class type##Internal; \
    static ScriptingTypeInitializer TypeInitializer; \
    FORCE_INLINE static const ScriptingType& GetScriptingType() { return TypeInitializer.GetType(); } \
    FORCE_INLINE static CLRClass* GetScriptingClass() { return TypeInitializer.GetType().ManagedClass; }

    /// <summary>
    /// Helper define used to declare required components for native types that have managed type (for objects that cannot be spawned).
    /// </summary>
    #define SCRIPTING_TYPE_NO_SPAWN(type) \
    public: \
    friend class type##Internal; \
    static ScriptingTypeInitializer TypeInitializer; \
    FORCE_INLINE static const ScriptingType& GetScriptingType() { return TypeInitializer.GetType(); } \
    FORCE_INLINE static CLRClass* GetScriptingClass() { return TypeInitializer.GetType().ManagedClass; }

    /// <summary>
    /// Helper define used to declare required components for native types that have managed type (for objects that can be spawned).
    /// </summary>
    #define SCRIPTING_TYPE(type) \
    SCRIPTING_TYPE_NO_SPAWN(type); \
    static type* Spawn(const SpawnParams& params) { return ::SE::New<type>(params); } \
    explicit type() : type(SpawnParams(UID::New(), type::TypeInitializer)) { } \
    explicit type(const SpawnParams& params)

    /// <summary>
    /// Helper define used to declare required components for native types that have managed type (for objects that can be spawned) including default constructors implementations.
    /// </summary>
    #define SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(type, baseType) \
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(type); \
    static type* Spawn(const SpawnParams& params) { return ::SE::<type>(params); } \
    explicit type(const SpawnParams& params) : baseType(params) { } \
    explicit type() : baseType(SpawnParams(UID::New(), type::TypeInitializer)) { }

    /// <summary>
    /// Helper define used to implement required components for native types that have managed type (for objects that can be spawned).
    /// </summary>
    #define IMPLEMENT_SCRIPTING_TYPE(type, baseType, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
    StringAnsiView(typeName, ARRAY_SIZE(typeName) - 1), \
    sizeof(type), \
    &type##Internal::InitRuntime, \
    (ScriptingType::SpawnHandler)&type::Spawn, \
    &baseType::TypeInitializer, \
    setupScriptVTable, \
    setupScriptObjectVTable \
    );

    /// <summary>
    /// Helper define used to implement required components for native types that have managed type (for objects that can be spawned).
    /// </summary>
    #define IMPLEMENT_SCRIPTING_TYPE_NO_BASE(type, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
    StringAnsiView(typeName, ARRAY_SIZE(typeName) - 1), \
    sizeof(type), \
    &type##Internal::InitRuntime, \
    (ScriptingType::SpawnHandler)&type::Spawn, \
    nullptr, \
    setupScriptVTable, \
    setupScriptObjectVTable \
    );

    /// <summary>
    /// Helper define used to implement required components for native types that have managed type (for objects that cannot be spawned). With base class specified.
    /// </summary>
    #define IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN_WITH_BASE(type, baseType, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
    StringAnsiView(typeName, ARRAY_SIZE(typeName) - 1), \
    sizeof(type), \
    &type##Internal::InitRuntime, \
    &ScriptingType::DefaultSpawn, \
    &baseType::TypeInitializer, \
    setupScriptVTable, \
    setupScriptObjectVTable \
    );

    /// <summary>
    /// Helper define used to implement required components for native types that have managed type (for objects that cannot be spawned).
    /// </summary>
    #define IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(type, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
    StringAnsiView(typeName, ARRAY_SIZE(typeName) - 1), \
    sizeof(type), \
    &type##Internal::InitRuntime, \
    &ScriptingType::DefaultSpawn, \
    nullptr, \
    setupScriptVTable, \
    setupScriptObjectVTable \
    );
}
