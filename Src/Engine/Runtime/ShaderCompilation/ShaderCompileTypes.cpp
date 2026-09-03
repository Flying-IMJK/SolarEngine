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
		case ShaderTargetPlatform::PS4:
			return SE_TEXT("PS4");
		case ShaderTargetPlatform::PS5:
			return SE_TEXT("PS5");
		default:
			return SE_TEXT("Unknown");
		}
	}

	String BuildTargetKey(const ShaderCompileTarget& target)
	{
		return ToString(target.Platform) + SE_TEXT("-") + ToString(target.Profile).ToString() + SE_TEXT("-") + String(ToString(target.Feature));
	}
}
