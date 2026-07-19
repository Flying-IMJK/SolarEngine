// Managed editor GUI feature implementation.
namespace SE.Editor.GUI
{
    public readonly struct GuiPoint
    {
        public GuiPoint(float x, float y)
        {
            X = x;
            Y = y;
        }

        public float X { get; }
        public float Y { get; }

        public static GuiPoint Zero => new GuiPoint(0, 0);
    }

    public readonly struct GuiSize
    {
        public GuiSize(float width, float height)
        {
            Width = width;
            Height = height;
        }

        public float Width { get; }
        public float Height { get; }

        public static GuiSize Zero => new GuiSize(0, 0);
    }

    public readonly struct GuiRect
    {
        public GuiRect(float x, float y, float width, float height)
        {
            X = x;
            Y = y;
            Width = width;
            Height = height;
        }

        public float X { get; }
        public float Y { get; }
        public float Width { get; }
        public float Height { get; }
        public float Left => X;
        public float Top => Y;
        public float Right => X + Width;
        public float Bottom => Y + Height;
        public GuiPoint Location => new GuiPoint(X, Y);
        public GuiSize Size => new GuiSize(Width, Height);

        public bool Contains(GuiPoint point)
        {
            return point.X >= Left && point.X <= Right && point.Y >= Top && point.Y <= Bottom;
        }
    }

    public readonly struct GuiMargin
    {
        public GuiMargin(float all)
            : this(all, all, all, all)
        {
        }

        public GuiMargin(float left, float right, float top, float bottom)
        {
            Left = left;
            Right = right;
            Top = top;
            Bottom = bottom;
        }

        public float Left { get; }
        public float Right { get; }
        public float Top { get; }
        public float Bottom { get; }
        public float Width => Left + Right;
        public float Height => Top + Bottom;
    }
}
