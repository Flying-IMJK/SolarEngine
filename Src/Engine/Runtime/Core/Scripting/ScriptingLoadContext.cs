// ScriptingLoadContext.cs
// Provides the AssemblyLoadContext used by SECore to load script assemblies.
//
// In editor mode (SE_EDITOR defined at build time via a conditional compile symbol):
//   - Uses a collectible (unloadable) ALC to support hot-reload.
// In game/runtime mode:
//   - Uses a non-collectible ALC for better performance.

using System;
using System.Reflection;
using System.Runtime.Loader;

namespace SE.Scripting
{
    /// <summary>
    /// AssemblyLoadContext used to isolate and (optionally) unload script assemblies.
    /// </summary>
    internal sealed class ScriptingLoadContext : AssemblyLoadContext
    {
        private readonly AssemblyDependencyResolver _resolver;

        /// <summary>
        /// Creates a new ScriptingLoadContext.
        /// </summary>
        /// <param name="assemblyPath">Absolute path to the primary assembly (.dll).</param>
        /// <param name="isCollectible">
        ///   True to create an unloadable (collectible) context — used in editor mode.
        ///   False for a non-collectible context — used in game/runtime mode.
        /// </param>
        public ScriptingLoadContext(string assemblyPath, bool isCollectible)
            : base(name: System.IO.Path.GetFileNameWithoutExtension(assemblyPath),
                   isCollectible: isCollectible)
        {
            _resolver = new AssemblyDependencyResolver(assemblyPath);
        }

        /// <inheritdoc/>
        protected override Assembly? Load(AssemblyName assemblyName)
        {
            string? assemblyPath = _resolver.ResolveAssemblyToPath(assemblyName);
            if (assemblyPath != null)
                return LoadFromAssemblyPath(assemblyPath);

            // Fall back to the default context (BCL, engine assemblies already loaded).
            return null;
        }

        /// <inheritdoc/>
        protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
        {
            string? libraryPath = _resolver.ResolveUnmanagedDllToPath(unmanagedDllName);
            if (libraryPath != null)
                return LoadUnmanagedDllFromPath(libraryPath);

            return IntPtr.Zero;
        }
    }

    /// <summary>
    /// Factory exposed to C++ via a native entry point.
    /// C++ calls <c>CreateScriptingContext</c> through load_assembly_and_get_function_pointer
    /// to obtain a context handle, then uses it to load additional assemblies.
    /// </summary>
    public static class ScriptingContextFactory
    {
        /// <summary>
        /// Creates a ScriptingLoadContext and loads the assembly at <paramref name="assemblyPath"/>.
        /// </summary>
        /// <param name="assemblyPath">Path to the assembly to load.</param>
        /// <param name="isCollectible">
        ///   Non-zero → collectible (editor/hot-reload mode).
        ///   Zero     → non-collectible (game/runtime mode).
        /// </param>
        /// <returns>
        ///   A <see cref="WeakReference"/> to the created context, boxed as <see cref="object"/>,
        ///   so C++ can hold a GC handle to it without preventing unloading.
        /// </returns>
        public static object? CreateScriptingContext(string assemblyPath, int isCollectible)
        {
            bool collectible =
#if SE_EDITOR
                true;   // Always collectible in editor builds regardless of parameter
#else
                isCollectible != 0;
#endif

            var context = new ScriptingLoadContext(assemblyPath, collectible);

            try
            {
                context.LoadFromAssemblyPath(assemblyPath);
            }
            catch (Exception ex)
            {
                // Log via Console; the C++ side reads stderr/stdout during startup.
                Console.Error.WriteLine($"[SECore] Failed to load assembly '{assemblyPath}': {ex}");
                return null;
            }

            // Return a WeakReference so C++ can hold a GC handle without rooting the context.
            // When C++ calls Unload() and drops all references, the GC can collect the context.
            return new WeakReference<ScriptingLoadContext>(context);
        }

        /// <summary>
        /// Initiates unloading of a collectible context obtained from <see cref="CreateScriptingContext"/>.
        /// No-op for non-collectible contexts.
        /// </summary>
        /// <param name="contextRef">
        ///   The <see cref="WeakReference{T}"/> returned by <see cref="CreateScriptingContext"/>.
        /// </param>
        public static void UnloadScriptingContext(object? contextRef)
        {
            if (contextRef is WeakReference<ScriptingLoadContext> weakRef &&
                weakRef.TryGetTarget(out var context))
            {
                if (context.IsCollectible)
                    context.Unload();
            }
        }
    }
}
