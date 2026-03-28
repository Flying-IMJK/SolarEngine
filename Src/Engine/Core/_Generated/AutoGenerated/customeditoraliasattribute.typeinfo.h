#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/customeditoraliasattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: CustomEditorAliasAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::CustomEditorAliasAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::CustomEditorAliasAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::CustomEditorAliasAttribute>>();

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
            id = TypeID(SE_TEXT("SE::CustomEditorAliasAttribute"));
            size = sizeof(::SE::CustomEditorAliasAttribute);
            alignment = alignof(::SE::CustomEditorAliasAttribute);
            name = SE_TEXT("CustomEditorAliasAttribute");
            fullName = SE_TEXT("SE::CustomEditorAliasAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::CustomEditorAliasAttribute>();
        }
    };

    TTypeMetaInfo<::SE::CustomEditorAliasAttribute> * TTypeMetaInfo<::SE::CustomEditorAliasAttribute>::s_pMetaInfo = nullptr;
}

