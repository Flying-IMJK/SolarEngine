#include "ClangParser.h"
#include "ClangVisitors.h"
#include "../ReflectorSettingsAndUtils.h"
#include "../Database/ReflectionDatabase.h"
#include "Core/Utilities/Timers.h"
#include <fstream>

#include "Core/Types/DateTime.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    ClangParser::ClangParser( SolutionInfo* pSolution, ReflectionDatabase* pDatabase, String const& reflectionDataPath )
        : m_context( pSolution, pDatabase )
        , m_totalParsingTime( 0 )
        , m_totalVisitingTime( 0 )
        , m_reflectionDataPath( reflectionDataPath )
    {}

    bool ClangParser::Parse(List<HeaderInfo*> const& headers)
    {
        // Create single amalgamated header file for all headers to parse
        //-------------------------------------------------------------------------

        std::ofstream reflectorFileStream;
		StringAnsi const reflectorHeader = m_reflectionDataPath.ToStringAnsi() + "Reflector.h";
        reflectorFileStream.open( reflectorHeader.Get(), std::ios::out | std::ios::trunc );
        ENGINE_ASSERT( !reflectorFileStream.fail() );

		StringAnsi includeStr;
        m_context.headersToVisit.Clear();
        for ( HeaderInfo const* pHeader : headers )
        {
            m_context.headersToVisit.Add( ClangParserContext::HeaderToVisit(pHeader->headerId, pHeader) );
            includeStr += "#include \"" + pHeader->filePath + "\"\n";
        }

        reflectorFileStream.write( includeStr.Get(), includeStr.Length() );
        reflectorFileStream.close();

        // Clang args
        List<StringAnsi> fullIncludePaths(10);
		List<char const*> clangArgs;
        int32_t const numIncludePaths = sizeof( Settings::g_includePaths ) / sizeof( Settings::g_includePaths[0] );
        for ( auto i = 0; i < numIncludePaths; i++ )
        {
			String const fullPath = m_context.pSolution->path + SE_TEXT("/") + Settings::g_includePaths[i];
//			StringAnsi const shortPath = Platform::GetShortPath( fullPath );
            fullIncludePaths.Add( "-I" + fullPath.ToStringAnsi() );
            clangArgs.Add( fullIncludePaths.Last().Get() );

            if ( !FileSystem::DirectoryExists( fullPath))
            {
                m_context.LogError(SE_TEXT("Invalid include path: {}"), fullPath.Get() );
                return false;
            }
        }

        clangArgs.Add( "-x" );
        clangArgs.Add( "c++" );
        clangArgs.Add( "-std=c++17" );
        clangArgs.Add( "-O0" );
        clangArgs.Add( "-D NDEBUG" );
		clangArgs.Add( "-D PLATFORM_WINDOWS" );
		clangArgs.Add( "-D PLATFORM_WIN32" );
        clangArgs.Add( "-Werror" );
        clangArgs.Add( "-Wno-deprecated-builtins" );
        clangArgs.Add( "-fparse-all-comments" );
        clangArgs.Add( "-fms-extensions" );
        clangArgs.Add( "-fms-compatibility" );
        clangArgs.Add( "-Wno-unknown-warning-option" );
        clangArgs.Add( "-Wno-return-type-c-linkage" );
        clangArgs.Add( "-Wno-gnu-folding-constant" );
        clangArgs.Add( "-Wno-nonportable-include-path" );

        //-------------------------------------------------------------------------

        // Set up clang
        auto idx = clang_createIndex( 0, 1 );
        uint32_t const clangOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;

        // Parse Headers
        CXTranslationUnit tu;
        CXErrorCode result = CXError_Failure;
        {
            ScopedTimer<PlatformClock> timer( m_totalParsingTime );
            result = clang_parseTranslationUnit2( idx, reflectorHeader.Get(), clangArgs.Get(), clangArgs.Count(), 0, 0, clangOptions, &tu );
        }

        // Handle result of parse
        if (result == CXError_Success)
        {
            ScopedTimer<PlatformClock> timer(m_totalVisitingTime);
            m_context.Reset( &tu );
            const auto cursor = clang_getTranslationUnitCursor(tu);
            clang_visitChildren( cursor, VisitTranslationUnit, &m_context );
        }
        else
        {
            switch ( result )
            {
                case CXError_Failure:
                m_context.LogError(SE_TEXT("Clang Unknown failure"));
                break;

                case CXError_Crashed:
                m_context.LogError( SE_TEXT("Clang crashed") );
                break;

                case CXError_InvalidArguments:
                m_context.LogError(SE_TEXT("Clang Invalid arguments") );
                break;

                case CXError_ASTReadError:
                m_context.LogError(SE_TEXT("Clang AST read error") );
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
            m_context.LogError(SE_TEXT("\n{0}"), m_context.GetErrorMessage());
        }

        return !m_context.HasErrorOccured();
    }
}