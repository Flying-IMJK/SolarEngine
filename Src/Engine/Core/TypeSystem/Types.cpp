#include "Types.h"
#include "Property/TypePropertyPath.h"
#include "Info/TypeEnumInfo.h"
#include "Info/TypeCompositeInfo.h"

#include "DefaultTypeInfos.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Collections/Sorting.h"
#include "Info/TypeMetaInfo.h"
#include "Property/TypeProperty.h"

//-------------------------------------------------------------------------

namespace SE
{
    Dictionary<TypeID, TypeCompositeInfo const *> Types::m_RegisteredTypes = Dictionary<TypeID, TypeCompositeInfo const *>();
    Dictionary<TypeID, TypeMetaInfo const*> Types::m_RegisteredMetas = Dictionary<TypeID, TypeMetaInfo const*>();
	Dictionary<TypeID, TypeEnumInfo const*> Types::m_RegisteredEnums = Dictionary<TypeID, TypeEnumInfo const*>();
    Dictionary<StableID, TypeID> Types::m_StableIDToTypeID = Dictionary<StableID, TypeID>();

    struct TypeRegisterInfo
    {
        TypeRegister* typeRegister;
        ITyper* typer;
    };

    List<TypeRegisterInfo>& GetTyperCache()
    {
        static List<TypeRegisterInfo> m_TyperInfoCache;
        return m_TyperInfoCache;
    }

    void Types::RegisterTyper(TypeRegister* pRegister)
    {
        ENGINE_ASSERT(pRegister != nullptr);

        TypeRegisterInfo typerInfo;
        typerInfo.typeRegister = pRegister;
        typerInfo.typer = nullptr;

        GetTyperCache().Add(typerInfo);
    }

    void Types::UnregisterTyper(TypeRegister* pRegister)
    {
        List<TypeRegisterInfo>& typerInfoCache = GetTyperCache();

        int findIndex = ListExtensions::IndexOf(typerInfoCache, CreateFunc([pRegister](const TypeRegisterInfo & info) -> bool
        {
            return info.typeRegister == pRegister;
        }));

        if (findIndex != INVALID_INDEX)
        {
            TypeRegisterInfo typerInfo = typerInfoCache[findIndex];
            if (typerInfo.typer != nullptr)
            {
                typerInfo.typer->UnregisterTypes();
                Delete(typerInfo.typer);
            }

            typerInfoCache.RemoveAt(findIndex);
        }
    }

    void Types::RegisterTypeID(StableID id, TypeID typeId)
    {
        ENGINE_ASSERT(m_StableIDToTypeID.ContainsKey(id) == false);
        m_StableIDToTypeID[id] = typeId;
    }

    void Types::UnRegisterTypeID(StableID id, TypeID typeId)
    {
        ENGINE_ASSERT(m_StableIDToTypeID.ContainsKey(id) == true);
        m_StableIDToTypeID.Remove(id);
    }

    void Types::InitTypeSystem()
    {
        CoreTypeRegistry::Initialize();

        List<TypeRegisterInfo>& typerInfoCache = GetTyperCache();
        for (auto& item : typerInfoCache)
        {
            if (item.typer == nullptr)
            {
                if (item.typeRegister == nullptr)
                {
                    LOG_ERROR("System", "Create System {0} failed, SystemRegister is null");
                    continue;
                }
                item.typer = item.typeRegister->Create();
            }
            item.typer->RegisterTypes();
        }

        // 初始元数据
        for (auto& item : typerInfoCache)
        {

        }
    }

    void Types::UnInitTypeSystem()
    {
        List<TypeRegisterInfo>& typerInfoCache = GetTyperCache();
        for (auto& item : typerInfoCache)
        {
            if (item.typer != nullptr)
            {
                item.typer->UnregisterTypes();
                Delete(item.typer);
            }
        }

        CoreTypeRegistry::Shutdown();
    }

    Types::Types()
    {
        TTypeCompositeInfo<IType>::RegisterType( *this );
    }

    Types::~Types()
    {
        TTypeCompositeInfo<IType>::UnregisterType( *this );
        ENGINE_ASSERT( m_RegisteredEnums.IsEmpty() && m_RegisteredTypes.IsEmpty());
    }

    //-------------------------------------------------------------------------

    void Types::RegisterType( TypeCompositeInfo const* pTypeInfo )
    {
        ENGINE_ASSERT(pTypeInfo != nullptr);
        ENGINE_ASSERT(pTypeInfo->id != TypeID::Invalid && !CoreTypeRegistry::IsCoreType( pTypeInfo->id ));
        ENGINE_ASSERT(!m_RegisteredTypes.ContainsKey(pTypeInfo->id));
        m_RegisteredTypes.Add(pTypeInfo->id, pTypeInfo);
    }

