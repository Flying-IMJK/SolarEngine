#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/ui/gui/panels/scrollbar.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE:: ScrollBar
//-------------------------------------------------------------------------

#include "Core/Serialization/Json.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Metadata/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::ScrollBar> final : public TypeCompositeInfo
    {
       static ::SE::ScrollBar const* s_pDefaultInstance_4023508597;

    public:
        static void RegisterType()
        {
            

            ::SE::ScrollBar::s_pTypeInfo = New<TTypeCompositeInfo<::SE::ScrollBar>>(s_pDefaultInstance_4023508597);
            Types::RegisterType(::SE::ScrollBar::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::ScrollBar::s_pTypeInfo);
            
            Delete(const_cast<TypeCompositeInfo*>(::SE::ScrollBar::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::ScrollBar"));
            size = sizeof(::SE::ScrollBar);
            alignment = alignof(::SE::ScrollBar);
            name = SE_TEXT("ScrollBar");
            fullName = SE_TEXT("SE::ScrollBar");

            // Add type metadata
            isAbstract = true;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Control::s_pTypeInfo;

            

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
            return s_pDefaultInstance_4023508597;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::ScrollBar>::s_pDefaultInstance_4023508597);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::ScrollBar>::s_pDefaultInstance_4023508597, propertyID, arrayIdx);
        }
    };

    ::SE::ScrollBar const* TTypeCompositeInfo<::SE::ScrollBar>::s_pDefaultInstance_4023508597 = nullptr;
}

