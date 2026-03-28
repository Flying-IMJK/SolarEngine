#pragma once

#include "Core/Types/Strings/StringView.h"
#include "Core/Logging/Logging.h"
#include "Core/Platform/StringUtils.h"
#include "Core/Types/Pair.h"
#include "Core/Types/Collections/List.h"
#include "Core/Utilities/Formatting.h"

namespace SE
{
	#define STRING_OPERATOR(String, Char)											\
		inline friend String operator+(const String& a, const String& b)			\
		{																			\
			return ConcatStrings<const String&, const String&, String>(a, b);		\
		}																			\
		inline friend String operator+(String&& a, const String& b)					\
		{																			\
			return ConcatStrings<String&&, const String&, String>(MoveTemp(a), b);	\
		}																			\
		inline friend String operator+(const String& a, String&& b)					\
		{																			\
			return ConcatStrings<const String&, String&&, String>(a, MoveTemp(b));	\
		}																			\
		inline friend String operator+(String&& a, String&& b)						\
		{																			\
			return ConcatStrings<String&&, String&&, String>(MoveTemp(a), MoveTemp(b));		\
		}																			\
		inline friend String operator+(const Char* a, const String& b)				\
		{																			\
			return ConcatCharsToString<const String&, String>(a, b);				\
		}																			\
		inline friend String operator+(const Char* a, String&& b)					\
		{																			\
			return ConcatCharsToString<String&&, String>(a, MoveTemp(b));			\
		}																			\
		inline friend String operator+(const String& a, const Char* b)				\
		{																			\
			return ConcatStringToChars<const String&, String>(a, b);				\
		}																			\
		inline friend String operator+(String&& a, const Char* b)					\
		{																			\
			return ConcatStringToChars<String&&, String>(MoveTemp(a), b);			\
		}																			\
		inline friend String operator+(const String& a, char b)						\
		{																			\
			return a + String(&b, 1);												\
		}																			\
		inline friend String operator+(String&& a, char b)							\
		{																			\
			return a + String(&b, 1);												\
		}																			\
		inline bool operator<=(const Char* other) const								\
		{																			\
			return StringUtils::Compare(this->GetText(), other) <= 0;				\
		}																			\
		inline bool operator<(const Char* other) const								\
		{																			\
			return StringUtils::Compare(this->GetText(), other) < 0;				\
		}																			\
		inline bool operator<(const String& other) const							\
		{																			\
			return StringUtils::Compare(this->GetText(), other.GetText()) < 0;		\
		}																			\
		inline bool operator>=(const Char* other) const								\
		{																			\
			return StringUtils::Compare(this->GetText(), other) >= 0;				\
		}																			\
		inline bool operator>(const Char* other) const								\
		{																			\
			return StringUtils::Compare(this->GetText(), other) > 0;				\
		}																			\
		inline bool operator>(const String& other) const							\
		{																			\
			return StringUtils::Compare(this->GetText(), other.GetText()) > 0;		\
		}																			\
		inline bool operator==(const Char* other) const								\
		{																			\
			return StringUtils::Compare(this->GetText(), other) == 0;				\
		}																			\
		inline bool operator==(const String& other) const							\
		{																			\
			return StringUtils::Compare(this->GetText(), other.GetText()) == 0;		\
		}																			\
		inline bool operator!=(const Char* other) const								\
		{																			\
			return StringUtils::Compare(this->GetText(), other) != 0;				\
		}																			\
		inline bool operator!=(const String& other) const							\
		{																			\
			return StringUtils::Compare(this->GetText(), other.GetText()) != 0;		\
		}																			\
		inline bool operator<=(const String& other) const							\
		{																			\
			return StringUtils::Compare(this->GetText(), other.GetText()) <= 0;		\
		}																			\
		inline bool operator>=(const String& other) const							\
		{																			\
			return StringUtils::Compare(this->GetText(), other.GetText()) >= 0;		\
		}

	/// <summary>
	/// Represents text as a sequence of characters. Container uses a single dynamic memory allocation to store the characters data. Characters sequence is always null-terminated.
	/// </summary>
	template<typename T>
	class StringBase
	{
	protected:
		T* m_data = nullptr;
		int32 m_length = 0;

		StringBase()
		{
			m_data = static_cast<T*>(Platform::Allocate(1 * sizeof(T), 16));
			m_data[0] = 0;
		}

	public:
		typedef T CharType;
		
		/// <summary>
		/// Finalizes an instance of the <see cref="StringBase"/> class.
		/// </summary>
		virtual ~StringBase()
		{
			if (m_data != nullptr)
			{
				Platform::Free(m_data);
			}
			m_data = nullptr;
			m_length = 0;
		}

	public:
		/// <summary>
		/// sets the string to empty.
		/// </summary>
		void Clear()
		{
			Platform::Free(m_data);
			m_data = static_cast<CharType*>(Platform::Allocate(1 * sizeof(CharType), 16));
			m_data[1] = '\0';
			m_length = 0;
		}

	public:
		/// <summary>
		/// Gets the character at the specific index.
		/// </summary>
		/// <param name="index">The index.</param>
		/// <returns>The character</returns>
		inline T& operator[](int32 index)
		{
			ENGINE_ASSERT(index >= 0 && index < m_length);
			return m_data[index];
		}

		/// <summary>
		/// Gets the character at the specific index.
		/// </summary>
		/// <param name="index">The index.</param>
		/// <returns>The character</returns>
		inline const T& operator[](int32 index) const
		{
			ENGINE_ASSERT(index >= 0 && index < m_length);
			return m_data[index];
		}

	public:
		/// <summary>
		/// Lexicographically tests how this string compares to the other given string.
		/// In case sensitive mode 'A' is less than 'a'.
		/// </summary>
		/// <param name="str">The another string test against.</param>
		/// <param name="searchCase">The case sensitivity mode.</param>
		/// <returns>0 if equal, negative number if less than, positive number if greater than.</returns>
		int32 Compare(const StringBase& str, const StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			if (searchCase == StringSearchCase::CaseSensitive)
			{
				return StringUtils::Compare(this->GetText(), str.GetText());
			}
			return StringUtils::CompareIgnoreCase(this->GetText(), str.GetText());
		}

	public:
		/// <summary>
		/// Returns true if string is empty.
		/// </summary>
		/// <returns>True if string is empty, otherwise false.</returns>
		inline bool IsEmpty() const
		{
			return m_length == 0 || m_data == nullptr;
		}

		/// <summary>
		/// Returns true if string isn't empty.
		/// </summary>
		/// <returns>True if string has characters, otherwise false.</returns>
		inline bool HasChars() const
		{
			return m_length != 0;
		}

		/// <summary>
		/// Gets the length of the string.
		/// </summary>
		/// <returns>The text length.</returns>
		inline int32 Length() const
		{
			return m_length;
		}

		/// <summary>
		/// Gets the pointer to the string (or null if text is empty).
		/// </summary>
		/// <returns>The string handle.</returns>
		inline const T* operator*() const
		{
			return m_data;
		}

		/// <summary>
		/// Gets the pointer to the string (or null if text is empty).
		/// </summary>
		/// <returns>The string handle.</returns>
		inline T* operator*()
		{
			return m_data;
		}

		/// <summary>
		/// Gets the pointer to the string(or null if text is empty).
		/// </summary>
		/// <returns>The string handle.</returns>
		inline T* Get()
		{
			return m_data;
		}

		/// <summary>
		/// Gets the pointer to the string (or null if text is empty).
		/// </summary>
		/// <returns>The string handle.</returns>
		inline const T* Get() const
		{
			return m_data;
		}

		/// <summary>
		/// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always valid (read-only).
		/// </summary>
		/// <returns>The string handle.</returns>
		virtual const T* GetText() = 0;

		virtual const T* GetText() const = 0;

		/// <summary>
		/// Sets an array of characters to the string.
		/// </summary>
		/// <param name="chars">The pointer to the start of an array of characters to set (UTF-16). This array need not be null-terminated, and null characters are not treated specially.</param>
		/// <param name="length">The number of characters to assign.</param>
		void Set(const T* chars, int32 length)
		{
			ENGINE_ASSERT(length >= 0);

			if (length == m_length)
			{
				if (m_data == chars)
					return;
				Platform::MemoryCopy(m_data, chars, length * sizeof(T));
			}
			else
			{
				T* data = nullptr;

				data = static_cast<T*>(Platform::Allocate((length + 1) * sizeof(T), 16));
				Platform::MemoryCopy(data, chars, length * sizeof(T));
				data[length] = 0;

				Platform::Free(m_data);
				m_data = data;
				m_length = length;
			}
		}

