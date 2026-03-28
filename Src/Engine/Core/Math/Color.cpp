#pragma once

#include "Color.h"
#include "Vector3.h"
#include "Core/Types/Strings/String.h"

//-------------------------------------------------------------------------

namespace SE
{
	Color32::Color32(Color c)
	{
		r = static_cast<uint8>(c.r * 255.0f);
		g = static_cast<uint8>(c.g * 255.0f);
		b = static_cast<uint8>(c.b * 255.0f);
		a = static_cast<uint8>(c.a * 255.0f);
	}

	Color::Color(const Float3& rgb, float a)
		: r(rgb.x)
		, g(rgb.y)
		, b(rgb.z)
		, a(a)
	{
	}

	Color::Color(const Float4& rgba)
		: r(rgba.x)
		, g(rgba.y)
		, b(rgba.z)
		, a(rgba.w)
	{
	}

	Color::Color(const Color32& color)
		: r(color.r / 255.0f)
		, g(color.g / 255.0f)
		, b(color.b / 255.0f)
		, a(color.a / 255.0f)
	{
	}

	Color Color::FromHex(const String& hex, bool& isValid)
	{
		int32 r, g, b, a = 255;
		isValid = true;
		int32 startIndex = !hex.IsEmpty() && hex[0] == char('#') ? 1 : 0;
		if (hex.Length() == 3 + startIndex)
		{
			r = StringUtils::HexDigit(hex[startIndex++]);
			g = StringUtils::HexDigit(hex[startIndex++]);
			b = StringUtils::HexDigit(hex[startIndex]);

			r = (r << 4) + r;
			g = (g << 4) + g;
			b = (b << 4) + b;
		}
		else if (hex.Length() == 6 + startIndex)
		{
			r = (StringUtils::HexDigit(hex[startIndex + 0]) << 4) + StringUtils::HexDigit(hex[startIndex + 1]);
			g = (StringUtils::HexDigit(hex[startIndex + 2]) << 4) + StringUtils::HexDigit(hex[startIndex + 3]);
			b = (StringUtils::HexDigit(hex[startIndex + 4]) << 4) + StringUtils::HexDigit(hex[startIndex + 5]);
		}
		else if (hex.Length() == 8 + startIndex)
		{
			r = (StringUtils::HexDigit(hex[startIndex + 0]) << 4) + StringUtils::HexDigit(hex[startIndex + 1]);
			g = (StringUtils::HexDigit(hex[startIndex + 2]) << 4) + StringUtils::HexDigit(hex[startIndex + 3]);
			b = (StringUtils::HexDigit(hex[startIndex + 4]) << 4) + StringUtils::HexDigit(hex[startIndex + 5]);
			a = (StringUtils::HexDigit(hex[startIndex + 6]) << 4) + StringUtils::HexDigit(hex[startIndex + 7]);
		}
		else
		{
			r = g = b = 0;
			isValid = false;
		}
		return FromBytes(r, g, b, a);
	}

	Color Color::FromHSV(float hue, float saturation, float value, float alpha)
	{
		const float hDiv60 = hue / 60.0f;
		const float hDiv60Floor = Math::Floor(hDiv60);
		const float hDiv60Fraction = hDiv60 - hDiv60Floor;

		const float rgbValues[4] =
			{
				value,
				value * (1.0f - saturation),
				value * (1.0f - hDiv60Fraction * saturation),
				value * (1.0f - (1.0f - hDiv60Fraction) * saturation),
			};
		const int32 rgbSwizzle[6][3]
			{
				{ 0, 3, 1 },
				{ 2, 0, 1 },
				{ 1, 0, 3 },
				{ 1, 2, 0 },
				{ 3, 1, 0 },
				{ 0, 1, 2 },
			};
		const int32 swizzleIndex = (int32)hDiv60Floor % 6;

		return Color(rgbValues[rgbSwizzle[swizzleIndex][0]],
			rgbValues[rgbSwizzle[swizzleIndex][1]],
			rgbValues[rgbSwizzle[swizzleIndex][2]],
			alpha);
	}

	Color Color::FromHSV(const Float3& hsv, float alpha)
	{
		return FromHSV(hsv.x, hsv.y, hsv.z, alpha);
	}

	Color Color::Random()
	{
		return FromRGB(rand(), 1);
	}

	String Color::ToString() const
	{
		return String::Format(SE_TEXT("{}:{}:{}:{}"), r, g, b, a);;
	}

