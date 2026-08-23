#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE SpriteAtlas
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::SpriteAtlas> final : public TypeCompositeInfo
    {
       static ::SE::SpriteAtlas const* s_pDefaultInstance_2238738421091615515;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_2238738421091615515 = New<::SE::SpriteAtlas>(nullptr);

            ::SE::SpriteAtlas::s_pTypeInfo = New<TTypeCompositeInfo<::SE::SpriteAtlas>>(s_pDefaultInstance_2238738421091615515);
            Types::RegisterType(::SE::SpriteAtlas::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::SpriteAtlas::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::SpriteAtlas*>(s_pDefaultInstance_2238738421091615515));
            Delete(const_cast<TypeCompositeInfo*>(::SE::SpriteAtlas::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::SpriteAtlas"));
            size = sizeof(::SE::SpriteAtlas);
            alignment = alignof(::SE::SpriteAtlas);
            name = SE_TEXT("SpriteAtlas");
            fullName = SE_TEXT("SE::SpriteAtlas");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::TextureBase::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::SpriteAtlas), alignof(::SE::SpriteAtlas));
            return new (pMemory) ::SE::SpriteAtlas(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::SpriteAtlas(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_2238738421091615515;
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
            
            return true;
        }


        virtual bool IsPropertyValueEqual(IType const* pTypeInstance, IType const* pOtherTypeInstance, uint32 propertyID, int32 arrayIdx = -1) const override final
        {
            
            return false;
        }

        virtual void ResetToDefault( IType* pTypeInstance, uint32 propertyID ) const override final
        {
            
        }

        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const override
        {
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::SpriteAtlas>::s_pDefaultInstance_2238738421091615515);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::SpriteAtlas>::s_pDefaultInstance_2238738421091615515, propertyID, arrayIdx);
        }
    };

    ::SE::SpriteAtlas const* TTypeCompositeInfo<::SE::SpriteAtlas>::s_pDefaultInstance_2238738421091615515 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/SpriteAtlas.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/SpriteAtlas.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class SpriteInternal
{
public:
};

}

namespace SE
{
class SpriteHandleInternal
{
public:
};

}

namespace SE
{
class SpriteAtlasInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static int32 Sprites_Get(::SE::SpriteAtlas* __obj)
#else
    static int32 Sprites_Get(::SE::SpriteAtlas* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_Sprites_Get)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->Sprites;
    }
#if defined(_MSC_VER)
    DLLEXPORT static void Sprites_Set(::SE::SpriteAtlas* __obj, int32 value)
#else
    static void Sprites_Set(::SE::SpriteAtlas* __obj, int32 value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_Sprites_Set)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->Sprites = value;
    }
#if defined(_MSC_VER)
    DLLEXPORT static int32 GetSpritesCount(::SE::SpriteAtlas* __obj)
#else
    static int32 GetSpritesCount(::SE::SpriteAtlas* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_GetSpritesCount)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->GetSpritesCount();
    }
#if defined(_MSC_VER)
    DLLEXPORT static void GetSprite(::SE::SpriteAtlas* __obj, int32 index, ::SE::BindingsInterop::SE_Sprite* __resultAsRef)
#else
    static void GetSprite(::SE::SpriteAtlas* __obj, int32 index, ::SE::BindingsInterop::SE_Sprite* __resultAsRef)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_GetSprite)
#endif
        if (__obj == nullptr)
        {
            if (__resultAsRef != nullptr) *__resultAsRef = {};
            return;
        }
        *__resultAsRef = BindingsInterop::ToManaged(__obj->GetSprite(index));
    }
#if defined(_MSC_VER)
    DLLEXPORT static void GetSpriteArea(::SE::SpriteAtlas* __obj, int32 index, ::SE::Rectangle* result)
#else
    static void GetSpriteArea(::SE::SpriteAtlas* __obj, int32 index, ::SE::Rectangle* result)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_GetSpriteArea)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->GetSpriteArea(index, *result);
    }
#if defined(_MSC_VER)
    DLLEXPORT static void SetSprite(::SE::SpriteAtlas* __obj, int32 index, ::SE::BindingsInterop::SE_Sprite* value)
