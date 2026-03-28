#pragma once

#include "Runtime/API.h"
#include "PixelFormat.h"

#include "Core/Types/Variable.h"
#include "../../../Core/Types/Hash.h"
#include "Core/TypeSystem/IType.h"


namespace SE
{
	SE_ENUM(GPUResourceUsage)
	enum class GPUResourceUsage
	{
		/// <summary>
		/// A resource that requires read and write access by the GPU.
		/// This is likely to be the most common usage choice.
		/// Memory will be used on device only, so fast access from the device is preferred.
		/// It usually means device-local GPU (video) memory.
		/// </summary>
		/// <remarks>
		/// Usage:
		/// - Resources written and read by device, e.g. images used as render targets.
		/// - Resources transferred from host once (immutable) or infrequently and read by
		///   device multiple times, e.g. textures to be sampled, vertex buffers, constant
		///   buffers, and majority of other types of resources used on GPU.
		/// </remarks>
		Default = 0,

		/// <summary>
		/// A resource that is accessible by both the GPU (read only) and the CPU (write only).
		/// A dynamic resource is a good choice for a resource that will be updated by the CPU at least once per frame.
		/// Dynamic buffers or textures are usually used to upload data to GPU and use it within a single frame.
		/// </summary>
		/// <remarks>
		/// Usage:
		/// - Resources written frequently by CPU (dynamic), read by device.
		///   E.g. textures, vertex buffers, uniform buffers updated every frame or every draw call.
		/// </remarks>
		Dynamic = 1,

		/// <summary>
		/// A resource that supports data transfer (copy) from the CPU to the GPU.
		/// It usually means CPU (system) memory. Resources created in this pool may still be accessible to the device, but access to them can be slow.
		/// </summary>
		/// <remarks>
		/// Usage:
		/// - Staging copy of resources used as transfer source.
		/// </remarks>
		StagingUpload = 2,

		/// <summary>
		/// A resource that supports data transfer (copy) from the GPU to the CPU.
		/// </summary>
		/// <remarks>
		/// Usage:
		/// - Resources written by device, read by host - results of some computations, e.g. screen capture, average scene luminance for HDR tone mapping.
		/// - Any resources read or accessed randomly on host, e.g. CPU-side copy of vertex buffer used as source of transfer, but also used for collision detection.
		/// </remarks>
		StagingReadback = 3,
	};

	/// <summary>
	/// Describes how a mapped GPU resource will be accessed.
	/// </summary>
	SE_ENUM(GPUResourceMapMode)
	enum class GPUResourceMapMode
	{
		/// <summary>
		/// The resource is mapped for reading.
		/// </summary>
		Read = 0x01,

		/// <summary>
		/// The resource is mapped for writing.
		/// </summary>
		Write = 0x02,

		/// <summary>
		/// The resource is mapped for reading and writing.
		/// </summary>
		ReadWrite = Read | Write,
	};

	/// <summary>
	/// Comparison function modes
	/// </summary>
	enum class ComparisonFunc : byte
	{
		// Never pass the comparison.
		Never = 1,
		// If the source data is less than the destination data, the comparison passes.
		Less = 2,
		// If the source data is equal to the destination data, the comparison passes.
		Equal = 3,
		// If the source data is less than or equal to the destination data, the comparison passes.
		LessEqual = 4,
		// If the source data is greater than the destination data, the comparison passes.
		Greater = 5,
		// If the source data is not equal to the destination data, the comparison passes.
		NotEqual = 6,
		// If the source data is greater than or equal to the destination data, the comparison passes.
		GreaterEqual = 7,
		// Always pass the comparison.
		Always = 8,

		END
	};

	/// <summary>
	/// Primitives types.
	/// </summary>
	enum class PrimitiveTopologyType
	{
		/// <summary>
		/// Unknown topology.
		/// </summary>
		Undefined = 0,

		/// <summary>
		/// Points list.
		/// </summary>
		Point = 1,

		/// <summary>
		/// Line list.
		/// </summary>
		Line = 2,

		/// <summary>
		/// Triangle list.
		/// </summary>
		Triangle = 3,
	};

