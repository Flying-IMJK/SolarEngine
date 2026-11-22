using System;

namespace SE
{
    /// <summary>
    /// A typed asset identifier that resolves the asset on use.
    /// </summary>
    public readonly struct SoftAssetRef<T> : IEquatable<SoftAssetRef<T>> where T : Asset
    {
        // Copies of this value share the cache without keeping the asset alive.
        private readonly WeakReference<T> m_CachedAsset;

        /// <summary>
        /// Gets the referenced asset identifier, or Guid.Empty for an empty reference.
        /// </summary>
        public Guid ID { get; }

        /// <summary>
        /// Creates a reference from an asset identifier without resolving it.
        /// </summary>
        public SoftAssetRef(Guid id)
        {
            ID = id;
            m_CachedAsset = id == Guid.Empty ? null : new WeakReference<T>(null);
        }

        /// <summary>
        /// Resolves the asset or returns null if the reference is empty or cannot be resolved.
        /// The returned asset may still be loading.
        /// </summary>
        public T Get()
        {
            if (m_CachedAsset == null)
            {
                return null;
            }

            if (m_CachedAsset.TryGetTarget(out T asset) && asset)
            {
                return asset;
            }

            asset = AssetContent.LoadAsync<T>(ID);
            m_CachedAsset.SetTarget(asset);
            return asset;
        }

        /// <inheritdoc />
        public bool Equals(SoftAssetRef<T> other) => ID == other.ID;

        /// <inheritdoc />
        public override bool Equals(object obj) => obj is SoftAssetRef<T> other && Equals(other);

        /// <inheritdoc />
        public override int GetHashCode() => ID.GetHashCode();
    }
}
