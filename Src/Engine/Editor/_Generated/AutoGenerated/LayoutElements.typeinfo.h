#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Editor/CustomEditors/Element/LayoutElements.h"

//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor ButtonElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::ButtonElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::ButtonElement const* s_pDefaultInstance_1449181936040216488;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_1449181936040216488 = New<::SE::Editor::ButtonElement>(nullptr);

            ::SE::Editor::ButtonElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::ButtonElement>>(s_pDefaultInstance_1449181936040216488);
            Types::RegisterType(::SE::Editor::ButtonElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::ButtonElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::ButtonElement*>(s_pDefaultInstance_1449181936040216488));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::ButtonElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::ButtonElement"));
            size = sizeof(::SE::Editor::ButtonElement);
            alignment = alignof(::SE::Editor::ButtonElement);
            name = SE_TEXT("ButtonElement");
            fullName = SE_TEXT("SE::Editor::ButtonElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::ButtonElement), alignof(::SE::Editor::ButtonElement));
            return new (pMemory) ::SE::Editor::ButtonElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::ButtonElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_1449181936040216488;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::ButtonElement>::s_pDefaultInstance_1449181936040216488);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::ButtonElement>::s_pDefaultInstance_1449181936040216488, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::ButtonElement const* TTypeCompositeInfo<::SE::Editor::ButtonElement>::s_pDefaultInstance_1449181936040216488 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor SpaceElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::SpaceElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::SpaceElement const* s_pDefaultInstance_14756218850428261714;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_14756218850428261714 = New<::SE::Editor::SpaceElement>(nullptr);

            ::SE::Editor::SpaceElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::SpaceElement>>(s_pDefaultInstance_14756218850428261714);
            Types::RegisterType(::SE::Editor::SpaceElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::SpaceElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::SpaceElement*>(s_pDefaultInstance_14756218850428261714));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::SpaceElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::SpaceElement"));
            size = sizeof(::SE::Editor::SpaceElement);
            alignment = alignof(::SE::Editor::SpaceElement);
            name = SE_TEXT("SpaceElement");
            fullName = SE_TEXT("SE::Editor::SpaceElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::SpaceElement), alignof(::SE::Editor::SpaceElement));
            return new (pMemory) ::SE::Editor::SpaceElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::SpaceElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_14756218850428261714;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::SpaceElement>::s_pDefaultInstance_14756218850428261714);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::SpaceElement>::s_pDefaultInstance_14756218850428261714, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::SpaceElement const* TTypeCompositeInfo<::SE::Editor::SpaceElement>::s_pDefaultInstance_14756218850428261714 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor TextBoxElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::TextBoxElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::TextBoxElement const* s_pDefaultInstance_14867075063023358908;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_14867075063023358908 = New<::SE::Editor::TextBoxElement>(nullptr);

            ::SE::Editor::TextBoxElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::TextBoxElement>>(s_pDefaultInstance_14867075063023358908);
            Types::RegisterType(::SE::Editor::TextBoxElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::TextBoxElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::TextBoxElement*>(s_pDefaultInstance_14867075063023358908));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::TextBoxElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::TextBoxElement"));
            size = sizeof(::SE::Editor::TextBoxElement);
            alignment = alignof(::SE::Editor::TextBoxElement);
            name = SE_TEXT("TextBoxElement");
            fullName = SE_TEXT("SE::Editor::TextBoxElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::TextBoxElement), alignof(::SE::Editor::TextBoxElement));
            return new (pMemory) ::SE::Editor::TextBoxElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::TextBoxElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_14867075063023358908;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::TextBoxElement>::s_pDefaultInstance_14867075063023358908);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::TextBoxElement>::s_pDefaultInstance_14867075063023358908, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::TextBoxElement const* TTypeCompositeInfo<::SE::Editor::TextBoxElement>::s_pDefaultInstance_14867075063023358908 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor CheckBoxElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::CheckBoxElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::CheckBoxElement const* s_pDefaultInstance_9665504872113322263;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_9665504872113322263 = New<::SE::Editor::CheckBoxElement>(nullptr);

            ::SE::Editor::CheckBoxElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::CheckBoxElement>>(s_pDefaultInstance_9665504872113322263);
            Types::RegisterType(::SE::Editor::CheckBoxElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::CheckBoxElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::CheckBoxElement*>(s_pDefaultInstance_9665504872113322263));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::CheckBoxElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::CheckBoxElement"));
            size = sizeof(::SE::Editor::CheckBoxElement);
            alignment = alignof(::SE::Editor::CheckBoxElement);
            name = SE_TEXT("CheckBoxElement");
            fullName = SE_TEXT("SE::Editor::CheckBoxElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::CheckBoxElement), alignof(::SE::Editor::CheckBoxElement));
            return new (pMemory) ::SE::Editor::CheckBoxElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::CheckBoxElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_9665504872113322263;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::CheckBoxElement>::s_pDefaultInstance_9665504872113322263);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::CheckBoxElement>::s_pDefaultInstance_9665504872113322263, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::CheckBoxElement const* TTypeCompositeInfo<::SE::Editor::CheckBoxElement>::s_pDefaultInstance_9665504872113322263 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor DoubleValueElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::DoubleValueElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::DoubleValueElement const* s_pDefaultInstance_17637612612195518910;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_17637612612195518910 = New<::SE::Editor::DoubleValueElement>(nullptr);

            ::SE::Editor::DoubleValueElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::DoubleValueElement>>(s_pDefaultInstance_17637612612195518910);
            Types::RegisterType(::SE::Editor::DoubleValueElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::DoubleValueElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::DoubleValueElement*>(s_pDefaultInstance_17637612612195518910));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::DoubleValueElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::DoubleValueElement"));
            size = sizeof(::SE::Editor::DoubleValueElement);
            alignment = alignof(::SE::Editor::DoubleValueElement);
            name = SE_TEXT("DoubleValueElement");
            fullName = SE_TEXT("SE::Editor::DoubleValueElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::DoubleValueElement), alignof(::SE::Editor::DoubleValueElement));
            return new (pMemory) ::SE::Editor::DoubleValueElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::DoubleValueElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_17637612612195518910;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::DoubleValueElement>::s_pDefaultInstance_17637612612195518910);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::DoubleValueElement>::s_pDefaultInstance_17637612612195518910, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::DoubleValueElement const* TTypeCompositeInfo<::SE::Editor::DoubleValueElement>::s_pDefaultInstance_17637612612195518910 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor IntegerValueElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::IntegerValueElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::IntegerValueElement const* s_pDefaultInstance_10825878003496823313;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_10825878003496823313 = New<::SE::Editor::IntegerValueElement>(nullptr);

            ::SE::Editor::IntegerValueElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::IntegerValueElement>>(s_pDefaultInstance_10825878003496823313);
            Types::RegisterType(::SE::Editor::IntegerValueElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::IntegerValueElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::IntegerValueElement*>(s_pDefaultInstance_10825878003496823313));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::IntegerValueElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::IntegerValueElement"));
            size = sizeof(::SE::Editor::IntegerValueElement);
            alignment = alignof(::SE::Editor::IntegerValueElement);
            name = SE_TEXT("IntegerValueElement");
            fullName = SE_TEXT("SE::Editor::IntegerValueElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::IntegerValueElement), alignof(::SE::Editor::IntegerValueElement));
            return new (pMemory) ::SE::Editor::IntegerValueElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::IntegerValueElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_10825878003496823313;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::IntegerValueElement>::s_pDefaultInstance_10825878003496823313);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::IntegerValueElement>::s_pDefaultInstance_10825878003496823313, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::IntegerValueElement const* TTypeCompositeInfo<::SE::Editor::IntegerValueElement>::s_pDefaultInstance_10825878003496823313 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor SliderElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::SliderElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::SliderElement const* s_pDefaultInstance_5770770141303025731;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_5770770141303025731 = New<::SE::Editor::SliderElement>(nullptr);

            ::SE::Editor::SliderElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::SliderElement>>(s_pDefaultInstance_5770770141303025731);
            Types::RegisterType(::SE::Editor::SliderElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::SliderElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::SliderElement*>(s_pDefaultInstance_5770770141303025731));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::SliderElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::SliderElement"));
            size = sizeof(::SE::Editor::SliderElement);
            alignment = alignof(::SE::Editor::SliderElement);
            name = SE_TEXT("SliderElement");
            fullName = SE_TEXT("SE::Editor::SliderElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::SliderElement), alignof(::SE::Editor::SliderElement));
            return new (pMemory) ::SE::Editor::SliderElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::SliderElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_5770770141303025731;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::SliderElement>::s_pDefaultInstance_5770770141303025731);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::SliderElement>::s_pDefaultInstance_5770770141303025731, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::SliderElement const* TTypeCompositeInfo<::SE::Editor::SliderElement>::s_pDefaultInstance_5770770141303025731 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor ComboBoxElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::ComboBoxElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::ComboBoxElement const* s_pDefaultInstance_132691559933285257;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_132691559933285257 = New<::SE::Editor::ComboBoxElement>(nullptr);

            ::SE::Editor::ComboBoxElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::ComboBoxElement>>(s_pDefaultInstance_132691559933285257);
            Types::RegisterType(::SE::Editor::ComboBoxElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::ComboBoxElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::ComboBoxElement*>(s_pDefaultInstance_132691559933285257));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::ComboBoxElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::ComboBoxElement"));
            size = sizeof(::SE::Editor::ComboBoxElement);
            alignment = alignof(::SE::Editor::ComboBoxElement);
            name = SE_TEXT("ComboBoxElement");
            fullName = SE_TEXT("SE::Editor::ComboBoxElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::ComboBoxElement), alignof(::SE::Editor::ComboBoxElement));
            return new (pMemory) ::SE::Editor::ComboBoxElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::ComboBoxElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_132691559933285257;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::ComboBoxElement>::s_pDefaultInstance_132691559933285257);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::ComboBoxElement>::s_pDefaultInstance_132691559933285257, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::ComboBoxElement const* TTypeCompositeInfo<::SE::Editor::ComboBoxElement>::s_pDefaultInstance_132691559933285257 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor EnumElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::EnumElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::EnumElement const* s_pDefaultInstance_2025243091768194499;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_2025243091768194499 = New<::SE::Editor::EnumElement>(nullptr);

            ::SE::Editor::EnumElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::EnumElement>>(s_pDefaultInstance_2025243091768194499);
            Types::RegisterType(::SE::Editor::EnumElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::EnumElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::EnumElement*>(s_pDefaultInstance_2025243091768194499));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::EnumElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::EnumElement"));
            size = sizeof(::SE::Editor::EnumElement);
            alignment = alignof(::SE::Editor::EnumElement);
            name = SE_TEXT("EnumElement");
            fullName = SE_TEXT("SE::Editor::EnumElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::EnumElement), alignof(::SE::Editor::EnumElement));
            return new (pMemory) ::SE::Editor::EnumElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::EnumElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_2025243091768194499;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::EnumElement>::s_pDefaultInstance_2025243091768194499);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::EnumElement>::s_pDefaultInstance_2025243091768194499, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::EnumElement const* TTypeCompositeInfo<::SE::Editor::EnumElement>::s_pDefaultInstance_2025243091768194499 = nullptr;
}


