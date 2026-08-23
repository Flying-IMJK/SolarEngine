#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/TypeSystem/MetaData/VisibleIfAttribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE VisibleIfAttribute
//-------------------------------------------------------------------------

#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::VisibleIfAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::VisibleIfAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::VisibleIfAttribute>>();

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
            id = TypeID(SE_TEXT("SE::VisibleIfAttribute"));
            size = sizeof(::SE::VisibleIfAttribute);
            alignment = alignof(::SE::VisibleIfAttribute);
            name = SE_TEXT("VisibleIfAttribute");
            fullName = SE_TEXT("SEVisibleIfAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::VisibleIfAttribute>();
        }
    };

    TTypeMetaInfo<::SE::VisibleIfAttribute> * TTypeMetaInfo<::SE::VisibleIfAttribute>::s_pMetaInfo = nullptr;
}

