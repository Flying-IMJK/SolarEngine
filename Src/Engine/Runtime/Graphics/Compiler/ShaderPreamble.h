#pragma once

#include "Core/Types/Variable.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/List.h"

namespace SE
{
    class ShaderPreamble
    {
    public:
        ShaderPreamble() {}

        bool IsSet() const { return m_Contents.Length() > 0; }
        const char *Get() const { return m_Contents.Get(); }
        void Clear();

        // #define...
        void AddDefine(String def);

        // #undef...
        void AddUndef(String undef);

        void AddText(String preambleText);

    protected:
        void FixLine(String &line);

        List<String> m_Processes;
        String m_Contents; // contents of preamble
    };
}