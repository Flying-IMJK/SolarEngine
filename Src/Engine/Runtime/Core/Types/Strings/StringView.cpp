
#include "Runtime/Core/Types/Strings/StringView.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	StringView StringView::Empty;

	StringView::StringView(const String& str) : StringViewBase<Char>(str.Get(), str.Length())
	{
	}

	bool StringView::operator==(const String& other) const
	{
		return this->Compare(StringView(other)) == 0;
	}

	bool StringView::operator!=(const String& other) const
	{
		return this->Compare(StringView(other)) != 0;
	}

	StringView StringView::Left(int32 count) const
	{
		const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
		return StringView(**this, countClamped);
	}

	StringView StringView::Right(int32 count) const
	{
		const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
		return StringView(**this + Length() - countClamped);
	}

	StringView StringView::Substring(int32 startIndex) const
	{
		ENGINE_ASSERT(startIndex >= 0 && startIndex < Length());
		return StringView(Get() + startIndex, Length() - startIndex);
	}

	StringView StringView::Substring(int32 startIndex, int32 count) const
	{
		ENGINE_ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
		return StringView(Get() + startIndex, count);
	}

	StringAnsi StringView::ToStringAnsi() const
	{
		return StringAnsi(m_data, m_length);
	}

	String StringView::ToString() const
	{
		return String(m_data, m_length);
	}

	bool operator==(const String& a, const StringView& b)
	{
		return a.Length() == b.Length() && StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) == 0;
	}

	bool operator!=(const String& a, const StringView& b)
	{
		return a.Length() != b.Length() || StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) != 0;
	}


	StringAnsiView StringAnsiView::Empty = StringAnsiView();

	StringAnsiView::StringAnsiView(const StringAnsi& str)
		: StringViewBase<char>(str.Get(), str.Length())
	{
	}

	bool StringAnsiView::operator==(const StringAnsi& other) const
	{
		return this->Compare(StringAnsiView(other)) == 0;
	}

	bool StringAnsiView::operator!=(const StringAnsi& other) const
	{
		return this->Compare(StringAnsiView(other)) != 0;
	}

	StringAnsi StringAnsiView::Substring(int32 startIndex) const
	{
		ENGINE_ASSERT(startIndex >= 0 && startIndex < Length());
		return StringAnsi(Get() + startIndex, Length() - startIndex);
	}

	StringAnsi StringAnsiView::Substring(int32 startIndex, int32 count) const
	{
		ENGINE_ASSERT(startIndex >= 0 && startIndex + count <= Length() && count >= 0);
		return StringAnsi(Get() + startIndex, count);
	}

	StringAnsi StringAnsiView::ToStringAnsi() const
	{
		return StringAnsi(m_data, m_length);
	}

	String StringAnsiView::ToString() const
	{
		return String(*this);
	}

	StringAnsiView StringAnsiView::Left(int32 count) const
	{
		const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
		return StringAnsiView(**this, countClamped);
	}

	StringAnsiView StringAnsiView::Right(int32 count) const
	{
		const int32 countClamped = count < 0 ? 0 : count < Length() ? count : Length();
		return StringAnsiView(**this + Length() - countClamped);
	}

	bool operator==(const StringAnsi& a, const StringAnsiView& b)
	{
		return a.Length() == b.Length() && StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) == 0;
	}

	bool operator!=(const StringAnsi& a, const StringAnsiView& b)
	{
		return a.Length() != b.Length() || StringUtils::Compare(a.GetText(), b.GetText(), b.Length()) != 0;
	}

	String StringBuilder::ToString() const
	{
		if (m_data.Count() <= 0)
		{
			return String::Empty;
		}
		else
		{
			return String(m_data.Get(), m_data.Count());
		}
	}

	StringAnsi StringBuilder::ToStringAnsi() const
	{
		if (m_data.Count() <= 0)
		{
			return StringAnsi::Empty;
		}
		else
		{
			return StringAnsi(m_data.Get(), m_data.Count());
		}
	}
}