#pragma once

#include "ReflectionProjectTypes.h"
#include "Core/TypeSystem/Info/TypeCompositeInfo.h"
#include "Core/TypeSystem/CoreTypes.h"
#include "Core/TypeSystem/Property/TypeProperty.h"

//-------------------------------------------------------------------------
namespace SE::ReflectTool
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
        StringAnsi cppType;
        StringAnsi name;
        bool       isPointer = false;
        bool       isConst = false;
        bool       isRef = false;
        bool       isOut = false;
        StringAnsi defaultValue;
        StringAnsi attributes;
    };

    struct ApiFunction
    {
        StringAnsi         name;
        StringAnsi         returnType;
        List<ApiParam>     params;
        bool               isStatic = false;
        bool               isVirtual = false;
        bool               isConst = false;
        StringAnsi         uniqueName;
        StringAnsi         entryPoint;
        bool               noProxy = false;
        bool               isHidden = false;
        bool               isSealed = false;
        StringAnsi         attributes;
        StringAnsi         tag;
        int                lineNumber = -1;
    };

    struct ApiProperty
    {
        StringAnsi cppType;
        StringAnsi name;
        StringAnsi getterName;
        StringAnsi setterName;
        StringAnsi getterUniqueName;
        StringAnsi setterUniqueName;
        StringAnsi getterEntryPoint;
        StringAnsi setterEntryPoint;
        bool       hasGetter = false;
        bool       hasSetter = false;
        AccessLevel getterAccess = AccessLevel::Public;
        AccessLevel setterAccess = AccessLevel::Public;
        StringAnsi attributes;
        int        lineNumber = -1;
    };

    struct ApiField
    {
        StringAnsi cppType;
        StringAnsi name;
        bool       isReadOnly = false;
        bool       isStatic = false;
        bool       isHidden = false;
        StringAnsi attributes;
        int        arraySize = 0;
        int        lineNumber = -1;
    };

    struct ApiEvent
    {
        StringAnsi         name;
        StringAnsi         cppType;
        StringAnsi         namespaceName;
        List<ApiParam>     params;
        bool               isStatic = false;
        AccessLevel        access = AccessLevel::Public;
        StringAnsi         attributes;
        int                lineNumber = -1;
    };

    struct ApiInterface
    {
        StringAnsi         name;
        StringAnsi         namespaceName;
        List<ApiFunction>  functions;
        AccessLevel        access = AccessLevel::Public;
        StringAnsi         attributes;
        int                lineNumber = -1;
    };

    struct ApiEnum
    {
        StringAnsi         name;
        StringAnsi         namespaceName;
        StringAnsi         underlyingType;
        List<StringAnsi>   valueNames;
        List<int64>        values;
        AccessLevel        access = AccessLevel::Public;
        StringAnsi         attributes;
        int                lineNumber = -1;
    };

    struct ApiClass
    {
        StringAnsi         name;
        StringAnsi         namespaceName;
        StringAnsi         baseClassName;
        StringAnsi         headerFilePath;
        List<ApiFunction>  functions;
        List<ApiProperty>  properties;
        List<ApiField>     fields;
        List<ApiInterface> interfaces;
        List<ApiEvent>     events;
        bool               isAbstract = false;
        bool               isStruct = false;
        bool               isSealed = false;
        bool               isStatic = false;
        bool               noSpawn = false;
        bool               noConstructor = false;
        bool               isScriptingObject = false;
        AccessLevel        access = AccessLevel::Public;
        StringAnsi         attributes;
        StringAnsi         tag;
        int                lineNumber = -1;
    };

    // -------------------------------------------------------------------------
    // Binding extension for DataType
    // -------------------------------------------------------------------------
    struct BindingInfo
    {
        List<ApiFunction>  functions;
        List<ApiProperty>  bindingProperties;
        List<ApiField>     fields;
        List<ApiInterface> interfaces;
        List<ApiEvent>     events;

        bool isSealed = false;
        bool isStatic = false;
        bool IsAbstract = false;
        bool noSpawn = false;
        bool noConstructor = false;
        bool isScriptingObject = false;
        StringAnsi baseClassName;
        StringAnsi attributes;
        StringAnsi tag;

        StringAnsi assemblyName;
        StringAnsi assemblyDir;
    };

    struct DataProperty
    {
    public:
        DataProperty() = default;

        DataProperty( StringAnsi const& name, int32_t lineNumber )
            : propertyID( name.ToString() )
            , name( name )
            , lineNumber( lineNumber )
        {}

        DataProperty( StringAnsi const& name, String const& typeName, int32_t lineNumber )
            : propertyID( name.ToString() )
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

        inline bool operator==( DataProperty const& RHS ) const { return propertyID == RHS.propertyID; }
        inline bool operator!=( DataProperty const& RHS ) const { return propertyID != RHS.propertyID; }

        // Dev Info
        //-------------------------------------------------------------------------

		StringAnsi GetFriendlyName() const;
        StringAnsiView GetCategory() const { return category; }

        // MetaData
        //-------------------------------------------------------------------------

        bool HasMetaData() const { return !metaData.IsEmpty(); }

    public:

        TypeID											propertyID;
        int												lineNumber = -1;
        TypeID											typeID;
		StringAnsi                                      name;
		StringAnsi                                      metaData;
		StringAnsi                                      description;
		StringAnsi                                      typeName;
		StringAnsi                                      templateArgTypeName;
        int												arraySize = -1;
        EnumFlags<TypeProperty::Flags>                  flags;
        bool                                            isDevOnly = true;

        // From MetaData
		StringAnsi		                                category;
        bool                                            isToolsReadOnly = false;
        bool                                            showInRestrictedMode = false;
    };

    //-------------------------------------------------------------------------

    struct ReflectedEnumConstant
    {
        StringID                                        ID;
		StringAnsi                                     	label;
        int												value;
		StringAnsi                                      description;
    };

    struct DataType
    {
        enum class Flags
        {
            IsAbstract = 1 << 0,
            IsEnum = 1 << 1,
        	IsStruct = 1 << 2,
        	IsMeta = 1 << 3,
        };

    public:
		DataType() = default;

        DataType(StringID typeID, StringAnsi const& name ) : typeID( typeID ), name( name )
        {}

        inline bool IsAbstract() const { return flags.IsFlag( Flags::IsAbstract ); }
        inline bool IsStruct() const { return flags.IsFlag( Flags::IsStruct ); }
        inline bool IsEnum() const { return flags.IsFlag( Flags::IsEnum ); }
    	inline bool IsMeta() const { return flags.IsFlag( Flags::IsMeta ); }

        // Structure functions
        DataProperty const* GetPropertyDescriptor( StringID propertyID ) const;

        // Enum functions
        void AddEnumConstant( ReflectedEnumConstant const& constant );
        bool IsValidEnumLabelID( StringID labelID ) const;
        bool GetValueFromEnumLabel( StringID labelID, uint32& value ) const;

        // Dev tools helpers
		StringAnsi GetFriendlyName() const;
		StringAnsi GetCategory() const;

        // Generate additional type info
        inline bool HasProperties() const { return !properties.IsEmpty(); }
        bool HasArrayProperties() const;
        bool HasDynamicArrayProperties() const;
        bool HasResourcePtrProperties() const;
        bool HasResourcePtrOrStructProperties() const;

    public:
        bool isReflect = false;
        bool isAPI = false;
        TypeID											typeID;
        HeaderID                                        headerID;
		StringAnsi                                		name = "Invalid";
		StringAnsi                                		namespaceName;
        EnumFlags<Flags>                                flags;

        // Structures
        StringID                                        parentTypeID;
        List<DataProperty>                              properties;

        // Enums
        TypeIDCore                                      underlyingType = TypeIDCore::Uint8;
		List<ReflectedEnumConstant>                     enumConstants;

        bool                                            isDevOnly = false;

        // Bindings extension (populated when API_CLASS/API_STRUCT/API_ENUM is present)
        BindingInfo                                     bindingInfo;
    };

    //-------------------------------------------------------------------------

    struct ReflectedResourceType
    {
        // Fill the resource type ID and the friendly name from the macro registration string
        bool TryParseResourceRegistrationMacroString( String const& registrationStr );

    public:

        TypeID                                          typeID;
        TypeID											resourceTypeID;
		StringAnsi                                      friendlyName;
        HeaderID                                        headerID;
		StringAnsi                                      className;
		StringAnsi                                      namespaceName;
		List<StringID>                                  parents;
    };
}