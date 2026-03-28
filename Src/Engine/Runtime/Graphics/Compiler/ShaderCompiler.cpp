#include "ShaderCompiler.h"

#include "Runtime/RHI/RHIObject.h"
/*
#define ENABLE_HLSL
// #include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/SpvTools.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
// #include <glslang/Include/BaseTypes.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <spirv-tools/include/spirv-tools/libspirv.h>

#include <SPIRV-Cross/spirv_hlsl.hpp>
#include <SPIRV-Cross/spirv_reflect.hpp>
#include <sstream>
*/

#define SHADERCOMPILER_ENABLED


//#include "Runtime/Platform/WindowsPlatform.h"

#include <wrl/client.h>
#define CComPtr Microsoft::WRL::ComPtr

#include <dxcapi.h>

namespace SE::ShaderCompiler
{
    /*
    struct HLSLToSpirVParame
    {
        Ref<List<const char *>> code;
        const char *entryPoint;
        ShaderIncluder *includer;
        EShLanguage shaderType;
        glslang::EShClient client;
        glslang::EShTargetClientVersion targetClientVersion;
        glslang::EShTargetLanguageVersion targetLanguageVersion;
    };

    bool HlslShaderCodeToSpirV(HLSLToSpirVParame &param,
                               List<uint32> &result,
                               String &error)
    {

        glslang::TShader shader(param.shaderType);
        glslang::EShSource esh_language = glslang::EShSource::EShSourceHlsl;

        shader.setStrings(param.code->data(), static_cast<int>(param.code->size()));
        shader.setEnvInput(esh_language, param.shaderType, param.client, 100);
        shader.setEntryPoint(param.entryPoint);
        shader.setSourceEntryPoint(param.entryPoint);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, param.targetClientVersion);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, param.targetLanguageVersion);

        shader.setHlslIoMapping(true);
        shader.setShiftSamplerBinding(BINDING_SHIFT_S);
        shader.setShiftTextureBinding(BINDING_SHIFT_T);
        shader.setShiftCbufferBinding(BINDING_SHIFT_B);
        shader.setShiftUavBinding(BINDING_SHIFT_U);

        EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

        if (!shader.parse(GetDefaultResources(), 100, false, messages, *param.includer))
        {
            error = shader.getInfoLog();
            return false;
        }

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(messages))
        {
            error = program.getInfoLog();
            return false;
        }

        glslang::TIntermediate *intermediate = program.getIntermediate(param.shaderType);

        if (!intermediate)
        {
            error = "shader 指定阶段编译失败";
            return false;
        }

        glslang::GlslangToSpv(*(program.getIntermediate(param.shaderType)), result);

        return true;
    }

    glslang::EShTargetClientVersion GetTargetClientVersion(ShaderCompileClientVersion clientVersion)
    {
        glslang::EShTargetClientVersion version = glslang::EShTargetClientVersion::EShTargetVulkan_1_0;

        switch (clientVersion)
        {
        case ShaderCompileClientVersion::Vulkan_1_0:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_0;
            break;
        case ShaderCompileClientVersion::Vulkan_1_1:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_1;
            break;
        case ShaderCompileClientVersion::Vulkan_1_2:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
            break;
        case ShaderCompileClientVersion::Vulkan_1_3:
            version = glslang::EShTargetClientVersion::EShTargetVulkan_1_3;
            break;
        }
        return version;
    }

    glslang::EShTargetLanguageVersion GetTargetSpirvVersion(ShaderCompileSpirvVersion spirvVersion)
    {
        glslang::EShTargetLanguageVersion version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_0;

        switch (spirvVersion)
        {
        case ShaderCompileSpirvVersion::Spv_1_0:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_0;
            break;
        case ShaderCompileSpirvVersion::Spv_1_1:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_1;
            break;
        case ShaderCompileSpirvVersion::Spv_1_2:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_2;
            break;
        case ShaderCompileSpirvVersion::Spv_1_3:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_3;
            break;
        case ShaderCompileSpirvVersion::Spv_1_4:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_4;
            break;
        case ShaderCompileSpirvVersion::Spv_1_5:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_5;
            break;
        case ShaderCompileSpirvVersion::Spv_1_6:
            version = glslang::EShTargetLanguageVersion::EShTargetSpv_1_6;
            break;
        }

        return version;
    }

    EShLanguage GetRHIShaderStage(RHIRHIShaderStage type)
    {
        switch (type)
        {
        case RHIRHIShaderStage::VS:
            return EShLanguage::EShLangVertex;

        case RHIRHIShaderStage::PS:
            return EShLanguage::EShLangFragment;
        }
        return EShLanguage::EShLangCount;
    }

    ShaderResult ShaderCompiler::CompireGraphicShader(GraphicShaderDesc &desc, String sourceCode)
    {
        ShaderResult result;
        result.success = true;
        result.code = List<uint32>();
        result.error = List<String>();

        glslang::EShClient client = glslang::EShClient::EShClientVulkan;

        if (desc.spirvVersion == ShaderCompileSpirvVersion::Spv_1_6)
        {
            result.success = false;
            result.error.emplace_back("不支持 SPIRV 1.6");
            return result;
        }

        glslang::EShTargetLanguageVersion targetLanguageVersion = GetTargetSpirvVersion(desc.spirvVersion);
        glslang::EShTargetClientVersion targetClientVersion = GetTargetClientVersion(desc.targetVersion);

        HLSLToSpirVParame parame;
        parame.includer = m_Includer;
        parame.client = client;
        parame.code = CreateRef<List<const char *>>();
        parame.targetClientVersion = targetClientVersion;
        parame.targetLanguageVersion = targetLanguageVersion;

        bool is_initialize_process = glslang::InitializeProcess();
        if (!is_initialize_process)
        {
            result.error.emplace_back("glslang::InitializeProcess failed");
            result.success = false;
            return result;
        }

        // String version = fmt::format("#version {0}", analyzer->GetVersion());

        if (!sourceCode.empty())
        {
            String error = "";

            switch (desc.target)
            {
            case ShaderCompilerTarget::DX:
                result.error.emplace_back("Compile Shader TO DX is not support.");
                result.success = false;
                break;
            case ShaderCompilerTarget::Vulkan:

                parame.code->clear();
                // parame.code->emplace_back(version.c_str());
                parame.code->emplace_back(sourceCode.c_str());
                parame.shaderType = GetRHIShaderStage(desc.stage);
                parame.entryPoint = desc.enterPoint.c_str();

                String erreo;
                if (HlslShaderCodeToSpirV(parame, result.code, error))
                {
                    result.success = true;
                }
                else
                {
                    result.success = false;
                    result.error.emplace_back(fmt::format("Compile failed \n{1}", error));
                }
                break;
            }
        }

        glslang::FinalizeProcess();

        return result;
    }

    ShaderCompiler::ShaderCompiler() : m_Includer(nullptr), m_Reflection(nullptr)
    {
        m_Includer = new ShaderIncluder();
        m_Reflection = new ShaderReflection();
    }

    ShaderCompiler::~ShaderCompiler()
    {
        delete m_Includer;
        delete m_Reflection;
    }
    */

