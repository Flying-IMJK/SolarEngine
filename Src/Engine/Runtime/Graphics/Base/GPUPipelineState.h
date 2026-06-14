#pragma once

#include "Core/TypeSystem/IType.h"
#include "Runtime/Graphics/Shaders/GPUShaderProgram.h"
#include "GPUResource.h"
#include "GPUEnums.h"

namespace SE
{
	/// <summary>
	/// Stencil operation modes.
	/// </summary>
	SE_ENUM(Reflect)
	enum class StencilOperation
	{
		// Keep the existing stencil data.
		Keep,
		// Set the stencil data to 0.
		Zero,
		// Set the stencil data to the reference value (set via GPUContext::SetStencilRef).
		Replace,
		// Increment the stencil value by 1, and clamp the result.
		IncrementSaturated,
		// Decrement the stencil value by 1, and clamp the result.
		DecrementSaturated,
		// Invert the stencil data.
		Invert,
		// Increment the stencil value by 1, and wrap the result if necessary.
		Increment,
		// Decrement the stencil value by 1, and wrap the result if necessary.
		Decrement,

		MAX
	};

	/// <summary>
	/// Describes full graphics pipeline state within single object.
	/// </summary>
	class SE_API_RUNTIME GPUPipelineState : public GPUResource
	{
	public:
		static GPUPipelineState* New();

	public:
		/// <summary>
		/// Pipeline state description
		/// </summary>
		struct SE_API_RUNTIME Description
		{

			/// <summary>
			/// Enable/disable depth (DepthFunc and DepthWriteEnable)
			/// </summary>
			bool DepthEnable = false;

			/// <summary>
			/// Enable/disable depth write
			/// </summary>
			bool DepthWriteEnable = false;

			/// <summary>
			/// Enable/disable depth clipping
			/// </summary>
			bool DepthClipEnable = false;

			/// <summary>
			/// A function that compares depth data against existing depth data
			/// </summary>
			ComparisonFunc DepthFunc = ComparisonFunc::GreaterEqual;

			/// <summary>
			/// Enable/disable stencil buffer usage
			/// </summary>
			bool StencilEnable = false;

			/// <summary>
			/// The read mask applied to the reference value and each stencil buffer entry to determine the significant bits for the stencil test.
			/// </summary>
			uint8 StencilReadMask = 0;

			/// <summary>
			/// The write mask applied to values written into the stencil buffer.
			/// </summary>
			uint8 StencilWriteMask = 0;

			/// <summary>
			/// The comparison function for the stencil test.
			/// </summary>
			ComparisonFunc StencilFunc = ComparisonFunc::GreaterEqual;

			/// <summary>
			/// The stencil operation to perform when stencil testing fails.
			/// </summary>
			StencilOperation StencilFailOp = StencilOperation::Zero;

			/// <summary>
			/// The stencil operation to perform when stencil testing passes and depth testing fails.
			/// </summary>
			StencilOperation StencilDepthFailOp = StencilOperation::Zero;

			/// <summary>
			/// The stencil operation to perform when stencil testing and depth testing both pass.
			/// </summary>
			StencilOperation StencilPassOp = StencilOperation::Zero;

			/// <summary>
			/// Vertex shader program
			/// </summary>
			GPUShaderProgramVS* VS = nullptr;

			/// <summary>
			/// Hull shader program
			/// </summary>
			GPUShaderProgramHS* HS = nullptr;

			/// <summary>
			/// Domain shader program
			/// </summary>
			GPUShaderProgramDS* DS = nullptr;

			/// <summary>
			/// Geometry shader program
			/// </summary>
			GPUShaderProgramGS* GS = nullptr;

			/// <summary>
			/// Pixel shader program
			/// </summary>
			GPUShaderProgramPS* PS = nullptr;

			/// <summary>
			/// Input primitives topology
			/// </summary>
			PrimitiveTopologyType PrimitiveTopology = PrimitiveTopologyType::Triangle;

			/// <summary>
			/// True if use wireframe rendering, otherwise false
			/// </summary>
			bool Wireframe = false;

			/// <summary>
			/// Primitives culling mode
			/// </summary>
			CullMode CullMode = CullMode::Normal;

			/// <summary>
			/// Colors blending mode
			/// </summary>
			BlendingMode BlendMode = BlendingMode::Opaque;

		public:
			/// <summary>
			/// Default description
			/// </summary>
			static Description Default;

			/// <summary>
			/// Default description without using depth buffer at all
			/// </summary>
			static Description DefaultNoDepth;

			/// <summary>
			/// Default description for fullscreen triangle rendering
			/// </summary>
			static Description DefaultFullscreenTriangle;
		};

	protected:
		ShaderBindings m_Meta;

		GPUPipelineState();

	public:
//#if BUILD_DEBUG
		/// <summary>
		/// The description of the pipeline state cached on creation in debug builds. Can be used to help with rendering crashes or issues and validation.
		/// </summary>
		Description debugDesc;
//#endif
//#if USE_EDITOR
		int32 complexity;
//#endif

	public:
		/// <summary>
		/// Gets constant buffers usage mask (each set bit marks usage of the constant buffer at the bit index slot). Combined from all the used shader stages.
		/// </summary>
		inline uint32 GetUsedCBsMask() const
		{
			return m_Meta.usedCBsMask;
		}

		/// <summary>
		/// Gets shader resources usage mask (each set bit marks usage of the shader resource slot at the bit index slot). Combined from all the used shader stages.
		/// </summary>
		inline uint32 GetUsedSRsMask() const
		{
			return m_Meta.usedSRsMask;
		}

		/// <summary>
		/// Gets unordered access usage mask (each set bit marks usage of the unordered access slot at the bit index slot). Combined from all the used shader stages.
		/// </summary>
		inline uint32 GetUsedUAsMask() const
		{
			return m_Meta.usedUAsMask;
		}

	public:
		/// <summary>
		/// Returns true if pipeline state is valid and ready to use
		/// </summary>
		virtual bool IsValid() const = 0;

		/// <summary>
		/// Create new state data
		/// </summary>
		/// <param name="desc">Full pipeline state description</param>
		/// <returns>True if cannot create state, otherwise false</returns>
		virtual bool Init(const Description& desc);

	public:
		// [GPUResource]
		GPUResourceType GetResType() const final override;
	};
}
