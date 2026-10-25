#include "ShaderCompileTypes.h"

namespace SE
{
	String BuildTargetKey(const ShaderCompileTarget& target)
	{
		return ToString(target.Profile).ToString() + SE_TEXT("-") + String(ToString(target.Feature));
	}
}
