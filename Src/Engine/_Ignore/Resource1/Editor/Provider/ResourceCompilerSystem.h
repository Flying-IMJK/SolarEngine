#pragma once

#include "Editor/Resource/ResourceCompiler.h"
#include "CompiledResourceDatabase.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Types/Collections/List.h"
#include "ResourceServerContext.h"

namespace SGE::Editor
{
	class CompilerRegistry;


    //-------------------------------------------------------------------------
    struct ResourceCompilerArgument
    {
    public:
        ResID           	resourceID;
        bool                triggerDebugBreak = false;
        bool                isForPackagedBuild = false;
        bool                isForcedCompilation = false;
        bool                isValid = false;
        String			    compiledResourceDatabasePath;

        void SetResourcePath(String& path)
        {
            ResPath const resourcePath(path);
            if ( resourcePath.IsValid() )
            {
				/*resourceID = ResID ( resourcePath );

                if ( resourceID.IsValid() )
                {
					isValid = true;
                }
                else
                {
                    LOG_ERROR("Resource", "Resource Compiler Invalid compile request: {0}\n", resourceID.ToString());
                }*/
            }
        }

        bool IsValid() const { return isValid; }
    };


    class ResourceCompilerSystem
    {
        struct CompileDependencyNode
        {
            void Reset();
            void DestroyDependencies();

            bool IsCompileableResource() const { return m_compilerVersion >= 0; }
            bool IsUpToDate() const;

        public:
            ResID                               	m_ID;
            String                        			m_sourcePath;
			String                        			m_targetPath;
            bool                                    m_sourceExists = false;
            bool                                    m_targetExists = false;
            bool                                    m_errorOccurredReadingDependencies = true;
            bool                                    m_forceRecompile = false;
            int32                                   m_compilerVersion = -1;
            CompiledResourceRecord                  m_compiledRecord;
            uint64                                  m_timestamp = 0;
            uint64                                  m_combinedHash = 0;

            CompileDependencyNode*                  m_pParentNode = nullptr;
            List<CompileDependencyNode*>          	m_dependencies;
        };

    public:

        static bool ShouldCheckCompileDependenciesForResourceType( ResID  const& resourceID );

    public:

        ResourceCompilerSystem();
        ~ResourceCompilerSystem();

        bool Initialize(ResourceServerContext const& context, ResourceCompilerArgument& argument);
        void Shutdown();

        Editor::CompilationResult Compile();

    private:

        bool BuildCompileDependencyTree(ResID  const& resourceID );
        bool TryReadCompileDependencies(ResID  const& resourceID, List<ResPath>& outDependencies );
        bool FillCompileDependencyNode(CompileDependencyNode* pNode, ResPath const& resourceID);

    private:
		ResourceServerContext const*			m_ServerContext;
        CompiledResourceDatabase                m_compiledResourceDB;
        Editor::CompileContext*                 m_pCompileContext = nullptr;
        bool                                    m_forceCompilation = false;

        List<ResID >                      		m_uniqueCompileDependencies;
        CompileDependencyNode                   m_compileDependencyTreeRoot;
        String                                  m_errorMessage;
    };
}