	public:
		/// <summary>
		/// Checks whether this string contains the specified substring.
		/// </summary>
		/// <param name="subStr">The string sequence to search for.</param>
		/// <param name="searchCase">The search case sensitivity mode.</param>
		/// <returns>True if the given substring is contained by ths string, otherwise false.</returns>
		inline bool Contains(const T* subStr, const StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			return Find(subStr, -1, searchCase) != INVALID_INDEX;
		}

		/// <summary>
		/// Checks whether this string contains the specified substring.
		/// </summary>
		/// <param name="subStr">The string sequence to search for.</param>
		/// <param name="searchCase">The search case sensitivity mode.</param>
		/// <returns>True if the given substring is contained by ths string, otherwise false.</returns>
		inline bool Contains(const StringBase& subStr,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			return Find(*subStr, -1, searchCase) != INVALID_INDEX;
		}

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
			return INVALID_INDEX;
		}

		/// <summary>
		/// Searches the string for the last occurrence of a character.
		/// </summary>
		/// <param name="c">The character to search for.</param>
		/// <returns>The index of the character position in the string or -1 if not found.</returns>
		int32 FindLast(T c, int32 startPosition = -1) const
		{
			const T* end = Get() + m_length;

			if (startPosition != -1)
			{
				end -= startPosition < Length() ? Length() - startPosition : 0;
			}

			for (const T* data = end, * dataStart = data - m_length; data != dataStart;)
			{
				--data;
				if (*data == c)
				{
					return static_cast<int32>(data - dataStart);
				}
			}
			return INVALID_INDEX;
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
		/// Searches the string starting from beginning for a substring, and returns index into this string of the first found instance.
		/// </summary>
		/// <param name="subStr">The string sequence to search for.</param>
		/// <param name="searchCase">The search case sensitivity mode.</param>
		/// <param name="startPosition">The start character position to search from.</param>
		/// <returns>The index of the found substring or -1 if not found.</returns>
		inline int32 Find(const StringBase& subStr, int32 startPosition = -1,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			return Find(subStr.Get(), startPosition, searchCase);
		}

		/// <summary>
		/// Searches the string starting from end for a substring, and returns index into this string of the first found instance.
		/// </summary>
		/// <param name="subStr">The string sequence to search for.</param>
		/// <param name="searchCase">The search case sensitivity mode.</param>
		/// <param name="startPosition">The start character position to search from.</param>
		/// <returns>The index of the found substring or -1 if not found.</returns>
		inline int32 FindLast(const StringBase& subStr, int32 startPosition = -1,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			return FindLast(subStr.Get(), startPosition, searchCase);
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

		/// <summary>
		/// Reserves space for the characters. Discards the existing contents. Caller is responsible to initialize contents (excluding null-termination character).
		/// </summary>
		/// <param name="length">The amount of characters to reserve space for (excluding null-terminated character).</param>
		void ReserveSpace(int32 length)
		{
			ENGINE_ASSERT(length >= 0);
			if (length == m_length)
			{
				return;
			}
			Platform::Free(m_data);
			if (length != 0)
			{
				m_data = (T*)Platform::Allocate((length + 1) * sizeof(T), 16);
				m_data[length] = 0;
			}
			else
			{
				m_data = nullptr;
			}
			m_length = length;
		}

		void FirstTrim()
		{
			int start = 0;
			int length = Length();

			while (start < length && StringUtils::IsWhitespace(m_data[start]))
			{
				++start;
			}

			if (start > 0)
			{
				int newLength = length - start;
				if (newLength > 0)
				{
					CharType* newData = static_cast<CharType*>(Platform::Allocate((newLength + 1) * sizeof(CharType), 16));
					Platform::MemoryCopy(newData, m_data + start, newLength * sizeof(CharType));
					newData[newLength] = '\0';

					Platform::Free(m_data);
					m_data = newData;
					m_length = newLength;
				}
				else
				{
					Clear();
				}
			}
		}

		void LastTrim()
		{
			int length = Length() - 1;

			// 找到最后一个非空白字符的索引
			while (length >= 0 && StringUtils::IsWhitespace(m_data[length]))
			{
				--length;
			}

			// 新长度应为最后一个非空白字符索引加1
			int newLength = length + 1;

			// 如果有空白字符被移除且新长度小于原始长度
			if (newLength < Length())
			{
				m_data[newLength] = '\0';

				// 无需分配新内存，直接调整长度即可
				m_length = newLength;
			}
		}

	public:
		bool StartsWith(T c, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			const int32 length = Length();
			if (searchCase == StringSearchCase::CaseSensitive)
			{
				return length > 0 && m_data[0] == c;
			}
			return length > 0 && StringUtils::ToLower(m_data[0]) == StringUtils::ToLower(c);
		}

		bool EndsWith(T c, StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			const int32 length = Length();
			if (searchCase == StringSearchCase::CaseSensitive)
			{
				return length > 0 && m_data[length - 1] == c;
			}
			return length > 0 && StringUtils::ToLower(m_data[length - 1]) == StringUtils::ToLower(c);
		}

		bool StartsWith(const StringBase& prefix, const StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			if (prefix.IsEmpty())
			{
				return true;
			}
			if (Length() < prefix.Length())
			{
				return false;
			}
			if (searchCase == StringSearchCase::IgnoreCase)
			{
				return StringUtils::CompareIgnoreCase(this->GetText(), *prefix, prefix.Length()) == 0;
			}
			return StringUtils::Compare(this->GetText(), *prefix, prefix.Length()) == 0;
		}

		bool EndsWith(const StringBase& suffix, const StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			if (suffix.IsEmpty())
			{
				return true;
			}
			if (Length() < suffix.Length())
			{
				return false;
			}
			if (searchCase == StringSearchCase::IgnoreCase)
			{
				return StringUtils::CompareIgnoreCase(&(*this)[Length() - suffix.Length()], *suffix) == 0;
			}
			return StringUtils::Compare(&(*this)[Length() - suffix.Length()], *suffix) == 0;
		}

		bool StartsWith(const T* prefix, const StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			int prefixLength = StringUtils::Length(prefix);
			if (prefix == nullptr)
			{
				return true;
			}
			if (Length() < prefixLength)
			{
				return false;
			}
			if (searchCase == StringSearchCase::IgnoreCase)
			{
				return StringUtils::CompareIgnoreCase(this->GetText(), prefix, prefixLength) == 0;
			}
			return StringUtils::Compare(this->GetText(), prefix, prefixLength) == 0;
		}

		bool EndsWith(const T* suffix, const StringSearchCase searchCase = StringSearchCase::CaseSensitive) const
		{
			int suffixLength = StringUtils::Length(suffix);
			if (suffix == nullptr)
			{
				return true;
			}
			if (Length() < suffixLength)
			{
				return false;
			}
			if (searchCase == StringSearchCase::IgnoreCase)
			{
				return StringUtils::CompareIgnoreCase(&(*this)[Length() - suffixLength], suffix) == 0;
			}
			return StringUtils::Compare(&(*this)[Length() - suffixLength], suffix) == 0;
		}

		int32 Replace(T searchChar, T replacementChar, const StringSearchCase searchCase = StringSearchCase::CaseSensitive)
		{
			int32 replacedChars = 0;
			int32 i;
			const int32 length = Length();
			if (searchCase == StringSearchCase::IgnoreCase)
			{
				const T toCompare = StringUtils::ToLower(searchChar);
				for (i = 0; i < length; i++)
				{
					if (StringUtils::ToLower(m_data[i]) == toCompare)
					{
						m_data[i] = replacementChar;
						replacedChars++;
					}
				}
			}
			else
			{
				for (i = 0; i < length; i++)
				{
					if (m_data[i] == searchChar)
					{
						m_data[i] = replacementChar;
						replacedChars++;
					}
				}
			}
			return replacedChars;
		}

		/// <summary>
		/// Replaces all occurences of searchText within current string with replacementText.
		/// </summary>
		/// <param name="searchText">String to search for. If empty or null no replacements are done.</param>
		/// <param name="replacementText">String to replace with. Null is treated as empty string.</param>
		/// <returns>Number of replacements made. (In case-sensitive mode if search text and replacement text are equal no replacements are done, and zero is returned.)</returns>
		int32 Replace(const T* searchText, const T* replacementText,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive)
		{
			const int32 searchTextLength = StringUtils::Length(searchText);
			const int32 replacementTextLength = StringUtils::Length(replacementText);
			return Replace(searchText, searchTextLength, replacementText, replacementTextLength, searchCase);
		}

		/// <summary>
		/// Replaces all occurences of searchText within current string with replacementText.
		/// </summary>
		/// <param name="searchText">String to search for.</param>
		/// <param name="searchTextLength">Length of searchText. Must be greater than zero.</param>
		/// <param name="replacementText">String to replace with. Null is treated as empty string.</param>
		/// <param name="replacementTextLength">Length of replacementText.</param>
		/// <returns>Number of replacements made (in other words number of occurences of searchText).</returns>
		int32 Replace(const T* searchText,
			int32 searchTextLength,
			const T* replacementText,
			int32 replacementTextLength,
			StringSearchCase searchCase = StringSearchCase::CaseSensitive)
		{
//			if (!HasChars() || searchTextLength == 0)
//			{
//				return 0;
//			}

			int32 replacedCount = 0;

			if (searchTextLength == replacementTextLength)
			{
				T* pos = (T*)(searchCase == StringSearchCase::IgnoreCase ?
							  StringUtils::FindIgnoreCase(m_data, searchText) :
							  StringUtils::Find(m_data, searchText));

				while (pos != nullptr)
				{
					replacedCount++;

					for (int32 i = 0; i < replacementTextLength; i++)
					{
						pos[i] = replacementText[i];
					}

					if (pos + searchTextLength - **this < Length())
					{
						pos = (T*)(searchCase == StringSearchCase::IgnoreCase ?
								   StringUtils::FindIgnoreCase(pos + searchTextLength, searchText) :
								   StringUtils::Find(pos + searchTextLength, searchText));
					}
					else
					{
						break;
					}
				}
			}
			else if (Contains(searchText, searchCase))
			{
				T* readPosition = m_data;
				T* searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ?
										 StringUtils::FindIgnoreCase(readPosition, searchText) :
										 StringUtils::Find(readPosition, searchText));
				while (searchPosition != nullptr)
				{
					replacedCount++;
					readPosition = searchPosition + searchTextLength;
					searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ?
										  StringUtils::FindIgnoreCase(readPosition, searchText) :
										  StringUtils::Find(readPosition, searchText));
				}

				const auto oldLength = m_length;
				const auto oldData = m_data;
				m_length += replacedCount * (replacementTextLength - searchTextLength);
				m_data = (T*)Platform::Allocate((m_length + 1) * sizeof(T), 16);

				T* writePosition = m_data;
				readPosition = oldData;
				searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ?
									  StringUtils::FindIgnoreCase(readPosition, searchText) :
									  StringUtils::Find(readPosition, searchText));

				while (searchPosition != nullptr)
				{
					const int32 writeOffset = (int32)(searchPosition - readPosition);
					Platform::MemoryCopy(writePosition, readPosition, writeOffset * sizeof(T));
					writePosition += writeOffset;

					if (replacementTextLength > 0)
					{
						Platform::MemoryCopy(writePosition, replacementText, replacementTextLength * sizeof(T));
					}
					writePosition += replacementTextLength;

					readPosition = searchPosition + searchTextLength;
					searchPosition = (T*)(searchCase == StringSearchCase::IgnoreCase ?
										  StringUtils::FindIgnoreCase(readPosition, searchText) :
										  StringUtils::Find(readPosition, searchText));
				}

				const int32 writeOffset = (int32)(oldData - readPosition) + oldLength;
				Platform::MemoryCopy(writePosition, readPosition, writeOffset * sizeof(T));

				m_data[m_length] = 0;
				Platform::Free(oldData);
			}