	String Color::ToHexString() const
	{
		static const Char * digits = SE_TEXT("0123456789ABCDEF");

		const byte r = static_cast<byte>(this->r * Max_uint8);
		const byte g = static_cast<byte>(this->g * Max_uint8);
		const byte b = static_cast<byte>(this->b * Max_uint8);
		const byte a = static_cast<byte>(this->a * Max_uint8);

		char result[8];

		result[0] = digits[r >> 4 & 0x0f];
		result[1] = digits[r & 0x0f];

		result[2] = digits[g >> 4 & 0x0f];
		result[3] = digits[g & 0x0f];

		result[4] = digits[b >> 4 & 0x0f];
		result[5] = digits[b & 0x0f];

		result[6] = digits[a >> 4 & 0x0f];
		result[7] = digits[a & 0x0f];

		return String(result, 8);
	}

	bool Color::IsTransparent() const
	{
		return Math::IsZero(r + g + b + a);
	}

	bool Color::HasOpacity() const
	{
		return a != 1;
	}

	bool Color::NearEqual(const Color& a, const Color& b)
	{
		return Math::IsNearEqual(a.r, b.r) &&
			Math::IsNearEqual(a.g, b.g) &&
			Math::IsNearEqual(a.b, b.b) &&
			Math::IsNearEqual(a.a, b.a);
	}

	bool Color::NearEqual(const Color& a, const Color& b, float epsilon)
	{
		return Math::IsNearEqual(a.r, b.r, epsilon) &&
			Math::IsNearEqual(a.g, b.g, epsilon) &&
			Math::IsNearEqual(a.b, b.b, epsilon) &&
			Math::IsNearEqual(a.a, b.a, epsilon);
	}

	Float3 Color::ToFloat3() const
	{
		return Float3(r, g, b);
	}

	Float4 Color::ToFloat4() const
	{
		return Float4(r, g, b, a);
	}

	Float3 Color::ToHSV() const
	{
		const float rgbMin = Math::Min(r, g, b);
		const float rgbMax = Math::Max(r, g, b);
		const float rgbRange = rgbMax - rgbMin;
		const float hue = rgbMax == rgbMin ? 0.0f : rgbMax == r ? Math::FMod((g - b) / rgbRange * 60.0f + 360.0f, 360.0f) : rgbMax == g ? (b - r) / rgbRange * 60.0f + 120.0f : rgbMax == b ? (r - g) / rgbRange * 60.0f + 240.0f : 0.0f;
		const float saturation = rgbMax == 0.0f ? 0.0f : rgbRange / rgbMax;
		const float value = rgbMax;
		return Float3(hue, saturation, value);
	}

	void Color::Lerp(const Color& start, const Color& end, float amount, Color& result)
	{
		result.r = Math::Lerp(start.r, end.r, amount);
		result.g = Math::Lerp(start.g, end.g, amount);
		result.b = Math::Lerp(start.b, end.b, amount);
		result.a = Math::Lerp(start.a, end.a, amount);
	}

	Color Color::Lerp(const Color& start, const Color& end, float amount)
	{
		Color result;
		Lerp(start, end, amount, result);
		return result;
	}

	Color Color::LinearToSrgb(const Color& linear)
	{
#define LINEAR_TO_SRGB(value) value < 0.00313067f ? value * 12.92f : Math::Pow(value, (1.0f / 2.4f)) * 1.055f - 0.055f
		return Color(LINEAR_TO_SRGB(linear.r), LINEAR_TO_SRGB(linear.g), LINEAR_TO_SRGB(linear.b), LINEAR_TO_SRGB(linear.a));
#undef LINEAR_TO_SRGB
	}

	Color Color::SrgbToLinear(const Color& srgb)
	{
#define SRGB_TO_LINEAR(value) value < 0.04045f ? value / 12.92f : Math::Pow((value + 0.055f) / 1.055f, 2.4f)
		return Color(SRGB_TO_LINEAR(srgb.r), SRGB_TO_LINEAR(srgb.g), SRGB_TO_LINEAR(srgb.b), SRGB_TO_LINEAR(srgb.a));
#undef LINEAR_TO_SRGB
	}

	uint32 GetHash(const Color &color)
	{
		const int32 range = 100000;
		int32 hashCode = (int32)color.r * range;
		hashCode = hashCode * 397 ^ (int32)(color.g * range);
		hashCode = hashCode * 397 ^ (int32)(color.b * range);
		hashCode = hashCode * 397 ^ (int32)(color.a * range);
		return hashCode;
	}

	uint32 GetHash(const Color32 & color)
	{
		const int32 range = 100000;
		int32 hashCode = (int32)color.r * range;
		hashCode = hashCode * 397 ^ (int32)(color.g * range);
		hashCode = hashCode * 397 ^ (int32)(color.b * range);
		hashCode = hashCode * 397 ^ (int32)(color.a * range);
		return hashCode;
	}
}