	/// <summary>
	/// Primitives culling mode.
	/// </summary>
	enum class CullMode : byte
	{
		/// <summary>
		/// Cull back-facing primitives only.
		/// </summary>
		Normal = 0,

		/// <summary>
		/// Cull front-facing primitives only.
		/// </summary>
		Inverted = 1,

		/// <summary>
		/// Disable face culling.
		/// </summary>
		TwoSided = 2,
	};

	/// <summary>
	/// Render target blending mode descriptor.
	/// </summary>
	struct SE_API_RUNTIME BlendingMode
	{
	public:
		/// <summary>
		/// Blending mode.
		/// </summary>
		enum class Blend
		{
			// The blend factor is (0, 0, 0, 0). No pre-blend operation.
			Zero = 1,
			// The blend factor is (1, 1, 1, 1). No pre-blend operation.
			One = 2,
			// The blend factor is (Rs, Gs, Bs, As), that is color data (RGB) from a pixel shader. No pre-blend operation.
			SrcColor = 3,
			// The blend factor is (1 - Rs, 1 - Gs, 1 - Bs, 1 - As), that is color data (RGB) from a pixel shader. The pre-blend operation inverts the data, generating 1 - RGB.
			InvSrcColor = 4,
			// The blend factor is (As, As, As, As), that is alpha data (A) from a pixel shader. No pre-blend operation.
			SrcAlpha = 5,
			// The blend factor is ( 1 - As, 1 - As, 1 - As, 1 - As), that is alpha data (A) from a pixel shader. The pre-blend operation inverts the data, generating 1 - A.
			InvSrcAlpha = 6,
			// The blend factor is (Ad Ad Ad Ad), that is alpha data from a render target. No pre-blend operation.
			DestAlpha = 7,
			// The blend factor is (1 - Ad 1 - Ad 1 - Ad 1 - Ad), that is alpha data from a render target. The pre-blend operation inverts the data, generating 1 - A.
			InvDestAlpha = 8,
			// The blend factor is (Rd, Gd, Bd, Ad), that is color data from a render target. No pre-blend operation.
			DestColor = 9,
			// The blend factor is (1 - Rd, 1 - Gd, 1 - Bd, 1 - Ad), that is color data from a render target. The pre-blend operation inverts the data, generating 1 - RGB.
			InvDestColor = 10,
			// The blend factor is (f, f, f, 1); where f = min(As, 1 - Ad). The pre-blend operation clamps the data to 1 or less.
			SrcAlphaSat = 11,
			// The blend factor is the blend factor set with GPUContext::SetBlendFactor. No pre-blend operation.
			BlendFactor = 14,
			// The blend factor is the blend factor set with GPUContext::SetBlendFactor. The pre-blend operation inverts the blend factor, generating 1 - blend_factor.
			BlendInvFactor = 15,
			// The blend factor is data sources both as color data output by a pixel shader. There is no pre-blend operation. This blend factor supports dual-source color blending.
			Src1Color = 16,
			// The blend factor is data sources both as color data output by a pixel shader. The pre-blend operation inverts the data, generating 1 - RGB. This blend factor supports dual-source color blending.
			InvSrc1Color = 17,
			// The blend factor is data sources as alpha data output by a pixel shader. There is no pre-blend operation. This blend factor supports dual-source color blending.
			Src1Alpha = 18,
			// The blend factor is data sources as alpha data output by a pixel shader. The pre-blend operation inverts the data, generating 1 - A. This blend factor supports dual-source color blending.
			InvSrc1Alpha = 19,

			END
		};

		/// <summary>
		/// Blending operation.
		/// </summary>
		enum class Operation
		{
			// Add source 1 and source 2.
			Add = 1,
			// Subtract source 1 from source 2.
			Subtract = 2,
			// Subtract source 2 from source 1.
			RevSubtract = 3,
			// Find the minimum of source 1 and source 2.
			Min = 4,
			// Find the maximum of source 1 and source 2.
			Max = 5,

			END
		};

		/// <summary>
		/// Render target write mask
		/// </summary>
		enum class ColorWrite
		{
			// No color writing.
			None = 0,