			return replacedCount;
		}

		/***
		 * 使用指定字符串在指定位置一定长度的字符串
		 * @param startIndex 替换开始位置
		 * @param replaceLength 替换字符串长度
		 * @param replacementText 替换字符串
		 * @param replacementTextLength 替换字符串长度
		 * @return
		 */
		bool Replace(int startIndex,
			int32 replaceLength,
			const T* replacementText,
			int32 replacementTextLength)
		{
			if (!HasChars() || startIndex < 0 || startIndex + replaceLength - 1 >= m_length || replacementTextLength <= 0)
			{
				return false;
			}

			if (replaceLength == replacementTextLength)
			{
				T* pos = &m_data[startIndex];

				Platform::MemoryCopy(pos, replacementText, replacementTextLength * sizeof(T));
			}
			else
			{
				T* replacePosition = &m_data[startIndex];
				T* readPosition = replacePosition + replaceLength;

				const auto oldLength = m_length;
				const auto oldData = m_data;
				m_length += (replacementTextLength - replaceLength);
				m_data = (T*)(Platform::Allocate((m_length + 1) * sizeof(T), 16));

				// 拷贝替换之前
				int32 writeOffset = (int32)(replacePosition - oldData);
				Platform::MemoryCopy(m_data, oldData, writeOffset * sizeof(T));

				// 拷贝替换内容
				replacePosition = &m_data[startIndex];
				Platform::MemoryCopy(replacePosition, replacementText, replacementTextLength * sizeof(T));

				// 拷贝替换内容之后
				writeOffset = (oldData - readPosition);
				T* writePosition = replacePosition + replacementTextLength;
				Platform::MemoryCopy(writePosition, readPosition, writeOffset * sizeof(T));

				m_data[m_length] = 0;
				Platform::Free(oldData);
			}

			return true;
		}

		/// <summary>
		/// Inserts string into current string instance at given location.
		/// </summary>
		/// <param name="startIndex">The index of the first character to insert.</param>
		/// <param name="other">The string to insert.</param>
		/// <param name="otherLength">The string length.</param>
		void InsertBase(int32 startIndex, const T* other, const int otherLength)
		{
			ENGINE_ASSERT(other != m_data);
			const int32 myLength = Length();
			ENGINE_ASSERT(startIndex >= 0 && startIndex <= m_length);

			if (otherLength == 0)
				return;

			if (myLength == 0)
			{
				Set(other, otherLength);
				return;
			}

			const auto oldData = m_data;
			const auto oldLength = m_length;

			m_length = oldLength + otherLength;
			m_data = static_cast<T*>(Platform::Allocate((m_length + 1) * sizeof(T), 16));

			Platform::MemoryCopy(m_data, oldData, startIndex * sizeof(T));
			Platform::MemoryCopy(m_data + startIndex, other, otherLength * sizeof(T));
			Platform::MemoryCopy(m_data + startIndex + otherLength, oldData + startIndex, (oldLength - startIndex) * sizeof(T));
			m_data[m_length] = 0;

			Platform::Free(oldData);
		}

		void Insert(int32 startIndex, const T* other)
		{
			InsertBase(startIndex, other, StringUtils::Length(other));
		}

		template<typename ChildClass>
		void Insert(int32 startIndex, const ChildClass& other)
		{
			InsertBase(startIndex, other.Get(), other.Length());
		}

		/// <summary>
		/// Reverses the string.
		/// </summary>
		void Reverse()
		{
			T c;
			int32 tmp, count = m_length, end = count / 2;
			for (int32 i = 0; i < end; i++)
			{
				tmp = count - i - 2;
				c = m_data[i];
				m_data[i] = m_data[tmp];
				m_data[tmp] = c;
			}
		}

		/// <summary>
		/// Resizes string contents.
		/// </summary>
		/// <param name="length">New length of the string.</param>
		void Resize(int32 length)
		{
			ENGINE_ASSERT(length >= 0);
			if (m_length != length)
			{
				const auto oldData = m_data;
				const auto minLength = m_length < length ? m_length : length;
				m_length = length;
				m_data = (T*)Platform::Allocate((length + 1) * sizeof(T), 16);
				Platform::MemoryCopy(m_data, oldData, minLength * sizeof(T));
				m_data[length] = 0;
				Platform::Free(oldData);
			}
		}

		/// <summary>
		/// Removes characters from the string at given location until the end.
		/// </summary>
		/// <param name="startIndex">The index of the first character to remove.</param>
		void Remove(int32 startIndex)
		{
			Remove(startIndex, m_length - startIndex);
		}

		/// <summary>
		/// Removes characters from the string at given location and length.
		/// </summary>
		/// <param name="startIndex">The index of the first character to remove.</param>
		/// <param name="length">The amount of characters to remove.</param>
		void Remove(int32 startIndex, int32 length)
		{
			const auto oldData = m_data;
			const auto oldLength = m_length;
			ENGINE_ASSERT(startIndex >= 0 && startIndex + length <= oldLength);

			if (startIndex == 0 && oldLength == length)
			{
				Clear();
				return;
			}

			m_length = oldLength - length;
			m_data = static_cast<T*>(Platform::Allocate((m_length + 1) * sizeof(T), 16));

			Platform::MemoryCopy(m_data, oldData, startIndex * sizeof(T));
			Platform::MemoryCopy(m_data + startIndex, oldData + startIndex + length, (m_length - startIndex) * sizeof(T));
			m_data[m_length] = 0;

			Platform::Free(oldData);
		}

	protected:

		template<typename D1, typename D2, class ChildClass>
		static ChildClass ConcatStrings(D1 left, D2 right)
		{
			if (left.IsEmpty())
				return MoveTemp(right);
			if (right.IsEmpty())
				return MoveTemp(left);

			const T* leftStr = left.Get();
			const int32 leftLen = left.Length();
			const T* rightStr = right.Get();
			const int32 rightLen = right.Length();

			ChildClass result;
			result.ReserveSpace(leftLen + rightLen);
			Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(T));
			Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(T));

			return result;
		}

		template<typename D, class ChildClass>
		static ChildClass ConcatCharsToString(const T* left, D right)
		{
			if (!left || !*left)
				return MoveTemp(right);

			const T* leftStr = left;
			const int32 leftLen = StringUtils::Length(left);
			const T* rightStr = right.Get();
			const int32 rightLen = right.Length();

			ChildClass result;
			result.ReserveSpace(leftLen + rightLen);
			Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(T));
			Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(T));

			return result;
		}

		template<typename D, class ChildClass>
		static ChildClass ConcatStringToChars(D left, const T* right)
		{
			if (!right || !*right)
				return MoveTemp(left);

			const T* leftStr = left.Get();
			const int32 leftLen = left.Length();
			const T* rightStr = right;
			const int32 rightLen = StringUtils::Length(right);

			ChildClass result;
			result.ReserveSpace(leftLen + rightLen);
			Platform::MemoryCopy(result.Get(), leftStr, leftLen * sizeof(T));
			Platform::MemoryCopy(result.Get() + leftLen, rightStr, rightLen * sizeof(T));

			return result;
		}

		/// <summary>
		/// Converts all uppercase characters to lowercase.
		/// </summary>
		/// <returns>The lowercase string.</returns>
		template<class ChildClass>
		ChildClass& ToLowerBase()
		{
			for (int32 i = 0; i < m_length; i++)
			{
				m_data[i] = StringUtils::ToLower(m_data[i]);
			}
			return *static_cast<ChildClass*>(this);;
		}

		/// <summary>
		/// Converts all lowercase characters to uppercase.
		/// </summary>
		/// <returns>The uppercase string.</returns>
		template<class ChildClass>
		ChildClass& ToUpperBase()
		{
			for (int32 i = 0; i < m_length; i++)
			{
				m_data[i] = StringUtils::ToUpper(m_data[i]);
			}

			return *static_cast<ChildClass*>(this);;
		}

		/// <summary>
		/// Appends an array of characters to the string.
		/// </summary>
		/// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
		/// <param name="count">The number of characters to append.</param>
		template<class ChildClass>
		ChildClass& AppendBase(const T* chars, const int32 count)
		{
			if (count == 0)
				return *static_cast<ChildClass*>(this);

			const auto oldData = m_data;
			const auto oldLength = m_length;

			m_length = oldLength + count;
			m_data = static_cast<T*>(Platform::Allocate((m_length + 1) * sizeof(T), 16));

			Platform::MemoryCopy(m_data, oldData, oldLength * sizeof(T));
			Platform::MemoryCopy(m_data + oldLength, chars, count * sizeof(T));
			m_data[m_length] = 0;

			Platform::Free(oldData);
			return *static_cast<ChildClass*>(this);
		}

		/// <summary>
		/// Gets the left most given number of characters.
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		template<class ChildClass>
		ChildClass LeftBase(int32 count) const
		{
			const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
			return ChildClass(m_data, countClamped);
		}

		/// <summary>
		/// Gets the string of characters from the right (end of the string).
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		template<class ChildClass>
		ChildClass RightBase(int32 count) const
		{
			const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
			return ChildClass(m_data + Length() - countClamped);
		}

		/// <summary>
		/// Retrieves substring created from characters starting from startIndex to the WString end.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <returns>The substring created from WString data.</returns>
		template<class ChildClass>
		ChildClass SubstringBase(int32 startIndex) const
		{
			ENGINE_ASSERT(startIndex >= 0 && startIndex < Length());
			return ChildClass(m_data + startIndex, m_length - startIndex);
		}

		/// <summary>
		/// Retrieves substring created from characters starting from start index.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <param name="count">The amount of characters to retrieve.</param>
		/// <returns>The substring created from WString data.</returns>
		template<class ChildClass>
		ChildClass SubstringBase(int32 startIndex, int32 count) const
		{
			ENGINE_ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
			return ChildClass(m_data + startIndex, count);
		}


		template<class ChildClass>
		void SplitBase(const T c, List<ChildClass>& results) const
		{
			results.Clear();
			int32 start = 0;
			int32 length = Length();

			for (int32 i = 0; i < length; i++)
			{
				if (m_data[i] == c)
				{
					int32 count = i - start;
					if (count > 0)
					{
						results.Add(SubstringBase<ChildClass>(start, count));
					}
					start = i + 1;
				}
			}

			const int32 count = length - start;
			if (count > 0)
			{
				results.Add(SubstringBase<ChildClass>(start, count));
			}
		}

		template<class ChildClass>
		void SplitBase(const T cArray[], List<ChildClass>& results) const
		{
			results.Clear();
			int cArrayLength = ARRAY_SIZE(cArray);

			if (cArrayLength <= 0)
			{
				return;
			}

			int32 start = 0;
			int32 length = Length();

			for (int32 i = 0; i < length; i++)
			{
				bool found = false;
				for (int cIndex = 0; cIndex < cArrayLength; cIndex++)
				{
					if (m_data[i] == cArray[cIndex])
					{
						found = true;
					}
				}

				if (found)
				{
					int32 count = i - start;
					if (count > 0)
					{
						results.Add(SubstringBase<ChildClass>(start, count));
					}
					start = i + 1;
				}
			}

			const int32 count = length - start;
			if (count > 0)
			{
				results.Add(SubstringBase<ChildClass>(start, count));
			}
		}

		template<class ChildClass>
		void SplitBase(std::initializer_list<T> cArray, List<ChildClass>& results) const
		{
			results.Clear();
			int32 start = 0;
			int32 length = Length();

			for (int32 i = 0; i < length; i++)
			{
				bool found = false;
				for (T c : cArray)
				{
					if (m_data[i] == c)
					{
						found = true;
					}
				}

				if (found)
				{
					int32 count = i - start;
					if (count > 0)
					{
						results.Add(SubstringBase<ChildClass>(start, count));
					}
					start = i + 1;
				}
			}

			const int32 count = length - start;
			if (count > 0)
			{
				results.Add(SubstringBase<ChildClass>(start, count));
			}
		}

		template<class ChildClass>
		void SplitBase(const T* c, const int delimiterLength, List<ChildClass>& results) const
		{
			results.Clear(); // 清除结果列表
			int32 start = 0; // 分割的起始位置
			int32 length = Length(); // 原始字符串长度

			if(delimiterLength == 0)
			{
				results.Add(*static_cast<const ChildClass*>(this)); // 如果分隔符为空，则整个字符串作为一个结果返回
				return;
			}

			// 查找分隔符出现的位置
			for (int32 i = 0; i <= length - delimiterLength; ) // 注意循环条件中的减法
			{
				// 对比每个分隔符字符
				bool found = true;
				for(int32 j = 0; j < length; ++j)
				{
					if(m_data[start + i + j] != c[j])
					{
						found = false;
						break;
					}
				}

				// 如果找到了分隔符，提取前面的子串作为结果之一
				if(found)
				{
					int32 count = i - start;
					if(count > 0)
					{
						results.Add(SubstringBase<ChildClass>(start, count));
					}
					i += delimiterLength; // 跳过分隔符
					start = i; // 更新下一段的开始位置
				}
				else
				{
					++i; // 如果当前位置不匹配，继续向后查找
				}
			}

			// 检查最后一个分隔符后是否还有剩余的字符串
			if(start < length)
			{
				results.Add(SubstringBase<ChildClass>(start, length - start));
			}
		}

		template<class ChildClass>
		void SplitBase(const T* cArray[], List<ChildClass>& results) const
		{
			results.Clear(); // 清除结果列表

			int cArrayLength = ARRAY_SIZE(cArray);

			if(cArrayLength == 0)
			{
				results.Add(*static_cast<const ChildClass*>(this)); // 如果分隔符为空，则整个字符串作为一个结果返回
				return;
			}

			int start = 0; // 分割的起始位置
			int length = Length(); // 原始字符串长度


			int minLengthC = Max_int32;
			int* cLength = NewArray<int>(cArrayLength);
			for(int j = 0; j < cArrayLength; ++j)
			{
				cLength[j] = StringUtils::Length(cArray[j]);
				if (cLength[j] < minLengthC)
				{
					minLengthC = cLength[j];
				}
			}

			// 查找分隔符出现的位置
			for (int32 i = 0; i <= length - minLengthC;)
			{
				bool found = true;
				int foundIndex = -1;
				for (int cIndex = 0; cIndex < cArrayLength; ++cIndex)
				{
					found = true;
					// 对比每个分隔符字符
					for(int32 j = 0; j < cLength[cIndex]; ++j)
					{
						if (start + i + j >= length)
						{
							found = false;
							break;
						}

						if(m_data[start + i + j] != cArray[cIndex][j])
						{
							found = false;
							break;
						}
					}

					if (found)
					{
						foundIndex = cIndex;
					}
				}

				// 如果找到了分隔符，提取前面的子串作为结果之一
				if(found)
				{
					int32 count = i - start;
					if(count > 0)
					{
						results.Add(SubstringBase<ChildClass>(start, count));
					}
					// 跳过分隔符
					i += cLength[foundIndex];
					start = i; // 更新下一段的开始位置
				}
				else
				{
					++i; // 如果当前位置不匹配，继续向后查找
				}
			}

			// 检查最后一个分隔符后是否还有剩余的字符串
			if(start < length)
			{
				results.Add(SubstringBase<ChildClass>(start, length - start));
			}
		}

		template<class ChildClass>
		void SplitBase(std::initializer_list<const T*> cArray, List<ChildClass>& results) const
		{
			results.Clear(); // 清除结果列表
			int start = 0; // 分割的起始位置
			int length = Length(); // 原始字符串长度

			if(cArray.size() == 0)
			{
				results.Add(*static_cast<const ChildClass*>(this)); // 如果分隔符为空，则整个字符串作为一个结果返回
				return;
			}

			int minLengthC = Max_int32;
			int* cLength = NewArray<int>(cArray.size());
			int cIndex = 0;
			for(auto cStart = cArray.begin(); cStart < cArray.end(); cStart++, cIndex++)
			{
				cLength[cIndex] = StringUtils::Length(static_cast<const T*>(*cStart));
				if (cLength[cIndex] < minLengthC)
				{
					minLengthC = cLength[cIndex];
				}
			}

			// 查找分隔符出现的位置
			for (int i = 0; i <= length - minLengthC;)
			{
				bool found = true;
				int foundIndex = -1;
				cIndex = 0;
				for(auto cStart = cArray.begin(); cStart < cArray.end(); cStart++, cIndex++)
				{
					found = true;
					// 对比每个分隔符字符
					for(int32 j = 0; j < cLength[cIndex]; ++j)
					{
						if (start + i + j >= length)
						{
							found = false;
							break;
						}

						if(m_data[start + i + j] != *cStart[j])
						{
							found = false;
							break;
						}
					}

					if (found)
					{
						foundIndex = cIndex;
					}
				}

				// 如果找到了分隔符，提取前面的子串作为结果之一
				if(found)
				{
					int32 count = i - start;
					if(count > 0)
					{
						results.Add(SubstringBase<ChildClass>(start, count));
					}
					// 跳过分隔符
					i += cLength[foundIndex];
					start = i; // 更新下一段的开始位置
				}
				else
				{
					++i; // 如果当前位置不匹配，继续向后查找
				}
			}

			// 检查最后一个分隔符后是否还有剩余的字符串
			if(start < length)
			{
				results.Add(SubstringBase<ChildClass>(start, length - start));
			}
		}

		/// <summary>
		/// Gets the first line of the text (searches for the line terminator char).
		/// </summary>
		/// <returns>The single line of text.</returns>
		template<class ChildClass>
		ChildClass GetFirstLineBase()
		{
			const int32 lineTerminatorIndex = Find(SE_TEXT('\n'));
			return lineTerminatorIndex == -1 ? *static_cast<ChildClass*>(this) : LeftBase<ChildClass>(lineTerminatorIndex);
		}

		/// <summary>
		/// Trims the string to the first null terminator character in the characters buffer.
		/// </summary>
		template<class ChildClass>
		ChildClass& TrimToNullTerminatorBase()
		{
			const int32 length = m_length;
			const int32 newLength = StringUtils::Length(m_data);
			if (length != 0 && length != newLength)
			{
				Resize(newLength);
			}
			return *static_cast<ChildClass*>(this);
		}

		/// <summary>
		/// Removes trailing whitespace characters from end and begin of the string.
		/// </summary>
		template<class ChildClass>
		ChildClass& TrimTrailingBase()
		{
			if (IsEmpty())
			{
				return *static_cast<ChildClass*>(this);
			}

			int32 start = 0;
			int32 end = Length() - 1;

			while (start <= end)
			{
				if (!StringUtils::IsWhitespace((*this)[start]))
				{
					break;
				}
				start++;
			}
			while (end >= 0)
			{
				if (!StringUtils::IsWhitespace((*this)[end]))
				{
					break;
				}
				end--;
			}

			const int32 count = end - start + 1;
			if (start >= 0 && start + count < Length() && count > 0 && count < Length())
			{
				T* newTemp = static_cast<T*>(PlatformAllocator::Allocate(sizeof(T) * count, 16));
				Memory::CopyItems(newTemp, m_data + start, count);
				PlatformAllocator::Free(m_data);
				m_data = newTemp;
				m_length = count;
			}

			return *static_cast<ChildClass*>(this);
		}
	};

	/// <summary>
	/// Represents text as a sequence of UTF-16 characters. Container uses a single dynamic memory allocation to store the characters data. Characters sequence is always null-terminated.
	/// </summary>
	class SE_API_CORE String : public StringBase<Char>
	{
	public:
		/// <summary>
		/// Instance of the empty string.
		/// </summary>
		static String Empty;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		String()
		{

		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The UTF-16 string.</param>
		String(const Char* str)
		{
			Set(str, StringUtils::Length(str));
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The UTF-16 string.</param>
		/// <param name="length">The UTF-16 string length.</param>
		String(const Char* str, const int32 length)
		{
			Set(str, length);
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The reference to the string.</param>
		String(const String& str): String()
		{
			Set(str.Get(), str.Length());
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The double reference to the string.</param>
		String(String&& str) noexcept
		{
			m_data = str.m_data;
			m_length = str.m_length;
			str.m_data = nullptr;
			str.m_length = 0;
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The reference to the string.</param>
		explicit String(const StringAnsi& str);

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">ANSI string</param>
		explicit String(const char* str) : String(str, StringUtils::Length(str))
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The ANSI string.</param>
		/// <param name="length">The ANSI string length.</param>
		explicit String(const char* str, int32 length) : String()
		{
			SetChars(str, length);
		}
		

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The other string.</param>
		String(const StringView& str);

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The other string.</param>
		explicit String(const StringAnsiView& str);

	public:
		/// <summary>
		/// Sets an array of characters to the string.
		/// </summary>
		/// <param name="chars">The pointer to the start of an array of characters to set (ANSI). This array need not be null-terminated, and null characters are not treated specially.</param>
		/// <param name="length">The number of characters to assign.</param>
		void SetChars(const char* chars, int32 length);

		/// <summary>
		/// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always valid (read-only).
		/// </summary>
		/// <returns>The string handle.</returns>
		const Char* GetText() override
		{
			return m_data ? m_data : SE_TEXT("");
		}

		const Char* GetText() const override
		{
			return m_data ? m_data : SE_TEXT("");
		}

		/// <summary>
		/// Sets an array of characters to the string.
		/// </summary>
		/// <param name="chars">The pointer to the start of an array of characters to set (UTF-8). This array need not be null-terminated, and null characters are not treated specially.</param>
		/// <param name="length">The number of characters to assign.</param>
		void SetUTF8(const char* chars, int32 length);

		/// <summary>
		/// Appends an array of characters to the string.
		/// </summary>
		/// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
		/// <param name="count">The number of characters to append.</param>
		String& Append(const char* chars, int32 count);

		String& Append(const char* chars)
		{
			return Append(chars, StringUtils::Length(chars));
		}

		/// <summary>
		/// Appends an array of characters to the string.
		/// </summary>
		/// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
		/// <param name="count">The number of characters to append.</param>
		String& Append(const Char* chars, const int32 count)
		{
			return AppendBase<String>(chars, count);
		}

		String& Append(const Char* chars)
		{
			return AppendBase<String>(chars, StringUtils::Length(chars));
		}

		String& Append(const Char c)
		{
			Append(&c, 1);
			return *this;
		}

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="text">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		String& Append(const String& text)
		{
			AppendBase<String>(text.Get(), text.Length());
			return *this;
		}

		/// <summary>
		/// Gets the left most given number of characters.
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		String Left(int32 count) const
		{
			return LeftBase<String>(count);
		}

		/// <summary>
		/// Gets the string of characters from the right (end of the string).
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		String Right(int32 count) const
		{
			return RightBase<String>(count);
		}

		/// <summary>
		/// Retrieves substring created from characters starting from startIndex to the WString end.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <returns>The substring created from WString data.</returns>
		String Substring(int32 startIndex) const
		{
			return SubstringBase<String>(startIndex);
		}

		/// <summary>
		/// Retrieves substring created from characters starting from start index.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <param name="count">The amount of characters to retrieve.</param>
		/// <returns>The substring created from WString data.</returns>
		String Substring(int32 startIndex, int32 count) const
		{
			return SubstringBase<String>(startIndex, count);
		}

		void Split(const Char c, List<String>& results) const
		{
			SplitBase<String>(c, results);
		}

		void Split(const Char cArray[], List<String>& results) const
		{
			SplitBase<String>(cArray, results);
		}

		void Split(std::initializer_list<Char> cArray, List<String>& results) const
		{
			SplitBase<String>(cArray, results);
		}

		void Split(const Char* c, const int delimiterLength, List<String>& results) const
		{
			SplitBase<String>(c, delimiterLength, results);
		}

		void Split(const Char* cArray[], List<String>& results) const
		{
			SplitBase<String>(cArray, results);
		}

		void Split(std::initializer_list<const Char*> cArray, List<String>& results) const
		{
			SplitBase<String>(cArray, results);
		}

		/// <summary>
		/// Gets the first line of the text (searches for the line terminator char).
		/// </summary>
		/// <returns>The single line of text.</returns>
		String GetFirstLine()
		{
			return GetFirstLineBase<String>();
		}

		/// <summary>
		/// Converts all uppercase characters to lowercase.
		/// </summary>
		/// <returns>The lowercase string.</returns>
		String& ToLower()
		{
			return ToLowerBase<String>();
		}

		/// <summary>
		/// Converts all lowercase characters to uppercase.
		/// </summary>
		/// <returns>The uppercase string.</returns>
		String& ToUpper()
		{
			return ToUpperBase<String>();
		}

		/// <summary>
		/// Trims the string to the first null terminator character in the characters buffer.
		/// </summary>
		String& TrimToNullTerminator()
		{
			return TrimToNullTerminatorBase<String>();
		}

		/// <summary>
		/// Removes trailing whitespace characters from end and begin of the string.
		/// </summary>
		String& TrimTrailing()
		{
			return TrimTrailingBase<String>();
		}

	public:
		STRING_OPERATOR(String, Char)

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		inline String& operator+=(const Char* str)
		{
			AppendBase<String>(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		inline String& operator+=(const char* str)
		{
			Append(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Appends the specified character to this string.
		/// </summary>
		/// <param name="c">The character to append.</param>
		/// <returns>The reference to this string.</returns>
		String& operator+=(const Char c);

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		inline String& operator+=(const String& str)
		{
			AppendBase<String>(str.Get(), str.Length());
			return *this;
		}

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		String& operator+=(const StringView& str);

		/// <summary>
		/// Concatenates a string with a character.
		/// </summary>
		/// <param name="a">The left string.</param>
		/// <param name="b">The right character.</param>
		/// <returns>The concatenated string.</returns>
		friend String operator+(const String& a, const Char b)
		{
			String result;
			result.m_length = a.Length() + 1;
			result.m_data = (Char*)Platform::Allocate((result.m_length + 1) * sizeof(Char), 16);
			Platform::MemoryCopy(result.m_data, a.Get(), a.Length() * sizeof(Char));
			result.m_data[a.Length()] = b;
			result.m_data[result.m_length] = 0;
			return result;
		}

	public:
		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="s">The other string.</param>
		/// <returns>The reference to this.</returns>
		inline String& operator=(String&& s) noexcept
		{
			if (this != &s)
			{
				Platform::Free(m_data);
				m_data = s.m_data;
				m_length = s.m_length;
				s.m_data = nullptr;
				s.m_length = 0;
			}
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="s">The other string.</param>
		/// <returns>The reference to this.</returns>
		inline String& operator=(const String& s)
		{
			if (this != &s)
				Set(s.Get(), s.Length());
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="s">The other string.</param>
		/// <returns>The reference to this.</returns>
		String& operator=(const StringView& s);

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="str">The other string.</param>
		/// <returns>The reference to this.</returns>
		String& operator=(const Char* str)
		{
			if (m_data != str)
				Set(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="str">The other string.</param>
		/// <returns>The reference to this.</returns>
		String& operator=(const char* str)
		{
			SetChars(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="c">The other character.</param>
		/// <returns>The reference to this.</returns>
		String& operator=(const Char c)
		{
			Set(&c, 1);
			return *this;
		}
	public:
		/// <summary>
		/// Checks if string contains only ANSI characters.
		/// </summary>
		/// <returns>True if contains only ANSI characters, otherwise false.</returns>
		bool IsANSI() const;

	public:
		/// <summary>
		/// Formats the message and gets it as a string.
		/// </summary>
		/// <param name="format">The format string.</param>
		/// <param name="args">The custom arguments.</param>
		/// <returns>The formatted text.</returns>
		template<typename... Args>
		static String Format(const Char* format, const Args& ... args)
		{
			fmtExtend::Allocator allocator;
			fmtExtend::memory_buffer buffer(allocator);
			fmtExtend::format(buffer, format, args...);
			return String(buffer.data(), (int32)buffer.size());
		}

	public:
		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		String& operator/=(const Char* str);

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		String& operator/=(const char* str);

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="c">The character to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		String& operator/=(Char c);

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		inline String& operator/=(const String& str)
		{
			return operator/=(*str);
		}

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		String& operator/=(const StringView& str);

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		inline String operator/(const Char* str) const
		{
			return String(*this) /= str;
		}

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		inline String operator/(const char* str) const
		{
			return String(*this) /= str;
		}

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="c">The character to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		inline String operator/(const Char c) const
		{
			return String(*this) /= c;
		}

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		inline String operator/(const String& str) const
		{
			return String(*this) /= str;
		}

		/// <summary>
		/// Concatenates this path with given path ensuring the '/' character is used between them.
		/// </summary>
		/// <param name="str">The string to be concatenated onto the end of this.</param>
		/// <returns>The combined path.</returns>
		inline String operator/(const StringView& str) const
		{
			return String(*this) /= str;
		}

	public:
		StringAnsi ToStringAnsi() const;
	};

	inline uint32 GetHash(const String& key)
	{
		return StringUtils::GetHashCode(key.Get());
	}


	/// <summary>
	/// Represents text as a sequence of ANSI characters. Container uses a single dynamic memory allocation to store the characters data. Characters sequence is always null-terminated.
	/// </summary>
	class SE_API_CORE StringAnsi : public StringBase<char>
	{
	public:
		/// <summary>
		/// Instance of the empty string.
		/// </summary>
		static StringAnsi Empty;

	public:
		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		StringAnsi() = default;

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The UTF-16 string.</param>
		StringAnsi(const char* str)
		{
			Set(str, StringUtils::Length(str));
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WString"/> class.
		/// </summary>
		/// <param name="str">The UTF-16 string.</param>
		/// <param name="length">The UTF-16 string length.</param>
		StringAnsi(const char* str, const int32 length)
		{
			Set(str, length);
		}

		
		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		/// <param name="str">The reference to the string.</param>
		StringAnsi(const StringAnsi& str)
		{
			Set(str.Get(), str.Length());
		}

		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		/// <param name="str">The double reference to the string.</param>
		StringAnsi(StringAnsi&& str) noexcept
		{
			m_data = str.m_data;
			m_length = str.m_length;
			str.m_data = nullptr;
			str.m_length = 0;
		}

		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		/// <param name="str">The reference to the string.</param>
		explicit StringAnsi(const String& str)
		{
			SetChars(str.Get(), str.Length());
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="String"/> class.
		/// </summary>
		/// <param name="str">ANSI string</param>
		explicit StringAnsi(const Char* str)
			: StringAnsi(str, StringUtils::Length(str))
		{
		}
		

		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		/// <param name="str">The UTF-16 string.</param>
		/// <param name="length">The UTF-16 string length.</param>
		explicit StringAnsi(const Char* str, int32 length)
		{
			SetChars(str, length);
		}

		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		/// <param name="str">The other string.</param>
		explicit StringAnsi(const StringView& str);

		/// <summary>
		/// Initializes a new instance of the
		/// </summary>
		/// <param name="str">The other string.</param>
		explicit StringAnsi(const StringAnsiView& str);

	public:

		/// <summary>
		/// Sets an array of characters to the string.
		/// </summary>
		/// <param name="chars">The pointer to the start of an array of characters to set (UTF-16). This array need not be null-terminated, and null characters are not treated specially.</param>
		/// <param name="length">The number of characters to assign.</param>
		void SetChars(const Char* chars, int32 length);

		/// <summary>
		/// Gets the pointer to the string or to the static empty text if string is null. Returned pointer is always valid (read-only).
		/// </summary>
		/// <returns>The string handle.</returns>
		const char* GetText() override
		{
			return m_data ? m_data : "";
		}

		const char* GetText() const override
		{
			return m_data ? m_data : "";
		}

		/// <summary>
		/// Appends an array of characters to the string.
		/// </summary>
		/// <param name="chars">The array of characters to append. It does not need be null-terminated, and null characters are not treated specially.</param>
		/// <param name="count">The number of characters to append.</param>
		StringAnsi& Append(const Char* chars, int32 count);

		StringAnsi& Append(const char* chars, int32 count)
		{
			return AppendBase<StringAnsi>(chars, count);
		}

		StringAnsi& Append(const Char c)
		{
			Append(&c, 1);
			return *this;
		}

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="text">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		StringAnsi& Append(const StringAnsi& text)
		{
			AppendBase<StringAnsi>(text.Get(), text.Length());
			return *this;
		}

				/// <summary>
		/// Gets the left most given number of characters.
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		StringAnsi Left(int32 count) const
		{
			return LeftBase<StringAnsi>(count);
		}

		/// <summary>
		/// Gets the string of characters from the right (end of the string).
		/// </summary>
		/// <param name="count">The characters count.</param>
		/// <returns>The substring.</returns>
		StringAnsi Right(int32 count) const
		{
			return RightBase<StringAnsi>(count);
		}

		/// <summary>
		/// Retrieves substring created from characters starting from startIndex to the WString end.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <returns>The substring created from WString data.</returns>
		StringAnsi Substring(int32 startIndex) const
		{
			return SubstringBase<StringAnsi>(startIndex);
		}

		/// <summary>
		/// Retrieves substring created from characters starting from start index.
		/// </summary>
		/// <param name="startIndex">The index of the first character to subtract.</param>
		/// <param name="count">The amount of characters to retrieve.</param>
		/// <returns>The substring created from WString data.</returns>
		StringAnsi Substring(int32 startIndex, int32 count) const
		{
			return SubstringBase<StringAnsi>(startIndex, count);
		}

		void Split(const char c, List<StringAnsi>& results) const
		{
			SplitBase<StringAnsi>(c, results);
		}

		void Split(const char cArray[], List<StringAnsi>& results) const
		{
			SplitBase<StringAnsi>(cArray, results);
		}

		void Split(std::initializer_list<char> cArray, List<StringAnsi>& results) const
		{
			SplitBase<StringAnsi>(cArray, results);
		}

		void Split(const char* c, const int delimiterLength, List<StringAnsi>& results) const
		{
			SplitBase<StringAnsi>(c, delimiterLength, results);
		}

		void Split(const char* cArray[], List<StringAnsi>& results) const
		{
			SplitBase<StringAnsi>(cArray, results);
		}

		void Split(std::initializer_list<const char*> cArray, List<StringAnsi>& results) const
		{
			SplitBase<StringAnsi>(cArray, results);
		}

		/// <summary>
		/// Gets the first line of the text (searches for the line terminator char).
		/// </summary>
		/// <returns>The single line of text.</returns>
		StringAnsi GetFirstLine()
		{
			return GetFirstLineBase<StringAnsi>();
		}

		/// <summary>
		/// Converts all uppercase characters to lowercase.
		/// </summary>
		/// <returns>The lowercase string.</returns>
		StringAnsi& ToLower()
		{
			return ToLowerBase<StringAnsi>();
		}

		/// <summary>
		/// Converts all lowercase characters to uppercase.
		/// </summary>
		/// <returns>The uppercase string.</returns>
		StringAnsi& ToUpper()
		{
			return ToUpperBase<StringAnsi>();
		}

		StringAnsi& TrimToNullTerminator()
		{
			return TrimToNullTerminatorBase<StringAnsi>();
		}

		/// <summary>
		/// Removes trailing whitespace characters from end and begin of the string.
		/// </summary>
		StringAnsi& TrimTrailing()
		{
			return TrimTrailingBase<StringAnsi>();
		}

	public:
		STRING_OPERATOR(StringAnsi, char)

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		inline StringAnsi& operator+=(const char* str)
		{
			AppendBase<StringAnsi>(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		inline StringAnsi& operator+=(const Char* str)
		{
			Append(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Appends the specified character to this string.
		/// </summary>
		/// <param name="c">The character to append.</param>
		/// <returns>The reference to this string.</returns>
		StringAnsi& operator+=(const char c);

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		inline StringAnsi& operator+=(const StringAnsi& str)
		{
			AppendBase<StringAnsi>(str.Get(), str.Length());
			return *this;
		}

		/// <summary>
		/// Appends the specified text to this string.
		/// </summary>
		/// <param name="str">The text to append.</param>
		/// <returns>The reference to this string.</returns>
		StringAnsi& operator+=(const StringAnsiView& str);

	public:
		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="s">The other string.</param>
		/// <returns>The reference to this.</returns>
		inline StringAnsi& operator=(StringAnsi&& s) noexcept
		{
			if (this != &s)
			{
				Platform::Free(m_data);
				m_data = s.m_data;
				m_length = s.m_length;
				s.m_data = nullptr;
				s.m_length = 0;
			}
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="s">The other string.</param>
		/// <returns>The reference to this.</returns>
		inline StringAnsi& operator=(const StringAnsi& s)
		{
			if (this != &s)
				Set(s.Get(), s.Length());
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="s">The other string.</param>
		/// <returns>The reference to this.</returns>
		StringAnsi& operator=(const StringAnsiView& s);

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="str">The other string.</param>
		/// <returns>The reference to this.</returns>
		StringAnsi& operator=(const char* str)
		{
			if (m_data != str)
				Set(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="str">The other string.</param>
		/// <returns>The reference to this.</returns>
		StringAnsi& operator=(const Char* str)
		{
			SetChars(str, StringUtils::Length(str));
			return *this;
		}

		/// <summary>
		/// Sets the text value.
		/// </summary>
		/// <param name="c">The other character.</param>
		/// <returns>The reference to this.</returns>
		StringAnsi& operator=(const char c)
		{
			Set(&c, 1);
			return *this;
		}
		
		void Split(StringAnsiView c, List<StringAnsi>& results);

	public:
		/// <summary>
		/// Formats the message and gets it as a string.
		/// </summary>
		/// <param name="format">The format string.</param>
		/// <param name="args">The custom arguments.</param>
		/// <returns>The formatted text.</returns>
		template<typename... Args>
		static StringAnsi Format(const char* format, const Args& ... args)
		{
			fmtExtend::Allocator_ansi allocatorAnsi;
			fmtExtend::memory_buffer_ansi buffer(allocatorAnsi);
			fmtExtend::format(buffer, format, args...);
			return StringAnsi(buffer.data(), (int32)buffer.size());
		}
		
		String ToString() const
		{
			return String(Get(), Length());
		}
	};

	inline uint32 GetHash(const StringAnsi& key)
	{
		return StringUtils::GetHashCode(key.Get());
	}


	/// <summary>
	/// Flax implementation for strings building class that supports UTF-16 (Unicode)
	/// </summary>
	class SE_API_CORE StringBuilder
	{
	private:
		/// <summary>
		/// List with characters of the string (it's not null-terminated)
		/// </summary>
		List<Char> m_data;

	public:
		/// <summary>
		/// Init
		/// </summary>
		StringBuilder()
		{

		}

		/// <summary>
		/// Create string builder with initial capacity
		/// </summary>
		/// <param name="capacity">Initial capacity for chars count</param>
		StringBuilder(int32 capacity) : m_data(capacity)
		{
			ENGINE_ASSERT(capacity > 0);
		}

	public:
		/// <summary>
		/// Gets capacity
		/// </summary>
		/// <returns>Capacity of the string builder</returns>
		inline int32 Capacity() const
		{
			return m_data.Capacity() - 1;
		}

		/// <summary>
		/// Sets capacity.
		/// </summary>
		/// <param name="capacity">Capacity to set</param>
		inline void SetCapacity(const int32 capacity)
		{
			m_data.SetCapacity(capacity);
		}

		/// <summary>
		/// Gets string length
		/// </summary>
		/// <returns>String length</returns>
		inline int32 Length() const
		{
			return m_data.Count();
		}

		/// <summary>
		/// Clears data
		/// </summary>
		inline void Clear()
		{
			m_data.Clear();
		}

	public:
		// Append single character to the string
		// @param c Character to append
		// @return Current String Builder instance
		StringBuilder &Append(const char c)
		{
			m_data.Add(c);
			return *this;
		}

		// Append single character to the string
		// @param c Character to append
		// @return Current String Builder instance
		StringBuilder &Append(const Char c)
		{
			m_data.Add(c);
			return *this;
		}

		// Append characters sequence to the string
		// @param str String to append
		// @return Current String Builder instance
		StringBuilder &Append(const Char *str)
		{
			const int32 length = StringUtils::Length(str);
			m_data.Add(str, length);
			return *this;
		}

		// Append characters sequence to the string
		// @param str String to append
		// @param length String length
		// @return Current String Builder instance
		StringBuilder &Append(const Char *str, int32 length)
		{
			m_data.Add(str, length);
			return *this;
		}

		// Append characters sequence to the string
		// @param str String to append
		// @return Current String Builder instance
		StringBuilder &Append(const char *str)
		{
			const int32 length = str && *str ? StringUtils::Length(str) : 0;
			const int32 prevCnt = m_data.Count();
			m_data.AddDefault(length);
			int32 tmp;
			StringUtils::ConvertANSI2UTF16(str, m_data.Get() + prevCnt, length, tmp);
			return *this;
		}

		// Append String to the string
		// @param str String to append
		// @return Current String Builder instance
		StringBuilder &Append(const String &str)
		{
			m_data.Add(*str, str.Length());
			return *this;
		}

		StringBuilder &Append(const StringView &str)
		{
			m_data.Add(*str, str.Length());
			return *this;
		}

		StringBuilder &Append(const StringAnsi &str)
		{
			m_data.Add(*str.Get());
			return *this;
		}

		StringBuilder &Append(const StringAnsiView &str)
		{
			m_data.Add(*str.Get());
			return *this;
		}


		// Append int to the string
		// @param val Value to append
		// @return Current String Builder instance
		StringBuilder &Append(int32 val)
		{
			auto str = StringUtils::ToString(val);
			m_data.Add(*str, str.Length());
			return *this;
		}

		// Append int to the string
		// @param val Value to append
		// @return Current String Builder instance
		StringBuilder &Append(uint32 val)
		{
			auto str = StringUtils::ToString(val);
			m_data.Add(*str, str.Length());
			return *this;
		}

		// Append formatted message to the string
		// @param format Format string
		// @param args List with custom arguments for the message
		template<typename... Args>
		StringBuilder &AppendFormat(const Char *format, const Args &... args)
		{
			fmtExtend::Allocator allocator;
			fmtExtend::memory_buffer buffer(allocator);
			fmtExtend::format(buffer, format, args...);
			return Append(buffer.data(), (int32)buffer.size());
		}

	public:
		StringBuilder &AppendLine()
		{
			Append(PLATFORM_LINE_TERMINATOR);
			return *this;
		}

		// Append int to the string
		// @param val Value to append
		// @return Current String Builder instance
		StringBuilder &AppendLine(int32 val)
		{
			Append(val);
			Append(PLATFORM_LINE_TERMINATOR);
			return *this;
		}

		// Append int to the string
		// @param val Value to append
		// @return Current String Builder instance
		StringBuilder &AppendLine(uint32 val)
		{
			Append(val);
			Append(PLATFORM_LINE_TERMINATOR);
			return *this;
		}

		// Append String to the string
		// @param str String to append
		// @return Current String Builder instance
		StringBuilder &AppendLine(const Char *str)
		{
			const int32 length = StringUtils::Length(str);
			m_data.Add(str, length);
			Append(PLATFORM_LINE_TERMINATOR);
			return *this;
		}

		// Append String to the string
		// @param str String to append
		// @return Current String Builder instance
		StringBuilder &AppendLine(const String &str)
		{
			m_data.Add(*str, str.Length());
			Append(PLATFORM_LINE_TERMINATOR);
			return *this;
		}

	public:
		// Retrieves substring created from characters starting from startIndex
		// @param startIndex Index of the first character to subtract
		// @param count Amount of characters to retrieve
		// @return Substring created from StringBuilder data
		String Substring(int32 startIndex, int32 count) const
		{
			ENGINE_ASSERT(startIndex >= 0 && startIndex + count <= m_data.Count() && count > 0);
			return String(m_data.Get() + startIndex, count);
		}

	public:
		// Get pointer to the stringToStringView
		// @returns Pointer to List of Chars if Num, otherwise the empty string
		inline const Char* operator*() const
		{
			return m_data.Count() > 0 ? m_data.Get() : SE_TEXT("");
		}

		// Get pointer to the string
		// @returns Pointer to List of Chars if Num, otherwise the empty string
		inline const Char* operator*()
		{
			return m_data.Count() > 0 ? m_data.Get() : SE_TEXT("");
		}

		// Get string as array of Chars
		inline List<Char>& GetCharArray()
		{
			return m_data;
		}

		// Get string as const array of Chars
		inline const List<Char>& GetCharArray() const
		{
			return m_data;
		}

	public:
		String ToString() const;

		StringAnsi ToStringAnsi() const;
	};

	inline uint32 GetHash(const StringBuilder &key)
	{
		return StringUtils::GetHashCode(key.GetCharArray().Get(), key.Length());
	}
}

namespace fmt
{
	template<>
	struct formatter<SE::String, SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::String& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			return ::fmt::detail::copy_str<SE::Char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::String, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::String& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			SE::StringAnsi s = v.ToStringAnsi();
			return ::fmt::detail::copy_str<SE::Char>(s.Get(), s.Get() + s.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::StringAnsi, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::StringAnsi& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			return ::fmt::detail::copy_str<char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::StringAnsi, SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::StringAnsi& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			SE::String s = v.ToString();
			return ::fmt::detail::copy_str<SE::Char>(s.Get(), s.Get() + s.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::StringBuilder, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::String& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			return ::fmt::detail::copy_str<char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<SE::StringBuilder, SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const SE::String& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			return ::fmt::detail::copy_str<SE::Char>(v.Get(), v.Get() + v.Length(), ctx.out());
		}
	};

}