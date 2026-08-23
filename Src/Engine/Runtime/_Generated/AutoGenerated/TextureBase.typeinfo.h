#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/Assets/Texture/TextureBase.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE TextureBase
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::TextureBase> final : public TypeCompositeInfo
    {
       static ::SE::TextureBase const* s_pDefaultInstance_16334306234790724625;

    public:
        static void RegisterType()
        {
            

            ::SE::TextureBase::s_pTypeInfo = New<TTypeCompositeInfo<::SE::TextureBase>>(s_pDefaultInstance_16334306234790724625);
            Types::RegisterType(::SE::TextureBase::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::TextureBase::s_pTypeInfo);
            
            Delete(const_cast<TypeCompositeInfo*>(::SE::TextureBase::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::TextureBase"));
            size = sizeof(::SE::TextureBase);
            alignment = alignof(::SE::TextureBase);
            name = SE_TEXT("TextureBase");
            fullName = SE_TEXT("SE::TextureBase");

            // Add type metadata
            isAbstract = true;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::BinaryAsset::s_pTypeInfo;

            

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
            return s_pDefaultInstance_16334306234790724625;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::TextureBase>::s_pDefaultInstance_16334306234790724625);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::TextureBase>::s_pDefaultInstance_16334306234790724625, propertyID, arrayIdx);
        }
    };

    ::SE::TextureBase const* TTypeCompositeInfo<::SE::TextureBase>::s_pDefaultInstance_16334306234790724625 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/Assets/Texture/TextureBase.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/Assets/Texture/TextureBase.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class TextureBaseInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static int32 Width(::SE::TextureBase* __obj)
#else
    static int32 Width(::SE::TextureBase* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(TextureBase_Width)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->Width();
    }
#if defined(_MSC_VER)
    DLLEXPORT static int32 Height(::SE::TextureBase* __obj)
#else
    static int32 Height(::SE::TextureBase* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(TextureBase_Height)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->Height();
    }
#if defined(_MSC_VER)
    DLLEXPORT static ::SE::Float2 Size(::SE::TextureBase* __obj)
#else
    static ::SE::Float2 Size(::SE::TextureBase* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(TextureBase_Size)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->Size();
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer TextureBase::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.TextureBase", ARRAY_SIZE("SE.TextureBase") - 1),
    sizeof(::SE::TextureBase),
    &TextureBaseInternal::InitRuntime,
    &ScriptingType::DefaultSpawn, 
    &::SE::BinaryAsset::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(int32) TextureBase_Width(void* __obj)
{
    return TextureBaseInternal::Width((::SE::TextureBase*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(int32) TextureBase_Height(void* __obj)
{
    return TextureBaseInternal::Height((::SE::TextureBase*)__obj);
}
#endif
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(::SE::Float2) TextureBase_Size(void* __obj)
{
    return TextureBaseInternal::Size((::SE::TextureBase*)__obj);
}
#endif
}

