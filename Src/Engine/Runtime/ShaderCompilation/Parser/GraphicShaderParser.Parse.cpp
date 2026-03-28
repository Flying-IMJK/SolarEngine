#include "GraphicShaderParser.h"

#include "Core/Logging/Logging.h"

namespace SE::ShaderParser
{
	VertexShaderMeta::InputType ShaderParser::ParseInputType(const Token& token)
	{
		struct InputDesc
		{
			VertexShaderMeta::InputType e;
			const char* s;
		};

#define _PARSE_ENTRY(x) { VertexShaderMeta::InputType::x, #x }
		const InputDesc formats[] =
			{
				_PARSE_ENTRY(Invalid),
				_PARSE_ENTRY(POSITION),
				_PARSE_ENTRY(COLOR),
				_PARSE_ENTRY(TEXCOORD),
				_PARSE_ENTRY(NORMAL),
				_PARSE_ENTRY(TANGENT),
				_PARSE_ENTRY(BITANGENT),
				_PARSE_ENTRY(ATTRIBUTE),
				_PARSE_ENTRY(BLENDINDICES),
				_PARSE_ENTRY(BLENDWEIGHT),
			};
#undef _PARSE_ENTRY

		for (int32 i = 0; i < ARRAY_SIZE(formats); i++)
		{
			if (token == formats[i].s)
				return formats[i].e;
		}

		return VertexShaderMeta::InputType::Invalid;
	}

	PixelFormat ShaderParser::ParsePixelFormat(const Token& token)
	{
		struct DataDesc
		{
			PixelFormat e;
			const char* s;
		};

#define _PARSE_ENTRY(x) { PixelFormat::x, #x }
		const DataDesc formats[] =
			{
				_PARSE_ENTRY(Undefined),
				_PARSE_ENTRY(R32G32B32A32_Float),
				_PARSE_ENTRY(R32G32B32A32_UInt),
				_PARSE_ENTRY(R32G32B32A32_SInt),
				_PARSE_ENTRY(R32G32B32_Float),
				_PARSE_ENTRY(R32G32B32_UInt),
				_PARSE_ENTRY(R32G32B32_SInt),
				_PARSE_ENTRY(R16G16B16A16_Float),
				_PARSE_ENTRY(R16G16B16A16_UNorm),
				_PARSE_ENTRY(R16G16B16A16_UInt),
				_PARSE_ENTRY(R16G16B16A16_SNorm),
				_PARSE_ENTRY(R16G16B16A16_SInt),
				_PARSE_ENTRY(R32G32_Float),
				_PARSE_ENTRY(R32G32_UInt),
				_PARSE_ENTRY(R32G32_SInt),
				_PARSE_ENTRY(R10G10B10A2_UNorm),
				_PARSE_ENTRY(R10G10B10A2_UInt),
				_PARSE_ENTRY(R11G11B10_Float),
				_PARSE_ENTRY(R8G8B8A8_UNorm),
				_PARSE_ENTRY(R8G8B8A8_UNorm_SRGB),
				_PARSE_ENTRY(R8G8B8A8_UInt),
				_PARSE_ENTRY(R8G8B8A8_SNorm),
				_PARSE_ENTRY(R8G8B8A8_SInt),
				_PARSE_ENTRY(R16G16_Float),
				_PARSE_ENTRY(R16G16_UNorm),
				_PARSE_ENTRY(R16G16_UInt),
				_PARSE_ENTRY(R16G16_SNorm),
				_PARSE_ENTRY(R16G16_SInt),
				_PARSE_ENTRY(R32_Float),
				_PARSE_ENTRY(R32_UInt),
				_PARSE_ENTRY(R32_SInt),
				_PARSE_ENTRY(R8G8_UNorm),
				_PARSE_ENTRY(R8G8_UInt),
				_PARSE_ENTRY(R8G8_SNorm),
				_PARSE_ENTRY(R8G8_SInt),
				_PARSE_ENTRY(R16_Float),
				_PARSE_ENTRY(R16_UNorm),
				_PARSE_ENTRY(R16_UInt),
				_PARSE_ENTRY(R16_SNorm),
				_PARSE_ENTRY(R16_SInt),
				_PARSE_ENTRY(R8_UNorm),
				_PARSE_ENTRY(R8_UInt),
				_PARSE_ENTRY(R8_SNorm),
				_PARSE_ENTRY(R8_SInt),
				_PARSE_ENTRY(BC1_UNorm),
				_PARSE_ENTRY(BC1_UNorm_SRGB),
				_PARSE_ENTRY(BC2_UNorm),
				_PARSE_ENTRY(BC2_UNorm_SRGB),
				_PARSE_ENTRY(BC3_UNorm),
				_PARSE_ENTRY(BC3_UNorm_SRGB),
				_PARSE_ENTRY(BC4_UNorm),
				_PARSE_ENTRY(BC4_SNorm),
				_PARSE_ENTRY(BC5_UNorm),
				_PARSE_ENTRY(BC5_SNorm),
				_PARSE_ENTRY(B8G8R8A8_UNorm),
				_PARSE_ENTRY(B8G8R8A8_UNorm_SRGB),
				_PARSE_ENTRY(BC7_UNorm),
			};
#undef _PARSE_ENTRY

		for (int32 i = 0; i < ARRAY_SIZE(formats); i++)
		{
			if (token.EqualsIgnoreCase(formats[i].s))
				return formats[i].e;
		}

		return PixelFormat::Undefined;
	}

	ShaderFlags ShaderParser::ParseShaderFlags(const Token& token)
	{
		struct DataDesc
		{
			ShaderFlags e;
			const char* s;
		};

#define _PARSE_ENTRY(x) { ShaderFlags::x, #x }
		const DataDesc data[] =
			{
				_PARSE_ENTRY(Default),
				_PARSE_ENTRY(Hidden),
				_PARSE_ENTRY(NoFastMath),
				_PARSE_ENTRY(VertexToGeometryShader),
			};
		static_assert(ARRAY_SIZE(data) == 4, "Invalid amount of Shader Flag data entries.");
#undef _PARSE_ENTRY

		for (int32 i = 0; i < ARRAY_SIZE(data); i++)
		{
			if (token.EqualsIgnoreCase(data[i].s))
				return data[i].e;
		}

		return ShaderFlags::Default;
	}
}

