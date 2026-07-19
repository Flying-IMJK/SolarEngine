#pragma once

//-------------------------------------------------------------------------
#include "Runtime/API.h"
#include "Runtime/Core/Types/Variable.h"
#ifdef RGB
#undef RGB
#endif

//-------------------------------------------------------------------------

namespace SE
{
    //-------------------------------------------------------------------------
    //  Color Abstraction
    //-------------------------------------------------------------------------
    // This is a simply helper to manage colors and the various formats
    // It assumes little endian systems and a uint32 format as follows: 0xAABBGGRR
    // If you need a different uint32 format, there are conversion functions provided

	struct Color;

    struct SE_API_RUNTIME Color32
    {
    public:
        union
        {
			struct
            {
                uint8 r;
                uint8 g;
                uint8 b;
				uint8 a;
            };

            uint32 value;
        };

        // Default color is transparent
        Color32() : value(0x00000000) {}

        // The format is as follows: 0xAAGGBBRR
        explicit Color32(uint32 c) : value(c) {}

        Color32(uint8 r, uint8 g, uint8 b, uint8 a = 255) : r(r), g(g), b(b), a(a) { }

    	Color32(Color c);

    	Color32(Float4 const &c);

        //-------------------------------------------------------------------------

        // Get a version with the a different alpha value
        inline Color32 GetAlphaVersion(uint8 newAlpha) const
        {
            Color32 newColor = *this;
            newColor.a = newAlpha;
            return newColor;
        }

        // Get a version with the a different alpha value
    	Color32 GetAlphaVersion(float alpha) const;

        // Scale the color values with a multiplier
    	void ScaleColor(float multiplier);

        // Get a version with color values scaled
    	Color32 GetScaledColor(float multiplier) const;

        // Equality
    	bool operator==(Color32 const &rhs) const { return value == rhs.value; }
    	bool operator!=(Color32 const &rhs) const { return value != rhs.value; }

        //-------------------------------------------------------------------------

        // Returns a uint32 color with this byte format: 0xAAGGBBRR
    	uint32 ToUInt32() const { return value; }

        // Returns a uint32 color with this byte format: 0xAAGGBBRR
    	operator uint32() const { return ToUInt32(); }

        //-------------------------------------------------------------------------

        // Returns a float4 color where x=R, y=G, z=B, w=A
        Float4 ToFloat4() const;

        // Returns a float4 color where x=R, y=G, z=B, w=A
    	operator Float4() const;
    };

    struct SE_API_RUNTIME Color
	{
	public:
		union
		{
			struct
			{
				/// <summary>
				/// The red channel value.
				/// </summary>
				float r;

				/// <summary>
				/// The green channel value.
				/// </summary>
				float g;

				/// <summary>
				/// The blue channel value.
				/// </summary>
				float b;

				/// <summary>
				/// The alpha channel value.
				/// </summary>
				float a;
			};

			/// <summary>
			/// The packed values into array floats.
			/// </summary>
			float raw[4];
		};

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		Color() = default;
		
		/// <summary>
		/// Initializes a new instance of the <see cref="Color"/> struct.
		/// </summary>
		/// <param name="rgba">The RGBA channels value.</param>
		explicit Color(float rgba)
			: r(rgba), g(rgba), b(rgba), a(rgba)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Color"/> struct.
		/// </summary>
		/// <param name="r">The red channel value.</param>
		/// <param name="g">The green channel value.</param>
		/// <param name="b">The blue channel value.</param>
		/// <param name="a">The alpha channel value.</param>
		Color(float r, float g, float b, float a = 1)
			: r(r), g(g), b(b), a(a)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Color"/> struct.
		/// </summary>
		/// <param name="rgb">The red, green and blue channels value.</param>
		/// <param name="a">The alpha channel value.</param>
		Color(const Color& rgb, float a)
			: r(rgb.r), g(rgb.g), b(rgb.b), a(a)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Color"/> struct.
		/// </summary>
		/// <param name="rgb">The red, green and blue channels value.</param>
		/// <param name="a">The alpha channel value.</param>
		Color(const Float3& rgb, float a);

