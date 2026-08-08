
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;

namespace SE.Interop
{
    public unsafe class ManagedArray
    {
        private ManagedHandle m_ManagedHandle;
        private IntPtr m_UnmanagedData;
        private int m_UnmanagedAllocationSize;
        private Type m_ArrayType;
        private Type m_ElementType;
        private int m_ElementSize;
        private int m_Length;

        [ThreadStatic] private static Dictionary<ManagedArray, ManagedHandle> pooledArrayHandles;

        public static ManagedArray WrapNewArray(Array arr) => new ManagedArray(arr, arr.GetType());

        public static ManagedArray WrapNewArray(Array arr, Type arrayType) => new ManagedArray(arr, arrayType);

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>The resources must be released by calling FreePooled() instead of Free()-method.</remarks>
        public static (ManagedHandle managedHandle, ManagedArray managedArray) WrapPooledArray(Array arr)
        {
            ManagedArray managedArray = ManagedArrayPool.Get();
            managedArray.WrapArray(arr, arr.GetType());

            if (pooledArrayHandles == null)
                pooledArrayHandles = new();
            if (!pooledArrayHandles.TryGetValue(managedArray, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(managedArray);
                pooledArrayHandles.Add(managedArray, handle);
            }
            return (handle, managedArray);
        }

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>The resources must be released by calling FreePooled() instead of Free()-method.</remarks>
        public static ManagedHandle WrapPooledArray(Array arr, Type arrayType)
        {
            ManagedArray managedArray = ManagedArrayPool.Get(arr.Length * NativeInterop.GetTypeSize(arr.GetType().GetElementType()));
            managedArray.WrapArray(arr, arrayType);

            if (pooledArrayHandles == null)
                pooledArrayHandles = new();
            if (!pooledArrayHandles.TryGetValue(managedArray, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(managedArray);
                pooledArrayHandles.Add(managedArray, handle);
            }
            return handle;
        }

        internal static ManagedArray AllocateNewArray(int length, Type arrayType, Type elementType)
            => new ManagedArray((IntPtr)NativeInterop.NativeAlloc(length, NativeInterop.GetTypeSize(elementType)), length, arrayType, elementType);

        internal static ManagedArray AllocateNewArray(IntPtr ptr, int length, Type arrayType, Type elementType)
            => new ManagedArray(ptr, length, arrayType, elementType);

        /// <summary>
        /// Returns an instance of ManagedArray from shared pool.
        /// </summary>
        /// <remarks>The resources must be released by calling FreePooled() instead of Free()-method. Do not release the returned ManagedHandle.</remarks>
        public static (ManagedHandle managedHandle, ManagedArray managedArray) AllocatePooledArray<T>(int length) where T : unmanaged
        {
            ManagedArray managedArray = ManagedArrayPool.Get(length * Unsafe.SizeOf<T>());
            managedArray.Allocate<T>(length);

            if (pooledArrayHandles == null)
                pooledArrayHandles = new();
            if (!pooledArrayHandles.TryGetValue(managedArray, out ManagedHandle handle))
            {
                handle = ManagedHandle.Alloc(managedArray);
                pooledArrayHandles.Add(managedArray, handle);
            }
            return (handle, managedArray);
        }

        public ManagedArray(Array arr, Type elementType) => WrapArray(arr, elementType);

        internal void WrapArray(Array arr, Type arrayType)
        {
            if (m_UnmanagedData != IntPtr.Zero)
                NativeInterop.NativeFree(m_UnmanagedData.ToPointer());
            if (m_ManagedHandle.IsAllocated)
                m_ManagedHandle.Free();
            m_ManagedHandle = ManagedHandle.Alloc(arr, GCHandleType.Pinned);
            m_UnmanagedData = Marshal.UnsafeAddrOfPinnedArrayElement(arr, 0);
            m_UnmanagedAllocationSize = 0;
            m_Length = arr.Length;
            m_ArrayType = arrayType;
            m_ElementType = arr.GetType().GetElementType();
            m_ElementSize = NativeInterop.GetTypeSize(m_ElementType);
        }

        internal void Allocate<T>(int length) where T : unmanaged
        {
            m_Length = length;
            m_ArrayType = typeof(T[]);
            m_ElementType = typeof(T);
            m_ElementSize = Unsafe.SizeOf<T>();

            // Try to reuse existing allocated buffer
            if (length * m_ElementSize > m_UnmanagedAllocationSize)
            {
                if (m_UnmanagedAllocationSize > 0)
                    NativeInterop.NativeFree(m_UnmanagedData.ToPointer());
                m_UnmanagedData = (IntPtr)NativeInterop.NativeAlloc(length, m_ElementSize);
                m_UnmanagedAllocationSize = m_ElementSize * length;
            }
        }

        private ManagedArray()
        {
        }

        private ManagedArray(IntPtr ptr, int length, Type arrayType, Type elementType)
        {
            Assert.IsTrue(arrayType.IsArray);
            m_ElementType = elementType;
            m_ElementSize = NativeInterop.GetTypeSize(elementType);
            m_UnmanagedData = ptr;
            m_UnmanagedAllocationSize = m_ElementSize * length;
            m_Length = length;
            m_ArrayType = arrayType;
        }

        ~ManagedArray()
        {
            if (m_UnmanagedData != IntPtr.Zero)
                Free();
        }

        public void Free()
        {
            GC.SuppressFinalize(this);
            if (m_ManagedHandle.IsAllocated)
            {
                m_ManagedHandle.Free();
                m_UnmanagedData = IntPtr.Zero;
            }
            if (m_UnmanagedData != IntPtr.Zero)
            {
                NativeInterop.NativeFree(m_UnmanagedData.ToPointer());
                m_UnmanagedData = IntPtr.Zero;
                m_UnmanagedAllocationSize = 0;
            }
        }

        public void FreePooled()
        {
            if (m_ManagedHandle.IsAllocated)
            {
                m_ManagedHandle.Free();
                m_UnmanagedData = IntPtr.Zero;
            }
            ManagedArrayPool.Put(this);
        }

        internal IntPtr Pointer => m_UnmanagedData;

        internal int Length => m_Length;

        internal int ElementSize => m_ElementSize;

        internal Type ElementType => m_ElementType;

        internal Type ArrayType => m_ArrayType;

        public Span<T> ToSpan<T>() where T : struct => new Span<T>(m_UnmanagedData.ToPointer(), m_Length);

        public T[] ToArray<T>() where T : struct => new Span<T>(m_UnmanagedData.ToPointer(), m_Length).ToArray();

        public Array ToArray() => ArrayCast.ToArray(new Span<byte>(m_UnmanagedData.ToPointer(), m_Length * m_ElementSize), m_ElementType);

        /// <summary>
        /// Creates an Array of the specified type from span of bytes.
        /// </summary>
        private static class ArrayCast
        {
            delegate Array GetDelegate(Span<byte> span);

            [ThreadStatic]
            private static Dictionary<Type, GetDelegate> s_Delegates;

            internal static Array ToArray(Span<byte> span, Type type)
            {
                if (s_Delegates == null)
                    s_Delegates = new();
                if (!s_Delegates.TryGetValue(type, out var deleg))
                {
                    deleg = typeof(ArrayCastInternal<>).MakeGenericType(type).GetMethod(nameof(ArrayCastInternal<int>.ToArray), BindingFlags.Static | BindingFlags.NonPublic).CreateDelegate<GetDelegate>();
                    s_Delegates.Add(type, deleg);
                }
                return deleg(span);
            }

            private static class ArrayCastInternal<T> where T : struct
            {
                internal static Array ToArray(Span<byte> span)
                {
                    return MemoryMarshal.Cast<byte, T>(span).ToArray();
                }
            }
        }

        /// <summary>
        /// Provides a pool of pre-allocated ManagedArray that can be re-used.
        /// </summary>
        private static class ManagedArrayPool
        {
            [ThreadStatic]
            private static List<(bool inUse, ManagedArray array)> pool;

            /// <summary>
            /// Borrows an array from the pool.
            /// </summary>
            /// <param name="minimumSize">Minimum size in bytes for the borrowed array. With value of 0, the returned array allocation is always zero.</param>
            /// <remarks>The returned array size may be smaller than the requested minimumSize.</remarks>
            internal static ManagedArray Get(int minimumSize = 0)
            {
                if (pool == null)
                    pool = new();

                int smallest = -1;
                int smallestSize = int.MaxValue;
                var poolSpan = CollectionsMarshal.AsSpan(pool);
                for (int i = 0; i < poolSpan.Length; i++)
                {
                    ref var tuple = ref poolSpan[i];
                    if (tuple.inUse)
                        continue;

                    // Try to get larger arrays than requested in order to avoid reallocations
                    if (minimumSize > 0)
                    {
                        if (tuple.array.m_UnmanagedAllocationSize >= minimumSize && tuple.array.m_UnmanagedAllocationSize < smallestSize)
                            smallest = i;
                        continue;
                    }
                    else if (minimumSize == 0 && tuple.Item2.m_UnmanagedAllocationSize != 0)
                        continue;

                    tuple.inUse = true;
                    return tuple.array;
                }
                if (minimumSize > 0 && smallest != -1)
                {
                    ref var tuple = ref poolSpan[smallest];
                    tuple.inUse = true;
                    return tuple.array;
                }

                var newTuple = (inUse: true, array: new ManagedArray());
                pool.Add(newTuple);
                return newTuple.array;
            }

            /// <summary>
            /// Returns the borrowed ManagedArray back to pool.
            /// </summary>
            /// <param name="obj">The array borrowed from the pool</param>
            internal static void Put(ManagedArray obj)
            {
                foreach (ref var tuple in CollectionsMarshal.AsSpan(pool))
                {
                    if (tuple.array != obj)
                        continue;

                    tuple.inUse = false;
                    return;
                }

                throw new NativeInteropException("Tried to free non-pooled ManagedArray as pooled ManagedArray");
            }
        }
    }

    public static class ManagedString
    {
        internal static ManagedHandle EmptyStringHandle = ManagedHandle.Alloc(string.Empty);

        [System.Diagnostics.DebuggerStepThrough]
        public static unsafe IntPtr ToNative(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return ManagedHandle.ToIntPtr(EmptyStringHandle);
            Assert.IsTrue(str.Length > 0);
            return ManagedHandle.ToIntPtr(str);
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static unsafe IntPtr ToNativeWeak(string str)
        {
            if (str == null)
                return IntPtr.Zero;
            else if (str == string.Empty)
                return ManagedHandle.ToIntPtr(EmptyStringHandle);
            Assert.IsTrue(str.Length > 0);
            return ManagedHandle.ToIntPtr(str, GCHandleType.Weak);
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static string ToManaged(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return null;
            return Unsafe.As<string>(ManagedHandle.FromIntPtr(ptr).Target);
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static void Free(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return;
            ManagedHandle handle = ManagedHandle.FromIntPtr(ptr);
            if (handle == EmptyStringHandle)
                return;
            handle.Free();
        }
    }

    /// <summary>
    /// Handle to managed objects which can be stored in native code.
    /// </summary>
    public struct ManagedHandle
    {
        private IntPtr m_NativeHandle;

        private ManagedHandle(IntPtr handle) => m_NativeHandle = handle;

        private ManagedHandle(object value, GCHandleType type) => m_NativeHandle = ManagedHandlePool.AllocateHandle(value, type);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle Alloc(object value) => new ManagedHandle(value, GCHandleType.Normal);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle Alloc(object value, GCHandleType type) => new ManagedHandle(value, type);

        public void Free()
        {
            if (m_NativeHandle == IntPtr.Zero)
                return;
            ManagedHandlePool.FreeHandle(m_NativeHandle);
            m_NativeHandle = IntPtr.Zero;
        }

        public object Target
        {
            get => ManagedHandlePool.GetObject(m_NativeHandle);
            set => ManagedHandlePool.SetObject(m_NativeHandle, value);
        }

        public bool IsAllocated => m_NativeHandle != IntPtr.Zero;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static explicit operator ManagedHandle(IntPtr value) => FromIntPtr(value);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ManagedHandle FromIntPtr(IntPtr value) => new ManagedHandle(value);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static explicit operator IntPtr(ManagedHandle value) => ToIntPtr(value);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr ToIntPtr(object value) => ManagedHandlePool.AllocateHandle(value, GCHandleType.Normal);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr ToIntPtr(object value, GCHandleType type) => ManagedHandlePool.AllocateHandle(value, type);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntPtr ToIntPtr(ManagedHandle value) => value.m_NativeHandle;

        public override int GetHashCode() => m_NativeHandle.GetHashCode();

        public override bool Equals(object obj) => obj is ManagedHandle other && m_NativeHandle == other.m_NativeHandle;

        public bool Equals(ManagedHandle other) => m_NativeHandle == other.m_NativeHandle;

        public static bool operator ==(ManagedHandle a, ManagedHandle b) => a.m_NativeHandle == b.m_NativeHandle;

        public static bool operator !=(ManagedHandle a, ManagedHandle b) => a.m_NativeHandle != b.m_NativeHandle;

        internal static class ManagedHandlePool
        {
            private const int WeakPoolCollectionSizeThreshold = 10000000;
            private const int WeakPoolCollectionTimeThreshold = 500;

            // Rolling numbers for handles, two bits reserved for the type
            private static ulong s_NormalHandleAccumulator = ((ulong)GCHandleType.Normal << 62) & 0xC000000000000000;
            private static ulong s_PinnedHandleAccumulator = ((ulong)GCHandleType.Pinned << 62) & 0xC000000000000000;
            private static ulong s_WeakHandleAccumulator = ((ulong)GCHandleType.Weak << 62) & 0xC000000000000000;

            // Dictionaries for storing the valid handles.
            // Note: Using locks seems to be generally the fastest when adding or fetching from the dictionary.
            // Concurrent dictionaries could also be considered, but they perform much slower when adding to the dictionary.
            private static Dictionary<IntPtr, object> s_PersistentPool = new();
            private static Dictionary<IntPtr, GCHandle> s_PinnedPool = new();

            // TODO: Performance of pinned handles are poor at the moment due to GCHandle wrapping.
            // TODO: .NET8: Experiment with pinned arrays for faster pinning: https://github.com/dotnet/runtime/pull/89293

            // Manage double-buffered pool for weak handles in order to avoid collecting in-flight handles.
            // Periodically when the pools are being accessed and conditions are met, the other pool is cleared and swapped.
            private static Dictionary<IntPtr, object> s_WeakPool = new();
            private static Dictionary<IntPtr, object> s_WeakPoolOther = new();
            private static object s_WeakPoolLock = new object();
            private static ulong s_NextWeakPoolCollection;
            private static int s_NextWeakPoolGCCollection;
            private static long s_LastWeakPoolCollectionTime;

            /// <summary>
            /// Tries to free all references to old weak handles so GC can collect them.
            /// </summary>
            internal static void TryCollectWeakHandles()
            {
                if (s_WeakHandleAccumulator < s_NextWeakPoolCollection)
                    return;

                s_NextWeakPoolCollection = s_WeakHandleAccumulator + 1000;

                // Try to swap pools after garbage collection or whenever the pool gets too large
                var gc0CollectionCount = GC.CollectionCount(0);
                if (gc0CollectionCount < s_NextWeakPoolGCCollection && s_WeakPool.Count < WeakPoolCollectionSizeThreshold)
                    return;
                s_NextWeakPoolGCCollection = gc0CollectionCount + 1;

                // Prevent huge allocations from swapping the pools in the middle of the operation
                if (System.Diagnostics.Stopwatch.GetElapsedTime(s_LastWeakPoolCollectionTime).TotalMilliseconds < WeakPoolCollectionTimeThreshold)
                    return;
                s_LastWeakPoolCollectionTime = System.Diagnostics.Stopwatch.GetTimestamp();

                // Swap the pools and release the oldest pool for GC
                (s_WeakPool, s_WeakPoolOther) = (s_WeakPoolOther, s_WeakPool);
                s_WeakPool.Clear();
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static IntPtr NewHandle(GCHandleType type) => type switch
            {
                GCHandleType.Normal => (IntPtr)Interlocked.Increment(ref s_NormalHandleAccumulator),
                GCHandleType.Pinned => (IntPtr)Interlocked.Increment(ref s_PinnedHandleAccumulator),
                GCHandleType.Weak => (IntPtr)Interlocked.Increment(ref s_WeakHandleAccumulator),
                GCHandleType.WeakTrackResurrection => (IntPtr)Interlocked.Increment(ref s_WeakHandleAccumulator),
                _ => throw new NotImplementedException(type.ToString())
            };

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static GCHandleType GetHandleType(IntPtr handle) => (GCHandleType)(((ulong)handle & 0xC000000000000000) >> 62);

            internal static IntPtr AllocateHandle(object value, GCHandleType type)
            {
                IntPtr handle = NewHandle(type);
                switch (type)
                {
                    case GCHandleType.Normal:
                        lock (s_PersistentPool)
                            s_PersistentPool.Add(handle, value);
                        break;
                    case GCHandleType.Pinned:
                        lock (s_PinnedPool)
                            s_PinnedPool.Add(handle, GCHandle.Alloc(value, GCHandleType.Pinned));
                        break;
                    case GCHandleType.Weak:
                    case GCHandleType.WeakTrackResurrection:
                        lock (s_WeakPoolLock)
                        {
                            TryCollectWeakHandles();
                            s_WeakPool.Add(handle, value);
                        }
                        break;
                }
                return handle;
            }

            internal static object GetObject(IntPtr handle)
            {
                switch (GetHandleType(handle))
                {
                    case GCHandleType.Normal:
                        lock (s_PersistentPool)
                        {
                            if (s_PersistentPool.TryGetValue(handle, out object value))
                                return value;
                        }
                        break;
                    case GCHandleType.Pinned:
                        lock (s_PinnedPool)
                        {
                            if (s_PinnedPool.TryGetValue(handle, out GCHandle gcHandle))
                                return gcHandle.Target;
                        }
                        break;
                    case GCHandleType.Weak:
                    case GCHandleType.WeakTrackResurrection:
                        lock (s_WeakPoolLock)
                        {
                            TryCollectWeakHandles();
                            if (s_WeakPool.TryGetValue(handle, out object value))
                                return value;
                            else if (s_WeakPoolOther.TryGetValue(handle, out value))
                                return value;
                        }
                        break;
                }
                throw new NativeInteropException("Invalid ManagedHandle");
            }

            internal static void SetObject(IntPtr handle, object value)
            {
                switch (GetHandleType(handle))
                {
                    case GCHandleType.Normal:
                        lock (s_PersistentPool)
                        {
                            ref object obj = ref CollectionsMarshal.GetValueRefOrNullRef(s_PersistentPool, handle);
                            if (!Unsafe.IsNullRef(ref obj))
                            {
                                obj = value;
                                return;
                            }
                        }
                        break;
                    case GCHandleType.Pinned:
                        lock (s_PinnedPool)
                        {
                            ref GCHandle gcHandle = ref CollectionsMarshal.GetValueRefOrNullRef(s_PinnedPool, handle);
                            if (!Unsafe.IsNullRef(ref gcHandle))
                            {
                                gcHandle.Target = value;
                                return;
                            }
                        }
                        break;
                    case GCHandleType.Weak:
                    case GCHandleType.WeakTrackResurrection:
                        lock (s_WeakPoolLock)
                        {
                            TryCollectWeakHandles();
                            {
                                ref object obj = ref CollectionsMarshal.GetValueRefOrNullRef(s_WeakPool, handle);
                                if (!Unsafe.IsNullRef(ref obj))
                                {
                                    obj = value;
                                    return;
                                }
                            }
                            {
                                ref object obj = ref CollectionsMarshal.GetValueRefOrNullRef(s_WeakPoolOther, handle);
                                if (!Unsafe.IsNullRef(ref obj))
                                {
                                    obj = value;
                                    return;
                                }
                            }
                        }
                        break;
                }
                throw new NativeInteropException("Invalid ManagedHandle");
            }

            internal static void FreeHandle(IntPtr handle)
            {
                switch (GetHandleType(handle))
                {
                    case GCHandleType.Normal:
                        lock (s_PersistentPool)
                        {
                            if (s_PersistentPool.Remove(handle))
                                return;
                        }
                        break;
                    case GCHandleType.Pinned:
                        lock (s_PinnedPool)
                        {
                            if (s_PinnedPool.Remove(handle, out GCHandle gcHandle))
                            {
                                gcHandle.Free();
                                return;
                            }
                        }
                        break;
                    case GCHandleType.Weak:
                    case GCHandleType.WeakTrackResurrection:
                        lock (s_WeakPoolLock)
                            TryCollectWeakHandles();
                        return;
                }
                throw new NativeInteropException("Invalid ManagedHandle");
            }
        }
    }
}
