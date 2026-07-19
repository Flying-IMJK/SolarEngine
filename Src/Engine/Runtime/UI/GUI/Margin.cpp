
#include "Margin.h"

#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	Margin Margin::Zero = Margin(0);

	Float2 Margin::GetLocation()
	{
		return Float2(Left, Top);
	}

	Float2 Margin::GetSize()
	{
		return Float2(Left + Right, Top + Bottom);
	}

	float Margin::GetWidth()
	{
		return Left + Right;
	}

	float Margin::GetHeight()
	{
		return Top + Bottom;
	}

	Margin::Margin(): Left(0), Right(0), Top(0), Bottom(0)
	{
	}

	Margin::Margin(float value) : Left(value), Right(value), Top(value), Bottom(value)
	{

	}

	Margin::Margin(float left, float right, float top, float bottom) : Left(left), Right(right), Top(top), Bottom(bottom)
	{

	}

	void Margin::ShrinkRectangle(Rectangle rect) const
	{
		rect.Location.x += Left;
		rect.Location.y += Top;
		rect.Size.x -= Left + Right;
		rect.Size.y -= Top + Bottom;
	}

	void Margin::ExpandRectangle(Rectangle rect) const
	{
		rect.Location.x -= Left;
		rect.Location.y -= Top;
		rect.Size.x += Left + Right;
		rect.Size.y += Top + Bottom;
	}
	Margin Margin::operator+(const Margin& value) const
	{
		return Margin(Left + value.Left, Right + value.Right, Top + value.Top, Bottom + value.Bottom);
	}

	Margin Margin::operator-(const Margin& value) const
	{
		return Margin(Left - value.Left, Right - value.Right, Top - value.Top, Bottom - value.Bottom);
	}

	bool Margin::operator==(const Margin& value) const
	{
		return Math::IsNearEqual(value.Left, Left) &&
			   Math::IsNearEqual(value.Right, Right) &&
			   Math::IsNearEqual(value.Top, Top) &&
			   Math::IsNearEqual(value.Bottom, Bottom);
	}

	bool Margin::operator!=(const Margin& value) const
	{
		return !(*this == value);
	}

	String Margin::ToString() const
	{
		return String::Format(SE_TEXT("Left:{0} Right:{1} Top:{2} Bottom:{3}"), Left, Right, Top, Bottom);
	}

} // SE