			// Allow data to be stored in the red component.
			Red = 1,
			// Allow data to be stored in the green component.
			Green = 2,
			// Allow data to be stored in the blue component.
			Blue = 4,
			// Allow data to be stored in the alpha component.
			Alpha = 8,

			// Allow data to be stored in all components.
			All = Red | Green | Blue | Alpha,
			// Allow data to be stored in red and green components.
			RG = Red | Green,
			// Allow data to be stored in red, green and blue components.
			RGB = Red | Green | Blue,
			// Allow data to be stored in all components.
			RGBA = Red | Green | Blue | Alpha,
		};

	public:
		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		bool AlphaToCoverageEnable;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		bool BlendEnable;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		Blend SrcBlend;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		Blend DestBlend;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		Operation BlendOp;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		Blend SrcBlendAlpha;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		Blend DestBlendAlpha;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		Operation BlendOpAlpha;

		/// <summary>
		/// Render target blending mode descriptor.
		/// </summary>
		ColorWrite RenderTargetWriteMask;

	public:
		bool operator==(const BlendingMode& other) const;

	public:
		/// <summary>
		/// Gets the opaque rendering (default). No blending is being performed.
		/// </summary>
		static BlendingMode Opaque;

		/// <summary>
		/// Gets the additive rendering. Adds the color and the alpha channel. Source color is multiplied by the alpha.
		/// </summary>
		static BlendingMode Additive;

		/// <summary>
		/// Gets the alpha blending. Source alpha controls the output color (0 - use destination color, 1 - use source color).
		/// </summary>
		static BlendingMode AlphaBlend;

		/// <summary>
		/// Gets the additive blending with pre-multiplied color.
		/// </summary>
		static BlendingMode Add;

		/// <summary>
		/// Gets the multiply blending (multiply output color with texture color).
		/// </summary>
		static BlendingMode Multiply;
	};

	uint32 GetHash(const BlendingMode& key);

	/// <summary>
	/// 多样本采样等级
	/// </summary>
	SE_ENUM(MSAALevel)
	enum class MSAALevel : int32
	{
		/// <summary>
		/// Disabled multisampling.
		/// </summary>
		None = 1,

		/// <summary>
		/// Two samples per pixel.
		/// </summary>
		X2 = 2,

		/// <summary>
		/// Four samples per pixel.
		/// </summary>
		X4= 4,

		/// <summary>
		/// Eight samples per pixel.
		/// </summary>
		X8 = 8,
	};

	StringView ToString(MSAALevel value);


	/// <summary>
	/// Shader profile types define the version and type of the shading language used by the graphics backend.
	/// </summary>
	enum class ShaderProfile
	{
		/// <summary>
		/// Unknown
		/// </summary>
		Unknown = 0,

		/// <summary>
		/// DirectX (Shader Model 4 compatible)
		/// </summary>
		DirectX_SM4 = 1,

		/// <summary>
		/// DirectX (Shader Model 5 compatible)
		/// </summary>
		DirectX_SM5 = 2,

		/// <summary>
		/// GLSL 410
		/// </summary>
		GLSL_410 = 3,

		/// <summary>
		/// GLSL 440
		/// </summary>
		GLSL_440 = 4,

		/// <summary>
		/// Vulkan (Shader Model 5 compatible)
		/// </summary>
		Vulkan_SM5 = 5,

		/// <summary>
		/// PlayStation 4
		/// </summary>
		PS4 = 6,

		/// <summary>
		/// DirectX (Shader Model 6 compatible)
		/// </summary>
		DirectX_SM6 = 7,

		/// <summary>
		/// PlayStation 5
		/// </summary>
		PS5 = 8,

		MAX
	};

	StringView ToString(ShaderProfile value);

	/// <summary>
	/// Graphics feature levels indicates what level of support can be relied upon.
	/// They are named after the graphics API to indicate the minimum level of the features set to support.
	/// Feature levels are ordered from the lowest to the most high-end so feature level enum can be used to switch between feature levels (e.g. don't use geometry shader if not supported).
	/// </summary>
	enum class FeatureLevel
	{
		/// <summary>
		/// The features set defined by the core capabilities of OpenGL ES2.
		/// </summary>
		ES2 = 0,

