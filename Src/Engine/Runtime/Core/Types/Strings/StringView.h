
#pragma once

#include "Runtime/Core/Platform/StringUtils.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Formatting.h"

namespace SE
{

	/// <summary>
	/// Represents static text view as a sequence of characters. Characters sequence might not be null-terminated.
	/// </summary>
	template<typename T>
	class StringViewBase
	{
	protected:
		const T* m_data;
		int32 m_length;

		constexpr StringViewBase()
			: m_data(nullptr), m_length(0)
		{
		}

		constexpr StringViewBase(const T* data, int32 length)
			: m_data(data), m_length(length)
		{
		}

	public:
		typedef T CharType;

		/// <summary>
		/// Gets the specific const character from this string.
		/// </summary>
		/// <param name="index">The index.</param>
		/// <returns>The character at given index.</returns>
		inline const T& operator[](int32 index) const
		{
			ENGINE_ASSERT(index >= 0 && index <= m_length);
			return m_data[index];
		}

		inline StringViewBase& operator=(const StringViewBase& other)
		{
			if (this != &other)
			{
				m_data = other.m_data;
				m_length = other.m_length;
			}
			return *this;
		}

		/// <summary>
		/// Lexicographically tests how this string compares to the other given string. In case sensitive mode 'A' is less than 'a'.
		/// </summary>
		/// <param name="str">The another string test against.</param>
		/// <param name="searchCase">The case sensitivity mode.</param>
		/// <returns>0 if equal, negative number if less than, positive number if greater than.</returns>
		int32 Compare(const StringViewBase& str, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			const bool thisIsShorter = Length() < str.Length();
			const int32 minLength = thisIsShorter ? Length() : str.Length();
			const int32 prefixCompare = searchCase == StringSearchCase::CaseSensitive
										? StringUtils::Compare(GetText(), str.GetText(), minLength)
										: StringUtils::CompareIgnoreCase(GetText(), str.GetText(), minLength);
			if (prefixCompare != 0)
				return prefixCompare;
			if (Length() == str.Length())
				return 0;
			return thisIsShorter ? -1 : 1;
		}

	public:
		/// <summary>
		/// Returns true if string is empty.
		/// </summary>
		inline bool IsEmpty() const
		{
			return m_length == 0;
		}

		/// <summary>
		/// Returns true if string isn't empty.
		/// </summary>
		inline bool HasChars() const
		{
			return m_length != 0;
		}

		/// <summary>
		/// Gets the length of the string.
		/// </summary>
		inline constexpr int32 Length() const
		{
			return m_length;
		}

		/// <summary>
		/// Gets the pointer to the string. Pointer can be null, and won't be null-terminated.
		/// </summary>
		inline constexpr const T* operator*() const
		{
			return m_data;
		}

		/// <summary>
		/// Gets the pointer to the string. Pointer can be null, and won't be null-terminated.
		/// </summary>
		inline constexpr const T* Get() const
		{
			return m_data;
		}

		/// <summary>
		/// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always non-null, but is not null-terminated.
		/// [Deprecated on 26.10.2022, expires on 26.10.2024] Use GetText()
		/// </summary>
		const T* GetNonTerminatedText() const
		{
			return m_data ? m_data : (const T*)SE_TEXT("");
		}

		/// <summary>
		/// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always valid (read-only).
		/// </summary>
		/// <returns>The string handle.</returns>
		inline const T* GetText() const
		{
			return m_data ? m_data : (const T*)SE_TEXT("");
		}

	public:
		/// <summary>
		/// Searches the string for the occurrence of a character.
		/// </summary>
		/// <param name="c">The character to search for.</param>
		/// <returns>The index of the character position in the string or -1 if not found.</returns>
		int32 Find(T c) const
		{
			const T* start = Get();
			for (const T* data = start, * dataEnd = data + m_length; data != dataEnd; ++data)
			{
				if (*data == c)
				{
					return static_cast<int32>(data - start);
				}
			}
			return -1;
		}

		/// <summary>
		/// Searches the string for the last occurrence of a character.
		/// </summary>
		/// <param name="c">The character to search for.</param>
		/// <returns>The index of the character position in the string or -1 if not found.</returns>
		int32 FindLast(T c) const
		{
			const T* end = Get() + m_length;
			for (const T* data = end, * dataStart = data - m_length; data != dataStart;)
			{
				--data;
				if (*data == c)
				{
					return static_cast<int32>(data - dataStart);
				}
			}
			return -1;
		}

		int32 FindLast(T c, int index) const
		{
			const T* end = Get() + m_length - index;
			for (const T* data = end, * dataStart = data - m_length; data != dataStart;)
			{
				--data;
				if (*data == c)
				{
					return static_cast<int32>(data - dataStart);
				}
			}
			return -1;
		}

