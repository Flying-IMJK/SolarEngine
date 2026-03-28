#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/customeditorattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: CustomEditorAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::CustomEditorAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::CustomEditorAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::CustomEditorAttribute>>();

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
            id = TypeID(SE_TEXT("SE::CustomEditorAttribute"));
            size = sizeof(::SE::CustomEditorAttribute);
            alignment = alignof(::SE::CustomEditorAttribute);
            name = SE_TEXT("CustomEditorAttribute");
            fullName = SE_TEXT("SE::CustomEditorAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::CustomEditorAttribute>();
        }
    };

    TTypeMetaInfo<::SE::CustomEditorAttribute> * TTypeMetaInfo<::SE::CustomEditorAttribute>::s_pMetaInfo = nullptr;
}

