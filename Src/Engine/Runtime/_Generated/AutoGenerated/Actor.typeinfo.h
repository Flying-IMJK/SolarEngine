#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actor.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE Actor
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Actor> final : public TypeCompositeInfo
    {
       static ::SE::Actor const* s_pDefaultInstance_8411106991848770634;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_8411106991848770634 = New<::SE::Actor>(nullptr);

            ::SE::Actor::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Actor>>(s_pDefaultInstance_8411106991848770634);
            Types::RegisterType(::SE::Actor::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Actor::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Actor*>(s_pDefaultInstance_8411106991848770634));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Actor::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Actor"));
            size = sizeof(::SE::Actor);
            alignment = alignof(::SE::Actor);
            name = SE_TEXT("Actor");
            fullName = SE_TEXT("SE::Actor");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::SceneObject::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Actor), alignof(::SE::Actor));
            return new (pMemory) ::SE::Actor(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Actor(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_8411106991848770634;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Actor>::s_pDefaultInstance_8411106991848770634);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Actor>::s_pDefaultInstance_8411106991848770634, propertyID, arrayIdx);
        }
    };

    ::SE::Actor const* TTypeCompositeInfo<::SE::Actor>::s_pDefaultInstance_8411106991848770634 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE RenderActor
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::RenderActor> final : public TypeCompositeInfo
    {
       static ::SE::RenderActor const* s_pDefaultInstance_9862484587083810066;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_9862484587083810066 = New<::SE::RenderActor>(nullptr);

            ::SE::RenderActor::s_pTypeInfo = New<TTypeCompositeInfo<::SE::RenderActor>>(s_pDefaultInstance_9862484587083810066);
            Types::RegisterType(::SE::RenderActor::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::RenderActor::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::RenderActor*>(s_pDefaultInstance_9862484587083810066));
            Delete(const_cast<TypeCompositeInfo*>(::SE::RenderActor::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::RenderActor"));
            size = sizeof(::SE::RenderActor);
            alignment = alignof(::SE::RenderActor);
            name = SE_TEXT("RenderActor");
            fullName = SE_TEXT("SE::RenderActor");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Actor::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::RenderActor), alignof(::SE::RenderActor));
            return new (pMemory) ::SE::RenderActor(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::RenderActor(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_9862484587083810066;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::RenderActor>::s_pDefaultInstance_9862484587083810066);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::RenderActor>::s_pDefaultInstance_9862484587083810066, propertyID, arrayIdx);
        }
    };

    ::SE::RenderActor const* TTypeCompositeInfo<::SE::RenderActor>::s_pDefaultInstance_9862484587083810066 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actor.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actor.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class ActorInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static CLRString* GetName(::SE::Actor* __obj)
#else
    static CLRString* GetName(::SE::Actor* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Actor_GetName)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return CLRUtils::ToString(__obj->GetName());
    }
#if defined(_MSC_VER)
    DLLEXPORT static void SetName(::SE::Actor* __obj, CLRString* value)
#else
    static void SetName(::SE::Actor* __obj, CLRString* value)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Actor_SetName)
#endif
        if (__obj == nullptr)
        {
            return;
        }
        __obj->SetName(CLRUtils::ToString((CLRString*)value));
    }
#if defined(_MSC_VER)
    DLLEXPORT static void* GetScene(::SE::Actor* __obj)
#else
    static void* GetScene(::SE::Actor* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Actor_GetScene)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>(__obj->GetScene()));
    }
#if defined(_MSC_VER)
    DLLEXPORT static int32 GetChildrenCount(::SE::Actor* __obj)
#else
    static int32 GetChildrenCount(::SE::Actor* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Actor_GetChildrenCount)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->GetChildrenCount();
    }
#if defined(_MSC_VER)
    DLLEXPORT static void* GetChild(::SE::Actor* __obj, int32 index)
#else
    static void* GetChild(::SE::Actor* __obj, int32 index)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Actor_GetChild)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>(__obj->GetChild(index)));
    }
#if defined(_MSC_VER)
    DLLEXPORT static bool GetIsActive(::SE::Actor* __obj)
#else
    static bool GetIsActive(::SE::Actor* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Actor_GetIsActive)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->GetIsActive();
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer Actor::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.Actor", ARRAY_SIZE("SE.Actor") - 1),
    sizeof(::SE::Actor),
    &ActorInternal::InitRuntime,
    (ScriptingType::SpawnHandler)&::SE::Actor::Spawn,
    &::SE::SceneObject::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(CLRString*) Actor_GetName(void* __obj)
{
    return ActorInternal::GetName((::SE::Actor*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) Actor_SetName(void* __obj, CLRString* value)
{
    ActorInternal::SetName((::SE::Actor*)__obj, value);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void*) Actor_GetScene(void* __obj)
{
    return ActorInternal::GetScene((::SE::Actor*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(int32) Actor_GetChildrenCount(void* __obj)
{
    return ActorInternal::GetChildrenCount((::SE::Actor*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void*) Actor_GetChild(void* __obj, int32 index)
{
    return ActorInternal::GetChild((::SE::Actor*)__obj, index);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(bool) Actor_GetIsActive(void* __obj)
{
    return ActorInternal::GetIsActive((::SE::Actor*)__obj);
}
#endif
}

namespace SE
{
class RenderActorInternal
{
public:
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer RenderActor::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.RenderActor", ARRAY_SIZE("SE.RenderActor") - 1),
    sizeof(::SE::RenderActor),
    &RenderActorInternal::InitRuntime,
    (ScriptingType::SpawnHandler)&::SE::RenderActor::Spawn,
    &::SE::Actor::TypeInitializer,
    nullptr,
    nullptr
);
}

