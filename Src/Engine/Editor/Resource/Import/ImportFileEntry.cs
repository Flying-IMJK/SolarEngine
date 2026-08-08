using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace SE.Editor
{
    public sealed class ImportRequest
    {
        public ImportRequest(string inputPath, string outputPath, bool isInBuilt = false, bool skipSettingsDialog = false, object? settings = null)
        {
            InputPath = inputPath ?? throw new ArgumentNullException(nameof(inputPath));
            OutputPath = outputPath ?? throw new ArgumentNullException(nameof(outputPath));
            IsInBuilt = isInBuilt;
            SkipSettingsDialog = skipSettingsDialog;
            Settings = settings;
        }

        public string InputPath { get; }
        public string OutputPath { get; }
        public bool IsInBuilt { get; }
        public bool SkipSettingsDialog { get; }
        public object? Settings { get; }
    }

    public interface IFileEntryAction
    {
        string SourceUrl { get; }
        string ResultUrl { get; }
        bool Execute();
    }

    public class ImportFileEntry : IFileEntryAction
    {
        public ImportFileEntry(ImportRequest request)
        {
            ArgumentNullException.ThrowIfNull(request);
            SourceUrl = request.InputPath;
            ResultUrl = request.OutputPath;
        }

        public string SourceUrl { get; }
        public string ResultUrl { get; private set; }
        public virtual object? Settings => null;
        public bool HasSettings => Settings != null;
        public virtual bool TryOverrideSettings(object settings) => false;

        public void ModifyResultFilename(string filename)
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(filename);
            string? directory = Path.GetDirectoryName(ResultUrl);
            string extension = Path.GetExtension(ResultUrl);
            ResultUrl = Path.Combine(directory ?? string.Empty, filename + extension);
        }

        public virtual bool Import()
        {
            if (!File.Exists(SourceUrl) && !Directory.Exists(SourceUrl))
                return false;

            string? outputDirectory = Path.GetDirectoryName(ResultUrl);
            if (!string.IsNullOrEmpty(outputDirectory))
                Directory.CreateDirectory(outputDirectory);

            if (Directory.Exists(SourceUrl))
            {
                CopyDirectory(SourceUrl, ResultUrl);
                return true;
            }

            File.Copy(SourceUrl, ResultUrl, overwrite: true);
            return true;
        }

        public bool Execute() => Import();

        protected static void CopyDirectory(string source, string destination)
        {
            Directory.CreateDirectory(destination);
            foreach (string file in Directory.EnumerateFiles(source))
            {
                File.Copy(file, Path.Combine(destination, Path.GetFileName(file)), overwrite: true);
            }
            foreach (string directory in Directory.EnumerateDirectories(source))
            {
                CopyDirectory(directory, Path.Combine(destination, Path.GetFileName(directory)));
            }
        }
    }

    public sealed class FolderImportEntry : ImportFileEntry
    {
        private readonly Action<IEnumerable<string>, ContentFolder, bool>? m_ImportChildren;
        private readonly ContentFolder? m_TargetFolder;

        public FolderImportEntry(ImportRequest request, ContentFolder? targetFolder = null, Action<IEnumerable<string>, ContentFolder, bool>? importChildren = null)
            : base(request)
        {
            SkipSettingsDialog = request.SkipSettingsDialog;
            m_TargetFolder = targetFolder;
            m_ImportChildren = importChildren;
        }

        public bool SkipSettingsDialog { get; }

        public override bool Import()
        {
            if (!Directory.Exists(SourceUrl))
                return false;

            Directory.CreateDirectory(ResultUrl);
            if (m_TargetFolder == null || m_ImportChildren == null)
                return true;

            m_ImportChildren(Directory.EnumerateFileSystemEntries(SourceUrl), m_TargetFolder, SkipSettingsDialog);
            return true;
        }
    }

    public sealed class ModelImportEntry : ImportFileEntry
    {
        public ModelImportEntry(ImportRequest request)
            : base(request)
        {
        }

        public override bool Import() => ResourceInterop.ImportBackend.ImportModel(SourceUrl, ResultUrl);
    }

    public sealed class TextureImportEntry : ImportFileEntry
    {
        public TextureImportEntry(ImportRequest request)
            : base(request)
        {
        }

        public override bool Import() => ResourceInterop.ImportBackend.ImportTexture(SourceUrl, ResultUrl);
    }

    /// <summary>
    /// Managed queue for choosing the correct entry type. Native importer calls are
    /// deliberately isolated in the two entry types above.
    /// </summary>
    public sealed class ResourceImportService
    {
        private static readonly HashSet<string> TextureExtensions = new(StringComparer.OrdinalIgnoreCase)
        {
            ".tga", ".png", ".bmp", ".gif", ".tiff", ".tif", ".jpeg", ".jpg", ".dds", ".hdr", ".raw", ".exr",
        };

        private static readonly HashSet<string> ModelExtensions = new(StringComparer.OrdinalIgnoreCase)
        {
            ".obj", ".fbx", ".x", ".dae", ".gltf", ".glb", ".blend", ".bvh", ".ase", ".ply", ".dxf", ".ifc", ".nff", ".smd", ".vta", ".mdl", ".md2", ".md3", ".md5mesh", ".q3o", ".q3s", ".ac", ".stl", ".lwo", ".lws", ".lxo",
        };

        public ImportFileEntry CreateEntry(ImportRequest request, ContentFolder? destination = null)
        {
            string extension = Path.GetExtension(request.InputPath);
            if (string.IsNullOrEmpty(extension))
                return new FolderImportEntry(request, destination, Import);
            if (TextureExtensions.Contains(extension))
                return new TextureImportEntry(request);
            if (ModelExtensions.Contains(extension))
                return new ModelImportEntry(request);
            return new ImportFileEntry(request);
        }

        public bool Import(string inputPath, ContentFolder destination, bool skipSettingsDialog = false)
        {
            ArgumentNullException.ThrowIfNull(destination);
            string fileName = Path.GetFileName(inputPath);
            string outputPath = Path.Combine(destination.Path, fileName);
            ImportRequest request = new(inputPath, outputPath, isInBuilt: true, skipSettingsDialog);
            return CreateEntry(request, destination).Execute();
        }

        public void Import(IEnumerable<string> inputPaths, ContentFolder destination, bool skipSettingsDialog = false)
        {
            foreach (string inputPath in inputPaths.Where(static path => !string.IsNullOrWhiteSpace(path)))
            {
                Import(inputPath, destination, skipSettingsDialog);
            }
        }
    }
}
