#include "TextureImport.h"
#include "Editor/Resource/Import/Importer.h"
#include "Editor/Resource/Import/ImportedImage.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Serialization/FileWriteStream.h"
#include "Runtime/Graphics/Base/PixelFormat.h"

#include <ispc_texcomp.h>
#include <dxgiformat.h>

//-------------------------------------------------------------------------

namespace SGE::Editor
{
	void TextureResourceDescriptor::Serialize(ISerializable::SerializeStream& stream, const void* otherObj)
	{
		stream.Key(SE_TEXT("ResPath"));
		stream.String(resPath.GetString());
		stream.Key(SE_TEXT("TextureType"));
		stream.Int((int )importType);
	}
	void TextureResourceDescriptor::Deserialize(ISerializable::DeserializeStream& stream)
	{
		resPath = ResPath(stream.GetString());
		importType = (TextureImportType)stream.GetInt();
	}


    TextureCompiler::TextureCompiler()
        : Compiler(SE_TEXT("TextureCompiler"), s_version)
    {
//        m_outputTypes.Add(RHITexture::GetStaticResourceTypeID());
        // m_outputTypes.push_back(CubemapTexture::GetStaticResourceTypeID());
    }

    CompilationResult TextureCompiler::Compile(CompileContext const &ctx) const
    {
//        if (ctx.m_resourceID.GetResourceTypeID() == RHITexture::GetStaticResourceTypeID())
//        {
//            return CompileTexture(ctx);
//        }
        // else if (ctx.m_resourceID.GetResourceTypeID() == CubemapTexture::GetStaticResourceTypeID())
        // {
        //     return CompileCubemapTexture(ctx);
        // }

		return CompilationResult::Failure;
    }

    CompilationResult TextureCompiler::CompileTexture(CompileContext const &ctx) const
    {
        TextureResourceDescriptor resourceDescriptor;
        if (!ResourceDescriptor::TryReadFromFile(ctx.inputFilePath, resourceDescriptor))
        {
            return Error(SE_TEXT("Failed to read resource descriptor from input file: {0}"), ctx.inputFilePath);
        }

        //-------------------------------------------------------------------------

		ResPath resPath = resourceDescriptor.resPath;
        String textureFilePath;
        if (!ConvertResourcePathToFilePath(resPath, textureFilePath))
        {
            return Error(SE_TEXT("Invalid texture data path: {0}"), resPath.GetString());
        }

        // Try to load the texture file
        //-------------------------------------------------------------------------

        Import::ReaderContext readerCtx =
		{
		{[this](Char const *pString){ Warning(pString); }},
		{[this](Char const *pString){ Error(pString); }}
		};
        Scope<Import::ImportedImage> importedImage = Import::ReadImage(readerCtx, textureFilePath);

        rgba_surface surface;
        surface.ptr = const_cast<uint8 *>(importedImage->GetImageData());
        surface.width = importedImage->GetWidth();
        surface.height = importedImage->GetHeight();
        surface.stride = importedImage->GetStride();

        // Run texture compression
        //-------------------------------------------------------------------------

        auto GetBytesPerBlock = [](DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC4_UNORM:
                return 8;

            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
            case DXGI_FORMAT_BC6H_UF16:
                return 16;

            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return 0;

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return 0;
            }
            }
        };

        

        auto IntegerDivideCeiling = [](int n, int d)
        {
            return (n + d - 1) / d;
        };

        int32 const blockSize = 4;

        List<uint8> compressedData;

        int32 const numCols = IntegerDivideCeiling(surface.width, blockSize);
        int32 const numRows = IntegerDivideCeiling(surface.height, blockSize);
        int32 const numBlocks = numCols * numRows;
        int32 bytesPerBlock = 0;

        PixelFormat compressedFormat;
        switch (resourceDescriptor.importType)
        {
        case TextureImportType::AmbientOcclusion:
        {
            compressedFormat = PixelFormat::BC4_UNorm;
            bytesPerBlock = PixelFormatGetBlockSize(compressedFormat);
            compressedData.Resize(numBlocks * bytesPerBlock);

            CompressBlocksBC4(&surface, compressedData.Get());
        }
        break;

        case TextureImportType::TangentSpaceNormals:
        {
            compressedFormat = PixelFormat::BC5_UNorm;
            bytesPerBlock = PixelFormatGetBlockSize(compressedFormat);
            compressedData.Resize(numBlocks * bytesPerBlock);

            CompressBlocksBC5(&surface, compressedData.Get());
        }
        break;

        case TextureImportType::Uncompressed:
        {
            compressedFormat = PixelFormat::BC7_UNorm;
            bytesPerBlock = PixelFormatGetBlockSize(compressedFormat);
            compressedData.Resize(numBlocks * bytesPerBlock);

            bc7_enc_settings encoderSettings;
            GetProfile_alpha_veryfast(&encoderSettings);
            CompressBlocksBC7(&surface, compressedData.Get(), &encoderSettings);
        }
        break;

        default:
        {
            compressedFormat = PixelFormat::R8G8B8A8_UNorm_SRGB;
            uint64 const requiredSize = 4 * surface.width * surface.height;
            compressedData.Resize(4 * surface.width * surface.height);
            memcpy(compressedData.Get(), surface.ptr, requiredSize);
        
        }
        break;
        }

        // Create DDS container and store it in the texture
        //-------------------------------------------------------------------------

