#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/graphics/base/gpuresource.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: GPUResourceType
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUResourceType> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUResourceType> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUResourceType>>();

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
            id = TypeID(SE_TEXT("SE::GPUResourceType"));
            size = sizeof(::SE::GPUResourceType);
            alignment = alignof(::SE::GPUResourceType);
            name = SE_TEXT("GPUResourceType");
            fullName = SE_TEXT("SE::GPUResourceType");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("RenderTarget"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU render target texture");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Texture"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 9;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU texture");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CubeTexture"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU cube texture (cubemap)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("VolumeTexture"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 10;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU volume texture (3D)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Buffer"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU buffer");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Shader"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU shader");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("PipelineState"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU pipeline state object (PSO)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Descriptor"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU binding descriptor");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Query"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU timer query");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Sampler"));
            constantInfo.value = 9;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU texture sampler");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 10;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "GPU texture sampler");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUResourceType> * TTypeEnumInfo<::SE::GPUResourceType>::s_pTypeInfo = nullptr;
}

