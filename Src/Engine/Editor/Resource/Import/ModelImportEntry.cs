
using System.Collections.Generic;



namespace SE.Editor
{
    /// <summary>
    /// Proxy object to present model import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class ModelImportSettings
    {
        /// <summary>
        /// The settings data.
        /// </summary>
        [EditorDisplay(null, EditorDisplayAttribute.InlineStyle)]
        public ModelUtilsOptions Settings = ModelUtilsOptions.Default;
    }

    /// <summary>
    /// Model asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public class ModelImportEntry : AssetImportEntry
    {
        private ModelImportSettings m_Settings = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="ModelImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public ModelImportEntry(ref Request request) : base(ref request)
        {
            // Try to restore target asset model import options (useful for fast reimport)
            AssetsImporting.TryRestoreImportOptions(ref m_Settings.Settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => m_Settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is ModelImportSettings s)
            {
                m_Settings.Settings = s.Settings;
                return true;
            }
            if (settings is ModelUtilsOptions o)
            {
                m_Settings.Settings = o;
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
