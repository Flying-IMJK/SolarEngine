#include "ShaderCompiler.h"

#include <spirv_hlsl.hpp>

namespace SE::ShaderCompiler
{

    typedef spirv_cross::CompilerHLSL SpirvCompiler;
    typedef spirv_cross::Resource SpirvCrossResource;
    typedef spirv_cross::SPIRConstant SpirConstant;

    void ReadResourceSize(const SpirvCompiler &compiler,
                          const SpirvCrossResource &resource,
                          ShaderResource &shader_resource)
    {
        const auto &spirv_type = compiler.get_type_from_variable(resource.id);

        uint32 array_size = 0;
        shader_resource.size = compiler.get_declared_struct_size_runtime_array(spirv_type, array_size);
    }

    void ReadResourceSize(const SpirvCompiler &compiler, const SpirConstant &constant, ShaderResource &shader_resource)
    {
        auto spirv_type = compiler.get_type(constant.constant_type);

        switch (spirv_type.basetype)
        {
        case spirv_cross::SPIRType::BaseType::Boolean:
        case spirv_cross::SPIRType::BaseType::Char:
        case spirv_cross::SPIRType::BaseType::Int:
        case spirv_cross::SPIRType::BaseType::UInt:
        case spirv_cross::SPIRType::BaseType::Float:
            shader_resource.size = 4;
            break;
        case spirv_cross::SPIRType::BaseType::Int64:
        case spirv_cross::SPIRType::BaseType::UInt64:
        case spirv_cross::SPIRType::BaseType::Double:
            shader_resource.size = 8;
            break;
        default:
            shader_resource.size = 0;
            break;
        }
    }

    void ReadResourceVecSize(const SpirvCompiler &compiler,
                             const SpirvCrossResource &resource,
                             ShaderResource &shader_resource)
    {
        const auto &spirv_type = compiler.get_type_from_variable(resource.id);

        shader_resource.vec_size = spirv_type.vecsize;
        shader_resource.columns = spirv_type.columns;
    }

    void ReadResourceArraySize(const SpirvCompiler &compiler,
                               const SpirvCrossResource &resource,
                               ShaderResource &shader_resource)
    {
        const auto &spirv_type = compiler.get_type_from_variable(resource.id);
        shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;
    }

    void ReadResourcesImage(const SpirvCompiler &compiler,
                            const SpirvCrossResource &resource,
                            ShaderResource &shader_resource)
    {
        const auto &spirv_type = compiler.get_type_from_variable(resource.id);

        shader_resource.image_arrayed = spirv_type.image.arrayed;
        shader_resource.image_depth = spirv_type.image.depth;

        switch (spirv_type.image.dim)
        {
        case spv::Dim::Dim1D:
            shader_resource.image_dim = ShaderResourceImageDim::_1D;
            break;
        case spv::Dim::Dim2D:
            shader_resource.image_dim = ShaderResourceImageDim::_2D;
            break;
        case spv::Dim::Dim3D:
            shader_resource.image_dim = ShaderResourceImageDim::_3D;
            break;
        case spv::Dim::DimCube:
            shader_resource.image_dim = ShaderResourceImageDim::_Cube;
            break;
        default:
            break;
        }
    }

    template <ShaderResourceType T>
    inline void ReadShaderResource(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
		LOG_ERROR("Shader", nullptr, "Not implemented! Read shader resources of type.");
    }

    template <spv::Decoration T>
    inline void ReadResourceDecoration(const SpirvCompiler &compiler, const SpirvCrossResource &resource, ShaderResource &shader_resource)
    {
		LOG_ERROR("Shader", nullptr,"Not implemented! Read resources decoration of type.");
    }

    template <>
    inline void ReadResourceDecoration<spv::DecorationLocation>(const SpirvCompiler &compiler,
                                                                const SpirvCrossResource &resource,
                                                                ShaderResource &shader_resource)
    {
        shader_resource.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
    }

