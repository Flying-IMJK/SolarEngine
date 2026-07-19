namespace SE.Editor.GUI
{
    public enum DragDropEffect
    {
        None,
        Copy,
        Move,
        Link,
    }

    public abstract class DragData : SE.GUI.DragData
    {
    }

    public sealed class DragDataText : DragData
    {
        public DragDataText(string text)
        {
            Text = text;
        }

        public string Text { get; set; }

        public override SE.GUI.DragDataType DataType => SE.GUI.DragDataType.Text;
    }
}