		/// <summary>
		/// Searches the string starting from beginning for a substring, and returns index into this string of the first found instance.
		/// </summary>
		/// <param name="subStr">The string sequence to search for.</param>
		/// <param name="searchCase">The search case sensitivity mode.</param>
		/// <param name="startPosition">The start character position to search from.</param>
		/// <returns>The index of the found substring or -1 if not found.</returns>
		int32 Find(const T* subStr,
			int32 startPosition = -1,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			if (subStr == nullptr || !m_data)
			{
				return INVALID_INDEX;
			}
			const T* start = m_data;
			if (startPosition != -1)
			{
				start += startPosition < Length() ? startPosition : Length();
			}

			const T* tmp = searchCase == StringSearchCase::IgnoreCase ?
						   StringUtils::FindIgnoreCase(start, subStr) :
						   StringUtils::Find(start, subStr);
			return tmp ? static_cast<int32>(tmp - **this) : INVALID_INDEX;
		}

		/// <summary>
		/// Searches the string starting from end for a substring, and returns index into this string of the first found instance.
		/// </summary>
		/// <param name="subStr">The string sequence to search for.</param>
		/// <param name="searchCase">The search case sensitivity mode.</param>
		/// <param name="startPosition">The start character position to search from.</param>
		/// <returns>The index of the found substring or -1 if not found.</returns>
		int32 FindLast(const T* subStr, int32 startPosition = -1,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			const int32 subStrLen = StringUtils::Length(subStr);
			if (subStrLen == 0 || !m_data)
			{
				return INVALID_INDEX;
			}
			if (startPosition == -1)
			{
				startPosition = Length();
			}
			const T* start = m_data;
			if (searchCase == StringSearchCase::IgnoreCase)
			{
				for (int32 i = startPosition - subStrLen; i >= 0; i--)
				{
					if (StringUtils::CompareIgnoreCase(start + i, subStr, subStrLen) == 0)
					{
						return i;
					}
				}
			}
			else
			{
				for (int32 i = startPosition - subStrLen; i >= 0; i--)
				{
					if (StringUtils::Compare(start + i, subStr, subStrLen) == 0)
					{
						return i;
					}
				}
			}
			return INVALID_INDEX;
		}

		/// <summary>
		/// Searches the string for the first character that matches the character specified.
		/// </summary>
		/// <param name="c">The character to search for.</param>
		/// <param name="startPos">The start position of the search. The search only includes characters at or after position, ignoring any possible occurrences before it.</param>
		/// <returns>The position of the first character that matches. If no matches are found, the function returns -1.</returns>
		int32 FindFirstOf(T c, int32 startPos = 0) const
		{
			for (int32 i = startPos; i < Length(); i++)
			{
				if (m_data[i] == c)
				{
					return i;
				}
			}
			return INVALID_INDEX;
		}

		/// <summary>
		/// Searches the string for the first character that matches any of the characters specified in its arguments.
		/// </summary>
		/// <param name="str">The pointer to the array of characters that are searched for.</param>
		/// <param name="startPos">The start position of the search. The search only includes characters at or after position, ignoring any possible occurrences before it.</param>
		/// <returns>The position of the first character that matches. If no matches are found, the function returns -1.</returns>
		int32 FindFirstOf(const T* str, int32 startPos = 0) const
		{
			if (!str)
			{
				return -1;
			}
			for (int32 i = startPos; i < m_length; i++)
			{
				const T c = m_data[i];
				const T* s = str;
				while (*s)
				{
					if (c == *s)
					{
						return i;
					}
					s++;
				}
			}
			return INVALID_INDEX;
		}

	protected:

		bool StartsWithBase(T c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			const int32 length = Length();
			if (searchCase == StringSearchCase::IgnoreCase)
				return length > 0 && StringUtils::ToLower(m_data[0]) == StringUtils::ToLower(c);
			return length > 0 && m_data[0] == c;
		}

		bool EndsWithBase(T c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			const int32 length = Length();
			if (searchCase == StringSearchCase::IgnoreCase)
				return length > 0 && StringUtils::ToLower(m_data[length - 1]) == StringUtils::ToLower(c);
			return length > 0 && m_data[length - 1] == c;
		}

		template<typename SubClass>
		bool StartsWithBase(const SubClass& prefix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			if (prefix.IsEmpty() || Length() < prefix.Length())
				return false;
			// We know that this WStringView is not empty, and therefore Get() below is valid.
			if (searchCase == StringSearchCase::IgnoreCase)
				return StringUtils::CompareIgnoreCase(this->Get(), *prefix, prefix.Length()) == 0;
			return StringUtils::Compare(this->Get(), *prefix, prefix.Length()) == 0;
		}

