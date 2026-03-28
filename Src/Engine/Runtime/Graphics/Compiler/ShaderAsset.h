#pragma once

#include "Core/Types/Collections/List.h"

#include "Runtime/RHI/RHIType.h"

namespace SE
{
    /// 着色器资源的类型
    /**
     * @param Image sampler2D组合图像采样器
     * @param Texture 只读纹理
     * @param RWTexture 可读写纹理
     * @param Buffer   (Buffer in HLSL，samplerBuffer in GLSL)
     * @param RWBUffer (RWBuffer in HLSL，imageBuffer in GLSL) 
     * @param Sampler 采样器
     * @param UniformBuffer (cbuffer in HLSL，uniform UBO in GLSL)
     * @param StorageBuffer @(StructuredBuffer in HLSL, buffer SSBO in GLSL)
    */
    enum class ShaderResourceType : uint8
    {
        None,
        Input,
        Output,
        InputAttachment,

        Image,
        SamplerImage,
        StorageImage,
        Buffer,
        RWBUffer,
        Sampler,
        UniformBuffer,
        StorageBuffer,

        PushConstant,
        SpecializationConstant, // 可修改常量
    };

    enum class ShaderResourceImageDim
    {
        _1D = 0,
        _2D = 1,
        _3D = 2,
        _Cube = 3,
    };


    /// 决定了描述符集应该如何创建和绑定资源
    enum class ShaderResourceMode : uint8
    {
        Static,
        Dynamic,
        UpdateAfterBind
    };

    /// 应用于资源的限定符位掩码
    struct ShaderResourceQualifiers
    {
        enum : uint32
        {
            None = 0,
            NonReadable = 1,
            NonWritable = 2,
        };
    };



    class ShaderResource
    {
    public:
        String name;

        //ShaderStageType stages;

        ShaderResourceType type;

        ShaderResourceMode mode;

        uint32 set;

        uint32 binding;

        uint32 location;

        uint32 array_size;

        // image
        bool image_arrayed;
        ShaderResourceImageDim image_dim;
        uint32 image_depth;

        /// 矢量 float4 = 4
        uint32 vec_size;
        /// 矢量 float4x3 = 3
        uint32 columns;

        /// ShaderResourceQualifiers
        uint32 qualifiers;

        /// ShaderResourceType::InputAttachment
        uint32 input_attachment_index;

        // ShaderResourceType::PushConstant
        uint32 offset;
        // ShaderResourceType::PushConstant
        uint32 size;

        // ShaderResourceType::SpecializationConstants
        uint32 constant_id;

        ShaderResource() : name(""),
                           type(ShaderResourceType::None),
                           mode(ShaderResourceMode::Static),
                           set(0),
                           binding(0),
                           location(0),
                           vec_size(0),
                           columns(0),
                           input_attachment_index(0),
                           offset(0),
                           size(0),
                           constant_id(0),
                           qualifiers(0){};
    };


    class ShaderStage
    {
    public:
        RHIShaderStage stageType;
        char* enterPointName;
        List<uint32> spirv;
    };

    class ShaderPass
    {
    public:
		List<String> macro;
		List<ShaderStage> shaderStage;
		List<ShaderResource> resources;
    };

    
    class ShaderAsset
    {
    public:
        String shaderFilePath;
		List<ShaderPass> pass;
    };

}