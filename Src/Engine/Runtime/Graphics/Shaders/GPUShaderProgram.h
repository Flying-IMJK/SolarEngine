#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/API.h"
#include "Runtime/Graphics/Base/GPUEnums.h"

#include "Config.h"

namespace SE
{
	class GPUShader;

	/// <summary>
	/// 着色器程序元数据容器。包含关于着色器使用的资源的描述。
	/// </summary>
	struct SE_API_RUNTIME ShaderBindings
	{
		uint32 instructionsCount = 0;
		uint32 usedCBsMask = 0;
		uint32 usedSRsMask = 0;
		uint32 usedUAsMask = 0;

		bool IsUsingCB(uint32 slotIndex) const
		{
			return (usedCBsMask & (1u << slotIndex)) != 0u;
		}

		bool IsUsingSR(uint32 slotIndex) const
		{
			return (usedSRsMask & (1u << slotIndex)) != 0u;
		}

		bool IsUsingUA(uint32 slotIndex) const
		{
			return (usedUAsMask & (1u << slotIndex)) != 0u;
		}
	};

	struct GPUShaderProgramInitializer
	{
		String Name;
		ShaderBindings Bindings;
		ShaderFlags Flags;
#if !BUILD_RELEASE
		GPUShader* Owner;
#endif
	};

	/// <summary>
	/// 可在GPU上运行的程序
	/// </summary>
	class SE_API_RUNTIME GPUShaderProgram
	{
		friend class GPUPipelineStateVulkan;
	protected:
		String m_Name;
		StringAnsi m_NativeName;
		ShaderBindings m_Bindings;
		ShaderFlags m_Flags;
//#if !BUILD_RELEASE
//			GPUShader* _owner;
//#endif

		void Init(const GPUShaderProgramInitializer& initializer)
		{
			m_Name = initializer.Name;
			m_NativeName = initializer.Name.ToStringAnsi();
			m_Bindings = initializer.Bindings;
			m_Flags = initializer.Flags;
//#if !BUILD_RELEASE
//				_owner = initializer.Owner;
//#endif
		}

	public:
		virtual ~GPUShaderProgram()
		{
		}

	public:
		inline const StringView GetName() const
		{
			return m_Name;
		}

		inline const ShaderBindings& GetBindings() const
		{
			return m_Bindings;
		}

		/// <summary>
		/// Gets the shader flags.
		/// </summary>
//			inline ShaderFlags GetFlags() const
//			{
//				return _flags;
//			}

	public:
		/**
		 * 获取Shader程序阶段
		 * @return
		 */
		virtual ShaderStage GetStage() const = 0;

		/**
		 * 获取shader程序数据
		 * @return
		 */
		virtual void* GetBufferHandle() const = 0;

		/**
		 * 获取shader程序数据大小(以字节为单位)。
		 * @return
		 */
		virtual uint32 GetBufferSize() const = 0;
	};

	/**
	 * Vertex Shader program
	 */
	class GPUShaderProgramVS : public GPUShaderProgram
	{
	public:
		// Input element run-time data (see VertexShaderMeta::InputElement for compile-time data)
		struct InputElement
		{
			byte Type; // VertexShaderMeta::InputType
			byte Index;
			byte Format; // PixelFormat
			byte InputSlot;
			uint32 AlignedByteOffset; // Fixed value or INPUT_LAYOUT_ELEMENT_ALIGN if auto
			byte InputSlotClass; // INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA or INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA
			uint32 InstanceDataStepRate; // 0 if per-vertex
		};

	public:
		/// <summary>
		/// Gets input layout description handle (platform dependent).
		/// </summary>
		virtual void* GetInputLayout() const = 0;

		/// <summary>
		/// Gets input layout description size (in bytes).
		/// </summary>
		virtual byte GetInputLayoutSize() const = 0;

	public:
		// [GPUShaderProgram]
		ShaderStage GetStage() const override
		{
			return ShaderStage::Vertex;
		}
	};

	/**
	 * Geometry Shader program.
	 */
	class GPUShaderProgramGS : public GPUShaderProgram
	{
	public:
		ShaderStage GetStage() const override
		{
			return ShaderStage::Geometry;
		}
	};

	/**
	 * Hull Shader program
	 */
	class GPUShaderProgramHS : public GPUShaderProgram
	{
	protected:
		int32 m_ControlPointsCount;

	public:
		/// <summary>
		/// Gets the input control points count (valid range: 1-32).
		/// </summary>
		inline int32 GetControlPointsCount() const
		{
			return m_ControlPointsCount;
		}

	public:
		// [GPUShaderProgram]
		ShaderStage GetStage() const override
		{
			return ShaderStage::Hull;
		}
	};

	/**
	 * Domain Shader program
	 */
	class GPUShaderProgramDS : public GPUShaderProgram
	{
	public:
		ShaderStage GetStage() const override
		{
			return ShaderStage::Domain;
		}
	}; 

	/**
	 * Pixel Shader program
	 */
	class GPUShaderProgramPS : public GPUShaderProgram
	{
	public:
		// [GPUShaderProgram]
		ShaderStage GetStage() const override
		{
			return ShaderStage::Pixel;
		}
	};

	/**
	 * Compute Shader program
	 */
	class GPUShaderProgramCS : public GPUShaderProgram
	{
	public:
		ShaderStage GetStage() const override
		{
			return ShaderStage::Compute;
		}
	};
}