		template<typename SubClass>
		bool EndsWithBase(const SubClass& suffix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			if (suffix.IsEmpty() || Length() < suffix.Length())
				return false;
			// We know that this WStringView is not empty, and therefore accessing data below is valid.
			if (searchCase == StringSearchCase::IgnoreCase)
				return StringUtils::CompareIgnoreCase(&(*this)[Length() - suffix.Length()], *suffix, suffix.Length()) == 0;
			return StringUtils::Compare(&(*this)[Length() - suffix.Length()], *suffix, suffix.Length()) == 0;
		}
	};

	/// <summary>
	/// Represents static text view as a sequence of UTF-16 characters. Characters sequence might not be null-terminated.
	/// </summary>
	class SE_API_RUNTIME StringView : public StringViewBase<Char>
	{
	public:
		/// <summary>
		/// Instance of the empty string.
		/// </summary>
		static StringView Empty;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="WStringView"/> class.
		/// </summary>
		constexpr StringView() : StringViewBase<Char>()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WStringView"/> class.
		/// </summary>
		/// <param name="str">The reference to the string.</param>
		StringView(const String& str);

		/// <summary>
		/// Initializes a new instance of the <see cref="WStringView"/> class.
		/// </summary>
		/// <param name="str">The reference to the static string.</param>
		constexpr StringView(const StringView& str) : StringViewBase<Char>(str.m_data, str.m_length)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WStringView"/> class.
		/// </summary>
		/// <param name="str">The characters sequence. If null, constructed WStringView will be empty.</param>
		StringView(const Char* str)
		{
			m_data = str;
			m_length = StringUtils::Length(str);
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WStringView"/> class.
		/// </summary>
		/// <param name="str">The characters sequence. Can be null if length is zero.</param>
		/// <param name="length">The characters sequence length (excluding null-terminator character).</param>
		constexpr StringView(const Char* str, int32 length)
			: StringViewBase<Char>(str, length)
		{
		}

	public:
		/// <summary>
		/// Assigns the static string.
		/// </summary>
		/// <param name="str">The other string.</param>
		/// <returns>The reference to this object.</returns>
		inline StringView& operator=(const Char* str)
		{
			m_data = str;
			m_length = StringUtils::Length(str);
			return *this;
		}

		/// <summary>
		/// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
		inline bool operator==(const StringView& other) const
		{
			return m_length == other.m_length
				&& (m_length == 0 || StringUtils::Compare(m_data, other.m_data, m_length) == 0);
		}

		/// <summary>
		/// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
		inline bool operator!=(const StringView& other) const
		{
			return !(*this == other);
		}

		/// <summary>
		/// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
		inline bool operator==(const Char* other) const
		{
			return *this == StringView(other);
		}

		/// <summary>
		/// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
		inline bool operator!=(const Char* other) const
		{
			return !(*this == StringView(other));
		}

		/// <summary>
		/// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
		bool operator==(const String& other) const;

		/// <summary>
		/// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
		bool operator!=(const String& other) const;

	public:
		bool StartsWith(Char c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return StartsWithBase(c, searchCase);
		}

		bool EndsWith(Char c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return EndsWithBase(c, searchCase);
		}

		bool StartsWith(const StringView& prefix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return StartsWithBase(prefix, searchCase);
		}

		bool EndsWith(const StringView& suffix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return EndsWithBase(suffix, searchCase);
		}

		/// <summary>
		/// Gets the left most given number of characters.
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		StringView Left(int32 count) const;

		/// <summary>
		/// Gets the string of characters from the right (end of the string).
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		StringView Right(int32 count) const;

		/// <summary>
		/// Retrieves substring created from characters starting from startIndex to the String end.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <returns>The substring created from String data.</returns>
		StringView Substring(int32 startIndex) const;

		/// <summary>
		/// Retrieves substring created from characters starting from start index.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <param name="count">The amount of characters to retrieve.</param>
		/// <returns>The substring created from String data.</returns>
		StringView Substring(int32 startIndex, int32 count) const;

	public:
		StringAnsi ToStringAnsi() const;
		String ToString() const;
	};

	/// <summary>
	/// Represents static text view as a sequence of ANSI characters. Characters sequence might not be null-terminated.
	/// </summary>
	class SE_API_RUNTIME StringAnsiView : public StringViewBase<char>
	{
	public:
		/// <summary>
		/// Instance of the empty string.
		/// </summary>
		static StringAnsiView Empty;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="StringAnsiView"/> class.
		/// </summary>
		constexpr StringAnsiView()
			: StringViewBase<char>()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="StringAnsiView"/> class.
		/// </summary>
		/// <param name="str">The reference to the string.</param>
		StringAnsiView(const StringAnsi& str);