//-------------------------------------------------------------------------
// TypeCompositeInfo: SE::Editor ImageElement
//-------------------------------------------------------------------------

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
    template<>
    class  TTypeCompositeInfo<::SE::Editor::ImageElement> final : public TypeCompositeInfo
    {
       static ::SE::Editor::ImageElement const* s_pDefaultInstance_9952580670024469555;

    public:
        static void RegisterType()
        {
            s_pDefaultInstance_9952580670024469555 = New<::SE::Editor::ImageElement>(nullptr);

            ::SE::Editor::ImageElement::s_pTypeInfo = New<TTypeCompositeInfo<::SE::Editor::ImageElement>>(s_pDefaultInstance_9952580670024469555);
            Types::RegisterType(::SE::Editor::ImageElement::s_pTypeInfo);
        }

        static void UnregisterType()
        {
            Types::UnregisterType(::SE::Editor::ImageElement::s_pTypeInfo);
            // Destroy default type instance
            Delete(const_cast<::SE::Editor::ImageElement*>(s_pDefaultInstance_9952580670024469555));
            Delete(const_cast<TypeCompositeInfo*>(::SE::Editor::ImageElement::s_pTypeInfo));
        }

    public:

        //-------------------------------------------------------------------------
        // Constructor Methods
        //-------------------------------------------------------------------------
        TTypeCompositeInfo(IType const* pDefaultInstance)
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::Editor::ImageElement"));
            size = sizeof(::SE::Editor::ImageElement);
            alignment = alignof(::SE::Editor::ImageElement);
            name = SE_TEXT("ImageElement");
            fullName = SE_TEXT("SE::Editor::ImageElement");

            // Add type metadata
            isAbstract = false;


            // Create dev tools info
            #ifdef SE_DEVELOPMENT
            category = "Editor";
            isForDevelopmentUseOnly = false;
            #endif

            // Add parent info
            // Parent types
            pParentTypeInfo = ::SE::Editor::LayoutElement::s_pTypeInfo;

            

            TypeProperty* propertyInfo = New<TypeProperty>();
            TypeID metaTypeID;
        }


        virtual IType* CreateType() const override final
        {
            auto pMemory = PlatformAllocator::Allocate(sizeof(::SE::Editor::ImageElement), alignof(::SE::Editor::ImageElement));
            return new (pMemory) ::SE::Editor::ImageElement(nullptr);
        }

        virtual void CreateTypeInPlace( IType* pAllocatedMemory ) const override final
        {
            ENGINE_ASSERT( pAllocatedMemory != nullptr );
            new (pAllocatedMemory) ::SE::Editor::ImageElement(nullptr);
         }

        virtual IType const *GetDefaultInstance() const override
        {
            return s_pDefaultInstance_9952580670024469555;
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
            return AreAllPropertyValuesEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::ImageElement>::s_pDefaultInstance_9952580670024469555);
        }

        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override
        {
            return IsPropertyValueEqual(pTypeInstance, TTypeCompositeInfo<::SE::Editor::ImageElement>::s_pDefaultInstance_9952580670024469555, propertyID, arrayIdx);
        }
    };

    ::SE::Editor::ImageElement const* TTypeCompositeInfo<::SE::Editor::ImageElement>::s_pDefaultInstance_9952580670024469555 = nullptr;
}

