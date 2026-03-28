#pragma once
#include "Core/Types/Property.h"
#include "Core/Types/Strings/StringView.h"

#include "Runtime/API.h"

namespace SE
{
    class FontAsset;
    class Font;
    
    class SE_API_RUNTIME FontReference
    {
    private:
        FontAsset* _font;
        float _size;
        Font* _cachedFont;

    public:
        
        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        FontReference();

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        /// <param name="size">The font size.</param>
        FontReference(FontAsset* font, float size);

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="other">The other font reference.</param>
        FontReference(const FontReference& other) ;

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        FontReference(Font* font);

        /// <summary>
        /// The font asset.
        /// </summary>
        PRO(Font, FontReference, FontAsset*, __GetFont, __SetFont);

        /// <summary>
        /// The size of the font characters.
        /// </summary>
        PRO(Size, FontReference, float, __GetSize, __SetSize);

        /// <summary>
        /// Gets the font object described by the structure.
        /// </summary>
        /// <returns>The font or null if descriptor is invalid.</returns>
        ::SE::Font* GetFont();

        /// <summary>
        /// Gets the bold font object described by the structure.
        /// </summary>
        /// <returns>The bold font asset.</returns>
        FontReference GetBold();

        /// <summary>
        /// Gets the italic font object described by the structure.
        /// </summary>
        /// <returns>The bold font asset.</returns>
        FontReference GetItalic();

        bool Equals(const FontReference& other) const;

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are equal, otherwise false.</returns>
        bool operator ==(const FontReference& rhs) const;

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are not equal, otherwise false.</returns>
        bool operator !=(const FontReference& rhs) const;

        uint32 GetHashCode() const;

    private:
        FontAsset* __GetFont()  { return _font; };
        void __SetFont(FontAsset* value);


        float __GetSize() { return _size; }
        void __SetSize(float value);

    };

    inline uint32 GeHash(const FontReference &value)
    {
        return value.GetHashCode();
    }
} // SE

