
#include "Core/Platform/StringUtils.h"
#include "Core/Logging/Logging.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/List.h"
#if PLATFORM_TEXT_IS_CHAR16
#include <string>
#endif

namespace SE
{
    constexpr char DirectorySeparatorChar = '\\';
    constexpr char AltDirectorySeparatorChar = '/';
    constexpr char VolumeSeparatorChar = ':';

    const Char *StringUtils::FindIgnoreCase(const Char *str, const Char *toFind)
    {
        if (toFind == nullptr || str == nullptr)
        {
            return nullptr;
        }

        const Char findInitial = ToUpper(*toFind);
        const int32 length = Length(toFind++) - 1;
        Char c = *str++;
        while (c)
        {
            c = ToUpper(c);
            if (c == findInitial && !CompareIgnoreCase(str, toFind, length))
            {
                return str - 1;
            }

            c = *str++;
        }

        return nullptr;
    }

    const char *StringUtils::FindIgnoreCase(const char *str, const char *toFind)
    {
        if (toFind == nullptr || str == nullptr)
        {
            return nullptr;
        }

        const char findInitial = (char) ToUpper(*toFind);
        const int32 length = Length(toFind++) - 1;
        char c = *str++;
        while (c)
        {
            c = (char) ToUpper(c);
            if (c == findInitial && !CompareIgnoreCase(str, toFind, length))
            {
                return str - 1;
            }

            c = *str++;
        }

        return nullptr;
    }

    void PrintUTF8Error(const char *from, uint32 fromLength)
    {
        LOG_ERROR("Platform", "Not a UTF-8 string. Length: {0}", fromLength);
        for (uint32 i = 0; i < fromLength; i++)
        {
			LOG_ERROR("Platform", "str[{0}] = {1}", i, from[i]);
        }
    }

    void ConvertUTF82UTF16Helper(List <uint32> &unicode, const char *from, int32 fromLength, int32 &toLength)
    {
        // Reference: https://stackoverflow.com/questions/7153935/how-to-convert-utf-8-stdstring-to-utf-16-stdwstring
        unicode.EnsureCapacity(fromLength);
        int32 i = 0, todo;
        uint32 uni;
        toLength = 0;
        while (i < fromLength)
        {
            byte ch = from[i++];

            if (ch <= 0x7F)
            {
                uni = ch;
                todo = 0;
            }
            else if (ch <= 0xBF)
            {
                PrintUTF8Error(from, fromLength);
                return;
            }
            else if (ch <= 0xDF)
            {
                uni = ch & 0x1F;
                todo = 1;
            }
            else if (ch <= 0xEF)
            {
                uni = ch & 0x0F;
                todo = 2;
            }
            else if (ch <= 0xF7)
            {
                uni = ch & 0x07;
                todo = 3;
            }
            else
            {
                PrintUTF8Error(from, fromLength);
                return;
            }

            for (int32 j = 0; j < todo; j++)
            {
                if (i == fromLength)
                {
                    PrintUTF8Error(from, fromLength);
                    return;
                }
                ch = from[i++];
                if (ch < 0x80 || ch > 0xBF)
                {
                    PrintUTF8Error(from, fromLength);
                    return;
                }

                uni <<= 6;
                uni += ch & 0x3F;
            }

            if ((uni >= 0xD800 && uni <= 0xDFFF) || uni > 0x10FFFF)
            {
                PrintUTF8Error(from, fromLength);
                return;
            }

            unicode.Add(uni);

            toLength++;
            if (uni > 0xFFFF)
            {
                toLength++;
            }
        }
    }

    void StringUtils::ConvertUTF82UTF16(const char *from, Char *to, int32 fromLength, int32 &toLength)
    {
        List<uint32> unicode;
        ConvertUTF82UTF16Helper(unicode, from, fromLength, toLength);
        for (int32 i = 0, j = 0; j < unicode.Count(); i++, j++)
        {
            uint32 uni = unicode[j];
            if (uni <= 0xFFFF)
            {
                to[i] = (Char) uni;
            }
            else
            {
                uni -= 0x10000;
                to[i++] += (Char) ((uni >> 10) + 0xD800);
                to[i] += (Char) ((uni & 0x3FF) + 0xDC00);
            }
        }
    }

