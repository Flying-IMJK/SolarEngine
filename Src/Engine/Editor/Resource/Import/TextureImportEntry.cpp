
#include "TextureImportEntry.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"

namespace SE::Editor
{
	TextureImportEntry::TextureImportEntry(ImportRequest& request) : ImportFileEntry(request)
	{
	    // Try to guess format type based on file name
	    String snl = FileSystem::GetFileNameWithoutExtension(SourceUrl);
	    snl.ToLower();

	    String extension = FileSystem::GetExtension(SourceUrl).ToLower();
	    /*if (extension == ".raw")
        {
            // Raw image data in 16bit gray-scale, preserve the quality
            _settings.Settings.Type = TextureFormatType.HdrRGBA;
            _settings.Settings.Compress = false;
        }
        else if (extension == ".exr")
        {
            // HDR image
            _settings.Settings.Type = TextureFormatType.HdrRGBA;
        }
        else if (extension == ".hdr")
        {
            // HDR sky texture
            _settings.Settings.Type = TextureFormatType.HdrRGB;
        }
        else if (_settings.Settings.Type != TextureFormatType.ColorRGB)
        {
            // Skip checking
        }
        else if (snl.EndsWith("_n")
                 || snl.EndsWith("nrm")
                 || snl.EndsWith("nm")
                 || snl.EndsWith("norm")
                 || snl.Contains("normal")
                 || snl.EndsWith("normals"))
        {
            // Normal map
            _settings.Settings.Type = TextureFormatType.NormalMap;
        }
        else if (snl.EndsWith("_d")
                 || snl.Contains("diffuse")
                 || snl.Contains("diff")
                 || snl.Contains("color")
                 || snl.Contains("_col")
                 || snl.Contains("basecolor")
                 || snl.Contains("albedo"))
        {
            // Albedo or diffuse map
            _settings.Settings.Type = TextureFormatType.ColorRGB;
            _settings.Settings.sRGB = true;
        }
        else if (snl.EndsWith("ao")
                 || snl.EndsWith("ambientocclusion")
                 || snl.EndsWith("gloss")
                 || snl.EndsWith("_r")
                 || snl.EndsWith("_displ")
                 || snl.EndsWith("_disp")
                 || snl.EndsWith("roughness")
                 || snl.EndsWith("_rgh")
                 || snl.EndsWith("_met")
                 || snl.EndsWith("metalness")
                 || snl.EndsWith("displacement")
                 || snl.EndsWith("spec")
                 || snl.EndsWith("specular")
                 || snl.EndsWith("occlusion")
                 || snl.EndsWith("height")
                 || snl.EndsWith("heights")
                 || snl.EndsWith("cavity")
                 || snl.EndsWith("metalic")
                 || snl.EndsWith("metallic"))
        {
            // Glossiness, metalness, ambient occlusion, displacement, height, cavity or specular
            _settings.Settings.Type = TextureFormatType.GrayScale;
        }
        */

	    // Try to restore target asset texture import options (useful for fast reimport)
	    // Editor.TryRestoreImportOptions(ref _settings.Settings, ResultUrl);
	}

    bool TextureImportEntry::Import()
	{
	    return AssetsImporting::Import(SourceUrl, ResultUrl, &m_Setting);
	}
} // SE