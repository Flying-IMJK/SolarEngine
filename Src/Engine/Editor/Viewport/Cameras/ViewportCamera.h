#pragma once

#include "Runtime/Core/Math/BoundingVolumes.h"

namespace SE
{
    class Actor;
}

namespace SE::Editor
{
    class ScenesGraphNode;
	class EditorViewport;

    class ViewportCamera
	{
	private:
	    EditorViewport* m_Viewport = nullptr;

	public:
        /// <summary>
        /// Gets the parent viewport.
        /// </summary>
	    EditorViewport* GetViewport() { return m_Viewport; }
	    void SetViewport(EditorViewport* value) { m_Viewport = value; }

        /// <summary>
        /// Gets a value indicating whether the viewport camera uses movement speed.
        /// </summary>
	    virtual bool GetUseMovementSpeed() {return true;};

        /// <summary>
        /// Focuses the viewport on the current selection of the gizmo.
        /// </summary>
        /// <param name="gizmos">The gizmo collection (from viewport).</param>
        /// <param name="orientation">The target view orientation.</param>
	    /*virtual void FocusSelection(GizmosCollection gizmos, Quaternion& orientation)
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

        /// <summary>
        /// Moves the viewport to visualize the actor.
        /// </summary>
        /// <param name="actor">The actor to preview.</param>
	    virtual void ShowActor(Actor* actor);

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
	    void ShowActors(List<ScenesGraphNode*> selection);

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        /// <param name="orientation">The used orientation.</param>
        virtual void ShowActors(List<ScenesGraphNode*> selection, Quaternion& orientation);

        /// <summary>
        /// Moves the camera to visualize given world area defined by the sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        void ShowSphere(BoundingSphere& sphere);

        /// <summary>
        /// Moves the camera to visualize given world area defined by the sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="orientation">The camera orientation.</param>
        virtual void ShowSphere(BoundingSphere& sphere, Quaternion& orientation);

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given target object bounds.
        /// </summary>
        /// <param name="objectBounds">The target object bounds.</param>
        /// <param name="marginDistanceScale">The margin distance scale of the orbit radius.</param>
        void SetArcBallView(const BoundingBox& objectBounds, float marginDistanceScale = 2.0f);

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given target object bounds.
        /// </summary>
        /// <param name="objectBounds">The target object bounds.</param>
        /// <param name="marginDistanceScale">The margin distance scale of the orbit radius.</param>
        void SetArcBallView(const BoundingSphere& objectBounds, float marginDistanceScale = 2.0f);

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given orbit radius.
        /// </summary>
        /// <param name="orbitRadius">The orbit radius.</param>
        void SetArcBallView(float orbitRadius);

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view.
        /// </summary>
        /// <param name="orientation">The view rotation.</param>
        /// <param name="orbitCenter">The orbit center location.</param>
        /// <param name="orbitRadius">The orbit radius.</param>
        virtual void SetArcBallView(Quaternion orientation, Float3 orbitCenter, float orbitRadius);

        /// <inheritdoc />
        virtual void Update(float deltaTime)
        {
        }

        /// <inheritdoc />
        virtual void UpdateView(float dt, const Float3& moveDelta, const Float2& mouseDelta, bool& centerMouse) = 0;
	};

} // SE
