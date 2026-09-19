#include "Viewport.h"

#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Math/Rectangle.h"

namespace SE
{
	Viewport::Viewport(const Rectangle& bounds)
		: X(bounds.Location.X)
		, Y(bounds.Location.Y)
		, Width(bounds.Size.X)
		, Height(bounds.Size.Y)
		, MinDepth(0.0f)
		, MaxDepth(1.0f)
	{
	}

	String Viewport::ToString() const
	{
		return String::Format(SE_TEXT("{x:{0} y:{1} width:{2} height:{3}}"), X, Y, Width, Height);
	}

	Rectangle Viewport::GetBounds() const
	{
		return Rectangle(Float2(X, Y), Float2(Width, Height));
	}

	void Viewport::SetBounds(const Rectangle& bounds)
	{
		X = bounds.Location.X;
		Y = bounds.Location.Y;
		Width = bounds.Size.X;
		Height = bounds.Size.Y;
	}

	void Viewport::Project(const Float3& source, const Matrix& vp, Float3& result) const
	{
		Float3::Transform(source, vp, result);
		const float a = source.X * vp.M14 + source.Y * vp.M24 + source.Z * vp.M34 + vp.M44;

		if (!Math::IsOne(a))
		{
			result /= a;
		}

		result.X = (result.X + 1.0f) * 0.5f * Width + X;
		result.Y = (-result.Y + 1.0f) * 0.5f * Height + Y;
		result.Z = result.Z * (MaxDepth - MinDepth) + MinDepth;
	}

	void Viewport::UnProject(const Float3& source, const Matrix& ivp, Float3& result) const
	{
		result.X = (source.X - X) / Width * 2.0f - 1.0f;
		result.Y = -((source.Y - Y) / Height * 2.0f - 1.0f);
		result.Z = (source.Z - MinDepth) / (MaxDepth - MinDepth);

		const float a = result.X * ivp.M14 + result.Y * ivp.M24 + result.Z * ivp.M34 + ivp.M44;
		Float3::Transform(result, ivp, result);

		if (!Math::IsOne(a))
		{
			result /= a;
		}
	}
}


