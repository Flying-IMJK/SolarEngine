#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actors/PointLight.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE PointLight
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::PointLight> final : public TypeCompositeInfo
    {
       static ::SE::PointLight const* s_pDefaultInstance_6906376467442758251;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_6906376467442758251 = New<::SE::PointLight>(nullptr);

            ::SE::PointLight::s_pTypeInfo = New<TTypeCompositeInfo<::SE::PointLight>>(s_pDefaultInstance_6906376467442758251);
            Types::RegisterType(::SE::PointLight::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::PointLight::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::PointLight*>(s_pDefaultInstance_6906376467442758251));
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
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::LightWithShadow::s_pTypeInfo;

            // Add properties
            auto pActualDefaultInstance = reinterpret_cast<::SE::PointLight const*>(pDefaultInstance);

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
            #ifdef SGE_DEVELOPMENT
            // Property SourceRadius------------------------
            propertyInfo->id = StringID(SE_TEXT("SourceRadius"));
            propertyInfo->typeID = TypeID(SE_TEXT("float"));
            propertyInfo->parentTypeID = TypeID(6906376467442758251u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("SourceRadius");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->SourceRadius;
            propertyInfo->offset = offsetof(::SE::PointLight, SourceRadius);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
            #ifdef SGE_DEVELOPMENT
            // Property SourceLength------------------------
            propertyInfo->id = StringID(SE_TEXT("SourceLength"));
            propertyInfo->typeID = TypeID(SE_TEXT("float"));
            propertyInfo->parentTypeID = TypeID(6906376467442758251u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("SourceLength");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->SourceLength;
            propertyInfo->offset = offsetof(::SE::PointLight, SourceLength);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
            #ifdef SGE_DEVELOPMENT
            // Property UseInverseSquaredFalloff------------------------
            propertyInfo->id = StringID(SE_TEXT("UseInverseSquaredFalloff"));
            propertyInfo->typeID = TypeID(SE_TEXT("bool"));
            propertyInfo->parentTypeID = TypeID(6906376467442758251u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("UseInverseSquaredFalloff");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->UseInverseSquaredFalloff;
            propertyInfo->offset = offsetof(::SE::PointLight, UseInverseSquaredFalloff);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
            #ifdef SGE_DEVELOPMENT
            // Property FallOffExponent------------------------
            propertyInfo->id = StringID(SE_TEXT("FallOffExponent"));
            propertyInfo->typeID = TypeID(SE_TEXT("float"));
            propertyInfo->parentTypeID = TypeID(6906376467442758251u);
            propertyInfo->templateArgumentTypeID = TypeID(SE_TEXT(""));
            propertyInfo->name = SE_TEXT("FallOffExponent");
            // Meta
            

            // Abstract types cannot have default values since they cannot be instantiated
            propertyInfo->pDefaultValue = &pActualDefaultInstance->FallOffExponent;
            propertyInfo->offset = offsetof(::SE::PointLight, FallOffExponent);

            
            
            
            propertyInfo->flags |= 0;
            properties.Add(propertyInfo);
            propertyMap.Add(propertyInfo->id, properties.Count() - 1);
            
            #endif
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
            return s_pDefaultInstance_6906376467442758251;
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
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 1282706862554350830)
            {
                
                return pType->SourceRadius == pOtherType->SourceRadius;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 2496286691009166980)
            {
                
                return pType->SourceLength == pOtherType->SourceLength;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 10182564000037131393)
            {
                
                return pType->UseInverseSquaredFalloff == pOtherType->UseInverseSquaredFalloff;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 10682711793280404038)
            {
                
                return pType->FallOffExponent == pOtherType->FallOffExponent;
            }
            #endif
            return false;
        }

        virtual void ResetToDefault( IType* pTypeInstance, uint32 propertyID ) const override final
        {
            auto pDefaultType = reinterpret_cast<::SE::PointLight const*>(s_pDefaultInstance_6906376467442758251);
            auto pActualType = reinterpret_cast<::SE::PointLight*>(pTypeInstance);
            ENGINE_ASSERT(pActualType != nullptr && pDefaultType != nullptr);
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 1282706862554350830)
            {
                
                pActualType->SourceRadius = pDefaultType->SourceRadius;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 2496286691009166980)
            {
                
                pActualType->SourceLength = pDefaultType->SourceLength;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 10182564000037131393)
            {
                
                pActualType->UseInverseSquaredFalloff = pDefaultType->UseInverseSquaredFalloff;
            }
            #endif
            #ifdef SGE_DEVELOPMENT
            if (propertyID == 10682711793280404038)
            {
                
                pActualType->FallOffExponent = pDefaultType->FallOffExponent;
            }
            #endif
        }

        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const override
        {
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::PointLight>::s_pDefaultInstance_6906376467442758251);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::PointLight>::s_pDefaultInstance_6906376467442758251, propertyID, arrayIdx);
        }
    };

    ::SE::PointLight const* TTypeCompositeInfo<::SE::PointLight>::s_pDefaultInstance_6906376467442758251 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actors/PointLight.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actors/PointLight.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class PointLightInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static float SourceRadius_Get(::SE::PointLight* __obj)
