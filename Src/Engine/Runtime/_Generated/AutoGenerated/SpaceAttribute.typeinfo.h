#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/TypeSystem/MetaData/SpaceAttribute.h"
//-------------------------------------------------------------------------
// Meta Info: SE SpaceAttribute
//-------------------------------------------------------------------------

#include "Runtime/Core/TypeSystem/Info/TypeMetaInfo.h"


namespace SE
{
    template <>
    class TTypeMetaInfo<::SE::SpaceAttribute> : public TypeMetaInfo
    {
    public:
        static TTypeMetaInfo<::SE::SpaceAttribute> * s_pMetaInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pMetaInfo == nullptr);
            s_pMetaInfo = New<TTypeMetaInfo<::SE::SpaceAttribute>>();

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
            id = TypeID(SE_TEXT("SE::SpaceAttribute"));
            size = sizeof(::SE::SpaceAttribute);
            alignment = alignof(::SE::SpaceAttribute);
            name = SE_TEXT("SpaceAttribute");
            fullName = SE_TEXT("SESpaceAttribute");
        }


        TypeMetaAttribute* Create() const override
        {
            return New<::SE::SpaceAttribute>();
        }
    };

    TTypeMetaInfo<::SE::SpaceAttribute> * TTypeMetaInfo<::SE::SpaceAttribute>::s_pMetaInfo = nullptr;
}

