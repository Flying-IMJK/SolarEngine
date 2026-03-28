
#include "Variable.h"

#include "Core/Math/Color.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Vector3.h"

namespace SE
{
	template<>
	int32 GetZero<int32>()
	{
		return 0;
	}

	template<>
	float GetZero<float>()
	{
		return 0.0f;
	}

	template<>
	double GetZero<double>()
	{
		return 0.0;
	}

	template<>
	Float3 GetZero<Float3>()
	{
		return Float3::Zero;
	}

	template<>
	Quaternion GetZero<Quaternion>()
	{
		return Quaternion::Identity;
	}

	template<>
	Transform GetZero<Transform>()
	{
		return Transform::Identity;
	}

	template<>
	Color GetZero<Color>()
	{
		return Colors::Black;
	}

	template<>
	Color32 GetZero<Color32>()
	{
		return Colors::Black;
	}

}
