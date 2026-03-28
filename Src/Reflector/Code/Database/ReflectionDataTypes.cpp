#include "ReflectionDataTypes.h"

#include "Core/Types/Strings/String.h"
#include "Core/ThirdParty/rapidjson/document.h"
//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    static void GenerateFriendlyName( StringAnsi& name )
    {
        if ( name.Length() <= 1 )
        {
            return;
        }

        //-------------------------------------------------------------------------

		name.Replace("_", " ");

        name[0] = (char) toupper( name[0] );

        int32_t i = 1;
        while ( i < name.Length() )
        {
            // Only insert a space before a Capital letter, if it isnt the last letter and if it isnt followed or preceded by a capital letter
            bool const shouldInsertSpace = isupper( name[i] ) && i != name.Length() - 1 && ( name[i - 1] != '/') && !isupper(name[i - 1]) && !isupper(name[i + 1]);
            if ( shouldInsertSpace )
            {
                name[i] = ' ';
                i++;
            }

            i++;
        }
    }

    //-------------------------------------------------------------------------

    ReflectedProperty const* ReflectedType::GetPropertyDescriptor( StringID propertyID ) const
    {
        ENGINE_ASSERT( typeID != StringID::Invalid && !IsAbstract() && !IsEnum() );
        for ( auto const& prop : properties )
        {
            if ( prop.propertyID == propertyID )
            {
                return &prop;
            }
        }

        return nullptr;
    }

	StringAnsi ReflectedProperty::GetFriendlyName() const
    {
		StringAnsi name = this->name;
		name.Replace("m_", "");

        if ( name.IsEmpty() )
        {
            return name;
        }

        if ( name.Length() > 1 && name[0] == 'p' && isupper( name[1] ) )
        {
            name = name.Substring( 1, name.Length() - 1 );
        }

        GenerateFriendlyName( name );

        return name;
    }
    //-------------------------------------------------------------------------

    void ReflectedType::AddEnumConstant( ReflectedEnumConstant const& constant )
    {
        ENGINE_ASSERT( typeID != StringID::Invalid && IsEnum() );
        ENGINE_ASSERT( constant.ID != StringID::Invalid );
        ENGINE_ASSERT( !IsValidEnumLabelID( constant.ID ) );

		enumConstants.Add( constant );
    }

    bool ReflectedType::GetValueFromEnumLabel( StringID labelID, uint32_t& value ) const
    {
        ENGINE_ASSERT( typeID != StringID::Invalid && IsEnum() );

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

    bool ReflectedType::IsValidEnumLabelID( StringID labelID ) const
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

	StringAnsi ReflectedType::GetFriendlyName() const
    {
		StringAnsi friendlyName = name;
        GenerateFriendlyName( friendlyName );
        return friendlyName;
    }

	StringAnsi ReflectedType::GetCategory() const
    {
		StringAnsi category = namespaceName;
		category.Replace(Settings::g_engineNamespacePlusDelimiter, "");
		category.Replace("::", "/");

        // Remove trailing slash
        if ( !category.IsEmpty() && category.EndsWith('/' ))
        {
            category.Remove(category.Length() - 1, 1);
        }

        GenerateFriendlyName( category );

        return category;
    }

    bool ReflectedType::HasArrayProperties() const
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

    bool ReflectedType::HasDynamicArrayProperties() const
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

    bool ReflectedType::HasResourcePtrProperties() const
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

    bool ReflectedType::HasResourcePtrOrStructProperties() const
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

    bool ReflectedResourceType::TryParseResourceRegistrationMacroString( String const& registrationStr )
    {
        /*// Generate type ID string and get friendly name
		int32 const resourceIDStartIdx = registrationStr.Find(SE_TEXT("\""), 0 );
        if ( resourceIDStartIdx == INVALID_INDEX )
        {
            return false;
        }

		int32 const resourceIDEndIdx = registrationStr.Find( SE_TEXT("\""), resourceIDStartIdx + 1 );
        if ( resourceIDEndIdx == INVALID_INDEX )
        {
            return false;
        }

		int32 const resourceFriendlyNameStartIdx = registrationStr.Find(SE_TEXT("\""), resourceIDEndIdx + 1 );
        if ( resourceFriendlyNameStartIdx == INVALID_INDEX )
        {
            return false;
        }

		int32 const resourceFriendlyNameEndIdx = registrationStr.Find(SE_TEXT("\""), resourceFriendlyNameStartIdx + 1 );
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
        
        m_friendlyName = registrationStr.Substring( resourceFriendlyNameStartIdx + 1, resourceFriendlyNameEndIdx - resourceFriendlyNameStartIdx - 1 ).ToStringAnsi();
*/
        return true;
    }
}