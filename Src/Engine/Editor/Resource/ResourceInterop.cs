using System;
using System.Runtime.InteropServices;

namespace SE.Editor
{
    /// <summary>
    /// Managed-to-native boundary for the Resource feature.
    /// UI state stays managed; only importer operations that require native engine
    /// registries cross this ABI.
    /// </summary>
    public interface IResourceImportBackend
    {
        bool ImportTexture(string inputPath, string outputPath);
        bool ImportModel(string inputPath, string outputPath);
    }

    public static class ResourceInterop
    {
        private static IResourceImportBackend s_ImportBackend = new NativeResourceImportBackend();

        public static IResourceImportBackend ImportBackend
        {
            get => s_ImportBackend;
            set => s_ImportBackend = value ?? throw new ArgumentNullException(nameof(value));
        }

        private sealed class NativeResourceImportBackend : IResourceImportBackend
        {
            public bool ImportTexture(string inputPath, string outputPath) => NativeImportTexture(inputPath, outputPath) != 0;
            public bool ImportModel(string inputPath, string outputPath) => NativeImportModel(inputPath, outputPath) != 0;

            [DllImport("SEEditor", EntryPoint = "ResourceInterop_ImportTexture", CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
            private static extern int NativeImportTexture(string inputPath, string outputPath);

            [DllImport("SEEditor", EntryPoint = "ResourceInterop_ImportModel", CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
            private static extern int NativeImportModel(string inputPath, string outputPath);
        }
    }
}
