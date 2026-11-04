// Adapted from Flax Editor. Copyright (c) Wojciech Figat. All rights reserved.

namespace SE.Editor.Gizmo
{
    public abstract partial class TransformGizmoBase
    {
        public override void Draw(GPUContext context)
        {
            if (!IsActive || !Visible || !UpdateGizmoPosition())
            {
                return;
            }
            Transform drawingTransform = m_GizmoWorld;
            if (IsTransforming && ActiveMode == Mode.Translate)
            {
                drawingTransform.Translation += m_LastTranslation;
            }
            Matrix world = drawingTransform.GetWorld();
            Color x = new Color(0.95f, 0.22f, 0.18f);
            Color y = new Color(0.30f, 0.85f, 0.22f);
            Color z = new Color(0.20f, 0.45f, 1.0f);
            Color focus = new Color(1.0f, 0.85f, 0.15f);

            if (ActiveMode != Mode.Rotate)
            {
                // Draw translucent planes before the solid axes and center handle.
                Owner.Renderer.Draw(Owner.RenderTask, context, 3, world, ActiveAxis == Axis.XY ? focus : new Color(z.R, z.G, z.B, 0.4f));
                Owner.Renderer.Draw(Owner.RenderTask, context, 3, Matrix.RotationX(Mathf.PiOverTwo) * world,
                    ActiveAxis == Axis.ZX ? focus : new Color(y.R, y.G, y.B, 0.4f));
                Owner.Renderer.Draw(Owner.RenderTask, context, 3, Matrix.RotationY(-Mathf.PiOverTwo) * world,
                    ActiveAxis == Axis.YZ ? focus : new Color(x.R, x.G, x.B, 0.4f));
                Owner.Renderer.Draw(Owner.RenderTask, context, 4, world, ActiveAxis == Axis.Center ? focus : Color.White);
            }

            int model = ActiveMode == Mode.Rotate ? 2 : ActiveMode == Mode.Scale ? 1 : 0;
            Owner.Renderer.Draw(Owner.RenderTask, context, model, world, (ActiveAxis & Axis.X) != 0 ? focus : x);
            Owner.Renderer.Draw(Owner.RenderTask, context, model, Matrix.RotationZ(Mathf.PiOverTwo) * world, (ActiveAxis & Axis.Y) != 0 ? focus : y);
            Owner.Renderer.Draw(Owner.RenderTask, context, model, Matrix.RotationY(-Mathf.PiOverTwo) * world, (ActiveAxis & Axis.Z) != 0 ? focus : z);
        }
    }
}