	Char *StringUtils::ConvertUTF82UTF16(const char *from, int32 fromLength, int32 &toLength)
    {
		List <uint32> unicode;
        ConvertUTF82UTF16Helper(unicode, from, fromLength, toLength);
        if (toLength == 0)
            return nullptr;
		Char *to = (Char *) PlatformAllocator::Allocate((toLength + 1) * sizeof(Char));
        for (int32 i = 0, j = 0; j < unicode.Count(); i++, j++)
        {
            uint32 uni = unicode[j];
            if (uni <= 0xFFFF)
            {
                to[i] = (Char) uni;
            }
            else
            {
                uni -= 0x10000;
                to[i++] += (Char) ((uni >> 10) + 0xD800);
                to[i] += (Char) ((uni & 0x3FF) + 0xDC00);
            }
        }
        to[toLength] = 0;
        return to;
    }

    void PrintUTF16Error(const Char *from, uint32 fromLength)
    {
		LOG_ERROR("Platform", "Not a UTF-16 string. Length: {0}", fromLength);
        for (uint32 i = 0; i < fromLength; i++)
        {
			LOG_ERROR("Platform", "str[{0}] = {0}", i, (uint32) from[i]);
        }
    }

    void ConvertUTF162UTF8Helper(List<uint32> &unicode, const Char *from, int32 fromLength, int32 &toLength)
    {
        // Reference: https://stackoverflow.com/questions/21456926/how-do-i-convert-a-string-in-utf-16-to-utf-8-in-c
        unicode.EnsureCapacity(fromLength);
        toLength = 0;
        int32 i = 0;
        while (i < fromLength)
        {
            uint32 uni = from[i++];
            if (uni < 0xD800U || uni > 0xDFFFU)
            {
            }
            else if (uni >= 0xDC00U)
            {
                PrintUTF16Error(from, fromLength);
                return;
            }
            else if (i + 1 == fromLength)
            {
                PrintUTF16Error(from, fromLength);
                return;
            }
            else if (i < fromLength)
            {
                uni = (uni & 0x3FFU) << 10;
                if ((from[i] < 0xDC00U) || (from[i] > 0xDFFFU))
                {
                    PrintUTF16Error(from, fromLength);
                    return;
                }
                uni |= from[i++] & 0x3FFU;
                uni += 0x10000U;
            }

            unicode.Add(uni);

            toLength += uni <= 0x7FU ? 1 :
				uni <= 0x7FFU ? 2 :
				uni <= 0xFFFFU ? 3 :
				uni <= 0x1FFFFFU ? 4 :
				uni <= 0x3FFFFFFU ? 5 :
				uni <= 0x7FFFFFFFU ? 6 : 7;
        }
    }

    void StringUtils::ConvertUTF162UTF8(const Char *from, char *to, int32 fromLength, int32 &toLength)
    {
		List <uint32> unicode;
        ConvertUTF162UTF8Helper(unicode, from, fromLength, toLength);
        for (int32 i = 0, j = 0; j < unicode.Count(); j++)
        {
            const uint32 uni = unicode[j];
            const uint32 count =
                    uni <= 0x7FU ? 1 : uni <= 0x7FFU ? 2 : uni <= 0xFFFFU ? 3 : uni <= 0x1FFFFFU ? 4 : uni <= 0x3FFFFFFU
                                                                                                       ? 5 : uni <=
                                                                                                             0x7FFFFFFFU
                                                                                                             ? 6 : 7;
            to[i++] = (char) (count <= 1 ? (byte) uni : ((byte(0xFFU) << (8 - count)) |
                                                         byte(uni >> (6 * (count - 1)))));
            for (uint32 k = 1; k < count; k++)
                to[i++] = char(byte(0x80U | (byte(0x3FU) & byte(uni >> (6 * (count - 1 - k))))));
        }
    }

    char *StringUtils::ConvertUTF162UTF8(const Char *from, int32 fromLength, int32 &toLength)
    {
		List <uint32> unicode;
        ConvertUTF162UTF8Helper(unicode, from, fromLength, toLength);
        if (toLength == 0)
            return nullptr;
        char *to = (char *) PlatformAllocator::Allocate(toLength + 1);
        for (int32 i = 0, j = 0; j < unicode.Count(); j++)
        {
            const uint32 uni = unicode[j];
            const uint32 count =
                    uni <= 0x7FU ? 1 : uni <= 0x7FFU ? 2 : uni <= 0xFFFFU ? 3 : uni <= 0x1FFFFFU ? 4 : uni <= 0x3FFFFFFU
                                                                                                       ? 5 : uni <=
                                                                                                             0x7FFFFFFFU
                                                                                                             ? 6 : 7;
            to[i++] = (char) (count <= 1 ? (byte) uni : ((byte(0xFFU) << (8 - count)) |
                                                         byte(uni >> (6 * (count - 1)))));
            for (uint32 k = 1; k < count; k++)
                to[i++] = char(byte(0x80U | (byte(0x3FU) & byte(uni >> (6 * (count - 1 - k))))));
        }
        to[toLength] = 0;
        return to;
    }



