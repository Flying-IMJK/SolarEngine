#pragma once

#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Half.h"
#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Utilities/Texture/Packed.h"

namespace SE
{

	GES_PACK_STRUCT(struct ModelVertex19
		{
		Float3 Position;
		Half2 TexCoord;
		Float1010102 Normal;
		Float1010102 Tangent;
		Half2 LightmapUVs;
		Color32 Color;
		});

	typedef ModelVertex19 ModelVertex;

	// For vertex data we use three buffers: one with positions, one with other attributes, and one with colors
	GES_PACK_STRUCT(struct VB0ElementType
	{
		Float3 Position;
	});

	GES_PACK_STRUCT(struct VB1ElementType
	{
		Half2 TexCoord;
		Float1010102 Normal;
		Float1010102 Tangent;
		Half2 LightmapUVs;
	});

	GES_PACK_STRUCT(struct VB2ElementType
	{
		Color32 Color;
	});

	typedef VB0ElementType VB0ElementType;
	typedef VB1ElementType VB1ElementType;
	typedef VB2ElementType VB2ElementType;
}
