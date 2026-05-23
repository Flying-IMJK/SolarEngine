#pragma once

#include "ReflectionProjectTypes.h"
#include "Core/TypeSystem/Info/TypeCompositeInfo.h"
#include "Core/TypeSystem/CoreTypes.h"
#include "Core/TypeSystem/Property/TypeProperty.h"

//-------------------------------------------------------------------------
namespace SE::ReflectTool
{
    struct ReflectedProperty
    {

    public:

        ReflectedProperty() = default;

        ReflectedProperty( StringAnsi const& name, int32_t lineNumber )
            : propertyID( name.ToString() )
            , name( name )
            , lineNumber( lineNumber )
        {}

        ReflectedProperty( StringAnsi const& name, String const& typeName, int32_t lineNumber )
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

        inline bool operator==( ReflectedProperty const& RHS ) const { return propertyID == RHS.propertyID; }
        inline bool operator!=( ReflectedProperty const& RHS ) const { return propertyID != RHS.propertyID; }

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

    struct ReflectedType
    {
        enum class Flags
        {
            IsAbstract = 1 << 0,
            IsEnum = 1 << 1,
        	IsStruct = 1 << 2,
        	IsMeta = 1 << 3,
        };

    public:

		ReflectedType() = default;

        ReflectedType(StringID typeID, StringAnsi const& name )
            : typeID( typeID )
            , name( name )
        {}

        inline bool IsAbstract() const { return flags.IsFlag( Flags::IsAbstract ); }
        inline bool IsEnum() const { return flags.IsFlag( Flags::IsEnum ); }
    	inline bool IsMeta() const { return flags.IsFlag( Flags::IsMeta ); }

        // Structure functions
        ReflectedProperty const* GetPropertyDescriptor( StringID propertyID ) const;

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

        TypeID											typeID;
        HeaderID                                        headerID;
		StringAnsi                                		name = "Invalid";
		StringAnsi                                		namespaceName;
        EnumFlags<Flags>                                flags;

        // Structures
        StringID                                        parentTypeID;
        List<ReflectedProperty>                         properties;

        // Enums
        TypeIDCore                                      underlyingType = TypeIDCore::Uint8;
		List<ReflectedEnumConstant>                     enumConstants;

        bool                                            m_isDevOnly = true;
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