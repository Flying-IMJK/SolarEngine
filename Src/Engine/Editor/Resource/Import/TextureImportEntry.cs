

namespace SE.Editor
{
    /// <summary>
    /// Proxy object to present texture import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class TextureImportSettings
    {
        /// <summary>
        /// The settings data.
        /// </summary>
        [EditorDisplay(null, EditorDisplayAttribute.InlineStyle)]
        public TextureUtilsOptions Settings = TextureUtilsOptions.Default;
    }

    /// <summary>
    /// Texture asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public class TextureImportEntry : AssetImportEntry
    {
        private TextureImportSettings m_Settings = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="TextureImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public TextureImportEntry(ref Request request) : base(ref request)
        {
            // Try to guess format type based on file name
            string snl = System.IO.Path.GetFileNameWithoutExtension(SourceUrl).ToLower();
            string extension = System.IO.Path.GetExtension(SourceUrl).ToLower();
            if (extension == ".raw")
            {
                // Raw image data in 16bit gray-scale, preserve the quality
                m_Settings.Settings.Type = TextureFormatType.HdrRGBA;
                m_Settings.Settings.Compress = false;
            }
            else if (extension == ".exr")
            {
                // HDR image
                m_Settings.Settings.Type = TextureFormatType.HdrRGBA;
            }
            else if (extension == ".hdr")
            {
                // HDR sky texture
                m_Settings.Settings.Type = TextureFormatType.HdrRGB;
            }
            else if (m_Settings.Settings.Type != TextureFormatType.ColorRGB)
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
                m_Settings.Settings.Type = TextureFormatType.NormalMap;
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
                m_Settings.Settings.Type = TextureFormatType.ColorRGB;
                m_Settings.Settings.sRGB = true;
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
                m_Settings.Settings.Type = TextureFormatType.GrayScale;
            }
            else if (m_Settings.Settings.Type == TextureFormatType.ColorRGB)
            {
                // Blind guess that common color texture is sRGB
                m_Settings.Settings.sRGB = true;
            }

            // Try to restore target asset texture import options (useful for fast reimport)
            AssetsImporting.TryRestoreImportOptions(ref m_Settings.Settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => m_Settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is TextureImportSettings s)
            {
                Sprite[] sprites = s.Settings.Sprites ?? m_Settings.Settings
                    .Sprites; // Preserve sprites if not specified to override
                m_Settings.Settings = s.Settings;
                m_Settings.Settings.Sprites = sprites;
                return true;
            }
            if (settings is TextureUtilsOptions o)
            {
                Sprite[] sprites = o.Sprites ?? m_Settings.Settings
                    .Sprites; // Preserve sprites if not specified to override
                m_Settings.Settings = o;
                m_Settings.Settings.Sprites = sprites;
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool Import()
        {
            return AssetsImporting.Import(SourceUrl, ResultUrl, m_Settings.Settings);
        }
    }
}
