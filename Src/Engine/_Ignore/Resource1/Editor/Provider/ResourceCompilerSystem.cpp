#include "ResourceCompilerSystem.h"
#include "CompiledResourceDatabase.h"
#include "Editor/Resource/ResourceCompiler.h"
#include "Runtime/EngineContext.h"
#include "Core/Platform/FileSystem.h"

//-------------------------------------------------------------------------
// Resource Compiler
//-------------------------------------------------------------------------

namespace SGE::Editor
{
    void ResourceCompilerSystem::CompileDependencyNode::Reset()
    {
        m_ID.Clear();
        m_compiledRecord.Clear();
        m_sourcePath.Clear();
        m_targetPath.Clear();
        m_timestamp = m_combinedHash = 0;
        m_sourceExists = m_targetExists = false;
        m_errorOccurredReadingDependencies = false;
        m_compilerVersion = -1;
        DestroyDependencies();
    }

    void ResourceCompilerSystem::CompileDependencyNode::DestroyDependencies()
    {
        for ( auto pDep : m_dependencies )
        {
            pDep->DestroyDependencies();
            Delete( pDep );
        }

        m_dependencies.Clear();
    }

    bool ResourceCompilerSystem::CompileDependencyNode::IsUpToDate() const
    {
        if ( m_forceRecompile )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( !m_sourceExists )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( IsCompileableResource() )
        {
            if ( !m_targetExists )
            {
                return false;
            }

            if ( !m_compiledRecord.IsValid() )
            {
                return false;
            }

            if ( m_compiledRecord.compilerVersion != m_compilerVersion )
            {
                return false;
            }

            if ( m_compiledRecord.sourceTimestampHash != m_combinedHash )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& pDep : m_dependencies )
        {
            if ( !pDep->IsUpToDate() )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    ResourceCompilerSystem::ResourceCompilerSystem()
    {}

    ResourceCompilerSystem::~ResourceCompilerSystem()
    {
        ENGINE_ASSERT( m_pCompileContext == nullptr );
    }

    bool ResourceCompilerSystem::Initialize(ResourceServerContext const& context, ResourceCompilerArgument& argument)
    {
        // Connect database
        //-------------------------------------------------------------------------
        if ( !m_compiledResourceDB.Connect(context.compiledResourceDatabasePath))
        {
            LOG_ERROR("Resource", "Resource Compiler Database connection error: {0}", m_compiledResourceDB.GetError());
            return false;
        }

        // Setup compile context
        //-------------------------------------------------------------------------
		m_forceCompilation = argument.isForcedCompilation;

		m_pCompileContext = New<Editor::CompileContext>(context.rawResourcePath,
			argument.isForPackagedBuild ? context.packagedBuildCompiledResourcePath : context.compiledResourcePath,
			argument.resourceID, argument.isForPackagedBuild );

		m_ServerContext = &context;

        //-------------------------------------------------------------------------

        return true;
    }

    void ResourceCompilerSystem::Shutdown()
    {
        m_compileDependencyTreeRoot.DestroyDependencies();
      
        Delete(m_pCompileContext);

        if ( m_compiledResourceDB.IsConnected() )
        {
            m_compiledResourceDB.Disconnect();
        }
    }

    bool ResourceCompilerSystem::ShouldCheckCompileDependenciesForResourceType( ResID  const& resourceID )
    {
/*        if ( resourceID.GetResourceTypeID() == ResTypeID( "map" ) )
        {
            return false;
        }

        if ( resourceID.GetResourceTypeID() == ResourceTypeID( "nav" ) )
        {
            return false;
        }*/

        return true;
    }

    Editor::CompilationResult ResourceCompilerSystem::Compile()
    {
        /*if ( !m_compiledResourceDB.IsConnected() )
        {
            LOG_ERROR("Resource", "Resource Compiler Database connection error: {0}", m_compiledResourceDB.GetError());
            return Editor::CompilationResult::Failure;
        }

        // Try create compilation context
        if ( m_pCompileContext != nullptr && !m_pCompileContext->IsValid() )
        {
            return Editor::CompilationResult::Failure;
        }

        // Try find compiler
        auto pCompiler = m_ServerContext->pCompiler->GetCompilerForResourceType( m_pCompileContext->resourceID.GetResourceTypeID() );
        if ( pCompiler == nullptr )
        {
            LOG_ERROR("Resource", "Resource Compiler Cant find appropriate resource compiler for type: {0}", m_pCompileContext->resourceID.GetResourceTypeID() );
            return Editor::CompilationResult::Failure;
        }

        // Validate request
        //-------------------------------------------------------------------------

        // Validate input path
        if ( pCompiler->IsInputFileRequired() && !FileSystem::FileExists( m_pCompileContext->inputFilePath ) )
        {
            LOG_ERROR("Resource", "Resource Compiler Source file for data path ('{0}') does not exist: '{0}'\n", m_pCompileContext->rawResourceDirectoryPath, m_pCompileContext->inputFilePath);
            return Editor::CompilationResult::Failure;
        }

        // Try create target directory
        if (!FileSystem::CreateDirectory(m_pCompileContext->outputFilePath))
        {
            LOG_ERROR("Resource", "Resource Compiler Destination path ({0}) doesnt exist!", m_pCompileContext->outputFilePath);
            return Editor::CompilationResult::Failure;
        }

        // Check that target file isnt read-only
        if (FileSystem::FileExists( m_pCompileContext->outputFilePath ) && FileSystem::IsReadOnly(m_pCompileContext->outputFilePath ) )
        {
            LOG_ERROR("Resource", "Resource Compiler: Destination file ({0}) is read-only!", m_pCompileContext->outputFilePath);
            return Editor::CompilationResult::Failure;
        }

        // Up-To-Date Check
        //-------------------------------------------------------------------------

        // Check compile dependency and if this resource needs compilation
        m_uniqueCompileDependencies.Clear();
        m_compileDependencyTreeRoot.Reset();
        if ( !FillCompileDependencyNode( &m_compileDependencyTreeRoot, m_pCompileContext->resourceID.GetResourcePath() ) )
        {
            LOG_ERROR("Resource", "Resource Compiler Failed to create dependency tree: {0}", m_errorMessage);
            return Editor::CompilationResult::Failure;
        }

        // If we are not forcing the compilation and we're up to date, there's nothing to do
        if ( m_compileDependencyTreeRoot.IsUpToDate() && !m_forceCompilation )
        {
            return Editor::CompilationResult::SuccessUpToDate;
        }

        m_pCompileContext->sourceResourceHash = m_compileDependencyTreeRoot.m_combinedHash;

        // Compile
        //-------------------------------------------------------------------------

        Editor::CompilationResult compilationResult = pCompiler->Compile(*m_pCompileContext);

        // Update database
        if ( compilationResult == Editor::CompilationResult::Success || compilationResult == Editor::CompilationResult::SuccessWithWarnings )
        {
            CompiledResourceRecord record;
            record.resourceID = m_pCompileContext->resourceID;
            record.compilerVersion = m_compileDependencyTreeRoot.m_compilerVersion;
            record.fileTimestamp = m_compileDependencyTreeRoot.m_timestamp;
            record.sourceTimestampHash = m_compileDependencyTreeRoot.m_combinedHash;
            m_compiledResourceDB.WriteRecord( record );
        }

        return compilationResult;*/
    }

    bool ResourceCompilerSystem::BuildCompileDependencyTree( ResID  const& resourceID )
    {
        ENGINE_ASSERT( resourceID.IsValid() );

        //-------------------------------------------------------------------------

        m_errorMessage.Clear();
        m_uniqueCompileDependencies.Clear();
        m_compileDependencyTreeRoot.Reset();
        return FillCompileDependencyNode( &m_compileDependencyTreeRoot, resourceID.GetResourcePath() );
    }

    bool ResourceCompilerSystem::TryReadCompileDependencies( ResID  const& resourceID, List<ResPath>& outDependencies )
    {
        ENGINE_ASSERT(resourceID.IsValid());

        if (resourceID.IsSubResourceID())
        {
            ResID  const parentResourceID = resourceID.GetParentResourceID();
			TypeID const parentResourceTypeID = parentResourceID.GetResourceTypeID();
/*            if ( !Types::IsRegisteredResourceType( parentResourceTypeID))
            {
                m_errorMessage = String::Format(SE_TEXT("Invalid parent resource type detected for: (0)"), resourceID.c_str());
                return false;
            }*/

            outDependencies.Add( parentResourceID.GetResourcePath() );
        }
        else
        {
            String const resourceFilePath = resourceID.ToFileSystemPath( m_pCompileContext->rawResourceDirectoryPath );

            auto pDescriptor = Editor::ResourceDescriptor::TryReadFromFile(resourceFilePath);
            if ( pDescriptor == nullptr )
            {
                return false;
            } 

            pDescriptor->GetCompileDependencies(outDependencies);

            Delete(pDescriptor);
        }

        return true;
    }

    bool ResourceCompilerSystem::FillCompileDependencyNode( CompileDependencyNode* pNode, ResPath const& resourcePath )
    {
        ENGINE_ASSERT( pNode != nullptr );
        ENGINE_ASSERT( resourcePath.IsValid() );

        // Basic resource info
        //-------------------------------------------------------------------------

        pNode->m_ID = resourcePath;
        pNode->m_sourcePath = ResPath::ToFileSystemPath( m_pCompileContext->rawResourceDirectoryPath, resourcePath );
        pNode->m_sourceExists = FileSystem::FileExists( pNode->m_sourcePath );
        pNode->m_timestamp = pNode->m_sourceExists ? FileSystem::GetFileLastEditTime(pNode->m_sourcePath).Ticks : 0;

        bool const isPotentiallyCompilableResource = pNode->m_ID.IsValid();// && Types::IsRegisteredResourceType(pNode->m_ID.GetResourceTypeID());
        bool skipDependencyCheck = true;

        // Handle compilable resources
        //-------------------------------------------------------------------------

        if (isPotentiallyCompilableResource)
        {
            ENGINE_ASSERT(pNode->m_ID.IsValid());

            Editor::Compiler const* pCompiler = m_ServerContext->pCompiler->GetCompilerForResourceType( pNode->m_ID.GetResourceTypeID());
            bool const isCompilableResource = pCompiler != nullptr;
            skipDependencyCheck = !isCompilableResource || !ShouldCheckCompileDependenciesForResourceType( pNode->m_ID );
            if ( isCompilableResource )
            {
                pNode->m_targetPath = ResPath::ToFileSystemPath( m_pCompileContext->compiledResourceDirectoryPath, resourcePath );
                pNode->m_targetExists = FileSystem::FileExists( pNode->m_targetPath );

                pNode->m_compilerVersion = pCompiler->GetVersion();
                m_compiledResourceDB.GetRecord( pNode->m_ID, pNode->m_compiledRecord );

                // Some compilers dont require an input file to run - these resources should always be recompiled!
                if ( !pNode->m_sourceExists && !pCompiler->IsInputFileRequired() )
                {
                    pNode->m_forceRecompile = true;
                    skipDependencyCheck = true;
                }
            }
        }

        // Generate dependencies
        //-------------------------------------------------------------------------

        if ( !skipDependencyCheck )
        {
            List<ResPath> dependencies;
            if ( TryReadCompileDependencies( resourcePath, dependencies ) )
            {
                for ( auto const& dependencyResourceID : dependencies )
                {
                    // Skip resources already in the tree!
                    if ( m_uniqueCompileDependencies.Contains(dependencyResourceID ) )
                    {
                        continue;
                    }

                    // Check for circular references
                    //-------------------------------------------------------------------------

                    auto pNodeToCheck = pNode;
                    while ( pNodeToCheck != nullptr )
                    {
                        if ( pNodeToCheck->m_ID == dependencyResourceID )
                        {
                            m_errorMessage = "Circular dependency detected!";
                            return false;
                        }

                        pNodeToCheck = pNodeToCheck->m_pParentNode;
                    }

                    // Create dependency
                    //-------------------------------------------------------------------------

                    auto pChildDependencyNode = New<CompileDependencyNode>();
						pNode->m_dependencies.Add(pChildDependencyNode);
                    pChildDependencyNode->m_pParentNode = pNode;
                    if ( !FillCompileDependencyNode( pChildDependencyNode, dependencyResourceID ) )
                    {
                        return false;
                    }

                    m_uniqueCompileDependencies.Add( dependencyResourceID );
                }
            }
            else
            {
                pNode->m_errorOccurredReadingDependencies = true;
                return false;
            }
        }

        // Generate combined hash
        //-------------------------------------------------------------------------

        pNode->m_combinedHash = pNode->m_timestamp;
        for ( auto const pDep : pNode->m_dependencies )
        {
            pNode->m_combinedHash += pDep->m_combinedHash;
        }

        return true;
    }
}