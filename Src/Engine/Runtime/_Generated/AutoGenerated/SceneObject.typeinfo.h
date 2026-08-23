#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/SceneObject.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE SceneObject
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::SceneObject> final : public TypeCompositeInfo
    {
       static ::SE::SceneObject const* s_pDefaultInstance_9254904937204525784;

    public:
        static void RegisterType()
        {
            

            ::SE::SceneObject::s_pTypeInfo = New<TTypeCompositeInfo<::SE::SceneObject>>(s_pDefaultInstance_9254904937204525784);
            Types::RegisterType(::SE::SceneObject::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::SceneObject::s_pTypeInfo);
            
            Delete(const_cast<TypeCompositeInfo*>(::SE::SceneObject::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::SceneObject"));
            size = sizeof(::SE::SceneObject);
            alignment = alignof(::SE::SceneObject);
            name = SE_TEXT("SceneObject");
            fullName = SE_TEXT("SE::SceneObject");

            // Add type metadata
            isAbstract = true;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::ScriptingObject::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            ENGINE_UNREACHABLE_CODE();
            return nullptr;
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_UNREACHABLE_CODE(); // Error! Trying to instantiate an abstract type!
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_9254904937204525784;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::SceneObject>::s_pDefaultInstance_9254904937204525784);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::SceneObject>::s_pDefaultInstance_9254904937204525784, propertyID, arrayIdx);
        }
    };

    ::SE::SceneObject const* TTypeCompositeInfo<::SE::SceneObject>::s_pDefaultInstance_9254904937204525784 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/SceneObject.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/SceneObject.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class SceneObjectInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static void* GetParent(::SE::SceneObject* __obj)
#else
    static void* GetParent(::SE::SceneObject* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SceneObject_GetParent)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>(__obj->GetParent()));
    }
#if defined(_MSC_VER)
    DLLEXPORT static void GetSceneObjectId(::SE::SceneObject* __obj, ::SE::UID* __resultAsRef)
#else
    static void GetSceneObjectId(::SE::SceneObject* __obj, ::SE::UID* __resultAsRef)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(SceneObject_GetSceneObjectId)
#endif
        if (__obj == nullptr)
        {
            if (__resultAsRef != nullptr) *__resultAsRef = {};
            return;
        }
        *__resultAsRef = __obj->GetSceneObjectId();
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer SceneObject::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.SceneObject", ARRAY_SIZE("SE.SceneObject") - 1),
    sizeof(::SE::SceneObject),
    &SceneObjectInternal::InitRuntime,
    &ScriptingType::DefaultSpawn, 
    &::SE::ScriptingObject::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void*) SceneObject_GetParent(void* __obj)
{
    return SceneObjectInternal::GetParent((::SE::SceneObject*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void) SceneObject_GetSceneObjectId(void* __obj, ::SE::UID* __resultAsRef)
{
    SceneObjectInternal::GetSceneObjectId((::SE::SceneObject*)__obj, __resultAsRef);
}
#endif
}

