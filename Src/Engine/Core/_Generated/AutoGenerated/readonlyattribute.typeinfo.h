#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/readonlyattribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: ReadOnlyAttribute
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::ReadOnlyAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::ReadOnlyAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::ReadOnlyAttribute>>();

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
            id = TypeID(SE_TEXT("SE::ReadOnlyAttribute"));
            size = sizeof(::SE::ReadOnlyAttribute);
            alignment = alignof(::SE::ReadOnlyAttribute);
            name = SE_TEXT("ReadOnlyAttribute");
            fullName = SE_TEXT("SE::ReadOnlyAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::ReadOnlyAttribute>();
        }
    };

    TTypeMetaInfo<::SE::ReadOnlyAttribute> * TTypeMetaInfo<::SE::ReadOnlyAttribute>::s_pMetaInfo = nullptr;
}

