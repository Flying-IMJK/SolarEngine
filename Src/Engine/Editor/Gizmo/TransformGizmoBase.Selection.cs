// Adapted from Flax Editor. Copyright (c) Wojciech Figat. All rights reserved.

namespace SE.Editor.Gizmo
{
    public abstract partial class TransformGizmoBase
    {
        private static readonly Axis[] s_Axes = { Axis.X, Axis.Y, Axis.Z };
        private static readonly Axis[] s_Handles = { Axis.X, Axis.Y, Axis.Z, Axis.XY, Axis.ZX, Axis.YZ, Axis.Center };
        // Boxes include a small screen-space tolerance around the matching FBX meshes.
        private static readonly BoundingBox[] s_HandleBoxes =
        {
            new BoundingBox(new Float3(0.13f, -0.075f, -0.075f), new Float3(1.03f, 0.075f, 0.075f)),
            new BoundingBox(new Float3(-0.075f, 0.13f, -0.075f), new Float3(0.075f, 1.03f, 0.075f)),
            new BoundingBox(new Float3(-0.075f, -0.075f, 0.13f), new Float3(0.075f, 0.075f, 1.03f)),
            new BoundingBox(new Float3(0.18f, 0.18f, -0.025f), new Float3(0.44f, 0.44f, 0.025f)),
            new BoundingBox(new Float3(0.18f, -0.025f, 0.18f), new Float3(0.44f, 0.025f, 0.44f)),
            new BoundingBox(new Float3(-0.025f, 0.18f, 0.18f), new Float3(0.025f, 0.44f, 0.44f)),
            new BoundingBox(new Float3(-0.09f), new Float3(0.09f)),
        };

        private Axis HitTest(Ray ray)
        {
            Ray localRay = new Ray(m_GizmoWorld.WorldToLocal(ray.Position),
                Float3.Normalize(m_GizmoWorld.WorldToLocalVector(ray.Direction)));
            Axis result = Axis.None;
            float closest = float.MaxValue;

            if (ActiveMode == Mode.Rotate)
            {
                foreach (Axis axis in s_Axes)
                {
                    Float3 normal = axis == Axis.X ? Float3.UnitX : axis == Axis.Y ? Float3.UnitY : Float3.UnitZ;
                    if (IntersectPlane(localRay, Float3.Zero, normal, out Float3 point))
                    {
                        float distance = (point - localRay.Position).Length;
                        if (Mathf.Abs(point.Length - 0.85f) <= 0.055f && distance < closest)
                        {
                            closest = distance;
                            result = axis;
                        }
                    }
                }
                return result;
            }

            for (int i = 0; i < s_HandleBoxes.Length; i++)
            {
                if (s_HandleBoxes[i].Intersects(ref localRay, out float distance) && distance < closest)
                {
                    closest = distance;
                    result = s_Handles[i];
                }
            }
            return result;
        }
    }
}
