using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// Base class for data transferred by GUI drag-and-drop operations.
    /// </summary>
    public abstract class DragData
    {
        /// <summary>
        /// Gets the kind of data exposed by this drag operation.
        /// </summary>
        public abstract DragDataType DataType { get; }
    }

    /// <summary>
    /// Identifies the data exposed by a <see cref="DragData"/> instance.
    /// </summary>
    public enum DragDataType
    {
        Text,
        Files,
    }

    /// <summary>
    /// Drag-and-drop text data.
    /// </summary>
    public sealed class DragDataText : DragData
    {
        public DragDataText(string text)
        {
            Text = text ?? throw new ArgumentNullException(nameof(text));
        }

        public override DragDataType DataType => DragDataType.Text;

        public string Text { get; }
    }

    /// <summary>
    /// Drag-and-drop file paths.
    /// </summary>
    public sealed class DragDataFiles : DragData
    {
        private readonly string[] _files;

        public DragDataFiles(IEnumerable<string> files)
        {
            ArgumentNullException.ThrowIfNull(files);

            _files = files is string[] array ? (string[])array.Clone() : new List<string>(files).ToArray();
        }

        public override DragDataType DataType => DragDataType.Files;

        public IReadOnlyList<string> Files => _files;
    }
}