		/// <summary>
		/// The features set defined by the core capabilities of OpenGL ES3.
		/// </summary>
		ES3 = 1,

		/// <summary>
		/// The features set defined by the core capabilities of OpenGL ES3.1.
		/// </summary>
		ES3_1 = 2,

		/// <summary>
		/// The features set defined by the core capabilities of DirectX 10 Shader Model 4.
		/// </summary>
		SM4 = 3,

		/// <summary>
		/// The features set defined by the core capabilities of DirectX 11 Shader Model 5.
		/// </summary>
		SM5 = 4,

		/// <summary>
		/// The features set defined by the core capabilities of DirectX 12 Shader Model 6.
		/// </summary>
		SM6 = 5,

		MAX
	};

	const Char* ToString(FeatureLevel value);

	/// <summary>
	/// Describes the shader function flags used for shader compilation.
	/// </summary>
	enum class ShaderFlags : uint32
	{
		/// <summary>
		/// The default set for flags.
		/// </summary>
		Default = 0,

		/// <summary>
		/// Hides the shader. It will exist in source and will be parsed but won't be compiled for the rendering.
		/// </summary>
		Hidden = 1,

		/// <summary>
		/// Disables any fast-math optimizations performed by the shader compiler.
		/// </summary>
		NoFastMath = 2,

		/// <summary>
		/// Indicates that vertex shader function outputs data for the geometry shader.
		/// </summary>
		VertexToGeometryShader = 4,
	};


	/// <summary>
	/// Describes the different tessellation methods supported by the graphics system.
	/// </summary>
	enum class TessellationMethod
	{
		/// <summary>
		/// No tessellation.
		/// </summary>
		None = 0,

		/// <summary>
		/// Flat tessellation. Also known as dicing tessellation.
		/// </summary>
		Flat = 1,

		/// <summary>
		/// Point normal tessellation.
		/// </summary>
		PointNormal = 2,

		/// <summary>
		/// Geometric version of Phong normal interpolation, not applied on normals but on the vertex positions.
		/// </summary>
		Phong = 3,
	};

	SE_ENUM(GPURendererType)
	enum class GPURendererType
	{
		Vulkan,
	};


	/////////////////////////////////////////////////////////////////////////


    // 验证模式
    enum class RHIValidationMode
    {
        Disabled, // No validation is enabled
        Enabled,  // CPU command validation
        GPU,      // CPU and GPU-based validation
        Verbose   // Print all warnings, errors and info messages
    };

    enum class RHIGPUPreference
    {
		// 独立显卡
        Discrete,
		// 集成显卡
        Integrated,
    };

    // 物理设备类型
    enum class RHIAdapterType
    {
        Other,
        IntegratedGpu,
        DiscreteGpu,
        VirtualGpu,
        Cpu,
    };

    enum class RHIShadingRate
    {
        RATE_1X1, // Default/full shading rate
        RATE_1X2,
        RATE_2X1,
        RATE_2X2,
        RATE_2X4,
        RATE_4X2,
        RATE_4X4,

        RATE_INVALID
    };



    enum class RHIShaderFormat
    {
        NONE,     // Not used
        HLSL5,    // DXBC
        HLSL6,    // DXIL
        SPIRV,    // SPIR-V
        HLSL6_XS, // XBOX Series Native
        PS5,      // Playstation 5
    };

	SE_ENUM(RHIShaderModel)
    enum class RHIShaderModel
    {
        SM_5_0,
        SM_6_0,
        SM_6_1,
        SM_6_2,
        SM_6_3,
        SM_6_4,
        SM_6_5,
        SM_6_6,
        SM_6_7,
    };



    enum class RHIBlendOp : uint8
    {
        ADD,
        SUBTRACT,
        REVERSE_SUBTRACT,
        MIN,
        MAX
    };

    enum class RHIBlendFactor : uint8
    {
        ZERO,
        ONE,
        SRC_COLOR,
        INV_SRC_COLOR,
        SRC_ALPHA,
        INV_SRC_ALPHA,
        DEST_ALPHA,
        INV_DEST_ALPHA,
        DEST_COLOR,
        INV_DEST_COLOR,
        SRC_ALPHA_SAT,
        BLEND_FACTOR,
        INV_BLEND_FACTOR,
        SRC1_COLOR,
        INV_SRC1_COLOR,
        SRC1_ALPHA,
        INV_SRC1_ALPHA,
    };

