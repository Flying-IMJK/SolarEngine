#include "DataTypes.h"

#include "Core/Utils.h"
#include "Core/Json.h"
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static void GenerateFriendlyName( std::string& name )
    {
        if ( name.length() <= 1 )
        {
            return;
        }

        //-------------------------------------------------------------------------

		Utils::String::ReplaceAll(name, "_", " ");

        name[0] = (char) toupper( name[0] );

        int32_t i = 1;
        while ( i < name.length() )
        {
            // Only insert a space before a Capital letter, if it isnt the last letter and if it isnt followed or preceded by a capital letter
            bool const shouldInsertSpace = isupper( name[i] ) && i != name.length() - 1 && ( name[i - 1] != '/') && !isupper(name[i - 1]) && !isupper(name[i + 1]);
            if ( shouldInsertSpace )
            {
                name[i] = ' ';
                i++;
            }

            i++;
        }
    }

    //-------------------------------------------------------------------------

    PropertyData const* TypeData::GetPropertyDescriptor( StringID propertyID ) const
    {
        ENGINE_ASSERT( typeID != StringID::Invalid && !IsFlag(TypeData::Flags::IsAbstract) && !IsFlag(TypeData::Flags::IsEnum));
        for ( auto const& prop : properties )
        {
            if ( prop.propertyID == propertyID )
            {
                return &prop;
            }
        }

        return nullptr;
    }

	std::string PropertyData::GetFriendlyName() const
    {
		std::string name = this->name;
		Utils::String::ReplaceAll(name, "m_", "");

        if ( name.empty() )
        {
            return name;
        }

        if ( name.length() > 1 && name[0] == 'p' && isupper( name[1] ) )
        {
            name = name.substr( 1, name.length() - 1 );
        }

        GenerateFriendlyName( name );

        return name;
    }
    //-------------------------------------------------------------------------

    void TypeData::AddEnumConstant( EnumDataConstant const& constant )
    {
        ENGINE_ASSERT( typeID != StringID::Invalid && IsFlag(TypeData::Flags::IsEnum) );
        ENGINE_ASSERT( constant.ID != StringID::Invalid );
        ENGINE_ASSERT( !IsValidEnumLabelID( constant.ID ) );

		enumConstants.push_back( constant );
    }

    bool TypeData::GetValueFromEnumLabel( StringID labelID, uint32_t& value ) const
    {
        ENGINE_ASSERT( typeID != StringID::Invalid && IsFlag(TypeData::Flags::IsEnum) );

        for ( auto const& constant : enumConstants )
        {
            if ( constant.ID == labelID )
            {
                value = constant.value;
                return true;
            }
        }

        return false;
    }

    bool TypeData::IsValidEnumLabelID( StringID labelID ) const
    {
        for ( auto const& constant : enumConstants )
        {
            if ( constant.ID == labelID )
            {
                return true;
            }
        }

        return false;
    }

	std::string TypeData::GetFriendlyName() const
    {
		std::string friendlyName = name;
        GenerateFriendlyName( friendlyName );
        return friendlyName;
    }

	std::string TypeData::GetCategory() const
    {
		std::string category = Utils::CombineStringList(namespaceScopeList, "::");
		Utils::String::ReplaceAll(category, Settings::g_engineNamespacePlusDelimiter, "");
		Utils::String::ReplaceAll(category, "::", "/");

        // Remove trailing slash
        if ( !category.empty() && Utils::String::EndsWith(category, '/' ))
        {
            category.erase(category.length() - 1, 1);
        }

        GenerateFriendlyName( category );

        return category;
    }

    bool TypeData::HasArrayProperties() const
    {
        for ( auto& propertyDesc : properties )
        {
            if ( propertyDesc.IsArrayProperty() )
            {
                return true;
            }
        }

        return false;
    }

    bool TypeData::HasDynamicArrayProperties() const
    {
        for ( auto& propertyDesc : properties )
        {
            if ( propertyDesc.IsDynamicArrayProperty() )
            {
                return true;
            }
        }

        return false;
    }

    bool TypeData::HasResourcePtrProperties() const
    {
/*        for ( auto& propertyDesc : m_properties )
        {
            if (propertyDesc.m_typeID == TypeIDCore::ResourcePtr )
            {
                return true;
            }

            if ( propertyDesc.m_typeID == TypeIDCore::TResourcePtr )
            {
                return true;
            }
        }*/

        return false;
    }

    bool TypeData::HasResourcePtrOrStructProperties() const
    {
/*        for ( auto& propertyDesc : m_properties )
        {
            if ( propertyDesc.m_typeID == TypeIDCore::ResourcePtr )
            {
                return true;
            }

            if ( propertyDesc.m_typeID == TypeIDCore::TResourcePtr )
            {
                return true;
            }

            if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
            {
                return true;
            }
        }*/

        return false;
    }

    //-------------------------------------------------------------------------

    bool ReflectedResourceType::TryParseResourceRegistrationMacroString( std::string const& registrationStr )
    {
        /*// Generate type ID string and get friendly name
		int32 const resourceIDStartIdx = registrationStr.Find("\"", 0 );
        if ( resourceIDStartIdx == INVALID_INDEX )
        {
            return false;
        }

		int32 const resourceIDEndIdx = registrationStr.Find( "\"", resourceIDStartIdx + 1 );
        if ( resourceIDEndIdx == INVALID_INDEX )
        {
            return false;
        }

		int32 const resourceFriendlyNameStartIdx = registrationStr.Find("\"", resourceIDEndIdx + 1 );
        if ( resourceFriendlyNameStartIdx == INVALID_INDEX )
        {
            return false;
        }

		int32 const resourceFriendlyNameEndIdx = registrationStr.Find("\"", resourceFriendlyNameStartIdx + 1 );
        if ( resourceFriendlyNameEndIdx == INVALID_INDEX )
        {
            return false;
        }

        ENGINE_ASSERT( resourceIDStartIdx != resourceIDEndIdx );
        ENGINE_ASSERT( resourceFriendlyNameStartIdx != resourceFriendlyNameEndIdx );

        //-------------------------------------------------------------------------

		String const resourceTypeIDString = registrationStr.Substring( resourceIDStartIdx + 1, resourceIDEndIdx - resourceIDStartIdx - 1 );

        m_resourceTypeID = ResTypeID(resourceTypeIDString.ToString());
        if ( !m_resourceTypeID.IsValid() )
        {
            return false;
        }
        
        m_friendlyName = registrationStr.Substring( resourceFriendlyNameStartIdx + 1, resourceFriendlyNameEndIdx - resourceFriendlyNameStartIdx - 1 );
*/
        return true;
    }
}
