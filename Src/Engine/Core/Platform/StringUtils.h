#pragma once

#include "Core/API.h"
#include "Core/Types/Variable.h"

namespace SE
{
    /// <summary>
    /// Describes case sensitivity options for string comparisons.
    /// </summary>
    enum class StringSearchCase
    {
        /// <summary>
        /// Case sensitive. Upper/lower casing must match for strings to be considered equal.
        /// </summary>
        CaseSensitive = 0,

        /// <summary>
        /// Ignore case. Upper/lower casing does not matter when making a comparison.
        /// </summary>
        IgnoreCase = 1,
    };

    /// <summary>
    /// The string operations utilities.
    /// </summary>
    class SE_API_CORE StringUtils
    {
    public:
        /// <summary>
        /// Calculates the hash code for input string.
        /// </summary>
        /// <param name="str">The pointer to the input characters sequence.</param>
        /// <returns>The unique hash value.</returns>
        template<typename CharType>
        static uint32 GetHashCode(const CharType *str)
        {
            uint32 hash = 5381;
            CharType c;
            if (str)
            {
                while ((c = *str++) != 0)
                    hash = ((hash << 5) + hash) + (uint32) c;
            }
            return hash;
        }

        /// <summary>
        /// Calculates the hash code for input string.
        /// </summary>
        /// <param name="str">The pointer to the input characters sequence.</param>
        /// <param name="length">The input sequence length (amount of characters).</param>
        /// <returns>The unique hash value.</returns>
        template<typename CharType>
        static uint32 GetHashCode(const CharType *str, int32 length)
        {
            uint32 hash = 5381;
            CharType c;
            if (str)
            {
                while ((c = *str++) != 0 && length-- > 0)
                    hash = ((hash << 5) + hash) + (uint32) c;
            }
            return hash;
        }

    public:
        // Returns true if character is uppercase
        static bool IsUpper(char c);

        // Returns true if character is lowercase
        static bool IsLower(char c);

        static bool IsAlpha(char c);

        static bool IsPunct(char c);

        static bool IsAlnum(char c);

        static bool IsDigit(char c);

        static bool IsHexDigit(char c);

        // Returns true if character is a whitespace
        static bool IsWhitespace(char c);

        // Convert wide character to upper case
        static char ToUpper(char c);

        // Convert wide character to lower case
        static char ToLower(char c);

    public:
        // Returns true if character is uppercase
        static bool IsUpper(Char c);

        // Returns true if character is lowercase
        static bool IsLower(Char c);

        static bool IsAlpha(Char c);

        static bool IsPunct(Char c);

        static bool IsAlnum(Char c);

        static bool IsDigit(Char c);

        static bool IsHexDigit(Char c);

        // Returns true if character is a whitespace
        static bool IsWhitespace(Char c);

        // Convert wide character to upper case
        static Char ToUpper(Char c);

        // Convert wide character to lower case
        static Char ToLower(Char c);

    public:
        // Compare two strings with case sensitive. Strings must not be null.
        static int32 Compare(const Char *str1, const Char *str2);

        // Compare two strings without case sensitive. Strings must not be null.
        static int32 Compare(const Char *str1, const Char *str2, int32 maxCount);

		static int32 Compare(const Char* str1, int32 strLength1, const Char* str2, int32 strLength2);

        // Compare two strings without case sensitive. Strings must not be null.
        static int32 CompareIgnoreCase(const Char *str1, const Char *str2);

        // Compare two strings without case sensitive. Strings must not be null.
        static int32 CompareIgnoreCase(const Char *str1, const Char *str2, int32 maxCount);

		static int32 CompareIgnoreCase(const Char* str1, int32 strLength1, const Char* str2, int32 strLength2);

        // Compare two strings with case sensitive. Strings must not be null.
        static int32 Compare(const char *str1, const char *str2);

        // Compare two strings without case sensitive. Strings must not be null.
        static int32 Compare(const char *str1, const char *str2, int32 maxCount);

		// Compare two strings without case sensitive. Strings must not be null.
		static int32 Compare(const char *str1, int32 strLength1, const char *str2, int32 strLength2);

        // Compare two strings without case sensitive. Strings must not be null.
        static int32 CompareIgnoreCase(const char *str1, const char *str2);

        // Compare two strings without case sensitive. Strings must not be null.
        static int32 CompareIgnoreCase(const char *str1, const char *str2, int32 maxCount);

		static int32 CompareIgnoreCase(const char* str1, int32 strLength1, const char* str2, int32 strLength2);

    public:
        // Get string length. Returns 0 if str is null.
        static int32 Length(const Char *str);

        // Get string length. Returns 0 if str is null.
        static int32 Length(const char *str);

        // Copy string
        static Char *Copy(Char *dst, const Char *src);

        // Copy string (count is maximum amount of characters to copy)
        static Char *Copy(Char *dst, const Char *src, int32 count);

        // Finds string in string, case sensitive
        // @param str The string to look through
        // @param toFind The string to find inside str
        // @return Position in str if Find was found, otherwise null
        static const Char *Find(const Char *str, const Char *toFind);

        // Finds string in string, case sensitive
        // @param str The string to look through
        // @param toFind The string to find inside str
        // @return Position in str if Find was found, otherwise null
        static const char *Find(const char *str, const char *toFind);

        // Finds string in string, case insensitive
        // @param str The string to look through
        // @param toFind The string to find inside str
        // @return Position in str if toFind was found, otherwise null
        static const Char *FindIgnoreCase(const Char *str, const Char *toFind);

