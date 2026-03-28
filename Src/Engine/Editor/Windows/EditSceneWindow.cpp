
#include "EditSceneWindow.h"

#include "Editor/Modules/SceneModule.h"
#include "Editor/Viewport/EditorGizmoViewport.h"
#include "Runtime/Render/RenderBuffers.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Common/Button.h"

namespace SE::Editor
{
	void EditSceneWindow::CameraPreview::OnTaskBegin(RenderTask* task, GPUContext* context)
	{
		SceneRenderTask* sceneTask = (SceneRenderTask*)task;

		// Copy camera view parameters for the scene rendering
		RenderView view = sceneTask->View;
		::SE::Viewport viewport = ::SE::Viewport(0, 0, sceneTask->Buffers->GetSize().x, sceneTask->Buffers->GetSize().y);
		view.CopyFrom(Camera, &viewport);
		sceneTask->View = view;
	}

	void EditSceneWindow::CameraPreview::UpdatePinButton()
	{
		if (_isPinned)
		{
			_pinButton->Text = "-";
			_pinButton->TooltipText = "Unpin preview";
		}
		else
		{
			_pinButton->Text = "+";
			_pinButton->TooltipText = "Pin preview";
		}
	}
	EditSceneWindow::CameraPreview::CameraPreview() : RenderOutputViewport(New<SceneRenderTask>())
	{
		// Don't steal focus
		AutoFocus = false;

		const float PinSize = 12.0f;
		const float PinMargin = 2.0f;

		_pinButton = New<Button>();
		_pinButton->AnchorPreset = AnchorPresets::TopRight,
		_pinButton->Parent = this,
		_pinButton->Bounds = Rectangle(Width - PinSize - PinMargin, PinMargin, PinSize, PinSize),

		_pinButton->Clicked.BindUnique(CreateFunc([this]()
		{
			IsPinned = !IsPinned;
		}));
		UpdatePinButton();

		m_Task->Begin.BindUnique<CameraPreview, &CameraPreview::OnTaskBegin>(this);
	}

	void EditSceneWindow::CameraPreview::Draw()
	{
		RenderOutputViewport::Draw();

		// Draw frame
		Render2D::DrawRectangle({Float2::Zero, Size}, Colors::Black);
	}

	void EditSceneWindow::CameraPreview::OnDestroy()
	{
		IsPinned = false;
		Camera = nullptr;
		_pinButton = nullptr;

		RenderOutputViewport::OnDestroy();
	}

	void EditSceneWindow::CameraPreview::__SetIsPinned(bool value)
	{
		if (_isPinned != value)
		{
			_isPinned = value;
			UpdatePinButton();
		}
	}

	::SE::Camera* EditSceneWindow::CameraPreview::__GetCamera()
	{
		return nullptr; //m_Task->Camera;
	}
	void EditSceneWindow::CameraPreview::__SetCamera(::SE::Camera* value)
	{
		// m_Task->Camera = value;
	}


	EditSceneWindow::EditSceneWindow() : EditorWindow(&EditorApp::Ins(), true, ScrollBars::None)
	{
		Title = SE_TEXT("Scene");

		// Create viewport
		Viewport = New<EditorGizmoViewport>(New<SceneRenderTask>(), editor->sceneModule->Root);
		Viewport->Parent = this;
		Viewport->NearPlane = 0.01f;
		Viewport->FarPlane = 20000.0f;
		// Viewport->GetTask()->ViewFlags = ViewFlags.DefaultEditor;

		// editor->sceneModule->SelectionChanged += OnSelectionChanged;
		// editor->sceneModule->ActorRemoved += SceneOnActorRemoved;
	}

	EditSceneWindow::~EditSceneWindow()
	{
		Delete(Viewport);
	}

	void EditSceneWindow::OnExit()
	{
		Viewport->Dispose();
	}

	void EditSceneWindow::UpdateCameraPreview()
	{
		/*// Disable rendering preview during GI baking
		if (Editor.StateMachine.CurrentState.IsPerformanceHeavy)
		{
			HideAllCameraPreviews();
			return;
		}

		var selection = Editor.SceneEditing.Selection;

		// Hide unpinned previews for which camera being previews is not selected
		for (int i = 0; i < _previews.Count; i++)
		{
			if (_previews[i].IsPinned)
				continue;
			var camera = _previews[i].Camera;
			var cameraNode = Editor.Scene.GetActorNode(camera);
			if (cameraNode == null || !selection.Contains(cameraNode))
			{
				// Hide it
				HideCameraPreview(_previews[i--]);
			}
		}

		if (Editor.Options.Options.Interface.ShowSelectedCameraPreview)
		{
			// Find any selected cameras and create previews for them
			for (int i = 0; i < selection.Count; i++)
			{
				if (selection[i] is CameraNode cameraNode)
				{
					// Check limit for cameras
					if (_previews.Count >= 8)
						break;

					var camera = (Camera)cameraNode.Actor;
					var preview = _previews.FirstOrDefault(x => x.Camera == camera);
					if (preview == null)
					{
						// Show it
						preview = new CameraPreview
						{
							Camera = camera,
							Parent = this
						};
						_previews.Add(preview);
					}
				}
			}
		}

		// Update previews locations
		int count = _previews.Count;
		if (count > 0)
		{
			// Update view dimensions and check if we can show it
			const float aspectRatio = 16.0f / 9.0f;
			const float minHeight = 20;
			const float minWidth = minHeight * aspectRatio;
			const float maxHeight = 150;
			const float maxWidth = maxHeight * aspectRatio;
			const float viewSpaceMaxPercentage = 0.7f;
			const float margin = 10;
			var totalSize = Size * viewSpaceMaxPercentage - margin;
			var singleSize = totalSize / count - count * margin;
			float sizeX = Mathf.Clamp(singleSize.X, minWidth, maxWidth);
			float sizeY = sizeX / aspectRatio;
			singleSize = new Float2(sizeX, sizeY);
			int countPerX = Mathf.FloorToInt(totalSize.X / singleSize.X);
			int countPerY = Mathf.FloorToInt(totalSize.Y / singleSize.Y);
			int index = 0;
			for (int y = 1; y <= countPerY; y++)
			{
				for (int x = 1; x <= countPerX; x++)
				{
					if (index == count)
						break;

					var pos = Size - (singleSize + margin) * new Float2(x, y);
					_previews[index++].Bounds = new Rectangle(pos, singleSize);
				}

				if (index == count)
					break;
			}
		}*/
	}
} // SE