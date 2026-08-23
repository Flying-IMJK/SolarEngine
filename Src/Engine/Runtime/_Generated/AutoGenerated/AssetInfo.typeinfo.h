#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Resource/AssetInfo.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE AssetInfo
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::AssetInfo> final : public TypeCompositeInfo
    {
       static ::SE::AssetInfo const* s_pDefaultInstance_13156786523613050927;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_13156786523613050927 = New<::SE::AssetInfo>(nullptr);

            ::SE::AssetInfo::s_pTypeInfo = New<TTypeCompositeInfo<::SE::AssetInfo>>(s_pDefaultInstance_13156786523613050927);
            Types::RegisterType(::SE::AssetInfo::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::AssetInfo::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::AssetInfo*>(s_pDefaultInstance_13156786523613050927));
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
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::IType::s_pTypeInfo;

            // Add properties
            auto pActualDefaultInstance = reinterpret_cast<::SE::AssetInfo const*>(pDefaultInstance);

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
            #ifdef SGE_DEVELOPMENT
            // Property id------------------------
            propertyInfo->id = StringID(SE_TEXT("id"));
            propertyInfo->typeID = TypeID(SE_TEXT("SE::UID"));
            propertyInfo->parentTypeID = TypeID(13156786523613050927u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("id");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->id;
            propertyInfo->offset = offsetof(::SE::AssetInfo, id);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
            #ifdef SGE_DEVELOPMENT
            // Property typeID------------------------
            propertyInfo->id = StringID(SE_TEXT("typeID"));
            propertyInfo->typeID = TypeID(SE_TEXT("SE::TypeID"));
            propertyInfo->parentTypeID = TypeID(13156786523613050927u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("typeID");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->typeID;
            propertyInfo->offset = offsetof(::SE::AssetInfo, typeID);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
            #ifdef SGE_DEVELOPMENT
            // Property path------------------------
            propertyInfo->id = StringID(SE_TEXT("path"));
            propertyInfo->typeID = TypeID(SE_TEXT("SE::String"));
            propertyInfo->parentTypeID = TypeID(13156786523613050927u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("path");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->path;
            propertyInfo->offset = offsetof(::SE::AssetInfo, path);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
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
            return s_pDefaultInstance_13156786523613050927;
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
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 628021283683842752)
            {
                
                return pType->id == pOtherType->id;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 7599593745613800568)
            {
                
                return pType->typeID == pOtherType->typeID;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 271672890340345462)
            {
                
                return pType->path == pOtherType->path;
            }
            #endif
            return false;
        }

        virtual void ResetToDefault( IType* pTypeInstance, uint32 propertyID ) const override final
        {
            auto pDefaultType = reinterpret_cast<::SE::AssetInfo const*>(s_pDefaultInstance_13156786523613050927);
            auto pActualType = reinterpret_cast<::SE::AssetInfo*>(pTypeInstance);
            ENGINE_ASSERT(pActualType != nullptr && pDefaultType != nullptr);
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 628021283683842752)
            {
                
                pActualType->id = pDefaultType->id;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 7599593745613800568)
            {
                
                pActualType->typeID = pDefaultType->typeID;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 271672890340345462)
            {
                
                pActualType->path = pDefaultType->path;
            }
            #endif
        }

        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const override
        {
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::AssetInfo>::s_pDefaultInstance_13156786523613050927);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::AssetInfo>::s_pDefaultInstance_13156786523613050927, propertyID, arrayIdx);
        }
    };

    ::SE::AssetInfo const* TTypeCompositeInfo<::SE::AssetInfo>::s_pDefaultInstance_13156786523613050927 = nullptr;
}

