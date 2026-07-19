#pragma once

#include "MemoryWriteStream.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Object.h"

namespace SE
{
    /// <summary>
    /// Useful tool to write many text data
    /// </summary>
    template<typename CharType>
    class TextWriter final : public Object
    {
        NON_COPYABLE(TextWriter)
    private:
        MemoryWriteStream m_Buffer;

    public:
        /// <summary>
        /// Init with default capacity
        /// </summary>
        /// <param name="capacity">Initial capacity in bytes</param>
        TextWriter(uint32 capacity = 1024)
            : m_Buffer(capacity)
        {
        }

        /// <summary>
        /// Destructor
        /// </summary>
        ~TextWriter()
        {
        }

    public:
        /// <summary>
        /// Gets writer private buffer.
        /// </summary>
        FORCE_INLINE MemoryWriteStream* GetBuffer()
        {
            return &m_Buffer;
        }

        /// <summary>
        /// Gets writer private buffer.
        /// </summary>
        FORCE_INLINE const MemoryWriteStream* GetBuffer() const
        {
            return &m_Buffer;
        }

    public:
        /// <summary>
        /// Write line terminator sign
        /// </summary>
        template<class Q = CharType>
        typename std::enable_if<std::is_same<Q, char>::value, void>::type WriteLine()
        {
            m_Buffer.WriteChar('\n');
        }

        /// <summary>
        /// Write line terminator sign
        /// </summary>
        template<class Q = CharType>
        typename std::enable_if<std::is_same<Q, Char>::value, void>::type WriteLine()
        {
            m_Buffer.WriteUint16(SE_TEXT('\n'));
        }

        /// <summary>
        /// Write single line of text to the buffer
        /// </summary>
        /// <param name="text">Data</param>
        void WriteLine(const CharType* text)
        {
            m_Buffer.WriteBytes((void*)text, StringUtils::Length(text) * sizeof(CharType));
            WriteLine();
        }

        /// <summary>
        /// Format text and write line to the buffer
        /// </summary>
        template<typename... Args>
        void WriteLine(const CharType* format, const Args& ... args)
        {
            ::fmt::basic_memory_buffer<Char, ::fmt::inline_buffer_size, STLAllocator<CharType>> w;
            fmtExtend::format(w, format, args...);
            const int32 len = (int32)w.size();
            m_Buffer.WriteBytes((void*)w.data(), len * sizeof(CharType));
            WriteLine();
        }

        /// <summary>
        /// Write text to the buffer
        /// </summary>
        /// <param name="text">Data</param>
        void Write(const StringViewBase<CharType>& text)
        {
            m_Buffer.WriteBytes((void*)text.Get(), text.Length() * sizeof(CharType));
        }

        /// <summary>
        /// Write text to the buffer
        /// </summary>
        /// <param name="text">Data</param>
        void Write(const StringBase<CharType>& text)
        {
            m_Buffer.WriteBytes((void*)text.Get(), text.Length() * sizeof(CharType));
        }

        /// <summary>
        /// Write text to the buffer
        /// </summary>
        /// <param name="text">Data</param>
        void Write(const CharType* text)
        {
            m_Buffer.WriteBytes((void*)text, StringUtils::Length(text) * sizeof(CharType));
        }

        /// <summary>
        /// Format text and write it
        /// </summary>
        template<typename... Args>
        void Write(const CharType* format, const Args& ... args)
        {
            ::fmt::basic_memory_buffer<Char, ::fmt::inline_buffer_size, STLAllocator<CharType>> w;
            fmtExtend::format(w, format, args...);
            const int32 len = (int32)w.size();
            m_Buffer.WriteBytes((void*)w.data(), len * sizeof(CharType));
        }

    public:
        /// <summary>
        /// Clear whole data
        /// </summary>
        void Clear()
        {
            m_Buffer.SetPosition(0);
        }

    public:
        // [Object]
        String ToString() const override
        {
            return String((CharType*)m_Buffer.GetHandle(), m_Buffer.GetPosition() / sizeof(CharType));
        }
    };

    typedef TextWriter<char> TextWriterANSI;
    typedef TextWriter<Char> TextWriterUnicode;
}
