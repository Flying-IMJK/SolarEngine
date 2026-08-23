#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/FontAsset.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE FontAsset
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::FontAsset> final : public TypeCompositeInfo
    {
       static ::SE::FontAsset const* s_pDefaultInstance_3021435367524448816;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_3021435367524448816 = New<::SE::FontAsset>(nullptr);

            ::SE::FontAsset::s_pTypeInfo = New<TTypeCompositeInfo<::SE::FontAsset>>(s_pDefaultInstance_3021435367524448816);
            Types::RegisterType(::SE::FontAsset::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::FontAsset::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::FontAsset*>(s_pDefaultInstance_3021435367524448816));
            Delete(const_cast<TypeCompositeInfo*>(::SE::FontAsset::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::FontAsset"));
            size = sizeof(::SE::FontAsset);
            alignment = alignof(::SE::FontAsset);
            name = SE_TEXT("FontAsset");
            fullName = SE_TEXT("SE::FontAsset");

            // Add type metadata
            isAbstract = false;


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
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::FontAsset), alignof(::SE::FontAsset));
            return new (pMemory) ::SE::FontAsset(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::FontAsset(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_3021435367524448816;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::FontAsset>::s_pDefaultInstance_3021435367524448816);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::FontAsset>::s_pDefaultInstance_3021435367524448816, propertyID, arrayIdx);
        }
    };

    ::SE::FontAsset const* TTypeCompositeInfo<::SE::FontAsset>::s_pDefaultInstance_3021435367524448816 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/FontAsset.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/FontAsset.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class FontAssetInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static void* CreateFont(::SE::FontAsset* __obj, float size)
#else
    static void* CreateFont(::SE::FontAsset* __obj, float size)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(FontAsset_CreateFont)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>(__obj->CreateFont(size)));
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer FontAsset::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.FontAsset", ARRAY_SIZE("SE.FontAsset") - 1),
    sizeof(::SE::FontAsset),
    &FontAssetInternal::InitRuntime,
    &ScriptingType::DefaultSpawn, 
    &::SE::BinaryAsset::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(void*) FontAsset_CreateFont(void* __obj, float size)
{
    return FontAssetInternal::CreateFont((::SE::FontAsset*)__obj, size);
}
#endif
}

