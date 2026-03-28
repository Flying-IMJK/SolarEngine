#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/resource/assetinfo.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE:: AssetInfo
//-------------------------------------------------------------------------

#include "Core/Serialization/Json.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Metadata/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::AssetInfo> final : public TypeCompositeInfo
    {
       static ::SE::AssetInfo const* s_pDefaultInstance_3109302141;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_3109302141 = New<::SE::AssetInfo>(nullptr);

            ::SE::AssetInfo::s_pTypeInfo = New<TTypeCompositeInfo<::SE::AssetInfo>>(s_pDefaultInstance_3109302141);
            Types::RegisterType(::SE::AssetInfo::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::AssetInfo::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::AssetInfo*>(s_pDefaultInstance_3109302141));
            Delete(const_cast<TypeCompositeInfo*>(::SE::AssetInfo::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::AssetInfo"));
            size = sizeof(::SE::AssetInfo);
            alignment = alignof(::SE::AssetInfo);
            name = SE_TEXT("AssetInfo");
            fullName = SE_TEXT("SE::AssetInfo");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::IType::s_pTypeInfo;

            // Add properties
            auto pActualDefaultInstance = reinterpret_cast<::SE::AssetInfo const*>(pDefaultInstance);

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
            // Property id------------------------
            propertyInfo->id = StringID(SE_TEXT("id"));
            propertyInfo->typeID = TypeID(SE_TEXT("SE::UID"));
            propertyInfo->parentTypeID = TypeID(3109302141u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("id");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->id;
            propertyInfo->offset = offsetof(::SE::AssetInfo, id);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            // Property typeID------------------------
            propertyInfo->id = StringID(SE_TEXT("typeID"));
            propertyInfo->typeID = TypeID(SE_TEXT("SE::TypeID"));
            propertyInfo->parentTypeID = TypeID(3109302141u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("typeID");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->typeID;
            propertyInfo->offset = offsetof(::SE::AssetInfo, typeID);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            // Property path------------------------
            propertyInfo->id = StringID(SE_TEXT("path"));
            propertyInfo->typeID = TypeID(SE_TEXT("SE::String"));
            propertyInfo->parentTypeID = TypeID(3109302141u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("path");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->path;
            propertyInfo->offset = offsetof(::SE::AssetInfo, path);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::AssetInfo), alignof(::SE::AssetInfo));
            return new (pMemory) ::SE::AssetInfo(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::AssetInfo(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_3109302141;
        }

        void CreateMeta(TypeID type, const StringAnsi& context, TypeProperty *propertyInfo)
        {
            TypeMetaInfo const* metaInfo = Types::GetMetaTypeInfo(type);
            if (metaInfo != nullptr)
            {
                rapidjson::Document document;
                document.Parse(context.Get(), context.Length());
                if (document.GetParseError() == rapidjson::kParseErrorNone)
                {
                    Json::Array metaDatas = document.GetArray();

                    TypeMetaAttribute* meta = metaInfo->Create();
                    meta->Parse(metaDatas);
                    propertyInfo->metaContainer->Add(type, meta);
                }
            }
        }

        //-------------------------------------------------------------------------
        // Array Methods
        //-------------------------------------------------------------------------

        virtual uint8* GetArrayElementDataPtr(IType* pType, uint32 arrayID, int64 arrayIdx) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
            return nullptr;
        }

        virtual int64 GetArraySize( IType const* pTypeInstance, uint32 arrayID ) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
            return 0;
        }

        virtual int64 GetArrayElementSize(uint32 arrayID) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
            return 0;
        }

        virtual void ClearArray(IType* pTypeInstance, uint32 arrayID) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
        }

        virtual void AddArrayElement(IType* pTypeInstance, uint32 arrayID) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
        }

        virtual void InsertArrayElement(IType* pTypeInstance, uint32 arrayID, int64 insertionIdx) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
        }

        virtual void MoveArrayElement(IType* pTypeInstance, uint32 arrayID, int64 originalElementIdx, int64 newElementIdx) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
        }

        virtual void RemoveArrayElement(IType* pTypeInstance, uint32 arrayID, int64 elementIdx) const override final
        {
            
            // We should never get here since we are asking for a ptr to an invalid property
            ENGINE_UNREACHABLE_CODE();
        }


        //-------------------------------------------------------------------------
        // Default Value Methods
        //-------------------------------------------------------------------------

        virtual bool AreAllPropertyValuesEqual( IType const* pTypeInstance, IType const* pOtherTypeInstance ) const override final
        {
            auto pType = reinterpret_cast<::SE::AssetInfo const*>(pTypeInstance);
            auto pOtherType = reinterpret_cast<::SE::AssetInfo const*>(pOtherTypeInstance);
            return true;
        }


        virtual bool IsPropertyValueEqual(IType const* pTypeInstance, IType const* pOtherTypeInstance, uint32 propertyID, int32 arrayIdx = -1) const override final
        {
            auto pType = reinterpret_cast<::SE::AssetInfo const*>(pTypeInstance);
            auto pOtherType = reinterpret_cast<::SE::AssetInfo const*>(pOtherTypeInstance);
            if (propertyID == 5863474)
            {
                
                return pType->id == pOtherType->id;
            }
            if (propertyID == 524428404)
            {
                
                return pType->typeID == pOtherType->typeID;
            }
            if (propertyID == 2090608114)
            {
                
                return pType->path == pOtherType->path;
            }
            return false;
        }

        virtual void ResetToDefault( IType* pTypeInstance, uint32 propertyID ) const override final
        {
            auto pDefaultType = reinterpret_cast<::SE::AssetInfo const*>(s_pDefaultInstance_3109302141);
            auto pActualType = reinterpret_cast<::SE::AssetInfo*>(pTypeInstance);
            ENGINE_ASSERT(pActualType != nullptr && pDefaultType != nullptr);
            if (propertyID == 5863474)
            {
                
                pActualType->id = pDefaultType->id;
            }
            if (propertyID == 524428404)
            {
                
                pActualType->typeID = pDefaultType->typeID;
            }
            if (propertyID == 2090608114)
            {
                
                pActualType->path = pDefaultType->path;
            }
        }

        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const override
        {
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::AssetInfo>::s_pDefaultInstance_3109302141);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::AssetInfo>::s_pDefaultInstance_3109302141, propertyID, arrayIdx);
        }
    };

    ::SE::AssetInfo const* TTypeCompositeInfo<::SE::AssetInfo>::s_pDefaultInstance_3109302141 = nullptr;
}

