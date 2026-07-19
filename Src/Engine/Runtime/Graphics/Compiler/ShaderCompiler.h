#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Types/Collections/List.h"

#include "Runtime/RHI/RHIType.h"
#include "ShaderAsset.h"
#include "GraphicShaderAnalyzer.h"
#include "ShaderIncluder.h"


namespace SE::ShaderCompiler
{   
    enum class ShaderCompilerTarget
    {
        DX = 0,
        Vulkan = 1,
    };

    enum class ShaderCompileClientVersion
    {
        Vulkan_1_0 = 0, // Vulkan 1.0
        Vulkan_1_1 = 1, // Vulkan 1.1
        Vulkan_1_2 = 2, // Vulkan 1.2
        Vulkan_1_3 = 3, // Vulkan 1.3
    };

    enum class ShaderCompileSpirvVersion
    {
        Spv_1_0 = 0, // SPIR-V 1.0
        Spv_1_1 = 1, // SPIR-V 1.1
        Spv_1_2 = 2, // SPIR-V 1.2
        Spv_1_3 = 3, // SPIR-V 1.3
        Spv_1_4 = 4, // SPIR-V 1.4
        Spv_1_5 = 5, // SPIR-V 1.5
        Spv_1_6 = 6, // SPIR-V 1.6
    };

    struct ShaderResult
    {
        bool success;
        List<String> error;
		List<uint32> code;
    };

    struct GraphicShaderDesc
    {
        RHIShaderStage stage;
        String enterPoint;
        ShaderCompilerTarget target;                // 编译目标平台
        ShaderCompileClientVersion targetVersion;   // 编译目标版本
        ShaderCompileSpirvVersion spirvVersion;     // Spirve版本
    };

    enum class Flags
	{
		NONE = 0,
		DISABLE_OPTIMIZATION = 1 << 0,
		STRIP_REFLECTION = 1 << 1,
	};

    struct CompilerInput
	{
		TBitMovedFlags<Flags> flags = Flags::NONE;
		RHIShaderFormat format = RHIShaderFormat::NONE;
		RHIShaderStage stage = RHIShaderStage::Count;
		// if the shader relies on a higher shader model feature, it must be declared here.
		//	But the compiler can also choose a higher one internally, if needed
		RHIShaderModel minshadermodel = RHIShaderModel::SM_5_0;
		String shadersourcefilename;
        char* shadersourcedata;
        uint64 shadersourceLength;
		String entrypoint = "main";
		List<String> include_directories;
		List<String> defines;
	};

	struct CompilerOutput
	{
		Ref<void> native;
		inline bool IsValid() const { return native.get() != nullptr; }

		const uint8* shaderBytedata = nullptr;
		uint64 shaderBytesize = 0;
		List<uint8> shaderhash;
		String error_message;
		List<String> dependencies;
	};


    void Compile(CompilerInput& input, CompilerOutput& output);
    bool ReflectShaderResources(const uint8 *code, const uint64 codeLength, List<ShaderResource> &resources);
}