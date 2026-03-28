#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/hideineditorattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: HideInEditorAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::HideInEditorAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::HideInEditorAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::HideInEditorAttribute>>();

            Types::RegisterMeta(s_pMetaInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo != nullptr);
            Types::UnregisterMeta(s_pMetaInfo);
            Delete(s_pMetaInfo);
        };

        TTypeMetaInfo()
        {
            id = TypeID(SE_TEXT("SE::HideInEditorAttribute"));
            size = sizeof(::SE::HideInEditorAttribute);
            alignment = alignof(::SE::HideInEditorAttribute);
            name = SE_TEXT("HideInEditorAttribute");
            fullName = SE_TEXT("SE::HideInEditorAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::HideInEditorAttribute>();
        }
    };

    TTypeMetaInfo<::SE::HideInEditorAttribute> * TTypeMetaInfo<::SE::HideInEditorAttribute>::s_pMetaInfo = nullptr;
}