    void Types::UnregisterType( TypeCompositeInfo const* pTypeInfo )
    {
        ENGINE_ASSERT( pTypeInfo != nullptr );
        ENGINE_ASSERT( pTypeInfo->id != TypeID::Invalid && !CoreTypeRegistry::IsCoreType( pTypeInfo->id ) );
        auto iter = m_RegisteredTypes.Find( pTypeInfo->id );
        ENGINE_ASSERT( iter != m_RegisteredTypes.end() );
        ENGINE_ASSERT( iter->Value == pTypeInfo );
        m_RegisteredTypes.Remove( iter );
    }

    TypeCompositeInfo const* Types::GetTypeInfo( TypeID typeID )
    {
        ENGINE_ASSERT( typeID != TypeID::Invalid && !CoreTypeRegistry::IsCoreType( typeID ) );
        auto iter = m_RegisteredTypes.Find( typeID );
        if ( iter != m_RegisteredTypes.end() )
        {
            return iter->Value;
        }
        else
        {
            return nullptr;
        }
    }

    TypeProperty const* Types::ResolvePropertyPath( TypeCompositeInfo const* pTypeInfo, TypePropertyPath const& pathID )
    {
        TypeCompositeInfo const* pParentTypeInfo = pTypeInfo;
        TypeProperty const* pFoundPropertyInfo = nullptr;

        // Resolve property path
        size_t const numPathElements = pathID.GetNumElements();
        size_t const lastElementIdx = numPathElements - 1;
        for ( size_t i = 0; i < numPathElements; i++ )
        {
            pFoundPropertyInfo = pParentTypeInfo->GetPropertyInfo( pathID[i].propertyID );
            if ( pFoundPropertyInfo == nullptr )
            {
                break;
            }

            if ( i != lastElementIdx )
            {
                // If this occurs, we have an invalid path as each element must contain other properties
                if ( CoreTypeRegistry::IsCoreType( pFoundPropertyInfo->typeID ) && !pFoundPropertyInfo->IsArrayProperty() )
                {
                    LOG_WARNING("TypeSystem", "Type Registry Cant resolve malformed property path");
                    pFoundPropertyInfo = nullptr;
                    break;
                }

                // Get the type desc of the property
                pParentTypeInfo = GetTypeInfo( pFoundPropertyInfo->typeID );
                if ( pParentTypeInfo == nullptr )
                {
                    LOG_ERROR( "TypeSystem", "Type Registry Cant resolve property path since it contains an unknown type");
                    pFoundPropertyInfo = nullptr;
                }
            }
        }

        return pFoundPropertyInfo;
    }

    bool Types::IsTypeDerivedFrom( TypeID typeID, TypeID parentTypeID )
    {
		if (typeID != TypeID::Invalid || parentTypeID != TypeID::Invalid)
		{
			return false;
		}
        ENGINE_ASSERT(!CoreTypeRegistry::IsCoreType( typeID ) && !CoreTypeRegistry::IsCoreType(parentTypeID));

        auto pTypeInfo = GetTypeInfo( typeID );
        ENGINE_ASSERT( pTypeInfo != nullptr );

        return pTypeInfo->IsDerivedFrom( parentTypeID );
    }

    List<TypeCompositeInfo const*> Types::GetAllTypes( bool includeAbstractTypes, bool sortAlphabetically )
    {
		List<TypeCompositeInfo const*> types;

        for ( auto const& typeInfoPair : m_RegisteredTypes )
        {
            if ( !includeAbstractTypes && typeInfoPair.Value->IsAbstractType() )
            {
                continue;
            }

            types.Add( typeInfoPair.Value );
        }

        if ( sortAlphabetically )
        {
			Function<bool(TypeCompositeInfo const* const&, TypeCompositeInfo const* const&)> sortPredicate =
				[] ( TypeCompositeInfo const* const& pTypeInfoA, TypeCompositeInfo const* const& pTypeInfoB )
            {
                #ifdef SE_DEVELOPMENT
                return pTypeInfoA->friendlyName < pTypeInfoB->friendlyName;
                #else
                return strcmp( pTypeInfoA->m_ID.c_str(), pTypeInfoB->m_ID.c_str() );
                #endif
            };

			Sorting::QuickSort(types, sortPredicate);
        }

        return types;
    }

    List<TypeCompositeInfo const*> Types::GetAllDerivedTypes( TypeID parentTypeID, bool includeParentTypeInResults,
		bool includeAbstractTypes, bool sortAlphabetically )
    {
        List<TypeCompositeInfo const*> matchingTypes;

        for ( auto const& typeInfoPair : m_RegisteredTypes )
        {
            if ( !includeParentTypeInResults && typeInfoPair.Key == parentTypeID )
            {
                continue;
            }

            if ( !includeAbstractTypes && typeInfoPair.Value->IsAbstractType() )
            {
                continue;
            }

            if ( typeInfoPair.Value->IsDerivedFrom( parentTypeID ) )
            {
                matchingTypes.Add( typeInfoPair.Value );
            }
        }

        if ( sortAlphabetically )
        {
            Function<bool(TypeCompositeInfo const* const&, TypeCompositeInfo const* const&)> sortPredicate =
				[] ( TypeCompositeInfo const* const& pTypeInfoA, TypeCompositeInfo const* const& pTypeInfoB )
            {
                #ifdef SE_DEVELOPMENT
                return pTypeInfoA->fullName < pTypeInfoB->fullName;
                #else
                return StringUtils::Compare( pTypeInfoA->m_ID.c_str(), pTypeInfoB->m_ID.c_str());
				#endif
            };

            Sorting::QuickSort(matchingTypes, sortPredicate);
        }

        return matchingTypes;
    }

