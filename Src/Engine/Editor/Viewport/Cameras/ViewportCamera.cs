

using System.Collections.Generic;
using SE.Editor.SceneGraph;

namespace SE.Editor
{
    /// <summary>
    /// Controls the view of an editor viewport.
    /// </summary>
    public abstract class ViewportCamera
    {
        private EditorViewport m_Viewport;

        public EditorViewport GetViewport()
        {
            return m_Viewport;
        }

        public void SetViewport(EditorViewport value)
        {
            m_Viewport = value;
        }

        public virtual bool GetUseMovementSpeed()
        {
            return true;
        }

        /*public virtual void FocusSelection(GizmosCollection gizmos, ref Quaternion orientation)
        {
            var transformGizmo = gizmos.Get<TransformGizmo>();
            if (transformGizmo == null || transformGizmo.SelectedParents.Count == 0)
                return;
            if (gizmos.Active != null)
            {
                var gizmoBounds = gizmos.Active.FocusBounds;
                if (gizmoBounds != BoundingSphere.Empty)
                {
                    ShowSphere(ref gizmoBounds, ref orientation);
                    return;
                }
            }
            ShowActors(transformGizmo.SelectedParents, ref orientation);
        }*/

        public virtual void ShowActor(Actor actor)
        {
            BoundingSphere sphere = default;
            // Editor.GetActorEditorSphere(actor, out sphere);
            ShowSphere(ref sphere);
        }

        public void ShowActors(List<ScenesGraphNode> selection)
        {
            Quaternion orientation = new Quaternion(0.424461186f, -0.0940724313f, 0.0443938486f, 0.899451137f);
            ShowActors(selection, ref orientation);
        }

        public virtual void ShowActors(List<ScenesGraphNode> selection, ref Quaternion orientation)
        {
            if (selection.Count == 0)
            {
                return;
            }

            BoundingSphere mergedSphere = BoundingSphere.Empty;
            /*for (int i = 0; i < selection.Count; i++)
            {
                selection[i].GetEditorSphere(out var sphere);
                BoundingSphere.Merge(ref mergedSphere, ref sphere, out mergedSphere);
            }*/
            if (mergedSphere == BoundingSphere.Empty)
            {
                return;
            }
            ShowSphere(ref mergedSphere, ref orientation);
        }

        public void ShowSphere(ref BoundingSphere sphere)
        {
            Quaternion orientation = new Quaternion(0.424461186f, -0.0940724313f, 0.0443938486f, 0.899451137f);
            ShowSphere(ref sphere, ref orientation);
        }

        public virtual void ShowSphere(ref BoundingSphere sphere, ref Quaternion orientation)
        {
            SetArcBallView(orientation, sphere.Center, sphere.Radius);
        }

        public void SetArcBallView(in BoundingBox objectBounds, float marginDistanceScale = 2.0f)
        {
            BoundingSphere sphere = BoundingSphere.FromBox(objectBounds);
            SetArcBallView(in sphere, marginDistanceScale);
        }

        public void SetArcBallView(in BoundingSphere objectBounds, float marginDistanceScale = 2.0f)
        {
            SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f),
                objectBounds.Center, objectBounds.Radius * marginDistanceScale);
        }

        public void SetArcBallView(float orbitRadius)
        {
            SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), Float3.Zero, orbitRadius);
        }

        public virtual void SetArcBallView(Quaternion orientation, Float3 orbitCenter, float orbitRadius)
        {
            m_Viewport.ViewOrientation = orientation;
            Float3 localPosition = m_Viewport.ViewDirection * (-orbitRadius);
            m_Viewport.ViewPosition = orbitCenter + localPosition;
        }

        public virtual void Update(float deltaTime)
        {
        }

        public abstract void UpdateView(float dt, in Float3 moveDelta, in Float2 mouseDelta, ref bool centerMouse);
    }
}
