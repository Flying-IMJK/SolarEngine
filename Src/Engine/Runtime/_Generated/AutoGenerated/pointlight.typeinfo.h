#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/level/actors/pointlight.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE:: PointLight
//-------------------------------------------------------------------------

#include "Core/Serialization/Json.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Metadata/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::PointLight> final : public TypeCompositeInfo
    {
       static ::SE::PointLight const* s_pDefaultInstance_3371449715;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_3371449715 = New<::SE::PointLight>(nullptr);

            ::SE::PointLight::s_pTypeInfo = New<TTypeCompositeInfo<::SE::PointLight>>(s_pDefaultInstance_3371449715);
            Types::RegisterType(::SE::PointLight::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::PointLight::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::PointLight*>(s_pDefaultInstance_3371449715));
            Delete(const_cast<TypeCompositeInfo*>(::SE::PointLight::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::PointLight"));
            size = sizeof(::SE::PointLight);
            alignment = alignof(::SE::PointLight);
            name = SE_TEXT("PointLight");
            fullName = SE_TEXT("SE::PointLight");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::LightWithShadow::s_pTypeInfo;

            // Add properties
            auto pActualDefaultInstance = reinterpret_cast<::SE::PointLight const*>(pDefaultInstance);

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
            // Property SourceRadius------------------------
            propertyInfo->id = StringID(SE_TEXT("SourceRadius"));
            propertyInfo->typeID = TypeID(SE_TEXT("float"));
            propertyInfo->parentTypeID = TypeID(3371449715u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("SourceRadius");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->SourceRadius;
            propertyInfo->offset = offsetof(::SE::PointLight, SourceRadius);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            // Property SourceLength------------------------
            propertyInfo->id = StringID(SE_TEXT("SourceLength"));
            propertyInfo->typeID = TypeID(SE_TEXT("float"));
            propertyInfo->parentTypeID = TypeID(3371449715u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("SourceLength");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->SourceLength;
            propertyInfo->offset = offsetof(::SE::PointLight, SourceLength);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            // Property UseInverseSquaredFalloff------------------------
            propertyInfo->id = StringID(SE_TEXT("UseInverseSquaredFalloff"));
            propertyInfo->typeID = TypeID(SE_TEXT("bool"));
            propertyInfo->parentTypeID = TypeID(3371449715u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("UseInverseSquaredFalloff");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->UseInverseSquaredFalloff;
            propertyInfo->offset = offsetof(::SE::PointLight, UseInverseSquaredFalloff);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            // Property FallOffExponent------------------------
            propertyInfo->id = StringID(SE_TEXT("FallOffExponent"));
            propertyInfo->typeID = TypeID(SE_TEXT("float"));
            propertyInfo->parentTypeID = TypeID(3371449715u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("FallOffExponent");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->FallOffExponent;
            propertyInfo->offset = offsetof(::SE::PointLight, FallOffExponent);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::PointLight), alignof(::SE::PointLight));
            return new (pMemory) ::SE::PointLight(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::PointLight(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_3371449715;
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
            auto pType = reinterpret_cast<::SE::PointLight const*>(pTypeInstance);
            auto pOtherType = reinterpret_cast<::SE::PointLight const*>(pOtherTypeInstance);
            return true;
        }


        virtual bool IsPropertyValueEqual(IType const* pTypeInstance, IType const* pOtherTypeInstance, uint32 propertyID, int32 arrayIdx = -1) const override final
        {
            auto pType = reinterpret_cast<::SE::PointLight const*>(pTypeInstance);
            auto pOtherType = reinterpret_cast<::SE::PointLight const*>(pOtherTypeInstance);
            if (propertyID == 1166378078)
            {
                
                return pType->SourceRadius == pOtherType->SourceRadius;
            }
            if (propertyID == 936666552)
            {
                
                return pType->SourceLength == pOtherType->SourceLength;
            }
            if (propertyID == 4195805949)
            {
                
                return pType->UseInverseSquaredFalloff == pOtherType->UseInverseSquaredFalloff;
            }
            if (propertyID == 3532688016)
            {
                
                return pType->FallOffExponent == pOtherType->FallOffExponent;
            }
            return false;
        }

        virtual void ResetToDefault( IType* pTypeInstance, uint32 propertyID ) const override final
        {
            auto pDefaultType = reinterpret_cast<::SE::PointLight const*>(s_pDefaultInstance_3371449715);
            auto pActualType = reinterpret_cast<::SE::PointLight*>(pTypeInstance);
            ENGINE_ASSERT(pActualType != nullptr && pDefaultType != nullptr);
            if (propertyID == 1166378078)
            {
                
                pActualType->SourceRadius = pDefaultType->SourceRadius;
            }
            if (propertyID == 936666552)
            {
                
                pActualType->SourceLength = pDefaultType->SourceLength;
            }
            if (propertyID == 4195805949)
            {
                
                pActualType->UseInverseSquaredFalloff = pDefaultType->UseInverseSquaredFalloff;
            }
            if (propertyID == 3532688016)
            {
                
                pActualType->FallOffExponent = pDefaultType->FallOffExponent;
            }
        }

        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const override
        {
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::PointLight>::s_pDefaultInstance_3371449715);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::PointLight>::s_pDefaultInstance_3371449715, propertyID, arrayIdx);
        }
    };

    ::SE::PointLight const* TTypeCompositeInfo<::SE::PointLight>::s_pDefaultInstance_3371449715 = nullptr;
}