#else
    static float SourceRadius_Get(::SE::PointLight* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_SourceRadius_Get)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->SourceRadius;
    }
#if defined(_MSC_VER)
    DLLEXPORT static void SourceRadius_Set(::SE::PointLight* __obj, float value)
#else
    static void SourceRadius_Set(::SE::PointLight* __obj, float value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_SourceRadius_Set)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->SourceRadius = value;
    }
#if defined(_MSC_VER)
    DLLEXPORT static float SourceLength_Get(::SE::PointLight* __obj)
#else
    static float SourceLength_Get(::SE::PointLight* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_SourceLength_Get)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->SourceLength;
    }
#if defined(_MSC_VER)
    DLLEXPORT static void SourceLength_Set(::SE::PointLight* __obj, float value)
#else
    static void SourceLength_Set(::SE::PointLight* __obj, float value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_SourceLength_Set)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->SourceLength = value;
    }
#if defined(_MSC_VER)
    DLLEXPORT static bool UseInverseSquaredFalloff_Get(::SE::PointLight* __obj)
#else
    static bool UseInverseSquaredFalloff_Get(::SE::PointLight* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_UseInverseSquaredFalloff_Get)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->UseInverseSquaredFalloff;
    }
#if defined(_MSC_VER)
    DLLEXPORT static void UseInverseSquaredFalloff_Set(::SE::PointLight* __obj, bool value)
#else
    static void UseInverseSquaredFalloff_Set(::SE::PointLight* __obj, bool value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_UseInverseSquaredFalloff_Set)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->UseInverseSquaredFalloff = value;
    }
#if defined(_MSC_VER)
    DLLEXPORT static float FallOffExponent_Get(::SE::PointLight* __obj)
#else
    static float FallOffExponent_Get(::SE::PointLight* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_FallOffExponent_Get)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->FallOffExponent;
    }
#if defined(_MSC_VER)
    DLLEXPORT static void FallOffExponent_Set(::SE::PointLight* __obj, float value)
#else
    static void FallOffExponent_Set(::SE::PointLight* __obj, float value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(PointLight_FallOffExponent_Set)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->FallOffExponent = value;
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer PointLight::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.PointLight", ARRAY_SIZE("SE.PointLight") - 1),
    sizeof(::SE::PointLight),
    &PointLightInternal::InitRuntime,
    (ScriptingType::SpawnHandler)&::SE::PointLight::Spawn,
    &::SE::LightWithShadow::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(float) PointLight_SourceRadius_Get(void* __obj)
{
    return PointLightInternal::SourceRadius_Get((::SE::PointLight*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) PointLight_SourceRadius_Set(void* __obj, float value)
{
    PointLightInternal::SourceRadius_Set((::SE::PointLight*)__obj, value);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(float) PointLight_SourceLength_Get(void* __obj)
{
    return PointLightInternal::SourceLength_Get((::SE::PointLight*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) PointLight_SourceLength_Set(void* __obj, float value)
{
    PointLightInternal::SourceLength_Set((::SE::PointLight*)__obj, value);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(bool) PointLight_UseInverseSquaredFalloff_Get(void* __obj)
{
    return PointLightInternal::UseInverseSquaredFalloff_Get((::SE::PointLight*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) PointLight_UseInverseSquaredFalloff_Set(void* __obj, bool value)
{
    PointLightInternal::UseInverseSquaredFalloff_Set((::SE::PointLight*)__obj, value);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(float) PointLight_FallOffExponent_Get(void* __obj)
{
    return PointLightInternal::FallOffExponent_Get((::SE::PointLight*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) PointLight_FallOffExponent_Set(void* __obj, float value)
{
    PointLightInternal::FallOffExponent_Set((::SE::PointLight*)__obj, value);
}
#endif
}

