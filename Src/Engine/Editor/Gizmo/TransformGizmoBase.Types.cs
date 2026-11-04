// Adapted from Flax Editor. Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace SE.Editor.Gizmo
{
    public abstract partial class TransformGizmoBase
    {
        [Flags]
        public enum Axis
        {
            None = 0,
            X = 1,
            Y = 2,
            Z = 4,
            XY = X | Y,
            ZX = Z | X,
            YZ = Y | Z,
            Center = 8,
        }

        public enum Mode { Translate, Rotate, Scale }
        public enum TransformSpace { Local, World }
        public enum PivotType { ObjectCenter, SelectionCenter, WorldOrigin }
    }
}
