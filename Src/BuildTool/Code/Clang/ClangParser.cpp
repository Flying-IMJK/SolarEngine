#include "ClangParser.h"
#include "../Database/ReflectionDatabase.h"
#include "Core/FileSystem.h"
#include "Core/Time.h"
#include "Core/Utils.h"

#include "ClangVisitors_TranslationUnit.h"

#include <fstream>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    ClangParser::ClangParser( SolutionInfo* pSolution, ReflectionDatabase* pDatabase, std::string const& reflectionDataPath )
        : m_context( pSolution, pDatabase )
        , m_totalParsingTime( 0 )
        , m_totalVisitingTime( 0 )
        , m_reflectionDataPath( reflectionDataPath )
    {}

    bool ClangParser::Parse(std::vector<HeaderInfo*> const& headers)
    {
        // Create single amalgamated header file for all headers to parse
        //-------------------------------------------------------------------------

        std::ofstream reflectorFileStream;
		std::string const reflectorHeader = m_reflectionDataPath + "Reflector.h";
        FileSystem::CreateDirectory(FileSystem::GetParentDirectory(reflectorHeader));
        reflectorFileStream.open( reflectorHeader, std::ios::out | std::ios::trunc );
        ENGINE_ASSERT( !reflectorFileStream.fail() );

		std::string includeStr;
        m_context.headersToVisit.clear();
        for ( HeaderInfo const* pHeader : headers )
        {
            m_context.headersToVisit.push_back( ClangParserContext::HeaderToVisit(pHeader->headerId, pHeader) );
            includeStr += "#include \"" + pHeader->filePath + "\"\n";
        }

        reflectorFileStream.write( includeStr.c_str(), includeStr.length() );
        reflectorFileStream.close();

        // Clang args
        std::vector<std::string> fullIncludePaths;
        fullIncludePaths.reserve(10);
		std::vector<char const*> clangArgs;
        int32_t const numIncludePaths = std::size(Settings::g_includePaths);
        for ( auto i = 0; i < numIncludePaths; i++ )
        {
            std::string const fullPath = m_context.pSolution->path + "/" + Settings::g_includePaths[i];
            fullIncludePaths.push_back( "-I" + fullPath );
            clangArgs.push_back( fullIncludePaths.back().c_str() );

            if (!FileSystem::DirectoryExists( fullPath))
            {
                m_context.LogError("Invalid include path: {0}", fullPath );
                return false;
            }
        }

        clangArgs.push_back( "-x" );
        clangArgs.push_back( "c++" );
        clangArgs.push_back( "-std=c++17" );
        clangArgs.push_back( "-O0" );
        clangArgs.push_back( "-D NDEBUG" );
		clangArgs.push_back( "-D PLATFORM_WINDOWS" );
		clangArgs.push_back( "-D PLATFORM_WIN32" );
        clangArgs.push_back( "-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH" );
        clangArgs.push_back( "-Werror" );
        clangArgs.push_back( "-Wno-deprecated-builtins" );
        clangArgs.push_back( "-fparse-all-comments" );
        clangArgs.push_back( "-fms-extensions" );
        clangArgs.push_back( "-fms-compatibility" );
        clangArgs.push_back( "-Wno-unknown-warning-option" );
        clangArgs.push_back( "-Wno-return-type-c-linkage" );
        clangArgs.push_back( "-Wno-gnu-folding-constant" );
        clangArgs.push_back( "-Wno-nonportable-include-path" );

        //-------------------------------------------------------------------------
        // Set up clang
        auto idx = clang_createIndex( 0, 1 );
        uint32_t const clangOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;

        // Parse Headers
        CXTranslationUnit tu;
        CXErrorCode result = CXError_Failure;
        {
            ScopedTimer<PlatformClock> timer( m_totalParsingTime );
            result = clang_parseTranslationUnit2( idx, reflectorHeader.c_str(), Utils::Vector::Data(clangArgs), (int)clangArgs.size(), 0, 0, clangOptions, &tu );
        }

        // Handle result of parse
        if (result == CXError_Success)
        {
            ScopedTimer<PlatformClock> timer(m_totalVisitingTime);
            m_context.Reset( &tu );
            const auto cursor = clang_getTranslationUnitCursor(tu);
            clang_visitChildren( cursor, VisitTranslationUnit, &m_context );
            if (!m_context.HasErrorOccured())
            {
                m_context.ResolvePendingTypeDefs();
            }
        }
        else
        {
            switch ( result )
            {
                case CXError_Failure:
                m_context.LogError("Clang Unknown failure");
                break;

                case CXError_Crashed:
                m_context.LogError( "Clang crashed" );
                break;

                case CXError_InvalidArguments:
                m_context.LogError("Clang Invalid arguments" );
                break;

                case CXError_ASTReadError:
                m_context.LogError("Clang AST read error" );
                break;
			case CXError_Success:
				break;
			}
        }
        clang_disposeIndex(idx);

        //-------------------------------------------------------------------------

        if (!m_context.HasErrorOccured())
        {
            m_context.CheckForOrphanedReflectionMacros();
        }

        // If we have an error from the parser, prepend the header to it
        if (m_context.HasErrorOccured())
        {
            m_context.LogError("\n{0}", m_context.GetErrorMessage());
        }

        return !m_context.HasErrorOccured();
    }
}
