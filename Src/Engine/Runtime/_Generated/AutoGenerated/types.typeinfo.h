#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/resource/importers/types.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: CreateAssetResult
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::CreateAssetResult> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::CreateAssetResult> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::CreateAssetResult>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::CreateAssetResult"));
            size = sizeof(::SE::CreateAssetResult);
            alignment = alignof(::SE::CreateAssetResult);
            name = SE_TEXT("CreateAssetResult");
            fullName = SE_TEXT("SE::CreateAssetResult");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Ok"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Abort"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Error"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CannotSaveFile"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("InvalidPath"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CannotAllocateChunk"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("InvalidTypeID"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Skip"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::CreateAssetResult> * TTypeEnumInfo<::SE::CreateAssetResult>::s_pTypeInfo = nullptr;
}