    struct InternalState_DXC
    {
        Platform::MODULE* dxcompiler;
        DxcCreateInstanceProc DxcCreateInstance;

        InternalState_DXC(const String &modifier = "")
        {
            const String library = String::Format("dxcompiler{0}.dll", modifier);
            ENGINE_ASSERT(Platform::EngineLoadLibrary(library.Get(), dxcompiler));

            if (dxcompiler != nullptr)
            {
                DxcCreateInstance = (DxcCreateInstanceProc)Platform::EngineGetProcAddress(dxcompiler, "DxcCreateInstance");
                if (DxcCreateInstance != nullptr)
                {
                    CComPtr<IDxcCompiler3> dxcCompiler;
                    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
                    ENGINE_ASSERT(SUCCEEDED(hr));
                    CComPtr<IDxcVersionInfo> info;
                    hr = dxcCompiler->QueryInterface(IID_PPV_ARGS(&info));
                    ENGINE_ASSERT(SUCCEEDED(hr));
                    uint32 minor = 0;
                    uint32 major = 0;
                    hr = info->GetVersion(&major, &minor);
                    ENGINE_ASSERT(SUCCEEDED(hr));
                    
                    LOG_INFO("Render", nullptr, "ShaderCompiler: loaded {0} (version: {1}.{2})", library,
						StringUtils::ToString(major),  StringUtils::ToString(minor));
                }

                CComPtr<IDxcCompiler3> dxcCompiler;
                HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
                ENGINE_ASSERT(SUCCEEDED(hr));
                CComPtr<IDxcVersionInfo> info;
                hr = dxcCompiler->QueryInterface(IID_PPV_ARGS(&info));
                ENGINE_ASSERT(SUCCEEDED(hr));
                uint32_t minor = 0;
                uint32_t major = 0;
                hr = info->GetVersion(&major, &minor);
                ENGINE_ASSERT(SUCCEEDED(hr));
                
                LOG_INFO("Render", nullptr, "Shadercompiler: loaded {0} (version: {1}.{2})", library,
                                StringUtils::ToString(major), StringUtils::ToString(minor));
            }
            else
            {
                LOG_ERROR("Render", nullptr, "Shadercompiler: could not load library {0}", library);
            }
        }

