#pragma once

#include "Editor/Resource/ResourceCompiler.h"
//#include "Runtime/Resource/Assets/Texture.h"

//-------------------------------------------------------------------------

namespace SGE::Editor
{
    SE_ENUM(TextureImportType)
	enum class TextureImportType
    {
        Default,
        AmbientOcclusion,
        TangentSpaceNormals,
        Uncompressed
    };

    //-------------------------------------------------------------------------

    struct SE_API_EDITOR TextureResourceDescriptor : public ResourceDescriptor
    {
        SE_CLASS(TextureResourceDescriptor, ResourceDescriptor)

        virtual bool IsValid() const override { return resPath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual TypeID GetCompiledResourceTypeID() const override { return TypeID(); }

        virtual void GetCompileDependencies(List<ResPath>& outDependencies ) override
        {
            if ( resPath.IsValid() )
            {
                outDependencies.Add( resPath);
            }
        }

        virtual void Clear() override
        {
			resPath.Clear();
			importType = TextureImportType::Default;
            name.Clear();
        }
		void Serialize(SerializeStream& stream, const void* otherObj) override;
		void Deserialize(DeserializeStream& stream) override;

	public:

        SE_PROPERTY()
        ResPath      resPath;
        SE_PROPERTY()
        TextureImportType importType = TextureImportType::Default;
        SE_PROPERTY()
        String       name; // Optional: needed for extracting textures out of container files (e.g. glb, fbx)
    };

    //-------------------------------------------------------------------------

    // struct SE_API_EDITOR CubemapTextureResourceDescriptor : public ResourceDescriptor
    // {
    //     SE_CLASS(CubemapTextureResourceDescriptor)

    //     virtual bool IsValid() const override { return m_path.IsValid(); }
    //     virtual bool IsUserCreateableDescriptor() const override { return true; }
    //     virtual ResTypeID GetCompiledResourceTypeID() const override { return CubemapTexture::GetStaticResourceTypeID(); }

    //     virtual void GetCompileDependencies(Vector<ResPath>& outDependencies ) override
    //     {
    //         if ( m_path.IsValid() )
    //         {
    //             outDependencies.emplace_back( m_path );
    //         }
    //     }

    //     virtual void Clear() override
    //     {
    //         m_path.Clear();
    //     }

    // public:

    //     SE_PROPERTY() ResPath     m_path;
    // };

    class TextureCompiler : public Compiler
    {
        SE_CLASS( TextureCompiler, Compiler)
        static const int32 s_version = 11;

    public:

        TextureCompiler();
        virtual CompilationResult Compile(CompileContext const& ctx ) const override;

    private:

        CompilationResult CompileTexture(CompileContext const& ctx ) const;
        // CompilationResult CompileCubemapTexture(CompileContext const& ctx ) const;
    };
}