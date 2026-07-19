#pragma once

#include "Runtime/Core/Platform/StringUtils.h"
#include "Runtime/Core/Memory/Memory.h"

namespace SE
{
	template<typename CharType, int InlinedSize = 256>
	class StringConverterBase
	{
	protected:
		const CharType* m_Static = nullptr;
		CharType* m_Dynamic = nullptr;
		CharType m_Inlined[InlinedSize];

	public:
		~StringConverterBase()
		{
			if (m_Dynamic)
			{
				PlatformAllocator::Free(m_Dynamic);
			}
			m_Dynamic = nullptr;
		}

	public:
		const CharType* Get() const
		{
			return m_Static ? m_Static : (m_Dynamic ? m_Dynamic : m_Inlined);
		}

		int32 Length() const
		{
			return StringUtils::Length(Get());
		}
	};

	template<int InlinedSize = 256>
	class StringAsANSI : public StringConverterBase<char, InlinedSize>
	{
	public:
		typedef char CharType;
		typedef StringConverterBase<CharType, InlinedSize> Base;

	public:
		explicit StringAsANSI(const char* text)
		{
			this->m_Static = text;
		}

		explicit StringAsANSI(const Char* text)
			: StringAsANSI(text, StringUtils::Length(text))
		{
		}

		StringAsANSI(const Char* text, int32 length)
		{
			if (length + 1 < InlinedSize)
			{
				StringUtils::ConvertUTF162ANSI(text, this->m_Inlined, length);
				this->m_Inlined[length] = 0;
			}
			else
			{
				this->m_Dynamic = (CharType*)PlatformAllocator::Allocate((length + 1) * sizeof(CharType));
				StringUtils::ConvertUTF162ANSI(text, this->m_Dynamic, length);
				this->m_Dynamic[length] = 0;
			}
		}
	};

	template<int InlinedSize = 256>
	class StringAsUTF8 : public StringConverterBase<char, InlinedSize>
	{
	public:
		typedef char CharType;
		typedef StringConverterBase<CharType, InlinedSize> Base;

	public:
		explicit StringAsUTF8(const char* text)
		{
			this->m_Static = text;
		}

		explicit StringAsUTF8(const Char* text)
			: StringAsUTF8(text, StringUtils::Length(text))
		{
		}

		StringAsUTF8(const Char* text, int32 length)
		{
			int32 lengthUtf8;
			if (length + 1 < InlinedSize)
			{
				StringUtils::ConvertUTF162UTF8(text, this->m_Inlined, length, lengthUtf8);
				this->m_Inlined[lengthUtf8] = 0;
			}
			else
			{
				this->m_Dynamic = StringUtils::ConvertUTF162UTF8(text, length, lengthUtf8);
				this->m_Dynamic[lengthUtf8] = 0;
			}
		}
	};

	template<int InlinedSize = 256>
	class StringAsUTF16 : public StringConverterBase<Char, InlinedSize>
	{
	public:
		typedef Char CharType;
		typedef StringConverterBase<CharType, InlinedSize> Base;

	public:
		explicit StringAsUTF16(const char* text)
			: StringAsUTF16(text, StringUtils::Length(text))
		{
		}

		StringAsUTF16(const char* text, int32 length)
		{
			if (length + 1 < InlinedSize)
			{
				StringUtils::ConvertANSI2UTF16(text, this->m_Inlined, length, length);
				this->m_Inlined[length] = 0;
			}
			else
			{
				this->m_Dynamic = (CharType*)PlatformAllocator::Allocate((length + 1) * sizeof(CharType));
				StringUtils::ConvertANSI2UTF16(text, this->m_Dynamic, length, length);
				this->m_Dynamic[length] = 0;
			}
		}

		explicit StringAsUTF16(const Char* text)
		{
			this->m_Static = text;
		}
	};
}