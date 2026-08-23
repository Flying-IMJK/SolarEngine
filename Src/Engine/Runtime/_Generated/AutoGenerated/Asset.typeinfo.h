#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Resource/Asset.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"
//-------------------------------------------------------------------------
// Enum Info: SE::Asset LoadResult
//-------------------------------------------------------------------------

#include "Runtime/Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::Asset::LoadResult> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::Asset::LoadResult> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::Asset::LoadResult>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Asset::LoadResult"));
            size = sizeof(::SE::Asset::LoadResult);
            alignment = alignof(::SE::Asset::LoadResult);
            name = SE_TEXT("LoadResult");
            fullName = SE_TEXT("SE::Asset::LoadResult");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Ok"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Failed"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MissingDataChunk"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CannotLoadData"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CannotLoadStorage"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CannotLoadInitData"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("InvalidData"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::Asset::LoadResult> * TTypeEnumInfo<::SE::Asset::LoadResult>::s_pTypeInfo = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE Asset
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Asset> final : public TypeCompositeInfo
    {
       static ::SE::Asset const* s_pDefaultInstance_16330907373871644011;

    public:
        static void RegisterType()
        {
            

            ::SE::Asset::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Asset>>(s_pDefaultInstance_16330907373871644011);
            Types::RegisterType(::SE::Asset::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Asset::s_pTypeInfo);
            
            Delete(const_cast<TypeCompositeInfo*>(::SE::Asset::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Asset"));
            size = sizeof(::SE::Asset);
            alignment = alignof(::SE::Asset);
            name = SE_TEXT("Asset");
            fullName = SE_TEXT("SE::Asset");

            // Add type metadata
            isAbstract = true;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::ManagedScriptingObject::s_pTypeInfo;

            

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
            return s_pDefaultInstance_16330907373871644011;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Asset>::s_pDefaultInstance_16330907373871644011);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Asset>::s_pDefaultInstance_16330907373871644011, propertyID, arrayIdx);
        }
    };

    ::SE::Asset const* TTypeCompositeInfo<::SE::Asset>::s_pDefaultInstance_16330907373871644011 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Resource/Asset.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Resource/Asset.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class AssetInternal
{
public:
#if defined(_MSC_VER)
    DLLEXPORT static int32 GetReferencesCount(::SE::Asset* __obj)
#else
    static int32 GetReferencesCount(::SE::Asset* __obj)
#endif
    {
#if defined(_MSC_VER)
        MSVC_FUNC_EXPORT(Asset_GetReferencesCount)
#endif
        if (__obj == nullptr)
        {
            return {};
        }
        return __obj->GetReferencesCount();
    }
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer Asset::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.Asset", ARRAY_SIZE("SE.Asset") - 1),
    sizeof(::SE::Asset),
    &AssetInternal::InitRuntime,
    &ScriptingType::DefaultSpawn, 
    &::SE::ManagedScriptingObject::TypeInitializer,
    nullptr,
    nullptr
);

// Plain-C exports
#if !defined(_MSC_VER)
DEFINE_INTERNAL_CALL(int32) Asset_GetReferencesCount(void* __obj)
{
    return AssetInternal::GetReferencesCount((::SE::Asset*)__obj);
}
#endif
}

