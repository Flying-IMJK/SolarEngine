#include "ShaderPreamble.h"
#include <fmt/format.h>

namespace SE
{
    void ShaderPreamble::Clear()
    {
        m_Processes.Clear();
        m_Contents.Clear();
    }

    void ShaderPreamble::AddDefine(String def)
    {
        FixLine(def);

        m_Processes.Add(Format("define-macro {0}", def));

        // The first "=" needs to turn into a space
        const int equal = def.FindFirstOf("=");
        if (equal != INVALID_INDEX)
            def[equal] = ' ';

        m_Contents.Append(Format("#define {0}\n", def));
    }

    void ShaderPreamble::AddUndef(String undef)
    {
        m_Contents.Append("#undef ");
        FixLine(undef);

        m_Processes.Add(Format("undef-macro {0}", undef));
        m_Contents.Append(Format("{0}\n", undef));
    }

    void ShaderPreamble::AddText(String preambleText)
    {
        FixLine(preambleText);

        m_Processes.Add(Format("preamble-text{0}", preambleText));
        m_Contents.Append(Format("{0}\n", preambleText));
    }

    void ShaderPreamble::FixLine(String &line)
    {
        // Can't go past a newline in the line
        const size_t end = line.FindFirstOf("\n");
        if (end != INVALID_INDEX)
		{
			line = line.Substring(0, end);
		}
    }
}
