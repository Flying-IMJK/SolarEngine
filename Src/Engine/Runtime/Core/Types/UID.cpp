#include "UID.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Strings/StringView.h"

namespace SE
{

	UID UID::Empty;

	String UID::ToString() const
	{
		return ToString(FormatType::N);
	}

	String UID::ToString(FormatType format) const
	{
		switch (format)
		{
		case FormatType::N:
			return String::Format(SE_TEXT("{:0>8x}{:0>8x}{:0>8x}{:0>8x}"), A, B, C, D);
		case FormatType::B:
			return String::Format(SE_TEXT("{{{:0>8x}-{:0>4x}-{:0>4x}-{:0>4x}-{:0>4x}{:0>8x}}}"), A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
		case FormatType::P:
			return String::Format(SE_TEXT("({:0>8x}-{:0>4x}-{:0>4x}-{:0>4x}-{:0>4x}{:0>8x})"), A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
		default:
			return String::Format(SE_TEXT("{:0>8x}-{:0>4x}-{:0>4x}-{:0>4x}-{:0>4x}{:0>8x}"), A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
		}
	}

	template<typename CharType>
	inline void UUIDToString(const CharType* cachedGuidDigits, CharType* buffer, const UID& value, UID::FormatType format)
	{
		switch (format)
		{
		case UID::FormatType::N:
		{
			for (int32 i = 0; i < 32; i++)
				buffer[i] = '0';
			buffer[32] = 0;
			uint32 n = value.A;
			CharType* p = buffer + 7;
			do
			{
				*p-- = cachedGuidDigits[(int32)(n & 0xf)];
			} while ((n >>= 4) != 0);
			n = value.B;
			p = buffer + 15;
			do
			{
				*p-- = cachedGuidDigits[(int32)(n & 0xf)];
			} while ((n >>= 4) != 0);
			n = value.C;
			p = buffer + 23;
			do
			{
				*p-- = cachedGuidDigits[(int32)(n & 0xf)];
			} while ((n >>= 4) != 0);
			n = value.D;
			p = buffer + 31;
			do
			{
				*p-- = cachedGuidDigits[(int32)(n & 0xf)];
			} while ((n >>= 4) != 0);
			break;
		}
		case UID::FormatType::B:
			// TODO: impl GuidToString for FormatType::B:
		case UID::FormatType::P:
			// TODO: impl GuidToString for FormatType::P:
		default:
			// TODO: impl GuidToString for FormatType::D:
			LOG_ERROR("UUID", "Missing UUIDToString impl.");
		}
	}

	uint32& UID::operator[](int32 index)
	{
		ENGINE_ASSERT(index >= 0 && index < 4);
		return Values[index];
	}

	const uint32& UID::operator[](int index) const
	{
		ENGINE_ASSERT(index >= 0 && index < 4);
		return Values[index];
	}

	void UID::ToString(char* buffer, FormatType format) const
	{
		const static char* CachedGuidDigits = "0123456789abcdef";
		return UUIDToString<char>(CachedGuidDigits, buffer, *this, format);
	}

	void UID::ToString(Char* buffer, FormatType format) const
	{
		const static Char* CachedGuidDigits = SE_TEXT("0123456789abcdef");
		return UUIDToString<Char>(CachedGuidDigits, buffer, *this, format);
	}

	template<typename StringType, typename StringViewType>
	inline bool UUIDParse(const StringViewType& text, UID& value)
	{
		switch (text.Length())
		{
			// FormatType::N
		case 32:
		{
			return
				StringUtils::ParseHex(*text + 0, 8, &value.A) ||
					StringUtils::ParseHex(*text + 8, 8, &value.B) ||
					StringUtils::ParseHex(*text + 16, 8, &value.C) ||
					StringUtils::ParseHex(*text + 24, 8, &value.D);
		}
			// FormatType::D
		case 36:
		{
			StringType b = StringType(text.Substring(9, 4)) + text.Substring(14, 4);
			StringType c = StringType(text.Substring(19, 4)) + text.Substring(24, 4);
			return
				StringUtils::ParseHex(*text + 0, 8, &value.A) ||
					StringUtils::ParseHex(*b, &value.B) ||
					StringUtils::ParseHex(*c, &value.C) ||
					StringUtils::ParseHex(*text + 28, 8, &value.D);
		}
			// FormatType::B
			// FormatType::P
		case 38:
		{
			StringType b = StringType(text.Substring(10, 4)) + text.Substring(15, 4);
			StringType c = StringType(text.Substring(20, 4)) + text.Substring(25, 4);
			return
				text[0] != text[text.Length() - 1] ||
					StringUtils::ParseHex(*text + 1, 8, &value.A) ||
					StringUtils::ParseHex(*b, &value.B) ||
					StringUtils::ParseHex(*c, &value.C) ||
					StringUtils::ParseHex(*text + 29, 8, &value.D);
		}
		default:
			return true;
		}
	}

	bool UID::Parse(const StringView& text, UID& value)
	{
		return UUIDParse<String, StringView>(text, value);
	}

	bool UID::Parse(const StringAnsiView& text, UID& value)
	{
		return UUIDParse<StringAnsi, StringAnsiView>(text, value);
	}

}