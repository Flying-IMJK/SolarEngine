#include "ResourceCompiler.h"
#include "Core/Platform/FileSystem.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
    CompileContext::CompileContext( String const& rawResourceDirectoryPath, String const& compiledResourceDirectoryPath, ResID const& resourceToCompile, bool isCompilingForShippingBuild )
        : rawResourceDirectoryPath( rawResourceDirectoryPath )
        , compiledResourceDirectoryPath( compiledResourceDirectoryPath )
        , isCompilingForPackagedBuild( isCompilingForShippingBuild )
        , resourceID( resourceToCompile )
    {
        ENGINE_ASSERT(FileSystem::IsDirectory(rawResourceDirectoryPath) && FileSystem::DirectoryExists( rawResourceDirectoryPath ) && resourceToCompile.IsValid() );

        // Resolve paths
        /*ResPath const& resourceToCompilePath = resourceToCompile.GetResourcePath();
        const_cast<String&>( inputFilePath ) = ResPath::ToFileSystemPath( rawResourceDirectoryPath, resourceToCompilePath );
        const_cast<String&>( outputFilePath ) = ResPath::ToFileSystemPath( compiledResourceDirectoryPath, resourceToCompilePath );*/
    }

    bool CompileContext::IsValid() const
    {
        if ( !inputFilePath.IsEmpty() || !outputFilePath.IsEmpty() || !resourceID.IsValid() )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void Compiler::Initialize(String const& rawResourceDirectoryPath )
    {
        m_rawResourceDirectoryPath = rawResourceDirectoryPath;
    }

    void Compiler::Shutdown()
    {
        m_rawResourceDirectoryPath.Clear();
    }


    CompilationResult Compiler::CompilationSucceeded( CompileContext const& ctx ) const
    {
        return Message(SE_TEXT("Compiled '{0}' to '{1}' successfully"), ctx.inputFilePath, ctx.outputFilePath );
    }

    CompilationResult Compiler::CompilationSucceededWithWarnings( CompileContext const& ctx ) const
    {
        return Warning(SE_TEXT("Compiled '{0}' to '{1}' successfully (with warnings)"), ctx.inputFilePath, ctx.outputFilePath );
    }

    CompilationResult Compiler::CompilationFailed( CompileContext const& ctx ) const
    {
        return Error(SE_TEXT("Failed to compile resource: '{0}'"), ctx.outputFilePath );
    }


	CompilerRegistry::CompilerRegistry(String const& rawResourceDirectoryPath )
	{
		List<TypeInfo const*> compilerTypes = Types::GetAllDerivedTypes( Compiler::GetStaticTypeID(),
			false, false, true );

		for ( auto pCompilerType : compilerTypes )
		{
			auto pCreatedCompiler = Cast<Compiler>( pCompilerType->CreateType() );
			pCreatedCompiler->Initialize(rawResourceDirectoryPath );
			m_compilers.Add(pCreatedCompiler);
			RegisterCompiler(pCreatedCompiler);
		}
	}

	CompilerRegistry::~CompilerRegistry()
	{
		for ( auto& pCompiler : m_compilers )
		{
			UnregisterCompiler( pCompiler );
			Delete( const_cast<Compiler*>(pCompiler));
		}

		ENGINE_ASSERT( m_compilerTypeMap.IsEmpty() );
	}

	void CompilerRegistry::RegisterCompiler( Compiler const* pCompiler )
	{
		ENGINE_ASSERT( pCompiler != nullptr );
		ENGINE_ASSERT( m_compilers.Contains(pCompiler ) );

		//-------------------------------------------------------------------------

		auto const& resourceTypes = pCompiler->GetOutputTypes();
		for ( auto& type : resourceTypes )
		{
			// Two compilers registering for the same resource type is not allowed
			ENGINE_ASSERT( !m_compilerTypeMap.ContainsKey( type ));
			m_compilerTypeMap.Add( type, pCompiler );
		}
	}

	void CompilerRegistry::UnregisterCompiler( Compiler const* pCompiler )
	{
		ENGINE_ASSERT( pCompiler != nullptr );
		ENGINE_ASSERT( m_compilers.Contains(pCompiler ) );

		//-------------------------------------------------------------------------

		auto const& resourceTypes = pCompiler->GetOutputTypes();
		for ( auto& type : resourceTypes )
		{
			m_compilerTypeMap.Remove( type );
		}
	}
}