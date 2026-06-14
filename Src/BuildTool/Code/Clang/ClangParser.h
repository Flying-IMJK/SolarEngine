#pragma once

#include "ClangParserContext.h"
#include "Core/Utilities/Time.h"
//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    class ClangParser
    {
    public:

        ClangParser( SolutionInfo* pSolution, ReflectionDatabase* pDatabase, String const& reflectionDataPath );

        inline Milliseconds GetParsingTime() const { return m_totalParsingTime; }
        inline Milliseconds GetVisitingTime() const { return m_totalVisitingTime; }

        bool Parse(List<HeaderInfo*> const& headers);
		StringAnsiView GetErrorMessage() const { return m_context.GetErrorMessage(); }

    private:

        ClangParserContext                  m_context;
        Milliseconds                        m_totalParsingTime;
        Milliseconds                        m_totalVisitingTime;
		String                    			m_reflectionDataPath;
    };
}
