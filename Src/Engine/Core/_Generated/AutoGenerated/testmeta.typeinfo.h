#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/typesystem/metadata/testmeta.h"
//-------------------------------------------------------------------------
// Meta Info: SE:: TestMeta
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::TestMeta> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::TestMeta> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::TestMeta>>();

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
            id = TypeID(SE_TEXT("SE::TestMeta"));
            size = sizeof(::SE::TestMeta);
            alignment = alignof(::SE::TestMeta);
            name = SE_TEXT("TestMeta");
            fullName = SE_TEXT("SE::TestMeta");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::TestMeta>();
        }
    };

    TTypeMetaInfo<::SE::TestMeta> * TTypeMetaInfo<::SE::TestMeta>::s_pMetaInfo = nullptr;
}

