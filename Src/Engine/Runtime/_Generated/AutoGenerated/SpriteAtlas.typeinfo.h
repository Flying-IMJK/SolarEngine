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
    static void InitRuntime()
    {
    }

    static void Ctor(void* ptr)
    {
        new(ptr)::SE::Sprite();
    }

    static void Dtor(void* ptr)
    {
        ((::SE::Sprite*)ptr)->~Sprite();
    }

    static void Copy(void* dst, void* src)
    {
        *(::SE::Sprite*)dst = *(::SE::Sprite*)src;
    }

    static CLRObject* Box(void* ptr)
    {
        return ::SE::CLRUtils::Box(*static_cast<::SE::Sprite*>(ptr), ::SE::Sprite::TypeInitializer.GetClass());
    }

    static void Unbox(void* ptr, CLRObject* managed)
    {
        *static_cast<::SE::Sprite*>(ptr) = ::SE::CLRUtils::Unbox<::SE::Sprite>(managed);
    }

    static void GetField(void* ptr, const String& name, Variant& value)
    {
        if (name == SE_TEXT("Name"))
        {
            value = Variant(((::SE::Sprite*)ptr)->Name);
        }
    }

    static void SetField(void* ptr, const String& name, const Variant& value)
    {
        if (name == SE_TEXT("Name"))
        {
            ((::SE::Sprite*)ptr)->Name = (StringView)value;
        }
    }
};

ScriptingTypeInitializer Sprite::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.Sprite", ARRAY_SIZE("SE.Sprite") - 1),
    sizeof(::SE::Sprite),
    &SpriteInternal::InitRuntime,
    &SpriteInternal::Ctor, &SpriteInternal::Dtor, &SpriteInternal::Copy,
    &SpriteInternal::Box, &SpriteInternal::Unbox, &SpriteInternal::GetField, &SpriteInternal::SetField,
    nullptr
);
}

namespace SE
{
class SpriteHandleInternal
{
public:
    static void InitRuntime()
    {
    }

    static void Ctor(void* ptr)
    {
        new(ptr)::SE::SpriteHandle();
    }

    static void Dtor(void* ptr)
    {
        ((::SE::SpriteHandle*)ptr)->~SpriteHandle();
    }

    static void Copy(void* dst, void* src)
    {
        *(::SE::SpriteHandle*)dst = *(::SE::SpriteHandle*)src;
    }

    static CLRObject* Box(void* ptr)
    {
        return ::SE::CLRUtils::Box(*static_cast<::SE::SpriteHandle*>(ptr), ::SE::SpriteHandle::TypeInitializer.GetClass());
    }

    static void Unbox(void* ptr, CLRObject* managed)
    {
        *static_cast<::SE::SpriteHandle*>(ptr) = ::SE::CLRUtils::Unbox<::SE::SpriteHandle>(managed);
    }

    static void GetField(void* ptr, const String& name, Variant& value)
    {
        if (name == SE_TEXT("Index"))
        {
            value = Variant(((::SE::SpriteHandle*)ptr)->Index);
        }
    }

    static void SetField(void* ptr, const String& name, const Variant& value)
    {
        if (name == SE_TEXT("Index"))
        {
            ((::SE::SpriteHandle*)ptr)->Index = (int32)value;
        }
    }
};

ScriptingTypeInitializer SpriteHandle::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.SpriteHandle", ARRAY_SIZE("SE.SpriteHandle") - 1),
    sizeof(::SE::SpriteHandle),
    &SpriteHandleInternal::InitRuntime,
    &SpriteHandleInternal::Ctor, &SpriteHandleInternal::Dtor, &SpriteHandleInternal::Copy,
    &SpriteHandleInternal::Box, &SpriteHandleInternal::Unbox, &SpriteHandleInternal::GetField, &SpriteHandleInternal::SetField,
    &::SE::ISerializable::TypeInitializer
);
}

namespace SE
{
class SpriteAtlasInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static CLRArray* Sprites_Get(::SE::SpriteAtlas* __obj, int32* __returnCount)
#else
    static CLRArray* Sprites_Get(::SE::SpriteAtlas* __obj, int32* __returnCount)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_Sprites_Get)
#endif
        if (__obj == nullptr)
        {
            if (__returnCount != nullptr) *__returnCount = 0;
            return {};
        }
        const auto& __collectionValue = __obj->Sprites;
        const int32 __collectionCount = __collectionValue.Count();
        if (__returnCount != nullptr) *__returnCount = __collectionCount;
        CLRClass* __elementClass = Scripting::FindClass(StringAnsiView("SE.Sprite"));
        if (__elementClass == nullptr) return nullptr;
        CLRArray* __result = CLRCore::Array::New(__elementClass, __collectionCount);
        if (__result == nullptr || __collectionCount == 0) return __result;
        auto* __resultItems = CLRCore::Array::GetAddress<::SE::BindingsInterop::SE_Sprite>(__result);
        for (int32 i = 0; i < __collectionCount; ++i) __resultItems[i] = BindingsInterop::ToManaged(__collectionValue[i]);
        return __result;
    }
#if defined(_MSC_VER)
    DLLEXPORT static void Sprites_Set(::SE::SpriteAtlas* __obj, CLRArray* value, int32 __valueCount)
#else
    static void Sprites_Set(::SE::SpriteAtlas* __obj, CLRArray* value, int32 __valueCount)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SpriteAtlas_Sprites_Set)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        ::SE::List<::SE::Sprite> __valueNative;
        const int32 __valueNativeCount = value ? (__valueCount < CLRCore::Array::GetLength(value) ? __valueCount : CLRCore::Array::GetLength(value)) : 0;
        __valueNative.Resize(__valueNativeCount);
        if (__valueNativeCount > 0)
        {
            auto* __valueItems = CLRCore::Array::GetAddress<::SE::BindingsInterop::SE_Sprite>(value);
            for (int32 i = 0; i < __valueNativeCount; ++i) __valueNative[i] = BindingsInterop::ToNative(__valueItems[i]);
        }
        __obj->Sprites = __valueNative;
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
DEFINE_INTERNAL_CALL(CLRArray*) SpriteAtlas_Sprites_Get(void* __obj, int32* __returnCount)
{
    return SpriteAtlasInternal::Sprites_Get((::SE::SpriteAtlas*)__obj, __returnCount);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SpriteAtlas_Sprites_Set(void* __obj, CLRArray* value, int32 __valueCount)
{
    SpriteAtlasInternal::Sprites_Set((::SE::SpriteAtlas*)__obj, value, __valueCount);
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

