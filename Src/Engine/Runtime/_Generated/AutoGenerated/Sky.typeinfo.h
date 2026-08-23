#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actors/Sky.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"
//-------------------------------------------------------------------------
// Enum Info: SE SkyType
//-------------------------------------------------------------------------

#include "Runtime/Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::SkyType> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::SkyType> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::SkyType>>();

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
            id = TypeID(SE_TEXT("SE::SkyType"));
            size = sizeof(::SE::SkyType);
            alignment = alignof(::SE::SkyType);
            name = SE_TEXT("SkyType");
            fullName = SE_TEXT("SE::SkyType");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("SkyBox"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Physical"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::SkyType> * TTypeEnumInfo<::SE::SkyType>::s_pTypeInfo = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE Sky
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Sky> final : public TypeCompositeInfo
    {
       static ::SE::Sky const* s_pDefaultInstance_17783222742286521446;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_17783222742286521446 = New<::SE::Sky>(nullptr);

            ::SE::Sky::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Sky>>(s_pDefaultInstance_17783222742286521446);
            Types::RegisterType(::SE::Sky::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Sky::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Sky*>(s_pDefaultInstance_17783222742286521446));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Sky::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Sky"));
            size = sizeof(::SE::Sky);
            alignment = alignof(::SE::Sky);
            name = SE_TEXT("Sky");
            fullName = SE_TEXT("SE::Sky");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "SE";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::RenderActor::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Sky), alignof(::SE::Sky));
            return new (pMemory) ::SE::Sky(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Sky(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_17783222742286521446;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Sky>::s_pDefaultInstance_17783222742286521446);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Sky>::s_pDefaultInstance_17783222742286521446, propertyID, arrayIdx);
        }
    };

    ::SE::Sky const* TTypeCompositeInfo<::SE::Sky>::s_pDefaultInstance_17783222742286521446 = nullptr;
}


//-------------------------------------------------------------------------
// Bindings
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Auto-generated by BindingsGenerator - do not edit manually.
// Source: E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actors/Sky.h
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Level/Actors/Sky.h"
#include "E:/EngineProject/SolarEngine/Src/Engine/Runtime/_Generated/AutoGenerated/BindingsInterop.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{
class SkyInternal
{
public:
    static void InitRuntime()
    {
    }
};

ScriptingTypeInitializer Sky::TypeInitializer(
    (BinaryModule*)GetBinaryModuleSERuntime(),
    StringAnsiView("SE.Sky", ARRAY_SIZE("SE.Sky") - 1),
    sizeof(::SE::Sky),
    &SkyInternal::InitRuntime,
    (ScriptingType::SpawnHandler)&::SE::Sky::Spawn,
    &::SE::RenderActor::TypeInitializer,
    nullptr,
    nullptr
);
}

