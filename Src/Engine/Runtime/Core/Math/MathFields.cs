namespace SE
{
    public partial struct BoundingBox
    {
        public Vector3 Minimum;
        public Vector3 Maximum;
    }

    public partial struct Color
    {
        public float R;
        public float G;
        public float B;
        public float A;
    }

    public partial struct Double2
    {
        public double X;
        public double Y;
    }

    public partial struct Double3
    {
        public double X;
        public double Y;
        public double Z;
    }

    public partial struct Double4
    {
        public double X;
        public double Y;
        public double Z;
        public double W;
    }

    public partial struct Float2
    {
        public float X;
        public float Y;
    }

    public partial struct Float3
    {
        public float X;
        public float Y;
        public float Z;
    }

    public partial struct Float4
    {
        public float X;
        public float Y;
        public float Z;
        public float W;
    }

    public partial struct Int2
    {
        public int X;
        public int Y;
    }

    public partial struct Int3
    {
        public int X;
        public int Y;
        public int Z;
    }

    public partial struct Int4
    {
        public int X;
        public int Y;
        public int Z;
        public int W;
    }

    public partial struct OrientedBoundingBox
    {
        public Vector3 Extents;
        public Transform Transformation;
    }

    public partial struct Quaternion
    {
        public float X;
        public float Y;
        public float Z;
        public float W;
    }

    public partial struct Ray
    {
        public Vector3 Position;
        public Vector3 Direction;
    }

    public partial struct Rectangle
    {
        public Float2 Location;
        public Float2 Size;
    }

    public partial struct Transform
    {
        public Vector3 Translation;
        public Quaternion Orientation;
        public Float3 Scale;
    }
}
