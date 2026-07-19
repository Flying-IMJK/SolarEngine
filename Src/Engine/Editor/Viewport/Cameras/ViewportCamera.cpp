
#include "ViewportCamera.h"

#include "Runtime/Core/Types/Collections/List.h"
#include "Editor/Viewport/EditorViewport.h"

namespace SE::Editor
{
	void ViewportCamera::ShowActor(Actor* actor)
	{
		BoundingSphere sphere;
		// Editor.GetActorEditorSphere(actor, BoundingSphere sphere);
		ShowSphere(sphere);
	}

	void ViewportCamera::ShowActors(List<ScenesGraphNode*> selection)
	{
		Quaternion q = Quaternion(0.424461186f, -0.0940724313f, 0.0443938486f, 0.899451137f);
		ShowActors(selection, q);
	}

	void ViewportCamera::ShowActors(List<ScenesGraphNode*> selection, Quaternion& orientation)
	{
		if (selection.Count() == 0)
			return;

		BoundingSphere mergesSphere = BoundingSphere::Empty;
		/*for (int i = 0; i < selection.Count(); i++)
		{
			selection[i].GetEditorSphere(out var sphere);
			BoundingSphere::Merge(mergesSphere, sphere, out mergesSphere);
		}*/

		if (mergesSphere == BoundingSphere::Empty)
			return;
		ShowSphere(mergesSphere, orientation);
	}

	void ViewportCamera::ShowSphere(BoundingSphere& sphere)
	{
		Quaternion q = Quaternion(0.424461186f, -0.0940724313f, 0.0443938486f, 0.899451137f);
		ShowSphere(sphere, q);
	}

	void ViewportCamera::ShowSphere(BoundingSphere& sphere, Quaternion& orientation)
	{
		SetArcBallView(orientation, sphere.Center, sphere.Radius);
	}

	void ViewportCamera::SetArcBallView(const BoundingBox& objectBounds, float marginDistanceScale)
	{
		BoundingSphere result;
		BoundingSphere::FromBox(objectBounds, result);
		SetArcBallView(result, marginDistanceScale);
	}

	void ViewportCamera::SetArcBallView(const BoundingSphere& objectBounds, float marginDistanceScale)
	{
		SetArcBallView(Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), objectBounds.Center, objectBounds.Radius * marginDistanceScale);
	}

	void ViewportCamera::SetArcBallView(float orbitRadius)
	{
		SetArcBallView(Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), Float3::Zero, orbitRadius);
	}

	void ViewportCamera::SetArcBallView(Quaternion orientation, Float3 orbitCenter, float orbitRadius)
	{
		// Rotate
		m_Viewport->ViewOrientation = orientation;

		// Move camera to look at orbit center point
		Float3 localPosition = m_Viewport->ViewDirection.Get() * (-1 * orbitRadius);
		m_Viewport->ViewPosition = orbitCenter + localPosition;
	}
} // SE