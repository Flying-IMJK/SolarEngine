#pragma once

#include "ClangParserContext.h"
#include "Core/Time.h"
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    class ClangParser
    {
    public:

        ClangParser( SolutionInfo* pSolution, ReflectionDatabase* pDatabase, std::string const& reflectionDataPath );

        inline Milliseconds GetParsingTime() const { return m_totalParsingTime; }
        inline Milliseconds GetVisitingTime() const { return m_totalVisitingTime; }

        bool Parse(std::vector<HeaderInfo*> const& headers);
		std::string_view GetErrorMessage() const { return m_context.GetErrorMessage(); }

    private:

        ClangParserContext                  m_context;
        Milliseconds                        m_totalParsingTime;
        Milliseconds                        m_totalVisitingTime;
		std::string                    			m_reflectionDataPath;
    };
}
