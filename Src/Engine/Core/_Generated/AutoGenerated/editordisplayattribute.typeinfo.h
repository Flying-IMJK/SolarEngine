#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/editordisplayattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: EditorDisplayAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::EditorDisplayAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::EditorDisplayAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::EditorDisplayAttribute>>();

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
            id = TypeID(SE_TEXT("SE::EditorDisplayAttribute"));
            size = sizeof(::SE::EditorDisplayAttribute);
            alignment = alignof(::SE::EditorDisplayAttribute);
            name = SE_TEXT("EditorDisplayAttribute");
            fullName = SE_TEXT("SE::EditorDisplayAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::EditorDisplayAttribute>();
        }
    };

    TTypeMetaInfo<::SE::EditorDisplayAttribute> * TTypeMetaInfo<::SE::EditorDisplayAttribute>::s_pMetaInfo = nullptr;
}