    List<TypeID> Types::GetAllCastableTypes( IType const* pType )
    {
        ENGINE_ASSERT( pType != nullptr );
        List<TypeID> parentTypeIDs(5);
        auto pParentTypeInfo = pType->GetTypeInfo()->pParentTypeInfo;
        while ( pParentTypeInfo != nullptr )
        {
            parentTypeIDs.Add( pParentTypeInfo->id );
            pParentTypeInfo = pParentTypeInfo->pParentTypeInfo;
        }
        return parentTypeIDs;
    }

    bool Types::AreTypesInTheSameHierarchy( TypeID typeA, TypeID typeB )
    {
        auto pTypeInfoA = GetTypeInfo( typeA );
        auto pTypeInfoB = GetTypeInfo( typeB );
        return AreTypesInTheSameHierarchy( pTypeInfoA, pTypeInfoB );
    }

    bool Types::AreTypesInTheSameHierarchy( TypeCompositeInfo const* pTypeInfoA, TypeCompositeInfo const* pTypeInfoB )
    {
        if ( pTypeInfoA->IsDerivedFrom( pTypeInfoB->id ) )
        {
            return true;
        }

        if ( pTypeInfoB->IsDerivedFrom( pTypeInfoA->id ) )
        {
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void Types::RegisterEnum( TypeEnumInfo const* type )
    {
        ENGINE_ASSERT(type->id != TypeID::Invalid);
        ENGINE_ASSERT(!m_RegisteredEnums.ContainsKey(type->id));
        m_RegisteredEnums[type->id] = type;
    }

    void Types::UnregisterEnum( TypeEnumInfo const* type )
    {
        ENGINE_ASSERT( type->id != TypeID::Invalid );
        auto iter = m_RegisteredEnums.Find( type->id );
        ENGINE_ASSERT( iter != m_RegisteredEnums.end() );
        m_RegisteredEnums.Remove(iter);
    }

    TypeEnumInfo const* Types::GetEnumInfo( TypeID enumID )
    {
        ENGINE_ASSERT( enumID != TypeID::Invalid );
        auto iter = m_RegisteredEnums.Find( enumID );
        if ( iter != m_RegisteredEnums.end() )
        {
            return iter->Value;
        }
        else
        {
            return nullptr;
        }
    }

    void Types::RegisterMeta(TypeMetaInfo const* pType)
    {
        ENGINE_ASSERT(pType != nullptr);
        ENGINE_ASSERT(pType->id != TypeID::Invalid);
        ENGINE_ASSERT(!m_RegisteredMetas.ContainsKey(pType->id));
        m_RegisteredMetas.Add(pType->id, pType);
    }
    void Types::UnregisterMeta(TypeMetaInfo const* pType)
    {
        ENGINE_ASSERT( pType != nullptr );
        ENGINE_ASSERT( pType->id != TypeID::Invalid);
        auto iter = m_RegisteredMetas.Find( pType->id );
        ENGINE_ASSERT( iter != m_RegisteredMetas.end() );
        ENGINE_ASSERT( iter->Value == pType );
        m_RegisteredMetas.Remove( iter );
    }

    TypeMetaInfo const* Types::GetMetaTypeInfo(TypeID typeID)
    {
        auto iter = m_RegisteredMetas.Find( typeID);
        if ( iter != m_RegisteredMetas.end() )
        {
            return iter->Value;
        }

        return nullptr;
    }

    String Types::GetEnumToString(TypeEnumInfo const * enumInfo, int64 value)
	{
        if (!enumInfo)
        {
            return String::Empty;
        }

		for (TypeEnumInfo::ConstantInfo info : enumInfo->constants)
		{
			if (info.value == value)
			{
				return info.id.ToString();
			}
		}

		return String();
	}

    //-------------------------------------------------------------------------

    int64 Types::GetTypeByteSize( TypeID typeID )
    {
        ENGINE_ASSERT( typeID != TypeID::Invalid );

        if ( IsCoreType( typeID ) )
        {
            return CoreTypeRegistry::GetTypeSize( typeID );
        }
        else
        {
            auto pChildTypeInfo = GetTypeInfo( typeID );
            ENGINE_ASSERT( pChildTypeInfo != nullptr );
            return pChildTypeInfo->size;
        }
    }
}