#else
    static void SetSprite(::SE::SpriteAtlas* __obj, int32 index, ::SE::BindingsInterop::SE_Sprite* value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_SetSprite)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->SetSprite(index, BindingsInterop::ToNative(*value));
    }
#if defined(_MSC_VER)
    DLLEXPORT static void FindSprite(::SE::SpriteAtlas* __obj, CLRString** name, ::SE::BindingsInterop::SE_SpriteHandle* __resultAsRef)
#else
    static void FindSprite(::SE::SpriteAtlas* __obj, CLRString** name, ::SE::BindingsInterop::SE_SpriteHandle* __resultAsRef)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_FindSprite)
#endif
        if (__obj == nullptr)
        {
            if (__resultAsRef != nullptr) *__resultAsRef = {};
            return;
        }
        *__resultAsRef = BindingsInterop::ToManaged(__obj->FindSprite(CLRUtils::ToString((CLRString*)*name)));
    }
#if defined(_MSC_VER)
    DLLEXPORT static void AddSprite(::SE::SpriteAtlas* __obj, ::SE::BindingsInterop::SE_Sprite* sprite, ::SE::BindingsInterop::SE_SpriteHandle* __resultAsRef)
#else
    static void AddSprite(::SE::SpriteAtlas* __obj, ::SE::BindingsInterop::SE_Sprite* sprite, ::SE::BindingsInterop::SE_SpriteHandle* __resultAsRef)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_AddSprite)
#endif
        if (__obj == nullptr)
        {
            if (__resultAsRef != nullptr) *__resultAsRef = {};
            return;
        }
        *__resultAsRef = BindingsInterop::ToManaged(__obj->AddSprite(BindingsInterop::ToNative(*sprite)));
    }
#if defined(_MSC_VER)
    DLLEXPORT static void RemoveSprite(::SE::SpriteAtlas* __obj, int32 index)
#else
    static void RemoveSprite(::SE::SpriteAtlas* __obj, int32 index)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_RemoveSprite)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->RemoveSprite(index);
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer SpriteAtlas::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.SpriteAtlas", ARRAY_SIZE("SE.SpriteAtlas") - 1),
    sizeof(::SE::SpriteAtlas),
    &SpriteAtlasInternal::InitRuntime,
    &ScriptingType::DefaultSpawn, 
    &::SE::TextureBase::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(int32) SpriteAtlas_Sprites_Get(void* __obj)
{
    return SpriteAtlasInternal::Sprites_Get((::SE::SpriteAtlas*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_Sprites_Set(void* __obj, int32 value)
{
    SpriteAtlasInternal::Sprites_Set((::SE::SpriteAtlas*)__obj, value);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(int32) SpriteAtlas_GetSpritesCount(void* __obj)
{
    return SpriteAtlasInternal::GetSpritesCount((::SE::SpriteAtlas*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_GetSprite(void* __obj, int32 index, ::SE::BindingsInterop::SE_Sprite* __resultAsRef)
{
    SpriteAtlasInternal::GetSprite((::SE::SpriteAtlas*)__obj, index, __resultAsRef);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_GetSpriteArea(void* __obj, int32 index, ::SE::Rectangle* result)
{
    SpriteAtlasInternal::GetSpriteArea((::SE::SpriteAtlas*)__obj, index, result);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_SetSprite(void* __obj, int32 index, ::SE::BindingsInterop::SE_Sprite* value)
{
    SpriteAtlasInternal::SetSprite((::SE::SpriteAtlas*)__obj, index, value);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_FindSprite(void* __obj, CLRString** name, ::SE::BindingsInterop::SE_SpriteHandle* __resultAsRef)
{
    SpriteAtlasInternal::FindSprite((::SE::SpriteAtlas*)__obj, name, __resultAsRef);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_AddSprite(void* __obj, ::SE::BindingsInterop::SE_Sprite* sprite, ::SE::BindingsInterop::SE_SpriteHandle* __resultAsRef)
{
    SpriteAtlasInternal::AddSprite((::SE::SpriteAtlas*)__obj, sprite, __resultAsRef);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_RemoveSprite(void* __obj, int32 index)
{
    SpriteAtlasInternal::RemoveSprite((::SE::SpriteAtlas*)__obj, index);
}
#endif
}

