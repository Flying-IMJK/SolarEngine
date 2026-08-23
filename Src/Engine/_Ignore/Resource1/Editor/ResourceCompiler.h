#pragma once

#include "ResourceDescriptor.h"
#include "Core/TypeSystem/IReflectedType.h"
#include "Core/Platform/FileSystem.h"

#include "Core/Types/Delegate.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
    enum class CompilationResult : int32
    {
        Failure = -1,
        SuccessUpToDate = 0,
        Success = 1,
        SuccessWithWarnings = 2,
    };

    // Log Delimiter
    //-------------------------------------------------------------------------

    struct SE_API_EDITOR CompilationLog
    {
        constexpr static Char const* const s_delimiter = SE_TEXT("Esoterica Resource Compiler\n");
    };

    // Context for a single compilation operation
    //-------------------------------------------------------------------------

    struct SE_API_EDITOR CompileContext
    {
        CompileContext( String const& rawResourceDirectoryPath,
			String const& compiledResourceDirectoryPath,
			ResID const& resourceToCompile,
			bool isCompilingForShippingBuild );

        bool IsValid() const;

        inline bool IsCompilingForDevelopmentBuild() const { return !isCompilingForPackagedBuild; }
        inline bool IsCompilingForPackagedBuild() const { return isCompilingForPackagedBuild; }

    public:
        String const                          rawResourceDirectoryPath;
        String const                          compiledResourceDirectoryPath;
        bool                                  isCompilingForPackagedBuild = false;

        ResID  const                          resourceID;
        String const                          inputFilePath;
        String const                          outputFilePath;

        uint64                                sourceResourceHash = 0; // The combined hash of the source resource and its dependencies
    };

    // Resource Compiler
    //-------------------------------------------------------------------------

    class SE_API_EDITOR Compiler : public IReflectedType
    {
        SE_CLASS( Compiler, IReflectedType);

    public:

        Compiler( String const& name, int32 version ) : m_version( version ), m_name( name ) {}
        virtual ~Compiler() {}
        virtual CompilationResult Compile( CompileContext const& ctx ) const = 0;

        String const& GetName() const { return m_name; }
        inline int32 GetVersion() const { return m_version; }

        void Initialize(String const& rawResourceDirectoryPath);
        void Shutdown();

        // The list of resource type we can compile
        virtual List<TypeID> const& GetOutputTypes() const { return m_outputTypes; }

        // Does this compiler actually require the input file or is it optional.
        virtual bool IsInputFileRequired() const { return true; }

        // Get all referenced resources needed at runtime
        virtual bool GetInstallDependencies( ResID  const& resourceID, List<ResID >& outReferencedResources ) const { return true; }

    protected:

        Compiler& operator=( Compiler const& ) = delete;

        template<typename... T>
        CompilationResult Error(Char const* pFormat, T &... args) const
        {
			LOG_ERROR("ResourceCompiler", "{0}{1}", m_name, String::Format(pFormat, args...));
            return CompilationResult::Failure;
        };

        template<typename... T>
        CompilationResult Warning(Char const* pFormat, T &&... args) const
        {
			LOG_WARNING("ResourceCompiler", "{0}{1}", m_name, String::Format(pFormat, args...));
            return CompilationResult::SuccessWithWarnings;
        };

        template<typename... T>
        CompilationResult Message(Char const* pFormat, T &&... args) const
        {
			LOG_INFO("ResourceCompiler", "{0}{1}", m_name, String::Format(pFormat, args...));
            return CompilationResult::Success;
        };

        CompilationResult CompilationSucceeded( CompileContext const& ctx ) const;
        CompilationResult CompilationSucceededWithWarnings( CompileContext const& ctx ) const;
        CompilationResult CompilationFailed( CompileContext const& ctx ) const;

        // Utilities
        //-------------------------------------------------------------------------

        // Converts a resource path to a file path
        inline bool ConvertResourcePathToFilePath( ResPath const& resourcePath, String& filePath ) const
        {
            if ( resourcePath.IsValid() )
            {
                filePath = ResPath::ToFileSystemPath( m_rawResourceDirectoryPath, resourcePath );
                return true;
            }
            else
            {
				LOG_ERROR("ResourceCompiler", "Failed to convert resource path to file system path: '{0}'", resourcePath.GetString());
                return false;
            }
        }

        // Converts a file path to a resource path
        inline bool ConvertFilePathToResourcePath( String const& filePath, ResPath& resourcePath ) const
        {
            if ( resourcePath.IsValid() )
            {
                resourcePath = ResPath::FromFileSystemPath( m_rawResourceDirectoryPath, filePath );
                return true;
            }
            else
            {
				LOG_ERROR( "ResourceCompiler", "Failed to convert file system path to resource path: '{0}'", filePath);
                return false;
            }
        }

        // Try to load a resource descriptor from a resource path
        // Optional: returns the json document read for the descriptor if you need to read additional data from it
        template<typename T>
        bool TryLoadResourceDescriptor(String const& descriptorFilePath, T& outDescriptor, Json::Document* pOutOptionalDescriptorDocument = nullptr ) const
        {
            if ( !descriptorFilePath.IsEmpty() )
            {
				LOG_ERROR("ResourceCompiler", "Invalid descriptor file path provided!" );
                return false;
            }

            if ( !FileSystem::FileExists(descriptorFilePath ) )
            {
				LOG_ERROR("ResourceCompiler", "Descriptor file doesnt exist: {0}", descriptorFilePath);
                return false;
            }

			StringAnsi context;
			if (!File::ReadAllText(descriptorFilePath, context))
			{
				LOG_ERROR( "Resource", "Resource Descriptor Failed to read resource descriptor file: {0}", descriptorFilePath);
				return false;
			}

			Json::Document stream;
			stream.Parse(context.Get(), context.Length());

			if (stream.HasParseError())
			{
				LOG_ERROR( "Resource", "Resource Descriptor file failed to parse : {0}", descriptorFilePath);
				return false;
			}

			String typeName;
			DESERIALIZE_MEMBER(TypeID, typeName);
			TypeID typeId(typeName);
			if (outDescriptor->GetType() != typeId)
			{
				LOG_ERROR( "Resource", "Resource Descriptor file type mismatch : {0}", descriptorFilePath);
				return false;
			}

			outDescriptor.Deserialize(stream);

            // Make a copy of the json document to read further data from
            if ( pOutOptionalDescriptorDocument != nullptr )
            {
                pOutOptionalDescriptorDocument->CopyFrom(stream, pOutOptionalDescriptorDocument->GetAllocator(), true);
            }

            return true;
        }
        
        // Try to load a resource descriptor from a resource path
        // Optional: returns the json document read for the descriptor if you need to read additional data from it
        template<typename T>
        bool TryLoadResourceDescriptor( ResPath const& descriptorResourcePath, T& outDescriptor, rapidjson::Document* pOutOptionalDescriptorDocument = nullptr ) const
        {
            if ( !descriptorResourcePath.IsValid() )
            {
				LOG_ERROR("ResourceCompiler",  "Invalid descriptor resource path provided!" );
                return false;
            }

            String descriptorFilePath;
            if ( !ConvertResourcePathToFilePath( descriptorResourcePath, descriptorFilePath ) )
            {
				LOG_ERROR("ResourceCompiler",  "Invalid descriptor resource path: {0}", descriptorResourcePath.c_str() );
                return false;
            }

            return TryLoadResourceDescriptor( descriptorFilePath, outDescriptor, pOutOptionalDescriptorDocument );
        }

        // Combines two results together and keeps the most severe one
        inline CompilationResult CombineResultCode( CompilationResult a, CompilationResult b ) const
        {
            if ( a == Editor::CompilationResult::Failure || b == Editor::CompilationResult::Failure )
            {
                return Editor::CompilationResult::Failure;
            }

            if ( a == Editor::CompilationResult::SuccessWithWarnings || b == Editor::CompilationResult::SuccessWithWarnings )
            {
                return Editor::CompilationResult::SuccessWithWarnings;
            }

            if ( a == Editor::CompilationResult::SuccessUpToDate || b == Editor::CompilationResult::SuccessUpToDate )
            {
                return Editor::CompilationResult::SuccessUpToDate;
            }

            return Editor::CompilationResult::Success;
        }

    protected:
        String                              m_rawResourceDirectoryPath;
        int32 const                                   m_version;
        String const                                  m_name;
		List<TypeID>                         m_outputTypes;
    };


	class SE_API_EDITOR CompilerRegistry
	{
	public:

		CompilerRegistry(String const& rawResourceDirectoryPath);
		~CompilerRegistry();

		//-------------------------------------------------------------------------

		inline List<Compiler const*> const& GetRegisteredCompilers() const { return m_compilers; }

		inline bool HasCompilerForResourceType( TypeID const& typeID ) const { return m_compilerTypeMap.Find( typeID ) != m_compilerTypeMap.end(); }

		inline Compiler const* GetCompilerForResourceType( TypeID const& typeID ) const
		{
			auto compilerTypeIter = m_compilerTypeMap.Find( typeID );
			if ( compilerTypeIter != m_compilerTypeMap.end() )
			{
				return compilerTypeIter->Value;
			}

			return nullptr;
		}

		inline int32 GetVersionForType( TypeID const& typeID ) const
		{
			auto pCompiler = GetCompilerForResourceType( typeID );
			ENGINE_ASSERT( pCompiler != nullptr );
			return pCompiler->GetVersion();
		}

	private:

		void RegisterCompiler( Compiler const* pCompiler );
		void UnregisterCompiler( Compiler const* pCompiler );

	private:

		List<Compiler const*>                       	m_compilers;
		Dictionary<TypeID, Compiler const*>    m_compilerTypeMap;
	};
}