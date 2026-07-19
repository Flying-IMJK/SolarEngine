#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "VulkanInclude.h"

namespace SE
{
	enum class SpirvShaderResourceType
	{
		Unknown = 0,
		ConstantBuffer = 1,
		Buffer = 2,
		Sampler = 3,
		Texture1D = 4,
		Texture2D = 5,
		Texture3D = 6,
		TextureCube = 7,
		Texture1DArray = 8,
		Texture2DArray = 9,
	};

	enum class SpirvShaderResourceBindingType : byte
	{
		INVALID = 0,
		CB = 1,
		SAMPLER = 2,
		SRV = 3,
		UAV = 4,
		MAX
	};

	struct SpirvShaderDescriptorInfo
	{
		enum
		{
			MaxDescriptors = 64,
		};

		/// <summary>
		/// A single descriptor data.
		/// </summary>
		struct Descriptor
		{
			/// <summary>
			/// The binding slot (the descriptors slot).
			/// </summary>
			byte binding;

			/// <summary>
			/// The layout slot (the descriptors set slot).
			/// </summary>
			byte set;

			/// <summary>
			/// The input slot (the pipeline slot).
			/// </summary>
			byte slot;

			/// <summary>
			/// The resource binding type (the graphics pipeline abstraction binding layer type).
			/// </summary>
			SpirvShaderResourceBindingType bindingType;

			/// <summary>
			/// The Vulkan descriptor type.
			/// </summary>
			VkDescriptorType descriptorType;

			/// <summary>
			/// The resource type.
			/// </summary>
			SpirvShaderResourceType resourceType;

			/// <summary>
			/// The amount of slots used by the descriptor (eg. array of textures size).
			/// </summary>
			uint32 count;
		};

		uint16 imageInfosCount;
		uint16 bufferInfosCount;
		uint32 texelBufferViewsCount;
		uint32 descriptorTypesCount;
		Descriptor descriptorTypes[MaxDescriptors];
	};

	struct SpirvShaderHeader
	{
		enum class Types
		{
			/// <summary>
			/// The raw SPIR-V byte code.
			/// </summary>
			Raw = 0,
		};

		/// <summary>
		/// The data type.
		/// </summary>
		Types type;

		/// <summary>
		/// The shader descriptors usage information.
		/// </summary>
		SpirvShaderDescriptorInfo descriptorInfo;

		// .. rest is just a actual data array
	};
}