		/// <summary>
		/// Initializes a new instance of the <see cref="StringAnsiView"/> class.
		/// </summary>
		/// <param name="str">The reference to the static string.</param>
		constexpr StringAnsiView(const StringAnsiView& str)
			: StringViewBase<char>(str.m_data, str.m_length)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="StringView"/> class.
		/// </summary>
		/// <param name="str">The characters sequence.</param>
		StringAnsiView(const char* str)
		{
			m_data = str;
			m_length = StringUtils::Length(str);
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="StringView"/> class.
		/// </summary>
		/// <param name="str">The characters sequence.</param>
		/// <param name="length">The characters sequence length (excluding null-terminator character).</param>
		constexpr StringAnsiView(const char* str, int32 length)
			: StringViewBase<char>(str, length)
		{
		}

	public:
		/// <summary>
		/// Assigns the static string.
		/// </summary>
		/// <param name="str">The other string.</param>
		/// <returns>The reference to this object.</returns>
		inline StringAnsiView& operator=(const char* str)
		{
			m_data = str;
			m_length = StringUtils::Length(str);
			return *this;
		}

		/// <summary>
		/// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
		inline bool operator==(const StringAnsiView& other) const
		{
			return m_length == other.m_length && (m_length == 0 || StringUtils::Compare(m_data, other.m_data, m_length) == 0);
		}

		/// <summary>
		/// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
		inline bool operator!=(const StringAnsiView& other) const
		{
			return !(*this == other);
		}

		/// <summary>
		/// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
		inline bool operator==(const char* other) const
		{
			return *this == StringAnsiView(other);
		}

		/// <summary>
		/// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
		inline bool operator!=(const char* other) const
		{
			return !(*this == StringAnsiView(other));
		}

		/// <summary>
		/// Lexicographically test whether this string is equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically equivalent to the other, otherwise false.</returns>
		bool operator==(const StringAnsi& other) const;

		/// <summary>
		/// Lexicographically test whether this string is not equivalent to the other given string (case sensitive).
		/// </summary>
		/// <param name="other">The other text.</param>
		/// <returns>True if this string is lexicographically is not equivalent to the other, otherwise false.</returns>
		bool operator!=(const StringAnsi& other) const;

	public:
		bool StartsWith(char c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return StartsWithBase(c, searchCase);
		}

		bool EndsWith(char c, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return EndsWithBase(c, searchCase);
		}

		bool StartsWith(const StringAnsiView& prefix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return StartsWithBase(prefix, searchCase);
		}

		bool EndsWith(const StringAnsiView& suffix, StringSearchCase searchCase = StringSearchCase::IgnoreCase) const
		{
			return EndsWithBase(suffix, searchCase);
		}

		/// <summary>
		/// Gets the left most given number of characters.
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		StringAnsiView Left(int32 count) const;

		/// <summary>
		/// Gets the string of characters from the right (end of the string).
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		StringAnsiView Right(int32 count) const;

		/// <summary>
		/// Retrieves substring created from characters starting from startIndex to the String end.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <returns>The substring created from String data.</returns>
		StringAnsi Substring(int32 startIndex) const;

		/// <summary>
		/// Retrieves substring created from characters starting from start index.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <param name="count">The amount of characters to retrieve.</param>
		/// <returns>The substring created from String data.</returns>
		StringAnsi Substring(int32 startIndex, int32 count) const;

	public:
		String ToString() const;
		StringAnsi ToStringAnsi() const;
	};

	inline uint32 GetHash(const StringView& key)
	{
		return StringUtils::GetHashCode(key.Get(), key.Length());
	}

	bool SE_API_RUNTIME operator==(const String& a, const StringView& b);

	bool SE_API_RUNTIME operator!=(const String& a, const StringView& b);

	inline uint32 GetHash(const StringAnsiView& key)
	{
		return StringUtils::GetHashCode(key.Get(), key.Length());
	}

	bool SE_API_RUNTIME operator==(const StringAnsi& a, const StringAnsiView& b);

	bool SE_API_RUNTIME operator!=(const StringAnsi& a, const StringAnsiView& b);
}

namespace fmt
{
	template<>
	struct SE_API_RUNTIME formatter<SE::StringView, SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::StringView& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			if (v.Length() <= 0)
			{
				return ctx.out();
			}
			return fmt::detail::copy_str<SE::Char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

	template<>
	struct SE_API_RUNTIME formatter<SE::StringView, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::StringView& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			if (v.Length() <= 0)
			{
				return ctx.out();
			}
			return fmt::detail::copy_str<char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::StringAnsiView, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::StringAnsiView& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			if (v.Length() <= 0)
			{
				return ctx.out();
			}
			return fmt::detail::copy_str<char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::StringAnsiView, SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::StringAnsiView& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			if (v.Length() <= 0)
			{
				return ctx.out();
			}
			return fmt::detail::copy_str<SE::Char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};
}
