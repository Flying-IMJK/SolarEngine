#include "ShaderCompileTypes.h"

namespace SE
{
	String ToString(const ShaderCompileStatus value)
	{
		switch (value)
		{
		case ShaderCompileStatus::Success:
			return SE_TEXT("Success");
		case ShaderCompileStatus::Failed:
			return SE_TEXT("Failed");
		default:
			return SE_TEXT("Failed");
		}
	}

	String ToString(const ShaderTargetPlatform value)
	{
		switch (value)
		{
		case ShaderTargetPlatform::Windows:
			return SE_TEXT("Windows");
		case ShaderTargetPlatform::Linux:
			return SE_TEXT("Linux");
		case ShaderTargetPlatform::MacOS:
			return SE_TEXT("MacOS");
		default:
			return SE_TEXT("Unknown");
		}
	}

	String ToString(const ShaderGraphicsBackend value)
	{
		switch (value)
		{
		case ShaderGraphicsBackend::Vulkan:
			return SE_TEXT("Vulkan");
		case ShaderGraphicsBackend::DirectX:
			return SE_TEXT("DirectX");
		default:
			return SE_TEXT("Unknown");
		}
	}

	String ToString(const ShaderCompileShaderModel value)
	{
		switch (value)
		{
		case ShaderCompileShaderModel::SM_5_0:
			return SE_TEXT("SM_5_0");
		case ShaderCompileShaderModel::SM_6_0:
			return SE_TEXT("SM_6_0");
		case ShaderCompileShaderModel::SM_6_6:
			return SE_TEXT("SM_6_6");
		default:
			return SE_TEXT("Unknown");
		}
	}

	String ToString(const SlangShaderStage value)
	{
		switch (value)
		{
		case SlangShaderStage::Vertex:
			return SE_TEXT("vs");
		case SlangShaderStage::Hull:
			return SE_TEXT("hs");
		case SlangShaderStage::Domain:
			return SE_TEXT("ds");
		case SlangShaderStage::Geometry:
			return SE_TEXT("gs");
		case SlangShaderStage::Pixel:
			return SE_TEXT("ps");
		case SlangShaderStage::Compute:
			return SE_TEXT("cs");
		default:
			return SE_TEXT("unknown");
		}
	}

	bool ParseSlangShaderStage(const char* text, SlangShaderStage& stage)
	{
		if (text == nullptr)
		{
			stage = SlangShaderStage::Unknown;
			return false;
		}
		const StringAnsi value(text);
		if (value == "vertex" || value == "vs")
		{
			stage = SlangShaderStage::Vertex;
			return true;
		}
		if (value == "hull" || value == "hs")
		{
			stage = SlangShaderStage::Hull;
			return true;
		}
		if (value == "domain" || value == "ds")
		{
			stage = SlangShaderStage::Domain;
			return true;
		}
		if (value == "geometry" || value == "gs")
		{
			stage = SlangShaderStage::Geometry;
			return true;
		}
		if (value == "fragment" || value == "pixel" || value == "ps")
		{
			stage = SlangShaderStage::Pixel;
			return true;
		}
		if (value == "compute" || value == "cs")
		{
			stage = SlangShaderStage::Compute;
			return true;
		}

		stage = SlangShaderStage::Unknown;
		return false;
	}

	String BuildTargetKey(const ShaderCompileTarget& target)
	{
		return ToString(target.Platform) + SE_TEXT("-") + ToString(target.Backend) + SE_TEXT("-") + ToString(target.ShaderModel);
	}
}
