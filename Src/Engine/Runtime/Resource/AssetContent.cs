using System;
using System.Runtime.CompilerServices;

namespace SE
{
    public static partial class AssetContent
    {
        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <param name="type">Type of the asset to load. Includes any asset types derived from the type.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static Asset LoadAsync(Guid id, Type type)
        {
            return LoadAsync(ref id, ref type);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T LoadAsync<T>(Guid id) where T : Asset
        {
            return (T)LoadAsync(id, typeof(T));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadAsync(Guid id)
        {
            return LoadAsync<Asset>(id);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <param name="type">Type of the asset to load. Includes any asset types derived from the type.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static Asset LoadAsync(string path, Type type)
        {
            return LoadAsync(path, ref type);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T LoadAsync<T>(string path) where T : Asset
        {
            return (T)LoadAsync(path, typeof(T));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadAsync(string path)
        {
            return LoadAsync(path, typeof(Asset));
        }

        /// <summary>
        /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset without an asset file extension.</param>
        /// <param name="type">Type of the asset to load. Includes any asset types derived from the type.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static Asset LoadAsyncInternal(string internalPath, Type type)
        {
            return LoadAsyncInternal(internalPath, ref type);
        }

        /// <summary>
        /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset without an asset file extension.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T LoadAsyncInternal<T>(string internalPath) where T : Asset
        {
            return (T)LoadAsyncInternal(internalPath, typeof(T));
        }

        /// <summary>
        /// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset without an asset file extension.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadAsyncInternal(string internalPath)
        {
            return LoadAsyncInternal(internalPath, typeof(Asset));
        }

        /// <summary>
        /// Loads asset to the Content Pool and waits until asset will be loaded.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset Load(Guid id, double timeoutInMilliseconds = 30000.0)
        {
            return Load<Asset>(id, timeoutInMilliseconds);
        }

        /// <summary>
        /// Loads asset to the Content Pool and waits until asset will be loaded.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset Load(string path, double timeoutInMilliseconds = 30000.0)
        {
            return Load<Asset>(path, timeoutInMilliseconds);
        }

        /// <summary>
        /// Loads internal engine asset and waits until asset will be loaded.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset without an asset file extension.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadInternal(string internalPath, double timeoutInMilliseconds = 30000.0)
        {
            return LoadInternal<Asset>(internalPath, timeoutInMilliseconds);
        }

        /// <summary>
        /// Loads asset to the Content Pool and waits until asset will be loaded.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T Load<T>(Guid id, double timeoutInMilliseconds = 30000.0) where T : Asset
        {
            var asset = LoadAsync<T>(id);
            if (asset && asset.WaitForLoaded(timeoutInMilliseconds) == false)
            {
                return asset;
            }
            return null;
        }

        /// <summary>
        /// Loads asset to the Content Pool and waits until asset will be loaded.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T Load<T>(string path, double timeoutInMilliseconds = 30000.0) where T : Asset
        {
            var asset = LoadAsync<T>(path);
            if (asset && asset.WaitForLoaded(timeoutInMilliseconds) == false)
            {
                return asset;
            }
            return null;
        }

        /// <summary>
        /// Loads internal engine asset and waits until asset will be loaded.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset without an asset file extension.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T LoadInternal<T>(string internalPath, double timeoutInMilliseconds = 30000.0) where T : Asset
        {
            var asset = LoadAsyncInternal<T>(internalPath);
            if (asset && asset.WaitForLoaded(timeoutInMilliseconds) == false)
            {
                return asset;
            }
            return null;
        }
    }
}
