
using System.Runtime.InteropServices;
using SE.Interop;

namespace SE
{
    /// <summary>
    /// Provides C# scripts profiling methods.
    /// </summary>
    /// <remarks>
    /// Profiler is available in the editor and Debug/Development builds. Release builds don't have profiling tools.
    /// </remarks>
    public static partial class Profiler
    {
        /// <summary>
        /// Begins profiling a piece of code with a custom label.
        /// </summary>
        /// <param name="name">The name of the event.</param>
        [LibraryImport("SERuntime", EntryPoint = "Internal_BeginEvent", StringMarshalling = StringMarshalling
            .Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        public static partial void BeginEvent(string name);

        /// <summary>
        /// Ends profiling an event.
        /// </summary>
        [LibraryImport("SERuntime", EntryPoint = "Internal_EndEvent", StringMarshalling = StringMarshalling.Custom,
            StringMarshallingCustomType = typeof(StringMarshaller))]
        public static partial void EndEvent();

        /// <summary>
        /// Begins GPU profiling a piece of code with a custom label.
        /// </summary>
        /// <param name="name">The name of the event.</param>
        [LibraryImport("SERuntime", EntryPoint = "Internal_BeginEventGPU", StringMarshalling = StringMarshalling
            .Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        public static partial void BeginEventGPU(string name);

        /// <summary>
        /// Ends GPU profiling an event.
        /// </summary>
        [LibraryImport("SERuntime", EntryPoint = "Internal_EndEventGPU", StringMarshalling = StringMarshalling
            .Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        public static partial void EndEventGPU();
    }

}