        ~InternalState_DXC()
        {
            Platform::EngineFreeLibrary(dxcompiler);
            dxcompiler = nullptr;
        }
    };

    inline InternalState_DXC &DXCCompiler()
    {
        static InternalState_DXC internal_state;
        return internal_state;
    }

    inline InternalState_DXC &DXCCompiler_xs()
    {
        static InternalState_DXC internal_state("_xs");
        return internal_state;
    }

    void Compile_DXCompiler(CompilerInput &input, CompilerOutput &output)
    {
        InternalState_DXC &compiler_internal = input.format == RHIShaderFormat::HLSL6_XS ? DXCCompiler_xs() : DXCCompiler();

        if (compiler_internal.DxcCreateInstance == nullptr)
        {
            return;
        }
        

        CComPtr<IDxcUtils> dxcUtils;
        CComPtr<IDxcCompiler3> dxcCompiler;

        HRESULT hr = compiler_internal.DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
        ENGINE_ASSERT(SUCCEEDED(hr));
        hr = compiler_internal.DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
        ENGINE_ASSERT(SUCCEEDED(hr));

        if (dxcCompiler == nullptr)
        {
            return;
        }

        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#dxcompiler-dll-interface

        List<WString> args = {
            // L"-res-may-alias",
            // L"-flegacy-macro-expansion",
            // L"-no-legacy-cbuf-layout",
            // L"-pack-optimized", // this has problem with tessellation shaders: https://github.com/microsoft/DirectXShaderCompiler/issues/3362
            // L"-all-resources-bound",
            // L"-Gis", // Force IEEE strictness
            // L"-Gec", // Enable backward compatibility mode
            // L"-Ges", // Enable strict mode
            // L"-O0", // Optimization Level 0
        };

        if (input.flags.IsFlagSet(Flags::DISABLE_OPTIMIZATION))
        {
            args.Add(L"-Od");
        }

        switch (input.format)
        {
        case RHIShaderFormat::HLSL6:
        case RHIShaderFormat::HLSL6_XS:
            args.Add(L"-rootsig-define");
            args.Add(L"WICKED_ENGINE_DEFAULT_ROOTSIGNATURE");
            if (input.flags.IsFlagSet(Flags::STRIP_REFLECTION))
            {
                args.Add(L"-Qstrip_reflect"); // only valid in HLSL6 compiler
            }
            break;
        case RHIShaderFormat::SPIRV:
            args.Add(L"-spirv");
            args.Add(L"-fspv-target-env=vulkan1.2");
            args.Add(L"-fvk-use-dx-layout");
            args.Add(L"-fvk-use-dx-position-w");
            args.Add(L"-fvk-b-shift"); 
			args.Add(L"0"); 
			args.Add(L"0");
            args.Add(L"-fvk-t-shift");
            args.Add(L"1000");
            args.Add(L"0");
            args.Add(L"-fvk-u-shift");
            args.Add(L"2000");
            args.Add(L"0");
            args.Add(L"-fvk-s-shift");
            args.Add(L"3000");
            args.Add(L"0");
            break;
        default:
            ENGINE_ASSERT(0);
            return;
        }

        args.Add(L"-T");
        switch (input.stage)
        {
        case RHIShaderStage::MS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"ms_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"ms_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"ms_6_7");
                break;
            }
            break;
        case RHIShaderStage::AS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"as_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"as_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"as_6_7");
                break;
            }
            break;
        case RHIShaderStage::VS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"vs_6_0");
                break;
            case RHIShaderModel::SM_6_1:
                args.Add(L"vs_6_1");
                break;
            case RHIShaderModel::SM_6_2:
                args.Add(L"vs_6_2");
                break;
            case RHIShaderModel::SM_6_3:
                args.Add(L"vs_6_3");
                break;
            case RHIShaderModel::SM_6_4:
                args.Add(L"vs_6_4");
                break;
            case RHIShaderModel::SM_6_5:
                args.Add(L"vs_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"vs_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"vs_6_7");
                break;
            }
            break;
        case RHIShaderStage::HS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"hs_6_0");
                break;
            case RHIShaderModel::SM_6_1:
                args.Add(L"hs_6_1");
                break;
            case RHIShaderModel::SM_6_2:
                args.Add(L"hs_6_2");
                break;
            case RHIShaderModel::SM_6_3:
                args.Add(L"hs_6_3");
                break;
            case RHIShaderModel::SM_6_4:
                args.Add(L"hs_6_4");
                break;
            case RHIShaderModel::SM_6_5:
                args.Add(L"hs_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"hs_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"hs_6_7");
                break;
            }
            break;
        case RHIShaderStage::DS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"ds_6_0");
                break;
            case RHIShaderModel::SM_6_1:
                args.Add(L"ds_6_1");
                break;
            case RHIShaderModel::SM_6_2:
                args.Add(L"ds_6_2");
                break;
            case RHIShaderModel::SM_6_3:
                args.Add(L"ds_6_3");
                break;
            case RHIShaderModel::SM_6_4:
                args.Add(L"ds_6_4");
                break;
            case RHIShaderModel::SM_6_5:
                args.Add(L"ds_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"ds_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"ds_6_7");
                break;
            }
            break;
        case RHIShaderStage::GS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"gs_6_0");
                break;
            case RHIShaderModel::SM_6_1:
                args.Add(L"gs_6_1");
                break;
            case RHIShaderModel::SM_6_2:
                args.Add(L"gs_6_2");
                break;
            case RHIShaderModel::SM_6_3:
                args.Add(L"gs_6_3");
                break;
            case RHIShaderModel::SM_6_4:
                args.Add(L"gs_6_4");
                break;
            case RHIShaderModel::SM_6_5:
                args.Add(L"gs_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"gs_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"gs_6_7");
                break;
            }
            break;
        case RHIShaderStage::PS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"ps_6_0");
                break;
            case RHIShaderModel::SM_6_1:
                args.Add(L"ps_6_1");
                break;
            case RHIShaderModel::SM_6_2:
                args.Add(L"ps_6_2");
                break;
            case RHIShaderModel::SM_6_3:
                args.Add(L"ps_6_3");
                break;
            case RHIShaderModel::SM_6_4:
                args.Add(L"ps_6_4");
                break;
            case RHIShaderModel::SM_6_5:
                args.Add(L"ps_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"ps_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"ps_6_7");
                break;
            }
            break;
        case RHIShaderStage::CS:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"cs_6_0");
                break;
            case RHIShaderModel::SM_6_1:
                args.Add(L"cs_6_1");
                break;
            case RHIShaderModel::SM_6_2:
                args.Add(L"cs_6_2");
                break;
            case RHIShaderModel::SM_6_3:
                args.Add(L"cs_6_3");
                break;
            case RHIShaderModel::SM_6_4:
                args.Add(L"cs_6_4");
                break;
            case RHIShaderModel::SM_6_5:
                args.Add(L"cs_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"cs_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"cs_6_7");
                break;
            }
            break;
        case RHIShaderStage::LIB:
            switch (input.minshadermodel)
            {
            default:
                args.Add(L"lib_6_5");
                break;
            case RHIShaderModel::SM_6_6:
                args.Add(L"lib_6_6");
                break;
            case RHIShaderModel::SM_6_7:
                args.Add(L"lib_6_7");
                break;
            }
            break;
        default:
            ENGINE_ASSERT(0);
            return;
        }

        for (auto &x : input.defines)
        {
            args.Add(L"-D");
			args.Add(WString(x));
        }

        for (auto &x : input.include_directories)
        {
            args.Add(L"-I");
			args.Add(WString(x));
        }

        // Entry point parameter:
        args.Add(L"-E");
        args.Add(WString(input.entrypoint));

        // Add source file name as last parameter. This will be displayed in error messages
        args.Add(WString(input.shadersourcefilename));

        DxcBuffer Source;
        Source.Ptr = input.shadersourcedata;
        Source.Size = input.shadersourceLength;
        Source.Encoding = DXC_CP_ACP;

        struct IncludeHandler : public IDxcIncludeHandler
        {
            const CompilerInput *input = nullptr;
            CompilerOutput *output = nullptr;
            CComPtr<IDxcIncludeHandler> dxcIncludeHandler;

            HRESULT STDMETHODCALLTYPE LoadSource(
                _In_z_ LPCWSTR pFilename,                                // Candidate filename.
                _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource // Resultant source object for included file, nullptr if not found.
                ) override
            {
                HRESULT hr = dxcIncludeHandler->LoadSource(pFilename, ppIncludeSource);
                if (SUCCEEDED(hr))
                {
                    output->dependencies.Add(String(pFilename));
                }
                return hr;
            }
            HRESULT STDMETHODCALLTYPE QueryInterface(
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
            {
                return dxcIncludeHandler->QueryInterface(riid, ppvObject);
            }

            ULONG STDMETHODCALLTYPE AddRef(void) override
            {
                return 0;
            }
            ULONG STDMETHODCALLTYPE Release(void) override
            {
                return 0;
            }
        } includehandler;
        includehandler.input = &input;
        includehandler.output = &output;

        hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler.dxcIncludeHandler);
        ENGINE_ASSERT(SUCCEEDED(hr));

        List<const wchar_t *> args_raw;
        for (auto &x : args)
        {
            args_raw.Add(x.Get());
        }

        CComPtr<IDxcResult> pResults;
        hr = dxcCompiler->Compile(
            &Source,                // Source buffer.
            args_raw.Get(),        // Array of pointers to arguments.
            (uint32)args.Count(),  // Number of arguments.
            &includehandler,        // User-provided interface to handle #include directives (optional).
            IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
        );
        ENGINE_ASSERT(SUCCEEDED(hr));

        CComPtr<IDxcBlobUtf8> pErrors = nullptr;
        hr = pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
        ENGINE_ASSERT(SUCCEEDED(hr));
        if (pErrors != nullptr && pErrors->GetStringLength() != 0)
        {
            output.error_message = pErrors->GetStringPointer();
        }

        HRESULT hrStatus;
        hr = pResults->GetStatus(&hrStatus);
        ENGINE_ASSERT(SUCCEEDED(hr));
        if (FAILED(hrStatus))
        {
            return;
        }

        CComPtr<IDxcBlob> pShader = nullptr;
        hr = pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);
        ENGINE_ASSERT(SUCCEEDED(hr));
        if (pShader != nullptr)
        {
            output.dependencies.Add(input.shadersourcefilename);
            output.shaderBytedata = (const uint8 *)pShader->GetBufferPointer();
            output.shaderBytesize = pShader->GetBufferSize();

            // keep the blob alive == keep shader pointer valid!
            output.native = CreateRef<CComPtr<IDxcBlob>>(pShader);
        }

        if (input.format == RHIShaderFormat::HLSL6)
        {
            CComPtr<IDxcBlob> pHash = nullptr;
            hr = pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&pHash), nullptr);
            ENGINE_ASSERT(SUCCEEDED(hr));
            if (pHash != nullptr)
            {
                DxcShaderHash *pHashBuf = (DxcShaderHash *)pHash->GetBufferPointer();
                for (int i = 0; i < _countof(pHashBuf->HashDigest); i++)
                {
                    output.shaderhash.Add(pHashBuf->HashDigest[i]);
                }
            }
        }
    }

    void Compile(CompilerInput &input, CompilerOutput &output)
    {
        output = CompilerOutput();

#ifdef SHADERCOMPILER_ENABLED
        switch (input.format)
        {
        default:
            break;
        case RHIShaderFormat::HLSL6:
        case RHIShaderFormat::SPIRV:
        case RHIShaderFormat::HLSL6_XS:
            Compile_DXCompiler(input, output);
            break;

#endif
        }
    }
}
