#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/graphics/base/gpuenums.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: GPUResourceUsage
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUResourceUsage> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUResourceUsage> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUResourceUsage>>();

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
            id = TypeID(SE_TEXT("SE::GPUResourceUsage"));
            size = sizeof(::SE::GPUResourceUsage);
            alignment = alignof(::SE::GPUResourceUsage);
            name = SE_TEXT("GPUResourceUsage");
            fullName = SE_TEXT("SE::GPUResourceUsage");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Default"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; A resource that requires read and write access by the GPU. This is likely to be the most common usage choice. Memory will be used on device only, so fast access from the device is preferred. It usually means device-local GPU (video) memory. &lt;/summary&gt; &lt;remarks&gt; Usage: - Resources written and read by device, e.g. images used as render targets. - Resources transferred from host once (immutable) or infrequently and read by device multiple times, e.g. textures to be sampled, vertex buffers, constant buffers, and majority of other types of resources used on GPU. &lt;/remarks&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Dynamic"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; A resource that is accessible by both the GPU (read only) and the CPU (write only). A dynamic resource is a good choice for a resource that will be updated by the CPU at least once per frame. Dynamic buffers or textures are usually used to upload data to GPU and use it within a single frame. &lt;/summary&gt; &lt;remarks&gt; Usage: - Resources written frequently by CPU (dynamic), read by device. E.g. textures, vertex buffers, uniform buffers updated every frame or every draw call. &lt;/remarks&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("StagingUpload"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; A resource that supports data transfer (copy) from the CPU to the GPU. It usually means CPU (system) memory. Resources created in this pool may still be accessible to the device, but access to them can be slow. &lt;/summary&gt; &lt;remarks&gt; Usage: - Staging copy of resources used as transfer source. &lt;/remarks&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("StagingReadback"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; A resource that supports data transfer (copy) from the GPU to the CPU. &lt;/summary&gt; &lt;remarks&gt; Usage: - Resources written by device, read by host - results of some computations, e.g. screen capture, average scene luminance for HDR tone mapping. - Any resources read or accessed randomly on host, e.g. CPU-side copy of vertex buffer used as source of transfer, but also used for collision detection. &lt;/remarks&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUResourceUsage> * TTypeEnumInfo<::SE::GPUResourceUsage>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GPUResourceMapMode
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUResourceMapMode> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUResourceMapMode> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUResourceMapMode>>();

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
            id = TypeID(SE_TEXT("SE::GPUResourceMapMode"));
            size = sizeof(::SE::GPUResourceMapMode);
            alignment = alignof(::SE::GPUResourceMapMode);
            name = SE_TEXT("GPUResourceMapMode");
            fullName = SE_TEXT("SE::GPUResourceMapMode");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Read"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The resource is mapped for reading. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Write"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The resource is mapped for writing. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("ReadWrite"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The resource is mapped for reading and writing. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUResourceMapMode> * TTypeEnumInfo<::SE::GPUResourceMapMode>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: MSAALevel
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::MSAALevel> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::MSAALevel> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::MSAALevel>>();

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
            id = TypeID(SE_TEXT("SE::MSAALevel"));
            size = sizeof(::SE::MSAALevel);
            alignment = alignof(::SE::MSAALevel);
            name = SE_TEXT("MSAALevel");
            fullName = SE_TEXT("SE::MSAALevel");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("None"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Disabled multisampling. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("X2"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Two samples per pixel. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("X4"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Four samples per pixel. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("X8"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Eight samples per pixel. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::MSAALevel> * TTypeEnumInfo<::SE::MSAALevel>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GPURendererType
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPURendererType> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPURendererType> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPURendererType>>();

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
            id = TypeID(SE_TEXT("SE::GPURendererType"));
            size = sizeof(::SE::GPURendererType);
            alignment = alignof(::SE::GPURendererType);
            name = SE_TEXT("GPURendererType");
            fullName = SE_TEXT("SE::GPURendererType");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Vulkan"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPURendererType> * TTypeEnumInfo<::SE::GPURendererType>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: RHIShaderModel
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::RHIShaderModel> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::RHIShaderModel> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::RHIShaderModel>>();

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
            id = TypeID(SE_TEXT("SE::RHIShaderModel"));
            size = sizeof(::SE::RHIShaderModel);
            alignment = alignof(::SE::RHIShaderModel);
            name = SE_TEXT("RHIShaderModel");
            fullName = SE_TEXT("SE::RHIShaderModel");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("SM_5_0"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_0"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_1"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_2"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_3"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_4"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_5"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_6"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("SM_6_7"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::RHIShaderModel> * TTypeEnumInfo<::SE::RHIShaderModel>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: RHIFilter
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::RHIFilter> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::RHIFilter> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::RHIFilter>>();

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
            id = TypeID(SE_TEXT("SE::RHIFilter"));
            size = sizeof(::SE::RHIFilter);
            alignment = alignof(::SE::RHIFilter);
            name = SE_TEXT("RHIFilter");
            fullName = SE_TEXT("SE::RHIFilter");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("MIN_MAG_MIP_POINT"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 32;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 33;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_POINT_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 34;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_POINT_MAG_MIP_LINEAR"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 35;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_LINEAR_MAG_MIP_POINT"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 28;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_LINEAR_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 29;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 30;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MIN_MAG_MIP_LINEAR"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 31;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("ANISOTROPIC"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_MAG_MIP_POINT"));
            constantInfo.value = 9;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 10;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 11;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_POINT_MAG_MIP_LINEAR"));
            constantInfo.value = 12;
            constantInfo.alphabeticalOrder = 9;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_LINEAR_MAG_MIP_POINT"));
            constantInfo.value = 13;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 14;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 15;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_MIN_MAG_MIP_LINEAR"));
            constantInfo.value = 16;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("COMPARISON_ANISOTROPIC"));
            constantInfo.value = 17;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_MAG_MIP_POINT"));
            constantInfo.value = 18;
            constantInfo.alphabeticalOrder = 24;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 19;
            constantInfo.alphabeticalOrder = 25;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 20;
            constantInfo.alphabeticalOrder = 26;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_POINT_MAG_MIP_LINEAR"));
            constantInfo.value = 21;
            constantInfo.alphabeticalOrder = 27;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_LINEAR_MAG_MIP_POINT"));
            constantInfo.value = 22;
            constantInfo.alphabeticalOrder = 20;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 23;
            constantInfo.alphabeticalOrder = 21;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 24;
            constantInfo.alphabeticalOrder = 22;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_MIN_MAG_MIP_LINEAR"));
            constantInfo.value = 25;
            constantInfo.alphabeticalOrder = 23;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MINIMUM_ANISOTROPIC"));
            constantInfo.value = 26;
            constantInfo.alphabeticalOrder = 19;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_MAG_MIP_POINT"));
            constantInfo.value = 27;
            constantInfo.alphabeticalOrder = 15;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 28;
            constantInfo.alphabeticalOrder = 16;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 29;
            constantInfo.alphabeticalOrder = 17;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_POINT_MAG_MIP_LINEAR"));
            constantInfo.value = 30;
            constantInfo.alphabeticalOrder = 18;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_LINEAR_MAG_MIP_POINT"));
            constantInfo.value = 31;
            constantInfo.alphabeticalOrder = 11;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR"));
            constantInfo.value = 32;
            constantInfo.alphabeticalOrder = 12;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_MAG_LINEAR_MIP_POINT"));
            constantInfo.value = 33;
            constantInfo.alphabeticalOrder = 13;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_MIN_MAG_MIP_LINEAR"));
            constantInfo.value = 34;
            constantInfo.alphabeticalOrder = 14;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAXIMUM_ANISOTROPIC"));
            constantInfo.value = 35;
            constantInfo.alphabeticalOrder = 10;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::RHIFilter> * TTypeEnumInfo<::SE::RHIFilter>::s_pTypeInfo = nullptr;
}

