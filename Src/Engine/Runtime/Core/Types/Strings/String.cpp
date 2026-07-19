#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Strings/StringView.h"
#include "Runtime/Core/Types/Collections/List.h"

namespace SE
{
	String String::Empty = String();

	String::String(const StringAnsi& str)
	{
		SetChars(str.Get(), str.Length());
	}

	String::String(const StringView& str)
	{
		if (m_data)
		{
			Platform::Free(m_data);
		}

		m_length = str.Length();

		ENGINE_ASSERT(m_length >= 0);
		m_data = static_cast<Char*>(Platform::Allocate((m_length + 1) * sizeof(Char), 16));
		m_data[m_length] = 0;
		if (m_length > 0)
		{
			Platform::MemoryCopy(m_data, str.Get(), m_length * sizeof(Char));
		}
	}

	String::String(const StringAnsiView& str)
	{
		SetChars(str.Get(), str.Length());
	}

	void String::SetChars(const char* chars, int32 length)
	{
		if (length != m_length)
		{
			Platform::Free(m_data);
			m_data = static_cast<Char*>(Platform::Allocate((length + 1) * sizeof(Char), 16));
			m_data[length] = 0;
			m_length = length;
		}
		if (chars && length)
			StringUtils::ConvertANSI2UTF16(chars, m_data, length, m_length);
	}

	void String::SetUTF8(const char* chars, int32 length)
	{
		Platform::Free(m_data);
		m_data = StringUtils::ConvertUTF82UTF16(chars, length, m_length);
	}

	String& String::Append(const char* chars, int32 count)
	{
		if (count == 0)
			return *this;

		const auto oldData = m_data;
		const auto oldLength = m_length;

		m_length = oldLength + count;
		m_data = static_cast<Char*>(Platform::Allocate((m_length + 1) * sizeof(Char), 16));

		Platform::MemoryCopy(m_data, oldData, oldLength * sizeof(Char));
		StringUtils::ConvertANSI2UTF16(chars, m_data + oldLength, count, m_length);
		m_length += oldLength;
		m_data[m_length] = 0;

		Platform::Free(oldData);
		return *this;
	}

	String& String::operator+=(const StringView& str)
	{
		Append(str.Get(), str.Length());
		return *this;
	}

	String& String::operator=(const StringView& s)
	{
		Set(s.Get(), s.Length());
		return *this;
	}

	bool String::IsANSI() const
	{
		bool result = true;
		for (int32 i = 0; i < m_length; i++)
		{
			if (m_data[i] > 127)
			{
				result = false;
				break;
			}
		}
		return result;
	}

	String& String::operator/=(const Char* str)
	{
		const int32 length = m_length;
		if (length > 0 && m_data[length - 1] != SE_TEXT('/') && m_data[length - 1] != SE_TEXT('\\')
			&& (str == nullptr || (str[0] != SE_TEXT('/') && str[0] != SE_TEXT('\\'))))
		{
			*this += SE_TEXT('/');
		}
		return *this += str;
	}

	String& String::operator/=(const char* str)
	{
		const int32 length = m_length;
		if (length > 0 && m_data[length - 1] != SE_TEXT('/') && m_data[length - 1] != SE_TEXT('\\')
			&& (str == nullptr || (str[0] != SE_TEXT('/') && str[0] != SE_TEXT('\\'))))
		{
			*this += SE_TEXT('/');
		}
		return *this += str;
	}

	String& String::operator/=(const Char c)
	{
		const int32 length = m_length;
		if (length > 0 && m_data[length - 1] != SE_TEXT('/') && m_data[length - 1] != SE_TEXT('\\'))
		{
			*this += SE_TEXT('/');
		}
		return *this += c;
	}

	String& String::operator/=(const StringView& str)
	{
		const int32 length = m_length;
		if (length > 1 && m_data[length - 1] != SE_TEXT('/') && m_data[length - 1] != SE_TEXT('\\')
			&& (str == nullptr || (str[0] != SE_TEXT('/') && str[0] != SE_TEXT('\\'))))
		{
			*this += SE_TEXT('/');
		}
		return *this += str;
	}

	String& String::operator+=(const Char c)
	{
		Append(&c, 1);
		return *this;
	}


	StringAnsi String::ToStringAnsi() const
	{
		return StringAnsi(*this);
	}


	StringAnsi StringAnsi::Empty = StringAnsi();

	StringAnsi::StringAnsi(const StringView& str)
	{
		SetChars(str.Get(), str.Length());
	}

	StringAnsi::StringAnsi(const StringAnsiView& str)
	{
		Set(str.Get(), str.Length());
	}

	void StringAnsi::SetChars(const Char* chars, int32 length)
	{
		if (length != m_length)
		{
			Platform::Free(m_data);
			m_data = static_cast<char*>(Platform::Allocate((length + 1) * sizeof(char), 16));
			m_data[length] = 0;
			m_length = length;
		}

		if (m_data)
			StringUtils::ConvertUTF162ANSI(chars, m_data, length);
	}

	StringAnsi& StringAnsi::Append(const Char* chars, int32 count)
	{
		if (count == 0)
			return *this;

		const auto oldData = m_data;
		const auto oldLength = m_length;

		m_length = oldLength + count;
		m_data = static_cast<char*>(Platform::Allocate((m_length + 1) * sizeof(char), 16));

		Platform::MemoryCopy(m_data, oldData, oldLength * sizeof(char));
		StringUtils::ConvertUTF162ANSI(chars, m_data + oldLength, count * sizeof(char));
		m_data[m_length] = 0;

		Platform::Free(oldData);

		return *this;
	}

	StringAnsi& StringAnsi::operator+=(const StringAnsiView& str)
	{
		Append(str.Get(), str.Length());
		return *this;
	}

	StringAnsi& StringAnsi::operator=(const StringAnsiView& s)
	{
		Set(s.Get(), s.Length());
		return *this;
	}


	void StringAnsi::Split(StringAnsiView c, List<StringAnsi>& results)
	{
		SplitBase(c.Get(), c.Length(), results);
	}

	StringAnsi& StringAnsi::operator+=(const char c)
	{
		ENGINE_ASSERT(c != 0);
		Append(&c, 1);
		return *this;
	}
}