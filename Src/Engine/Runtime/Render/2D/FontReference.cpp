

#include "FontReference.h"
#include "Font.h"
#include "FontAsset.h"

namespace SE
{
	FontReference::FontReference()
	{
		_font = nullptr;
		_size = 30;
		_cachedFont = nullptr;
	}

	FontReference::FontReference(FontAsset* font, float size)
	{
		_font = font;
		_size = size;
		_cachedFont = nullptr;
	}

	FontReference::FontReference(const FontReference& other)
	{
		_font = other._font;
		_size = other._size;
		_cachedFont = other._cachedFont;
	}

	FontReference::FontReference(::SE::Font* font)
	{
		if (font)
		{
			_font = font->GetAsset();
			_size = font->GetSize();
		}
		else
		{
			_font = nullptr;
			_size = 30;
		}
		_cachedFont = font;
	}

	::SE::Font* FontReference::GetFont()
	{
		if (_cachedFont)
			return _cachedFont;
		if (_font)
			_cachedFont = _font->CreateFont(_size);
		return _cachedFont;
	}

	FontReference FontReference::GetBold()
	{
		return FontReference(_font->GetBold(), _size);
	}

	FontReference FontReference::GetItalic()
	{
		return FontReference(_font->GetItalic(), _size);
	}

	bool FontReference::Equals(const FontReference& other) const
	{
		return _font == other._font && _size == other._size;
	}

	bool FontReference::operator==(const FontReference& rhs) const
	{
		return this->Equals(rhs);
	}

	bool FontReference::operator!=(const FontReference& rhs) const
	{
		return !this->Equals(rhs);
	}

	uint32 FontReference::GetHashCode() const
	{
		int hashCode = _font ? GetHash(_font) : 0;
		hashCode = (hashCode * 397) ^ GetHash(_size);
		return hashCode;
	}

	void FontReference::__SetFont(FontAsset* value)
	{
		if (_font != value)
		{
			_font = value;
			_cachedFont = nullptr;
		}
	}

	void FontReference::__SetSize(float value)
	{
		if (_size != value)
		{
			_size = value;
			_cachedFont = nullptr;
		}
	}
} // SE