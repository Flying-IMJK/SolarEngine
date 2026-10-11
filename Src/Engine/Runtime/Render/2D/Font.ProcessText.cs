using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SE
{
    public unsafe partial class Font
    {
        [LibraryImport("SERuntime", EntryPoint = "Font_ProcessTextLines", StringMarshalling = StringMarshalling.Utf16)]
        [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
        private static partial int Internal_ProcessTextLines(
            IntPtr font,
            string text,
            TextLayoutOptions layout,
            FontLineCacheMarshaller.FontLineCacheInternal* outputLines,
            int outputCapacity);

        public FontLineCache[] ProcessText(string text, TextLayoutOptions layout)
        {
            int count = Internal_ProcessTextLines(__unmanagedPtr, text, layout, null, 0);
            if (count == 0)
            {
                return Array.Empty<FontLineCache>();
            }

            var nativeLines = new FontLineCacheMarshaller.FontLineCacheInternal[count];
            fixed (FontLineCacheMarshaller.FontLineCacheInternal* output = nativeLines)
            {
                Internal_ProcessTextLines(__unmanagedPtr, text, layout, output, count);
            }

            var result = new FontLineCache[count];
            for (int i = 0; i < count; i++)
            {
                result[i] = FontLineCacheMarshaller.ConvertToManaged(nativeLines[i]);
            }

            return result;
        }
    }
}