        // Finds string in string, case insensitive
        // @param str The string to look through
        // @param toFind The string to find inside str
        // @return Position in str if toFind was found, otherwise null
        static const char *FindIgnoreCase(const char *str, const char *toFind);

    public:
        // Converts characters from ANSI to UTF-16
        static void ConvertANSI2UTF16(const char *from, Char *to, int32 fromLength, int32 &toLength);

        // Converts characters from UTF-16 to ANSI
        static void ConvertUTF162ANSI(const Char *from, char *to, int32 len);

        // Convert characters from UTF-8 to UTF-16
        static void ConvertUTF82UTF16(const char *from, Char *to, int32 fromLength, int32 &toLength);

        // Convert characters from UTF-8 to UTF-16 (allocates the output buffer with Allocator::Allocate of size (toLength + 1) * sizeof(Char), call Allocator::Free after usage). Returns null on empty or invalid string.
        static Char *ConvertUTF82UTF16(const char *from, int32 fromLength, int32 &toLength);

        // Convert characters from UTF-16 to UTF-8
        static void ConvertUTF162UTF8(const Char *from, char *to, int32 fromLength, int32 &toLength);

        // Convert characters from UTF-16 to UTF-8 (allocates the output buffer with Allocator::Allocate of size toLength + 1, call Allocator::Free after usage). Returns null on empty or invalid string.
        static char *ConvertUTF162UTF8(const Char *from, int32 fromLength, int32 &toLength);

    public:
        // Converts hexadecimal character into the value.
        static int32 HexDigit(Char c);

		static int32 HexDigit(char c);

        // Parse text to unsigned integer value
        // @param str String to parse
        // @return Result value
        // @returns False if cannot convert data, otherwise false
        template<typename CharType>
        static bool ParseHex(const CharType *str, uint32 *result)
        {
            uint32 sum = 0;
            const CharType *p = str;

            if (*p == '0' && *(p + 1) == 'x')
                p += 2;

            while (*p)
            {
                int32 c = *p - '0';

                if (c < 0 || c > 9)
                {
                    c = ToLower(*p) - 'a' + 10;
                    if (c < 10 || c > 15)
                        return false;
                }

                sum = 16 * sum + c;

                p++;
            }

            *result = sum;
            return true;
        }

        // Parse text to unsigned integer value
        // @param str String to parse
        // @param length Text length
        // @return Result value
        // @returns False if cannot convert data, otherwise false
        template<typename CharType>
        static bool ParseHex(const CharType *str, int32 length, uint32 *result)
        {
            uint32 sum = 0;
            const CharType *p = str;
            const CharType *end = str + length;

            if (*p == '0' && *(p + 1) == 'x')
                p += 2;

            while (*p && p < end)
            {
                int32 c = *p - '0';

                if (c < 0 || c > 9)
                {
                    c = ToLower(*p) - 'a' + 10;
                    if (c < 10 || c > 15)
                        return false;
                }

                sum = 16 * sum + c;

                p++;
            }

            *result = sum;
            return true;
        }

        // Parse text to scalar value
        // @param str String to parse
        // @return Result value
        // @returns False if cannot convert data, otherwise false
        template<typename T, typename U>
        static bool Parse(const T *str, U *result)
        {
            U sum = 0;
            const T *p = str;
            while (*p)
            {
                int32 c = *p++ - 48;
                if (c < 0 || c > 9)
                    return false;
                sum = 10 * sum + c;
            }
            *result = sum;
            return true;
        }

        // Parse text to scalar value
        // @param str String to parse
        // @param length Text length to read
        // @return Result value
        // @returns False if cannot convert data, otherwise false
        template<typename T, typename U>
        static bool Parse(const T *str, uint32 length, U *result)
        {
            U sum = 0;
            const T *p = str;
            while (length--)
            {
                int32 c = *p++ - 48;
                if (c < 0 || c > 9)
                    return false;
                sum = 10 * sum + c;
            }
            *result = sum;
            return true;
        }

        // Parse Unicode text to float value
        // @param str String to parse
        // @return Result value
        // @returns False if cannot convert data, otherwise false
        static bool Parse(const Char *str, float *result);

        static bool Parse(const char *str, float *result);

		// Parse Unicode text to double value
		// @param str String to parse
		// @return Result value
		// @returns False if cannot convert data, otherwise false
		static bool Parse(const Char *str, double *result);

		static bool Parse(const char *str, double *result);

    public:
        static String ToString(int32 value);

        static String ToString(int64 value);

        static String ToString(uint32 value);

        static String ToString(uint64 value);

        static String ToString(float value);

        static String ToString(double value);

		static StringAnsi ToStringAnsi(int32 value);

		static StringAnsi ToStringAnsi(int64 value);

		static StringAnsi ToStringAnsi(uint32 value);

		static StringAnsi ToStringAnsi(uint64 value);

		static StringAnsi ToStringAnsi(float value);

		static StringAnsi ToStringAnsi(double value);

    public:
        // Returns the String to double null-terminated string
        // @param str Double null-terminated string
        // @return Double null-terminated String
        static String GetZZString(const Char *str);
    };

    inline uint32 GetHash(const char *key)
    {
        return StringUtils::GetHashCode(key);
    }

    inline uint32 GetHash(const Char *key)
    {
        return StringUtils::GetHashCode(key);
    }

    inline uint32 GetHash(const char *key, int32 length)
    {
        return StringUtils::GetHashCode(key, length);
    }

    inline uint32 GetHash(const Char *key, int32 length)
    {
        return StringUtils::GetHashCode(key, length);
    }
}