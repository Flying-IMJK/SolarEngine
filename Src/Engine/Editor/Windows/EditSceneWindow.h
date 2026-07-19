#pragma once
#include "EditorWindow.h"
#include "Runtime/Core/Math/BoundingVolumes.h"
#include "Runtime/Core/Math/Transform.h"
#include "Editor/EditorApp.h"
#include "Editor/GUI/Viewport/RenderOutputViewport.h"

namespace SE
{
	class Actor;
	class Camera;
	class Button;
}

namespace SE::Editor
{
	class EditorGizmoViewport;

	class EditSceneWindow : public EditorWindow
	{
	public:
	    /// <summary>
        /// Camera preview output control.
        /// </summary>
	    class CameraPreview : RenderOutputViewport
        {
	    private:
	    	bool _isPinned;
	    	Button* _pinButton;

	    	void OnTaskBegin(RenderTask* task, GPUContext* context);

	    	void UpdatePinButton();

	    public:
            /// <summary>
            /// Gets or sets a value indicating whether this preview is pinned.
            /// </summary>
            PRO(IsPinned, CameraPreview, bool, __GetIsPinned, __SetIsPinned);
            /// <summary>
            /// Gets or sets the camera.
            /// </summary>
	    	PRO(Camera, CameraPreview, ::SE::Camera*, __GetCamera, __SetCamera);

            /// <summary>
            /// Initializes a new instance of the <see cref="CameraPreview"/> class.
            /// </summary>
	    	CameraPreview();

            /// <inheritdoc />
            void Draw() override;

            /// <inheritdoc />
            void OnDestroy() override;

	    private:
	    	bool __GetIsPinned() { return _isPinned; }
	    	void __SetIsPinned(bool value);
	    	::SE::Camera* __GetCamera();
	    	void __SetCamera(::SE::Camera* value);

        };

	private:
		List<CameraPreview*> _previews;
		Actor* _pilotActor;
		BoundingBox _pilotBounds;
		Transform _pilotStart;

	public:
		/// <summary>
		/// The viewport control.
		/// </summary>
		EditorGizmoViewport* Viewport;

		EditSceneWindow();
		~EditSceneWindow() override;

		void OnExit() override;

		void OnEditorStateChanged() override
		{
			EditorWindow::OnEditorStateChanged();

			UpdateCameraPreview();
		}

	private:
		void OnSelectionChanged()
		{
			UpdateCameraPreview();
		}

        /// <summary>
        /// Updates the camera previews.
        /// </summary>
		void UpdateCameraPreview();
	};

} // SE
