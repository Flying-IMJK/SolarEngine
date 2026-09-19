
using System.Runtime.CompilerServices;

namespace SE
{
    /// <summary>
    /// Font reference that defines the font asset and font size to use.
    /// </summary>
    public class FontReference
    {
        private FontAsset m_Font;
        private float m_Size;
        private Font m_CachedFont;

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        public FontReference()
        {
            m_Font = null;
            m_Size = 30;
            m_CachedFont = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        /// <param name="size">The font size.</param>
        public FontReference(FontAsset font, float size)
        {
            m_Font = font;
            m_Size = size;
            m_CachedFont = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="other">The other font reference.</param>
        public FontReference(FontReference other)
        {
            m_Font = other.m_Font;
            m_Size = other.m_Size;
            m_CachedFont = other.m_CachedFont;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        public FontReference(Font font)
        {
            if (font)
            {
                m_Font = font.GetAsset();
                m_Size = font.GetSize();
            }
            else
            {
                m_Font = null;
                m_Size = 30;
            }
            m_CachedFont = font;
        }

        /// <summary>
        /// The font asset.
        /// </summary>
        public FontAsset Font
        {
            get => m_Font;
            set
            {
                if (m_Font != value)
                {
                    m_Font = value;
                    m_CachedFont = null;
                }
            }
        }

        /// <summary>
        /// The size of the font characters.
        /// </summary>
        public float Size
        {
            get => m_Size;
            set
            {
                if (m_Size != value)
                {
                    m_Size = value;
                    m_CachedFont = null;
                }
            }
        }

        /// <summary>
        /// Gets the font object described by the structure.
        /// </summary>
        /// <returns>The font or null if descriptor is invalid.</returns>
        public Font GetFont()
        {
            if (m_CachedFont)
                return m_CachedFont;
            if (m_Font)
                m_CachedFont = m_Font.CreateFont(m_Size);
            return m_CachedFont;
        }

        /// <summary>
        /// Gets the bold font object described by the structure.
        /// </summary>
        /// <returns>The bold font asset.</returns>
        public FontReference GetBold()
        {
            return new FontReference(m_Font?.GetBold(), m_Size);
        }

        /// <summary>
        /// Gets the italic font object described by the structure.
        /// </summary>
        /// <returns>The bold font asset.</returns>
        public FontReference GetItalic()
        {
            return new FontReference(m_Font?.GetItalic(), m_Size);
        }

        /// <summary>
        /// Determines whether the specified <see cref="FontReference" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="FontReference" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="FontReference" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(FontReference other)
        {
            return !(other is null) && m_Font == other.m_Font && m_Size == other.m_Size;
        }

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="lhs">The left.</param>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are equal, otherwise false.</returns>
        public static bool operator ==(FontReference lhs, FontReference rhs)
        {
            if (lhs is null)
                return rhs is null;
            return lhs.Equals(rhs);
        }

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="lhs">The left.</param>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are not equal, otherwise false.</returns>
        public static bool operator !=(FontReference lhs, FontReference rhs)
        {
            if (lhs is null)
                return !(rhs is null);
            return !lhs.Equals(rhs);
        }

        /// <inheritdoc />
        public override bool Equals(object other)
        {
            if (!(other is FontReference))
                return false;
            var fontReference = (FontReference)other;
            return Equals(fontReference);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = m_Font ? m_Font.GetHashCode() : 0;
                hashCode = (hashCode * 397) ^ m_Size.GetHashCode();
                return hashCode;
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format("{0}, size {1}", m_Font ? m_Font.ToString() : string.Empty, m_Size);
        }
    }
}
