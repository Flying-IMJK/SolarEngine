#pragma once

#include "ReflectionProjectTypes.h"
#include "Core/TypeSystem/Property/TypeProperty.h"

//-------------------------------------------------------------------------
namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Access level for API types and members
    // -------------------------------------------------------------------------

    enum class AccessLevel
    {
        Private,
        Protected,
        Public,
        Internal,
    };

    // -------------------------------------------------------------------------
    // Type mapping table (C++ <-> C#)
    // -------------------------------------------------------------------------

    struct TypeMapping
    {
        const char* cppType;
        const char* csType;
        const char* csInterop;
        bool        isBlittable;
        bool        isString;
        bool        isObject;
    };

    // -------------------------------------------------------------------------
    // API annotation data types
    // -------------------------------------------------------------------------

    struct ApiParam
    {
        std::string cppType;
        std::string name;
        bool       isPointer = false;
        bool       isConst = false;
        bool       isRef = false;
        bool       isOut = false;
        std::string defaultValue;
        std::string attributes;
        std::string marshalAs;
        std::string comment;
    };

    struct ApiFunction
    {
        std::string         name;
        std::string         returnType;
        std::vector<ApiParam>     params;
        bool               isStatic = false;
        bool               isVirtual = false;
        bool               isConst = false;
        std::string         uniqueName;
        std::string         entryPoint;
        bool               noProxy = false;
        bool               isHidden = false;
        bool               isSealed = false;
        bool               isDeprecated = false;
        AccessLevel        access = AccessLevel::Public;
        std::string         attributes;
        std::string         tag;
        std::string         comment;
        std::string         marshalAs;
        int                lineNumber = -1;
    };

    struct ApiProperty
    {
        std::string cppType;
        std::string name;
        std::string getterName;
        std::string setterName;
        std::string getterUniqueName;
        std::string setterUniqueName;
        std::string getterEntryPoint;
        std::string setterEntryPoint;
        bool       hasGetter = false;
        bool       hasSetter = false;
        AccessLevel getterAccess = AccessLevel::Public;
        AccessLevel setterAccess = AccessLevel::Public;
        std::string attributes;
        std::string comment;
        std::string marshalAs;
        int        lineNumber = -1;
    };

    struct ApiField
    {
        std::string cppType;
        std::string name;
        bool       isReadOnly = false;
        bool       isStatic = false;
        bool       isHidden = false;
        bool       isDeprecated = false;
        std::string attributes;
        std::string defaultValue;
        std::string comment;
        std::string marshalAs;
        int        arraySize = 0;
        int        lineNumber = -1;
    };

    struct ApiEvent
    {
        std::string         name;
        std::string         cppType;
        std::vector<std::string>   namespaceNameList;
        std::vector<ApiParam>     params;
        bool               isStatic = false;
        AccessLevel        access = AccessLevel::Public;
        std::string         attributes;
        std::string         comment;
        int                lineNumber = -1;
    };

    struct ApiInterface
    {
        std::string         name;
        std::string         nativeName;
        std::vector<std::string>   namespaceNameList;
        std::vector<ApiFunction>  functions;
        AccessLevel        access = AccessLevel::Public;
        std::string         attributes;
        std::string         comment;
        int                lineNumber = -1;
    };

    struct ApiEnum
    {
        std::string         name;
        std::vector<std::string>   namespaceScopeList;
        std::vector<std::string>   structScopeList;
        std::string         underlyingType;
        std::vector<std::string>   valueNames;
        std::vector<int64>        values;
        std::vector<std::string>   valueComments;
        AccessLevel        access = AccessLevel::Public;
        std::string         attributes;
        std::string         comment;
        int                lineNumber = -1;
    };

    struct ApiClass
    {
        std::string               name;
        std::string               nativeName;
        std::vector<std::string>  namespaceNameList;
        std::vector<std::string>  structScopeList;
        std::string               baseClassName;
        std::string               headerFilePath;
        std::vector<ApiFunction>  functions;
        std::vector<ApiProperty>  properties;
        std::vector<ApiField>     fields;
        std::vector<ApiInterface> interfaces;
        std::vector<ApiEvent>     events;
        bool               isAbstract = false;
        bool               isTemplate = false;
        bool               isStruct = false;
        bool               isPod = false;
        bool               isSealed = false;
        bool               isStatic = false;
        bool               noSpawn = false;
        bool               noConstructor = false;
        bool               isScriptingObject = false;
        bool               isInterface = false;
        bool               isDeprecated = false;
        AccessLevel        access = AccessLevel::Public;
        std::string         attributes;
        std::string         tag;
        std::string         comment;
        std::string         marshalAs;
        int                lineNumber = -1;
    };

    // -------------------------------------------------------------------------
    // Binding extension for DataType
    // -------------------------------------------------------------------------
    struct BindingInfo
    {
        std::vector<ApiFunction>  functions;
        std::vector<ApiProperty>  bindingProperties;
        std::vector<ApiField>     fields;
        std::vector<ApiInterface> interfaces;
        std::vector<ApiEvent>     events;

        bool isSealed = false;
        bool isStatic = false;
        bool IsAbstract = false;
        bool noSpawn = false;
        bool noConstructor = false;
        bool isScriptingObject = false;
        bool isInterface = false;
        bool isDeprecated = false;
        std::string name = "";
        std::string baseClassName = "";
        std::string attributes = "";
        std::string tag = "";
        std::string comment = "";
        std::string marshalAs = "";

        std::string assemblyName = "";
        std::string assemblyDir = "";
    };

    struct PropertyData
    {
    public:
        PropertyData() = default;

        PropertyData( std::string const& name, int32_t lineNumber )
            : propertyID( name )
            , name( name )
            , lineNumber( lineNumber )
        {}

        PropertyData( std::string const& name, std::string const& typeName, int32_t lineNumber )
            : propertyID( name )
            , name( name )
            , typeName( typeName )
            , lineNumber( lineNumber )
        {}

        inline bool IsStructureProperty() const { return flags.IsFlag( TypeProperty::Flags::IsStructure ); }
        inline bool IsEnumProperty() const { return flags.IsFlag( TypeProperty::Flags::IsEnum ); }
        inline bool IsBitFlagsProperty() const { return flags.IsFlag( TypeProperty::Flags::IsBitFlags ); }
        inline bool IsArrayProperty() const { return flags.IsFlag( TypeProperty::Flags::IsArray ) || flags.IsFlag( TypeProperty::Flags::IsDynamicArray ); }
        inline bool IsStaticArrayProperty() const { return flags.IsFlag( TypeProperty::Flags::IsArray ) && !flags.IsFlag( TypeProperty::Flags::IsDynamicArray ); }
        inline bool IsDynamicArrayProperty() const { return flags.IsFlag( TypeProperty::Flags::IsDynamicArray ); }
        inline uint32_t GetArraySize() const { ENGINE_ASSERT( arraySize > 0 ); return (uint32_t) arraySize; }

        inline bool operator==( PropertyData const& RHS ) const { return propertyID == RHS.propertyID; }
        inline bool operator!=( PropertyData const& RHS ) const { return propertyID != RHS.propertyID; }

        // Dev Info
        //-------------------------------------------------------------------------

		std::string GetFriendlyName() const;
        std::string_view GetCategory() const { return category; }

        // MetaData
        //-------------------------------------------------------------------------

        bool HasMetaData() const { return !metaData.empty(); }

    public:

        TypeID											propertyID;
        int												lineNumber = -1;
        TypeID											typeID;
		std::string                                     name;
		std::string                                     metaData;
		std::string                                     description;
		std::string                                     typeName;
		std::string                                     templateArgTypeName;
        int												arraySize = -1;
        EnumFlags<TypeProperty::Flags>                  flags;
        bool                                            isDevOnly = true;

        // From MetaData
		std::string		                                category;
        bool                                            isToolsReadOnly = false;
        bool                                            showInRestrictedMode = false;
    };

    //-------------------------------------------------------------------------

    struct EnumDataConstant
    {
        StringID                                        ID;
		std::string                                     label;
        int												value;
		std::string                                     description;
    };

    struct TypeData
    {
        enum class Flags
        {
            IsStruct = 1,
            IsEnum = 2,
            IsMeta = 4,
            IsAbstract = 8,
            IsTemplate = 16
        };

    public:
        TypeData() = default;

        TypeData(StringID typeID, std::string const& name ) : typeID( typeID ), name( name )
        {}

        bool IsFlag(Flags flag) const { return flags.IsFlag( flag ); }

        // Structure functions
        PropertyData const* GetPropertyDescriptor( StringID propertyID ) const;

        // Enum functions
        void AddEnumConstant( EnumDataConstant const& constant );
        bool IsValidEnumLabelID( StringID labelID ) const;
        bool GetValueFromEnumLabel( StringID labelID, uint32& value ) const;

        // Dev tools helpers
        std::string GetFriendlyName() const;
        std::string GetCategory() const;

        // Generate additional type info
        inline bool HasProperties() const { return !properties.empty(); }
        bool HasArrayProperties() const;
        bool HasDynamicArrayProperties() const;
        bool HasResourcePtrProperties() const;
        bool HasResourcePtrOrStructProperties() const;

    public:
        bool isReflect = false;
        bool isAPI = false;
        TypeID											typeID;
        HeaderID                                        headerID;
        std::string                                		name = "Invalid";
        std::vector<std::string>                        namespaceScopeList;
        std::vector<std::string>                        structScopeList;
        EnumFlags<Flags>                                flags;

        // Structures
        StringID                                        parentTypeID;
        std::vector<PropertyData>                       properties;

        // Enums
        Utils::TypeIDCore                               underlyingType = Utils::TypeIDCore::Uint8;
        std::vector<EnumDataConstant>                   enumConstants;

        bool                                            isDevOnly = false;

        BindingInfo                                     bindingInfo;
    };

    //-------------------------------------------------------------------------

    struct ReflectedResourceType
    {
        // Fill the resource type ID and the friendly name from the macro registration string
        bool TryParseResourceRegistrationMacroString( std::string const& registrationStr );

    public:

        TypeID                                          typeID;
        TypeID											resourceTypeID;
		std::string                                      friendlyName;
        HeaderID                                        headerID;
		std::string                                      className;
		std::string                                      namespaceName;
		std::vector<StringID>                                  parents;
    };
}