    enum class RHIStencilOp : uint8
    {
        Keep,
        ZERO,
        Replace,
        IncrementAndClamp,
        DecrementAndClamp,
        Invert,
        IncrementAndWarp,
        DecrementAndWarp
    };

	SE_ENUM(RHIFilter)
    enum class RHIFilter
    {
        MIN_MAG_MIP_POINT,
        MIN_MAG_POINT_MIP_LINEAR,
        MIN_POINT_MAG_LINEAR_MIP_POINT,
        MIN_POINT_MAG_MIP_LINEAR,
        MIN_LINEAR_MAG_MIP_POINT,
        MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MIN_MAG_LINEAR_MIP_POINT,
        MIN_MAG_MIP_LINEAR,
        ANISOTROPIC,
        COMPARISON_MIN_MAG_MIP_POINT,
        COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
        COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
        COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
        COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
        COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        COMPARISON_MIN_MAG_MIP_LINEAR,
        COMPARISON_ANISOTROPIC,
        MINIMUM_MIN_MAG_MIP_POINT,
        MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
        MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
        MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
        MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
        MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
        MINIMUM_MIN_MAG_MIP_LINEAR,
        MINIMUM_ANISOTROPIC,
        MAXIMUM_MIN_MAG_MIP_POINT,
        MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
        MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
        MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
        MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
        MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
        MAXIMUM_MIN_MAG_MIP_LINEAR,
        MAXIMUM_ANISOTROPIC,
    };