    template <>
    inline void ReadResourceDecoration<spv::DecorationDescriptorSet>(const SpirvCompiler &compiler,
                                                                     const SpirvCrossResource &resource,
                                                                     ShaderResource &shader_resource)
    {
        shader_resource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    }

    template <>
    inline void ReadResourceDecoration<spv::DecorationBinding>(const SpirvCompiler &compiler,
                                                               const SpirvCrossResource &resource,
                                                               ShaderResource &shader_resource)
    {
        shader_resource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
    }

    template <>
    inline void ReadResourceDecoration<spv::DecorationInputAttachmentIndex>(const SpirvCompiler &compiler,
                                                                            const SpirvCrossResource &resource,
                                                                            ShaderResource &shader_resource)
    {
        shader_resource.input_attachment_index = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
    }

    template <>
    inline void ReadResourceDecoration<spv::DecorationNonWritable>(const SpirvCompiler &compiler,
                                                                   const SpirvCrossResource &resource,
                                                                   ShaderResource &shader_resource)
    {
        shader_resource.qualifiers |= ShaderResourceQualifiers::NonWritable;
    }

    template <>
    inline void ReadResourceDecoration<spv::DecorationNonReadable>(const SpirvCompiler &compiler,
                                                                   const SpirvCrossResource &resource,
                                                                   ShaderResource &shader_resource)
    {
        shader_resource.qualifiers |= ShaderResourceQualifiers::NonReadable;
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::Input>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto input_resources = compiler.get_shader_resources().stage_inputs;

        for (auto &resource : input_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::Input;
            shader_resource.name = resource.name.c_str();

            ReadResourceVecSize(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationLocation>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::InputAttachment>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto subpass_resources = compiler.get_shader_resources().subpass_inputs;

        for (auto &resource : subpass_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::InputAttachment;
            shader_resource.name = resource.name.c_str();

            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationInputAttachmentIndex>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::Output>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto output_resources = compiler.get_shader_resources().stage_outputs;

        for (auto &resource : output_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::Output;
            shader_resource.name = resource.name.c_str();

            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceVecSize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationLocation>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::Image>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto image_resources = compiler.get_shader_resources().sampled_images;

        for (auto &resource : image_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::Image;
            shader_resource.name = resource.name.c_str();

            ReadResourcesImage(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::SamplerImage>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto image_resources = compiler.get_shader_resources().separate_images;

        for (auto &resource : image_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::SamplerImage;
            shader_resource.name = resource.name.c_str();
            
            ReadResourcesImage(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::StorageImage>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto storage_resources = compiler.get_shader_resources().storage_images;

        for (auto &resource : storage_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::StorageImage;
            shader_resource.name = resource.name.c_str();

            ReadResourcesImage(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationNonReadable>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationNonWritable>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::Sampler>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto sampler_resources = compiler.get_shader_resources().separate_samplers;

        for (auto &resource : sampler_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::Sampler;
            shader_resource.name = resource.name.c_str();

            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::UniformBuffer>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto uniform_resources = compiler.get_shader_resources().uniform_buffers;

        for (SpirvCrossResource &resource : uniform_resources)
        {
            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::UniformBuffer;
            shader_resource.name = resource.name.c_str();

            ReadResourceSize(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }

    template <>
    inline void ReadShaderResource<ShaderResourceType::StorageBuffer>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto storage_resources = compiler.get_shader_resources().storage_buffers;

        for (spirv_cross::Resource &resource : storage_resources)
        {
            ShaderResource shader_resource;
            shader_resource.type = ShaderResourceType::StorageBuffer;
            shader_resource.name = resource.name.c_str();

            ReadResourceSize(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationNonReadable>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationNonWritable>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }


    template <>
    inline void ReadShaderResource<ShaderResourceType::Buffer>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto storage_resources = compiler.get_shader_resources().storage_buffers;

        for (spirv_cross::Resource &resource : storage_resources)
        {
            ShaderResource shader_resource;
            shader_resource.type = ShaderResourceType::StorageBuffer;
            shader_resource.name = resource.name.c_str();

            ReadResourcesImage(compiler, resource, shader_resource);
            ReadResourceSize(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationNonReadable>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationNonWritable>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }


    template <>
    inline void ReadShaderResource<ShaderResourceType::RWBUffer>(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto storage_resources = compiler.get_shader_resources().storage_buffers;

        for (spirv_cross::Resource &resource : storage_resources)
        {
            ShaderResource shader_resource;
            shader_resource.type = ShaderResourceType::StorageBuffer;
            shader_resource.name = resource.name.c_str();

            ReadResourcesImage(compiler, resource, shader_resource);
            ReadResourceSize(compiler, resource, shader_resource);
            ReadResourceArraySize(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource);

            resources.Add(shader_resource);
        }
    }


    void ParseShaderResources(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        //ReadShaderResource<ShaderResourceType::Input>(compiler, resources);
        ReadShaderResource<ShaderResourceType::InputAttachment>(compiler, resources);
        //ReadShaderResource<ShaderResourceType::Output>(compiler, resources);
        ReadShaderResource<ShaderResourceType::Image>(compiler, resources);
        ReadShaderResource<ShaderResourceType::SamplerImage>(compiler, resources);
        ReadShaderResource<ShaderResourceType::StorageImage>(compiler, resources);
        ReadShaderResource<ShaderResourceType::Sampler>(compiler, resources);
        ReadShaderResource<ShaderResourceType::UniformBuffer>(compiler, resources);
        ReadShaderResource<ShaderResourceType::StorageBuffer>(compiler, resources);
        ReadShaderResource<ShaderResourceType::Buffer>(compiler, resources);
        ReadShaderResource<ShaderResourceType::RWBUffer>(compiler, resources);
    }

    void ParsePushConstants(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto shader_resources = compiler.get_shader_resources();

        for (auto &resource : shader_resources.push_constant_buffers)
        {
            const auto &spivr_type = compiler.get_type_from_variable(resource.id);

            std::uint32_t offset = std::numeric_limits<std::uint32_t>::max();

            for (auto i = 0U; i < spivr_type.member_types.size(); ++i)
            {
                auto mem_offset = compiler.get_member_decoration(spivr_type.self, i, spv::DecorationOffset);

                offset = std::min(offset, mem_offset);
            }

            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::PushConstant;
            shader_resource.name = resource.name.c_str();
            shader_resource.offset = offset;

            ReadResourceSize(compiler, resource, shader_resource);

            shader_resource.size -= shader_resource.offset;

            resources.Add(shader_resource);
        }
    }

    void ParseSpecializationConstants(const SpirvCompiler &compiler, List<ShaderResource> &resources)
    {
        auto specialization_constants = compiler.get_specialization_constants();

        for (auto &resource : specialization_constants)
        {
            const SpirConstant &spirv_value = compiler.get_constant(resource.id);

            ShaderResource shader_resource{};
            shader_resource.type = ShaderResourceType::SpecializationConstant;
            shader_resource.name = compiler.get_name(resource.id).c_str();
            shader_resource.offset = 0;
            shader_resource.constant_id = resource.constant_id;

            ReadResourceSize(compiler, spirv_value, shader_resource);

            resources.Add(shader_resource);
        }
    }


    bool ReflectShaderResources(const uint8 *code, const uint64 codeLength, List<ShaderResource> &resources)
    {
        SpirvCompiler compiler((uint32*)code, codeLength / sizeof(uint32));

        auto opts = compiler.get_common_options();
        opts.enable_420pack_extension = true;

        compiler.set_common_options(opts);

        ParseShaderResources(compiler, resources);
        ParsePushConstants(compiler, resources);
        ParseSpecializationConstants(compiler, resources);
        return true;
    }


}