		/// <summary>
		/// Initializes a new instance of the <see cref="Color"/> struct.
		/// </summary>
		/// <param name="rgba">The red, green, blue and alpha channels value.</param>
		Color(const Float4& rgba);

		/// <summary>
		/// Initializes a new instance of the <see cref="Color"/> struct.
		/// </summary>
		/// <param name="color">The other color (32-bit RGBA).</param>
		Color(const Color32& color);
	public:
		/// <summary>
		/// Initializes from values in range [0;255].
		/// </summary>
		/// <param name="r">The red channel.</param>
		/// <param name="g">The green channel.</param>
		/// <param name="b">The blue channel.</param>
		/// <param name="a">The alpha channel.</param>
		/// <returns>The color.</returns>
		static Color FromBytes(byte r, byte g, byte b, byte a = 255)
		{
			return Color(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f);
		}

		/// <summary>
		/// Initializes from packed RGB value (bottom bits contain Blue) of the color and separate alpha channel value.
		/// </summary>
		/// <param name="rgb">The packed RGB value (bottom bits contain Blue).</param>
		/// <param name="a">The alpha channel.</param>
		/// <returns>The color.</returns>
		static Color FromRGB(uint32 rgb, float a = 1.0f)
		{
			return Color(static_cast<float>(rgb >> 16 & 0xff) / 255.0f, static_cast<float>(rgb >> 8 & 0xff) / 255.0f, static_cast<float>(rgb & 0xff) / 255.0f, a);
		}

		/// <summary>
		/// Initializes from packed ARGB value (bottom bits contain Blue).
		/// </summary>
		/// <param name="argb">The packed ARGB value (bottom bits contain Blue).</param>
		/// <returns>The color.</returns>
		static Color FromARGB(uint32 argb)
		{
			return Color(static_cast<float>((argb >> 16) & 0xff) / 255.0f,static_cast<float>((argb >> 8) & 0xff) / 255.0f, static_cast<float>((argb >> 0) & 0xff) / 255.0f, static_cast<float>((argb >> 24) & 0xff) / 255.0f);
		}

		/// <summary>
		/// Initializes from packed RGBA value (bottom bits contain Alpha).
		/// </summary>
		/// <param name="rgba">The packed RGBA value (bottom bits contain Alpha).</param>
		/// <returns>The color.</returns>
		static Color FromRGBA(uint32 rgba)
		{
			return Color(static_cast<float>((rgba >> 24) & 0xff) / 255.0f,static_cast<float>((rgba >> 16) & 0xff) / 255.0f, static_cast<float>((rgba >> 8) & 0xff) / 255.0f, static_cast<float>((rgba >> 0) & 0xff) / 255.0f);
		}

		static Color FromHex(const String& hex)
		{
			bool isValid;
			return FromHex(hex, isValid);
		}

		static Color FromHex(const String& hex, bool& isValid);

		/// <summary>
		/// Creates RGB color from Hue[0-360], Saturation[0-1] and Value[0-1].
		/// </summary>
		/// <param name="hue">The hue angle in degrees [0-360].</param>
		/// <param name="saturation">The saturation normalized [0-1].</param>
		/// <param name="value">The value normalized [0-1].</param>
		/// <param name="alpha">The alpha value. Default is 1.</param>
		/// <returns>The RGB color.</returns>
		static Color FromHSV(float hue, float saturation, float value, float alpha = 1.0f);

		/// <summary>
		/// Creates RGB color from Hue[0-360], Saturation[0-1] and Value[0-1] packed to XYZ vector.
		/// </summary>
		/// <param name="hsv">The HSV color.</param>
		/// <param name="alpha">The alpha value. Default is 1.</param>
		/// <returns>The RGB color.</returns>
		static Color FromHSV(const Float3& hsv, float alpha = 1.0f);

		/// <summary>
		/// Gets random color with opaque alpha.
		/// </summary>
		/// <returns>The color.</returns>
		static Color Random();

	public:
		String ToString() const;
		String ToHexString() const;

	public:
		bool operator==(const Color& other) const
		{
			return r == other.r && g == other.g && b == other.b && a == other.a;
		}