    enum class RHIAddressMode : uint8
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
        MirroreOnce
    };

    enum class RHIBorderColor : uint8
    {
        TransparentBlack,
        OpaqueBlack,
        OpaqueWhite,
    };

    enum class RHIUsage : uint8
    {
        Default,  // CPU no access, GPU read/write
        Upload,   // CPU write, GPU read
        Readback, // CPU read, GPU write
    };

    enum class ComponentSwizzle : uint8
    {
        R,
        G,
        B,
        A,
        ZERO,
        ONE,
    };

    enum class RHIPrimitiveTopology : uint8
    {
        Undefined,
        TriangleList,
        TriangleStrip,
        PointList,
        LineList,
        LineStrip,
        PatchList,
    };

    enum class RHIIndexBufferFormat
    {
        UInt16 = 0,
        UInt32 = 1,
    };

    enum class RHIColorSpace
    {
        SRGB,         // SDR color space (8 or 10 bits per channel)
        HDR10_ST2084, // HDR10 color space (10 bits per channel)
        HDR_LINEAR,   // HDR color space (16 bits per channel)
    };

    enum class RHIImageAspect
    {
        Color,
        Depth,
        Stencil,
        Luminance,
        Chrominance,
    };

    enum class RHIResourceViewType
    {
        SRV, // shader resource view
        UAV, // unordered access view
        RTV, // render target view
        DSV, // depth stencil view
    };

    enum RHIQueueType
    {
        RHIQueueType_Graphics,
        RHIQueueType_Compute,
        RHIQueueType_Copy,
        RHIQueueType_Count
    };

    /*************Flag*************/

    // 设备功能支持
    enum class RHIGraphicsDeviceCapabilityFlag
    {
        NONE = 0,
        // 曲面细分
        Tessellation = 1 << 0,
        CONSERVATIVE_RASTERIZATION = 1 << 1,
        RASTERIZER_ORDERED_VIEWS = 1 << 2,
        Uav_Load_Format_Views = 1 << 3, // eg: R16G16B16A16_Float, R8G8B8A8_UNorm and more common ones
        Uav_Load_Format_R11G11B10_Float = 1 << 4,
        RenderTarget_Add_Viewport_Arrayindex_Without_GS = 1 << 5,
        // pipeline VRS 可变着色率
        Variable_Rate_Shading = 1 << 6,
        // attachment VRS
        Variable_Rate_Shading_Attachment = 1 << 7,
        MeshShader = 1 << 8,
        RayTracting = 1 << 9,
		// 是否支持条件渲染
        Predication = 1 << 10,
        Sampler_Minmax = 1 << 11,
        // 深度边界测试
        Depth_Bounds_Test = 1 << 12,
        Sparse_Buffer = 1 << 13,
        Sparse_Texture2D = 1 << 14,
        Sparse_Texture3D = 1 << 15,
        Sparse_Null_Mapping = 1 << 16,
        Generic_Sparse_Tile_Pool = 1 << 17, // allows using ResourceMiscFlag::SPARSE_TILE_POOL (non resource type specific version)
                                            // 深度解析模式， 样本值中的最小值，样本值中的最大值
        Depth_Resolve_Min_Max = 1 << 18,
        Stencil_Resolve_Min_Max = 1 << 19,
        Cache_Coherent_UMA = 1 << 20, // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_architecture
        VIDEO_DECODE_H264 = 1 << 21,
        R9G9B9E5_SHAREDEXP_RENDERABLE = 1 << 22, // indicates supporting R9G9B9E5_SHAREDEXP format for rendering to
    };

    enum class RHIColorWriteFlag
    {
        Disable = 0,
        Enable_Red = 1 << 0,
        Enable_Green = 1 << 1,
        Enable_Blue = 1 << 2,
        Enable_Alpha = 1 << 3,
        Enable_All = ~0,
    };

    enum class RHIBindFlag
    {
        None = 0,
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        ConstantBuffer = 1 << 2,
        ShaderResources = 1 << 3,
        RenderTarget = 1 << 4,
        DepthStencil = 1 << 5,
        UnorderedAccess = 1 << 6,
        ShadingRate = 1 << 7,
    };

    enum class RHIAccessFlagBits
    {
        INDIRECT_COMMAND_READ_BIT = 0x00000001,
        INDEX_READ_BIT = 0x00000002,
        VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
        UNIFORM_READ_BIT = 0x00000008,
        INPUT_ATTACHMENT_READ_BIT = 0x00000010,
        SHADER_READ_BIT = 0x00000020,
        SHADER_WRITE_BIT = 0x00000040,
        COLOR_ATTACHMENT_READ_BIT = 0x00000080,
        COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
        DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
        DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
        TRANSFER_READ_BIT = 0x00000800,
        TRANSFER_WRITE_BIT = 0x00001000,
        HOST_READ_BIT = 0x00002000,
        HOST_WRITE_BIT = 0x00004000,
        MEMORY_READ_BIT = 0x00008000,
        MEMORY_WRITE_BIT = 0x00010000,
        TRANSFORM_FEEDBACK_WRITE_BIT_EXT = 0x02000000,
        TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT = 0x04000000,
        TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT = 0x08000000,
        CONDITIONAL_RENDERING_READ_BIT_EXT = 0x00100000,
        COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT = 0x00080000,
        ACCELERATION_STRUCTURE_READ_BIT_KHR = 0x00200000,
        ACCELERATION_STRUCTURE_WRITE_BIT_KHR = 0x00400000,
        FRAGMENT_DENSITY_MAP_READ_BIT_EXT = 0x01000000,
        FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR = 0x00800000,
        COMMAND_PREPROCESS_READ_BIT_NV = 0x00020000,
        COMMAND_PREPROCESS_WRITE_BIT_NV = 0x00040000,
        NONE_KHR = 0,
        SHADING_RATE_IMAGE_READ_BIT_NV = FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
        ACCELERATION_STRUCTURE_READ_BIT_NV = ACCELERATION_STRUCTURE_READ_BIT_KHR,
        ACCELERATION_STRUCTURE_WRITE_BIT_NV = ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
        FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
    };

    enum class RHIResourceMiscFlag
    {
        None = 0,
        TextureCube = 1 << 0,
        IndirectArgs = 1 << 1,
        BufferRaw = 1 << 2,
        BufferStructured = 1 << 3,
        RayTracing = 1 << 4,
        Predication = 1 << 5,
        // hint: used in renderpass, without needing to write content to memory (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        TransientAttachment = 1 << 6,
        // sparse resource without backing memory allocation
        Sparse = 1 << 7,
        // buffer only, makes it suitable for containing tile memory for sparse buffers
        SparseTilePoolBuffer = 1 << 8,
        // buffer only, makes it suitable for containing tile memory for sparse textures that are non render targets nor depth stencils
        SparseTilePoolTextureNonRTDS = 1 << 9,
        // buffer only, makes it suitable for containing tile memory for sparse textures that are either render targets or depth stencils
        SparseTilePoolTextureRTDS = 1 << 10,
        // buffer only, makes it suitable for containing tile memory for all kinds of sparse resources. Requires GraphicsDeviceCapability::GENERIC_SPARSE_TILE_POOL to be supported
        SparseTilePool = SparseTilePoolBuffer | SparseTilePoolTextureNonRTDS | SparseTilePoolTextureRTDS,
        // enable casting formats between same type and different modifiers: eg. UNorm -> SRGB
        TypedFormatCasting = 1 << 11,
        // enable casting formats to other formats that have the same bit-width and channel layout: eg. R32_Float -> R32_UInt
        TypelessFormatCasting = 1 << 12,
        // skips creation of default descriptors for resources
        NoDefaultDescripiors = 1 << 14,
        // optimization that can enable sampling from compressed textures
        TextureCompatibleCompression = 1 << 15,
    };

    enum class RHIResourceStateFlag
    {
        // Common resource states:
        Undefined = 0,                  // invalid state (don't preserve contents)
        ShaderResource = 1 << 0,        // shader resource, read only
        ShaderResourceCompute = 1 << 1, // shader resource, read only, non-pixel shader
        UnorderedAccess = 1 << 2,       // shader resource, write enabled
        CopySrc = 1 << 3,               // copy from
        CopyDst = 1 << 4,               // copy to

        // Texture specific resource states:
        Rendertarget = 1 << 5,         // render target, write enabled
        DepthStencil = 1 << 6,         // depth stencil, write enabled
        DepthStencilReadonly = 1 << 7, // depth stencil, read only
        ShadingRateSource = 1 << 8,    // shading rate control per tile

        // GPUBuffer specific resource states:
        VertexBuffer = 1 << 9,                     // vertex buffer, read only
        IndexBuffer = 1 << 10,                     // index buffer, read only
        ConstantBuffer = 1 << 11,                  // constant buffer, read only
        IndirectArgument = 1 << 12,                // argument buffer to DrawIndirect() or DispatchIndirect()
        RaytracingAccelerationStructure = 1 << 13, // acceleration structure storage or scratch
        Predication = 1 << 14,                     // storage for predication comparison value

        // Other:
        VideoDecodeSrc = 1 << 15, // video decode operation source (bitstream buffer or DPB texture)
        VideoDecodeDst = 1 << 16, // video decode operation destination DPB texture
    };

    enum class RHIRenderPassFlags
    {
        None = 0,
        Allow_UAV_Writes = 1 << 0,
        Suspending = 1 << 1,
        Resuming = 1 << 2,
    };





    constexpr RHIIndexBufferFormat RHIIndexBufferFormatGet(PixelFormat format)
    {
        switch (format)
        {
        default:
        case PixelFormat::R32_UInt:
            return RHIIndexBufferFormat::UInt32;
        case PixelFormat::R16_UInt:
            return RHIIndexBufferFormat::UInt16;
        }
    }
    constexpr RHIIndexBufferFormat RHIIndexBufferFormatGet(uint32 vertex_count)
    {
        return vertex_count > 65536 ? RHIIndexBufferFormat::UInt32 : RHIIndexBufferFormat::UInt16;
    }
    constexpr PixelFormat RHIIndexBufferFormatGetRaw(uint32 vertex_count)
    {
        return vertex_count > 65536 ? PixelFormat::R32_UInt : PixelFormat::R16_UInt;
    }
    
    constexpr const char *IndexBufferFormatGetString(RHIIndexBufferFormat format)
    {
        switch (format)
        {
        default:
        case RHIIndexBufferFormat::UInt32:
            return "UINT32";
        case RHIIndexBufferFormat::UInt16:
            return "UINT16";
        }
    }
}