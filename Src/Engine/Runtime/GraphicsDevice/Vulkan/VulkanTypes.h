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

}