		bool operator!=(const Color& other) const
		{
			return r != other.r || g != other.g || b != other.b || a != other.a;
		}

		Color operator+(const Color& color) const
		{
			return Color(r + color.r, g + color.g, b + color.b, a + color.a);
		}

		Color operator-(const Color& color) const
		{
			return Color(r - color.r, g - color.g, b - color.b, a - color.a);
		}

		inline Color operator*(const Color& color) const
		{
			return Color(r * color.r, g * color.g, b * color.b, a * color.a);
		}

		Color& operator+=(const Color& color)
		{
			r += color.r;
			g += color.g;
			b += color.b;
			a += color.a;
			return *this;
		}

		Color& operator-=(const Color& color)
		{
			r -= color.r;
			g -= color.g;
			b -= color.b;
			a -= color.a;
			return *this;
		}

		Color& operator*=(const Color& color)
		{
			r *= color.r;
			g *= color.g;
			b *= color.b;
			a *= color.a;
			return *this;
		}

		Color& operator*=(const float value)
		{
			r = r * value;
			g = g * value;
			b = b * value;
			a = a * value;
			return *this;
		}

		Color operator+(float v) const
		{
			return Color(r + v, g + v, b + v, a + v);
		}

		Color operator-(float v) const
		{
			return Color(r - v, g - v, b - v, a - v);
		}

		Color operator*(float v) const
		{
			return Color(r * v, g * v, b * v, a * v);
		}

		Color operator/(float v) const
		{
			return Color(r / v, g / v, b / v, a / v);
		}

		// Returns true if color is fully transparent (all components are equal zero).
		bool IsTransparent() const;

		// Returns true if color has opacity channel in use (different from 1).
		bool HasOpacity() const;

	public:
		static bool NearEqual(const Color& a, const Color& b);
		static bool NearEqual(const Color& a, const Color& b, float epsilon);

	public:
		// Get color as vector structure.
		Float3 ToFloat3() const;

		// Get color as vector structure.
		Float4 ToFloat4() const;

		/// <summary>
		/// Gets Hue[0-360], Saturation[0-1] and Value[0-1] from RGB color.
		/// </summary>
		/// <returns>HSV color</returns>
		Float3 ToHSV() const;

		/// <summary>
		/// Performs a linear interpolation between two colors.
		/// </summary>
		/// <param name="start">The start color.</param>
		/// <param name="end">The end color.</param>
		/// <param name="amount">The value between 0 and 1 indicating the weight of interpolation.</param>
		/// <param name="result">When the method completes, contains the linear interpolation of the two colors.</param>
		static void Lerp(const Color& start, const Color& end, float amount, Color& result);

		/// <summary>
		/// Performs a linear interpolation between two colors.
		/// </summary>
		/// <param name="start">The start color.</param>
		/// <param name="end">The end color.</param>
		/// <param name="amount">The value between 0 and 1 indicating the weight of interpolation.</param>
		/// <returns>The linear interpolation of the two colors.</returns>
		static Color Lerp(const Color& start, const Color& end, float amount);

		// Converts a [0.0, 1.0] linear value into a [0.0, 1.0] sRGB value.
		static Color LinearToSrgb(const Color& linear);

		// Converts a [0.0, 1.0] sRGB value into a [0.0, 1.0] linear value.
		static Color SrgbToLinear(const Color& srgb);

		/// <summary>
		/// Returns the color with RGB channels multiplied by the given scale factor. The alpha channel remains the same.
		/// </summary>
		/// <param name="multiplier">The multiplier.</param>
		/// <returns>The modified color.</returns>
		Color RGBMultiplied(float multiplier) const
		{
			return Color(r * multiplier, g * multiplier, b * multiplier, a);
		}

		/// <summary>
		/// Returns the color with RGB channels multiplied by the given color. The alpha channel remains the same.
		/// </summary>
		/// <param name="multiplier">The multiplier.</param>
		/// <returns>The modified color.</returns>
		Color RGBMultiplied(Color multiplier) const
		{
			return Color(r * multiplier.r, g * multiplier.g, b * multiplier.b, a);
		}

