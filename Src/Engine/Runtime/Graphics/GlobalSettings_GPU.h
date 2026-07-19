#pragma once

#include "Runtime/Graphics/Base/GPUEnums.h"
#include "Runtime/Settings/Settings.h"

//-------------------------------------------------------------------------

namespace SE
{
    SE_CLASS(Reflect)
    class GPUGlobalSettings : public GlobalSettings
    {
        SE_DEFINE_CLASS(GPUGlobalSettings, GlobalSettings);

    public:
    	GPUGlobalSettings();
        virtual bool LoadSettings( IniFile const& ini ) override;
        virtual bool SaveSettings( IniFile& ini ) const override;

    public:
		GPURendererType     type;
    	ShaderProfile		shaderProfile;
        RHIValidationMode   validationMode;
        RHIGPUPreference    gpuPreference;
    };

	// 绑定渲染目标的最大数量
	#define GPU_MAX_RT_BINDED 6

	// 绑定的着色器资源的最大数量
	#define GPU_MAX_SR_BINDED 32

	// 绑定常量缓冲区的最大数量
	#define GPU_MAX_CB_BINDED 4

	// 绑定的无序访问资源的最大数量
	#define GPU_MAX_UA_BINDED 4

	// 绑定纹理采样器资源的最大数量
	#define GPU_MAX_SAMPLER_BINDED 16

	// 全局采样器的初始插槽数量(静态，4个普通采样器+ 2个比较采样器)
	#define GPU_STATIC_SAMPLERS_COUNT 6

	// 绑定顶点缓冲区的最大数量
	#define GPU_MAX_VB_BINDED 4

	// 计算着色器调度的每个维度的线程组的最大数量
	#define GPU_MAX_CS_DISPATCH_THREAD_GROUPS 65535

	// 启用/禁用图形层断言
	#define GPU_ENABLE_ASSERTION 1

	// Enable/disable dynamic textures quality streaming
	#define GPU_ENABLE_TEXTURES_STREAMING 1

	// Enable/disable creating Shader Resource View for window backbuffer surface
	#define GPU_USE_WINDOW_SRV 1

	// True if allow graphics profile events and markers
	#define GPU_ALLOW_PROFILE_EVENTS (!BUILD_RELEASE)

	// Enable/disable creating GPU resources on separate threads (otherwise only the main thread can be used)
	#define GPU_ENABLE_ASYNC_RESOURCES_CREATION 1

	// Enable/disable force shaders recompilation
	#define GPU_FORCE_RECOMPILE_SHADERS 0

	// Define default back buffer(s) format
	#ifndef GPU_BACK_BUFFER_PIXEL_FORMAT
	#define GPU_BACK_BUFFER_PIXEL_FORMAT PixelFormat::R8G8B8A8_UNorm
	#endif

	// Default depth buffer pixel format
	#ifndef GPU_DEPTH_BUFFER_PIXEL_FORMAT
	#define GPU_DEPTH_BUFFER_PIXEL_FORMAT PixelFormat::D32_Float
	#endif

	// Enable/disable gpu resources naming
	#define GPU_ENABLE_RESOURCE_NAMING 1 // (!BUILD_RELEASE)

	// True if use debug tools and flow for shaders
	#define GPU_USE_SHADERS_DEBUG_LAYER (BUILD_DEBUG)

	// Maximum size of the texture that is supported by the engine (specific platforms can have lower limit)
	#define GPU_MAX_TEXTURE_SIZE 16384
	#define GPU_MAX_TEXTURE_MIP_LEVELS 15
	#define GPU_MAX_TEXTURE_ARRAY_SIZE 1024

	// Validate configuration
	#if !ENABLE_ASSERTION
	#undef GPU_ENABLE_ASSERTION
	#define GPU_ENABLE_ASSERTION 0
	#endif
}