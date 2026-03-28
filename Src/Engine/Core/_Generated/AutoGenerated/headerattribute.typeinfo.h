#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/headerattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: HeaderAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::HeaderAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::HeaderAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::HeaderAttribute>>();

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
            id = TypeID(SE_TEXT("SE::HeaderAttribute"));
            size = sizeof(::SE::HeaderAttribute);
            alignment = alignof(::SE::HeaderAttribute);
            name = SE_TEXT("HeaderAttribute");
            fullName = SE_TEXT("SE::HeaderAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::HeaderAttribute>();
        }
    };

    TTypeMetaInfo<::SE::HeaderAttribute> * TTypeMetaInfo<::SE::HeaderAttribute>::s_pMetaInfo = nullptr;
}

