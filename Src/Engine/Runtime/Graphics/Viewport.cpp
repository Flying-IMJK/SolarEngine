#include "Viewport.h"

#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Math/Rectangle.h"

namespace SE
{
	Viewport::Viewport(const Rectangle& bounds)
		: x(bounds.Location.x)
		, y(bounds.Location.y)
		, width(bounds.Size.x)
		, height(bounds.Size.y)
		, minDepth(0.0f)
		, maxDepth(1.0f)
	{
	}

	String Viewport::ToString() const
	{
		return String::Format(SE_TEXT("{x:{0} y:{1} width:{2} height:{3}}"), x, y, width, height);
	}

	Rectangle Viewport::GetBounds() const
	{
		return Rectangle(Float2(x, y), Float2(width, height));
	}

	void Viewport::SetBounds(const Rectangle& bounds)
	{
		x = bounds.Location.x;
		y = bounds.Location.y;
		width = bounds.Size.x;
		height = bounds.Size.y;
	}

	void Viewport::Project(const Float3& source, const Matrix& vp, Float3& result) const
	{
		Float3::Transform(source, vp, result);
		const float a = source.x * vp.M14 + source.y * vp.M24 + source.z * vp.M34 + vp.M44;

		if (!Math::IsOne(a))
		{
			result /= a;
		}

		result.x = (result.x + 1.0f) * 0.5f * width + x;
		result.y = (-result.y + 1.0f) * 0.5f * height + y;
		result.z = result.z * (maxDepth - minDepth) + minDepth;
	}

	void Viewport::UnProject(const Float3& source, const Matrix& ivp, Float3& result) const
	{
		result.x = (source.x - x) / width * 2.0f - 1.0f;
		result.y = -((source.y - y) / height * 2.0f - 1.0f);
		result.z = (source.z - minDepth) / (maxDepth - minDepth);

		const float a = result.x * ivp.M14 + result.y * ivp.M24 + result.z * ivp.M34 + ivp.M44;
		Float3::Transform(result, ivp, result);

		if (!Math::IsOne(a))
		{
			result /= a;
		}
	}
}


