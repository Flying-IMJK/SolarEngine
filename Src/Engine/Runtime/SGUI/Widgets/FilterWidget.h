#pragma once

#include "Runtime/API.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/BitFlags.h"

#ifdef SE_DEVELOPMENT

namespace SE::GUI
{
    // A simple filter entry widget that allows you to string match to some entered text
    class SE_API_RUNTIME FilterWidget
    {
        constexpr static uint32 const s_bufferSize = 255;

    public:
        enum Flags : uint8
        {
            TakeInitialFocus = 1 << 0
        };

    public:

        // Draws the filter. Returns true if the filter has been updated
        bool UpdateAndDraw(float width = -1, EnumFlags<Flags> flags = EnumFlags<Flags>());

        // Manually set the filter buffer
        void SetFilter(String const &filterText);

        // Set the help text shown when we dont have focus and the filter is empty
        void SetFilterHelpText(String const &helpText) { m_filterHelpText = helpText; }

        // Clear the filter
        inline void Clear();

        // Do we have a filter set?
        inline bool HasFilterSet() const { return !m_tokens.IsEmpty(); }

        // Get the split filter text token
        inline List<String> const &GetFilterTokens() const { return m_tokens; }

        // Does a provided string match the current filter - the string copy is intentional!
        bool MatchesFilter(StringView string);

    private:
        void OnBufferUpdated();

    private:
		String m_buffer;
		List<String> m_tokens;
        String m_filterHelpText = SE_TEXT("Filter...");
    };

}
#endif