		/// <summary>
		/// Returns the color with alpha channel multiplied by the given scale factor. The RGB channels remain the same.
		/// </summary>
		/// <param name="multiplier">The multiplier.</param>
		/// <returns>The modified color.</returns>
		Color AlphaMultiplied(float multiplier) const
		{
			return Color(r, g, b, a * multiplier);
		}
	};

	uint32 SE_API_RUNTIME GetHash(const Color32 &color);

	uint32 SE_API_RUNTIME GetHash(const Color &color);
}

//-------------------------------------------------------------------------
// Predefined list of X11 Colors

namespace SE::Colors
{
	static Color const Clear = Color::FromRGBA(0x00000000);
    static Color const Transparent = Color::FromRGBA(0x00000000);
    static Color const AliceBlue = Color::FromRGBA(0xFFFFF8F0);
    static Color const AntiqueWhite = Color::FromRGBA(0xFFD7EBFA);
    static Color const Aqua = Color::FromRGBA(0xFFFFFF00);
    static Color const Aquamarine = Color::FromRGBA(0xFFD4FF7F);
    static Color const Azure = Color::FromRGBA(0xFFFFFFF0);
    static Color const Beige = Color::FromRGBA(0xFFDCF5F5);
    static Color const Bisque = Color::FromRGBA(0xFFC4E4FF);
    static Color const Black = Color::FromRGBA(0x000000FF);
    static Color const BlanchedAlmond = Color::FromRGBA(0xFFCDEBFF);
    static Color const Blue = Color::FromRGBA(0xFFFF0000);
    static Color const BlueViolet = Color::FromRGBA(0xFFE22B8A);
    static Color const Brown = Color::FromRGBA(0xFF2A2AA5);
    static Color const BurlyWood = Color::FromRGBA(0xFF87B8DE);
    static Color const CadetBlue = Color::FromRGBA(0xFFA09E5F);
    static Color const Chartreuse = Color::FromRGBA(0xFF00FF7F);
    static Color const Chocolate = Color::FromRGBA(0xFF1E69D2);
    static Color const Coral = Color::FromRGBA(0xFF507FFF);
    static Color const CornflowerBlue = Color::FromRGBA(0xFFED9564);
    static Color const Cornsilk = Color::FromRGBA(0xFFDCF8FF);
    static Color const Crimson = Color::FromRGBA(0xFF3C14DC);
    static Color const Cyan = Color::FromRGBA(0xFFFFFF00);
    static Color const DarkBlue = Color::FromRGBA(0xFF8B0000);
    static Color const DarkCyan = Color::FromRGBA(0xFF8B8B00);
    static Color const DarkGoldenRod = Color::FromRGBA(0xFF0B86B8);
    static Color const DarkGray = Color::FromRGBA(0xFFA9A9A9);
    static Color const DarkGreen = Color::FromRGBA(0xFF006400);
    static Color const DarkKhaki = Color::FromRGBA(0xFF6BB7BD);
    static Color const DarkMagenta = Color::FromRGBA(0xFF8B008B);
    static Color const DarkOliveGreen = Color::FromRGBA(0xFF2F6B55);
    static Color const DarkOrange = Color::FromRGBA(0xFF008CFF);
    static Color const DarkOrchid = Color::FromRGBA(0xFFCC3299);
    static Color const DarkRed = Color::FromRGBA(0xFF00008B);
    static Color const DarkSalmon = Color::FromRGBA(0xFF7A96E9);
    static Color const DarkSeaGreen = Color::FromRGBA(0xFF8FBC8F);
    static Color const DarkSlateBlue = Color::FromRGBA(0xFF8B3D48);
    static Color const DarkSlateGray = Color::FromRGBA(0xFF4F4F2F);
    static Color const DarkTurquoise = Color::FromRGBA(0xFFD1CE00);
    static Color const DarkViolet = Color::FromRGBA(0xFFD30094);
    static Color const DeepPink = Color::FromRGBA(0xFF9314FF);
    static Color const DeepSkyBlue = Color::FromRGBA(0xFFFFBF00);
    static Color const DimGray = Color::FromRGBA(0xFF696969);
    static Color const DodgerBlue = Color::FromRGBA(0xFFFF901E);
    static Color const FireBrick = Color::FromRGBA(0xFF2222B2);
    static Color const FloralWhite = Color::FromRGBA(0xFFF0FAFF);
    static Color const ForestGreen = Color::FromRGBA(0xFF228B22);
    static Color const Fuchsia = Color::FromRGBA(0xFFFF00FF);
    static Color const Gainsboro = Color::FromRGBA(0xFFDCDCDC);
    static Color const GhostWhite = Color::FromRGBA(0xFFFFF8F8);
    static Color const Gold = Color::FromRGBA(0xFF00D7FF);
    static Color const GoldenRod = Color::FromRGBA(0xFF20A5DA);
    static Color const Gray = Color::FromRGBA(0xFF808080);
    static Color const Green = Color::FromRGBA(0xFF008000);
    static Color const GreenYellow = Color::FromRGBA(0xFF2FFFAD);
    static Color const HoneyDew = Color::FromRGBA(0xFFF0FFF0);
    static Color const HotPink = Color::FromRGBA(0xFFB469FF);
    static Color const IndianRed = Color::FromRGBA(0xFF5C5CCD);
    static Color const Indigo = Color::FromRGBA(0xFF82004B);
    static Color const Ivory = Color::FromRGBA(0xFFF0FFFF);
    static Color const Khaki = Color::FromRGBA(0xFF8CE6F0);
    static Color const Lavender = Color::FromRGBA(0xFFFAE6E6);
    static Color const LavenderBlush = Color::FromRGBA(0xFFF5F0FF);
    static Color const LawnGreen = Color::FromRGBA(0xFF00FC7C);
    static Color const LemonChiffon = Color::FromRGBA(0xFFCDFAFF);
    static Color const LightBlue = Color::FromRGBA(0xFFE6D8AD);
    static Color const LightCoral = Color::FromRGBA(0xFF8080F0);
    static Color const LightCyan = Color::FromRGBA(0xFFFFFFE0);
    static Color const LightGoldenRodYellow = Color::FromRGBA(0xFFD2FAFA);
    static Color const LightGray = Color::FromRGBA(0xFFD3D3D3);
    static Color const LightGreen = Color::FromRGBA(0xFF90EE90);
    static Color const LightPink = Color::FromRGBA(0xFFC1B6FF);
    static Color const LightSalmon = Color::FromRGBA(0xFF7AA0FF);
    static Color const LightSeaGreen = Color::FromRGBA(0xFFAAB220);
    static Color const LightSkyBlue = Color::FromRGBA(0xFFFACE87);
    static Color const LightSlateGray = Color::FromRGBA(0xFF998877);
    static Color const LightSteelBlue = Color::FromRGBA(0xFFDEC4B0);
    static Color const LightYellow = Color::FromRGBA(0xFFE0FFFF);
    static Color const Lime = Color::FromRGBA(0xFF00FF00);
    static Color const LimeGreen = Color::FromRGBA(0xFF32CD32);
    static Color const Linen = Color::FromRGBA(0xFFE6F0FA);
    static Color const Magenta = Color::FromRGBA(0xFFFF00FF);
    static Color const Maroon = Color::FromRGBA(0xFF000080);
    static Color const MediumAquaMarine = Color::FromRGBA(0xFFAACD66);
    static Color const MediumBlue = Color::FromRGBA(0xFFCD0000);
    static Color const MediumOrchid = Color::FromRGBA(0xFFD355BA);
    static Color const MediumPurple = Color::FromRGBA(0xFFDB7093);
    static Color const MediumSeaGreen = Color::FromRGBA(0xFF71B33C);
    static Color const MediumSlateBlue = Color::FromRGBA(0xFFEE687B);
    static Color const MediumSpringGreen = Color::FromRGBA(0xFF9AFA00);
    static Color const MediumTurquoise = Color::FromRGBA(0xFFCCD148);
    static Color const MediumVioletRed = Color::FromRGBA(0xFF8515C7);
    static Color const MidnightBlue = Color::FromRGBA(0xFF701919);
    static Color const MintCream = Color::FromRGBA(0xFFFAFFF5);
    static Color const MistyRose = Color::FromRGBA(0xFFE1E4FF);
    static Color const Moccasin = Color::FromRGBA(0xFFB5E4FF);
    static Color const NavajoWhite = Color::FromRGBA(0xFFADDEFF);
    static Color const Navy = Color::FromRGBA(0xFF800000);
    static Color const OldLace = Color::FromRGBA(0xFFE6F5FD);
    static Color const Olive = Color::FromRGBA(0xFF008080);
    static Color const OliveDrab = Color::FromRGBA(0xFF238E6B);
    static Color const Orange = Color::FromRGBA(0xFF00A5FF);
    static Color const OrangeRed = Color::FromRGBA(0xFF0045FF);
    static Color const Orchid = Color::FromRGBA(0xFFD670DA);
    static Color const PaleGoldenRod = Color::FromRGBA(0xFFAAE8EE);
    static Color const PaleGreen = Color::FromRGBA(0xFF98FB98);
    static Color const PaleTurquoise = Color::FromRGBA(0xFFEEEEAF);
    static Color const PaleVioletRed = Color::FromRGBA(0xFF9370DB);
    static Color const PapayaWhip = Color::FromRGBA(0xFFD5EFFF);
    static Color const PeachPuff = Color::FromRGBA(0xFFB9DAFF);
    static Color const Peru = Color::FromRGBA(0xFF3F85CD);
    static Color const Pink = Color::FromRGBA(0xFFCBC0FF);
    static Color const Plum = Color::FromRGBA(0xFFDDA0DD);
    static Color const PowderBlue = Color::FromRGBA(0xFFE6E0B0);
    static Color const Purple = Color::FromRGBA(0xFF800080);
    static Color const Red = Color::FromRGBA(0xFF0000FF);
    static Color const MediumRed = Color::FromRGBA(0xFF0000C9);
    static Color const RosyBrown = Color::FromRGBA(0xFF8F8FBC);
    static Color const RoyalBlue = Color::FromRGBA(0xFFE16941);
    static Color const SaddleBrown = Color::FromRGBA(0xFF13458B);
    static Color const Salmon = Color::FromRGBA(0xFF7280FA);
    static Color const SandyBrown = Color::FromRGBA(0xFF60A4F4);
    static Color const SeaGreen = Color::FromRGBA(0xFF578B2E);
    static Color const SeaShell = Color::FromRGBA(0xFFEEF5FF);
    static Color const Sienna = Color::FromRGBA(0xFF2D52A0);
    static Color const Silver = Color::FromRGBA(0xFFC0C0C0);
    static Color const SkyBlue = Color::FromRGBA(0xFFEBCE87);
    static Color const SlateBlue = Color::FromRGBA(0xFFCD5A6A);
    static Color const SlateGray = Color::FromRGBA(0xFF908070);
    static Color const Snow = Color::FromRGBA(0xFFFAFAFF);
    static Color const SpringGreen = Color::FromRGBA(0xFF7FFF00);
    static Color const SteelBlue = Color::FromRGBA(0xFFB48246);
    static Color const Tan = Color::FromRGBA(0xFF8CB4D2);
    static Color const Teal = Color::FromRGBA(0xFF808000);
    static Color const Thistle = Color::FromRGBA(0xFFD8BFD8);
    static Color const Tomato = Color::FromRGBA(0xFF4763FF);
    static Color const Turquoise = Color::FromRGBA(0xFFD0E040);
    static Color const Violet = Color::FromRGBA(0xFFEE82EE);
    static Color const Wheat = Color::FromRGBA(0xFFB3DEF5);
    static Color const White = Color::FromRGBA(0xFFFFFFFF);
    static Color const WhiteSmoke = Color::FromRGBA(0xFFF5F5F5);
    static Color const Yellow = Color::FromRGBA(0xFFFF00FF);
    static Color const YellowGreen = Color::FromRGBA(0xFF32CD9A);
}