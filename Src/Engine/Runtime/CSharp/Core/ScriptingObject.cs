// ScriptingObject.cs
// C# base class for objects that have a corresponding C++ ScriptingObject instance.
//
// Each C# ScriptingObject holds:
//   __unmanagedPtr  — raw pointer to the C++ ScriptingObject (set by C++ via NativeInterop)
//   __internalId    — the object's unique GUID (set by C++ via NativeInterop)
//
// Lifecycle:
//   - C++ creates the managed instance via SECore::ScriptingObjectHelper::CreateScriptingObject
//   - C++ sets __unmanagedPtr and __internalId via SetInternalValues
//   - When C++ destroys the object, it calls OnScriptingDispose() on this instance
//   - When the GC collects a ManagedScriptingObject, C++ is notified via OnManagedInstanceDeleted
//
// Requirements: 3.1, 3.2, 3.3, 3.4, 3.5

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SE.Scripting
{
    /// <summary>
    /// Base class for all C# objects that are paired with a C++ ScriptingObject instance.
    /// </summary>
    public unsafe class ScriptingObject
    {
        // -----------------------------------------------------------------------
        // Fields set by C++ via NativeInterop.SetInternalValues
        // -----------------------------------------------------------------------

        /// <summary>
        /// Raw pointer to the corresponding C++ ScriptingObject.
        /// Set by C++ after the managed instance is created.
        /// Null after the C++ object is destroyed.
        /// </summary>
        internal void* __unmanagedPtr;

        /// <summary>
        /// Unique identifier shared between the C++ and C# sides.
        /// Set by C++ after the managed instance is created.
        /// </summary>
        internal Guid __internalId;

        // -----------------------------------------------------------------------
        // Construction
        // -----------------------------------------------------------------------

        /// <summary>
        /// Default constructor — called by C++ via Activator.CreateInstance or
        /// by derived C# classes.
        /// </summary>
        public ScriptingObject()
        {
            __unmanagedPtr = null;
            __internalId   = Guid.Empty;
        }

        // -----------------------------------------------------------------------
        // Properties
        // -----------------------------------------------------------------------

        /// <summary>
        /// Returns true if the C++ object is still alive (unmanagedPtr is non-null).
        /// </summary>
        public bool IsNativeAlive => __unmanagedPtr != null;

        /// <summary>
        /// Gets the unique object ID.
        /// </summary>
        public Guid ID => __internalId;

        // -----------------------------------------------------------------------
        // Virtual lifecycle callbacks
        // -----------------------------------------------------------------------

        /// <summary>
        /// Called by C++ when the C++ ScriptingObject is being destroyed.
        /// Override to perform cleanup on the C# side.
        /// After this call, __unmanagedPtr will be set to null by C++.
        /// </summary>
        public virtual void OnScriptingDispose()
        {
        }

        // -----------------------------------------------------------------------
        // Static C++ → C# callbacks
        // -----------------------------------------------------------------------

        /// <summary>
        /// Called by C++ (via NativeInterop) when a weak GC handle becomes invalid,
        /// meaning the GC has collected the managed instance.
        /// Forwards to the C++ side via the unmanaged pointer.
        /// </summary>
        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
        internal static void OnManagedInstanceDeleted(void* unmanagedPtr)
        {
            // C++ will handle the deletion of the C++ object.
            // This callback is a notification only — no managed-side action needed here
            // because the GC is already collecting this object.
            // The C++ side (ManagedScriptingObject::OnManagedInstanceDeleted) will
            // call DeleteObject() to clean up the C++ instance.
            //
            // Note: We cannot safely access 'this' here because the GC may have
            // already started collecting the object. The C++ pointer is passed
            // directly so C++ can handle cleanup without touching managed memory.
            _ = unmanagedPtr; // suppress unused warning; C++ uses this directly
        }

        /// <summary>
        /// Sets the internal C++ pointer and ID fields.
        /// Called by C++ via NativeInterop after creating the managed instance.
        /// </summary>
        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
        internal static void SetInternalValues(void* managedObjHandle, void* unmanagedPtr, Guid* id)
        {
            if (managedObjHandle == null)
                return;

            try
            {
                var gcHandle = GCHandle.FromIntPtr(new IntPtr(managedObjHandle));
                if (gcHandle.Target is ScriptingObject obj)
                {
                    obj.__unmanagedPtr = unmanagedPtr;
                    obj.__internalId   = id != null ? *id : Guid.Empty;
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"[ScriptingObject] SetInternalValues failed: {ex.Message}");
            }
        }

        /// <summary>
        /// Creates a new managed ScriptingObject instance of the given type,
        /// sets its __unmanagedPtr and __internalId, and returns a GC handle to it.
        /// Called by C++ via NativeInterop.
        /// </summary>
        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
        internal static void* CreateScriptingObject(void* typeHandle, void* unmanagedPtr, Guid* id)
        {
            if (typeHandle == null)
                return null;

            try
            {
                var gcHandle = GCHandle.FromIntPtr(new IntPtr(typeHandle));
                if (gcHandle.Target is not Type type)
                    return null;

                // Create the managed instance.
                var instance = Activator.CreateInstance(type) as ScriptingObject;
                if (instance == null)
                    return null;

                // Set the C++ pointer and ID.
                instance.__unmanagedPtr = unmanagedPtr;
                instance.__internalId   = id != null ? *id : Guid.Empty;

                // Return a strong GC handle so C++ can store it.
                var handle = GCHandle.Alloc(instance, GCHandleType.Normal);
                return (void*)(nint)GCHandle.ToIntPtr(handle);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"[ScriptingObject] CreateScriptingObject failed: {ex.Message}");
                return null;
            }
        }
    }
}