    int32 StringUtils::HexDigit(Char c)
    {
        int32 result = 0;
        if (c >= '0' && c <= '9')
            result = c - '0';
        else if (c >= 'a' && c <= 'f')
            result = c + 10 - 'a';
        else if (c >= 'A' && c <= 'F')
            result = c + 10 - 'A';
        return result;
    }

	int32 StringUtils::HexDigit(char c)
	{
		int32 result = 0;
		if (c >= '0' && c <= '9')
			result = c - '0';
		else if (c >= 'a' && c <= 'f')
			result = c + 10 - 'a';
		else if (c >= 'A' && c <= 'F')
			result = c + 10 - 'A';
		return result;
	}

    bool StringUtils::Parse(const Char *str, float *result)
    {
#if PLATFORM_CHAR16
        std::u16string u16str = str;
		std::wstring wstr(u16str.begin(), u16str.end());
		float v = wcstof(wstr.c_str(), nullptr);
#else
        float v = wcstof(str, nullptr);
#endif
        *result = v;
        if (v == 0)
        {
            const int32 len = Length(str);
            return (str[0] == '0' && ((len == 1) || (len == 3 && (str[1] == ',' || str[1] == '.') && str[2] == '0')));
        }
        return true;
    }

    bool StringUtils::Parse(const char *str, float *result)
    {
        *result = (float) atof(str);
        return true;
    }

	bool StringUtils::Parse(const Char *str, double *result)
	{
#if PLATFORM_CHAR16
		std::u16string u16str = str;
		std::wstring wstr(u16str.begin(), u16str.end());
		double v = _wtof_l(wstr.c_str(), nullptr);
#else
		double v = _wtof_l(str, nullptr);
#endif
		*result = v;
		if (v == 0)
		{
			const int32 len = Length(str);
			return (str[0] == '0' && ((len == 1) || (len == 3 && (str[1] == ',' || str[1] == '.') && str[2] == '0')));
		}
		return true;
	}

	bool StringUtils::Parse(const char *str, double *result)
	{
		*result = (float) atof(str);
		return true;
	}

    String StringUtils::ToString(int32 value)
    {
        return String::Format(SE_TEXT("{}"), value);
    }

    String StringUtils::ToString(int64 value)
    {
        return String::Format(SE_TEXT("{}"), value);
    }

    String StringUtils::ToString(uint32 value)
    {
        return String::Format(SE_TEXT("{}"), value);
    }

    String StringUtils::ToString(uint64 value)
    {
        return String::Format(SE_TEXT("{}"), value);
    }

    String StringUtils::ToString(float value)
    {
        return String::Format(SE_TEXT("{}"), value);
    }

    String StringUtils::ToString(double value)
    {
        return String::Format(SE_TEXT("{}"), value);
    }

	StringAnsi StringUtils::ToStringAnsi(int32 value)
	{
		return StringAnsi::Format("{}", value);
	}

	StringAnsi StringUtils::ToStringAnsi(int64 value)
	{
		return StringAnsi::Format("{}", value);
	}

	StringAnsi StringUtils::ToStringAnsi(uint32 value)
	{
		return StringAnsi::Format("{}", value);
	}

	StringAnsi StringUtils::ToStringAnsi(uint64 value)
	{
		return StringAnsi::Format("{}", value);
	}

	StringAnsi StringUtils::ToStringAnsi(float value)
	{
		return StringAnsi::Format("{}", value);
	}

	StringAnsi StringUtils::ToStringAnsi(double value)
	{
		return StringAnsi::Format("{}", value);
	}

    String StringUtils::GetZZString(const Char *str)
    {
        const Char *end = str;
        while (*end != '\0')
        {
            end++;
            if (*end == '\0')
                end++;
        }
        return String(str, (int32) (end - str));
    }
}