#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/editororderattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: EditorOrderAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::EditorOrderAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::EditorOrderAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::EditorOrderAttribute>>();

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
            id = TypeID(SE_TEXT("SE::EditorOrderAttribute"));
            size = sizeof(::SE::EditorOrderAttribute);
            alignment = alignof(::SE::EditorOrderAttribute);
            name = SE_TEXT("EditorOrderAttribute");
            fullName = SE_TEXT("SE::EditorOrderAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::EditorOrderAttribute>();
        }
    };

    TTypeMetaInfo<::SE::EditorOrderAttribute> * TTypeMetaInfo<::SE::EditorOrderAttribute>::s_pMetaInfo = nullptr;
}