/*        GPUT::Desc texDesc;
        texDesc.width = surface.width;
        texDesc.height = surface.height;
        texDesc.format = compressedFormat;
        texDesc.arraySize = 1;
        texDesc.depth = 1;
        texDesc.mipLevels = 1;
        texDesc.type = RHITexture::Type::_2D;

        RHITexture compressedTexture;
        compressedTexture.type = RHIResource::Type::Texture;
        compressedTexture.desc = texDesc;
        compressedTexture.mapped_data = compressedData.Get();
        compressedTexture.mapped_size = compressedData.Count();*/

        //-------------------------------------------------------------------------

//        ResourceHeader hdr(s_version, RHITexture::GetStaticResourceTypeID(), ctx.m_sourceResourceHash);

		FileWriteStream* stream = FileWriteStream::Open(ctx.outputFilePath);
		if (stream != nullptr)
		{
			Delete(stream);
			return CompilationSucceeded(ctx);
		}
		else
		{
			Delete(stream);
			return CompilationFailed(ctx);
		}
    }






    // CompilationResult TextureCompiler::CompileCubemapTexture(CompileContext const &ctx) const
    // {
    //     CubemapTextureResourceDescriptor resourceDescriptor;
    //     if (!ResourceDescriptor::TryReadFromFile(*m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor))
    //     {
    //         return Error("Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str());
    //     }

    //     // Create cubemap
    //     //-------------------------------------------------------------------------

    //     CubemapTexture texture;
    //     texture.m_format = TextureFormat::DDS;

    //     FileSystem::Path const sourceTexturePath = resourceDescriptor.m_path.ToFileSystemPath(m_rawResourceDirectoryPath);
    //     if (!FileSystem::Exists(sourceTexturePath))
    //     {
    //         return Error("Failed to open specified source file: %s", sourceTexturePath.c_str());
    //     }

    //     if (!FileSystem::LoadFile(sourceTexturePath, texture.m_rawData))
    //     {
    //         return Error("Failed to read specified source file: %s", sourceTexturePath.c_str());
    //     }

    //     //-------------------------------------------------------------------------

    //     ResourceHeader hdr(s_version, Texture::GetStaticResourceTypeID(), ctx.m_sourceResourceHash);

    //     Serialization::BinaryOutputArchive archive;
    //     archive << hdr << texture;

    //     if (archive.WriteToFile(ctx.m_outputFilePath))
    //     {
    //         return CompilationSucceeded(ctx);
    //     }
    //     else
    //     {
    //         return CompilationFailed(ctx);
    //     }
    // }

}