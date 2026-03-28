
#include "EditorViewport.h"

#include "Cameras/ViewportCamera.h"
#include "Editor/EditorIcons.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Editor/GUI/ContextMenu/ContextMenuChildMenu.h"
#include "Editor/GUI/Input/FloatValueBox.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/UI/GUI/Common/CheckBox.h"
#include "Runtime/Utilities/Time.h"
#include "Widgets/ViewportWidgetButton.h"
#include "Widgets/ViewportWidgetsContainer.h"

namespace SE::Editor
{
	void EditorViewport::Input::Gather(Window* window, bool useMouse, Input& prevInput)
	{
		IsControlDown = window->GetKey(KeyboardKeys::Control);
		IsShiftDown = window->GetKey(KeyboardKeys::Shift);
		IsAltDown = window->GetKey(KeyboardKeys::Alt);
		WasAltDownBefore = prevInput.WasAltDownBefore || prevInput.IsAltDown;

		IsMouseRightDown = useMouse && window->GetMouseButton(MouseButton::Right);
		IsMouseMiddleDown = useMouse && window->GetMouseButton(MouseButton::Middle);
		IsMouseLeftDown = useMouse && window->GetMouseButton(MouseButton::Left);
	}

	const Char* EditorViewport::GetMovementSpeedTextFormat()
	{
		if (Math::Abs(m_MovementSpeed - m_MaxMovementSpeed) < Math::EPSILON || Math::Abs(m_MovementSpeed - m_MinMovementSpeed) < Math::EPSILON)
			return SE_TEXT("{0}");

		if (m_MovementSpeed < 10.0f)
			return SE_TEXT("{0:2f}");
		else if (m_MovementSpeed < 100.0f)
			return SE_TEXT("{0:1f}");
		else
			return SE_TEXT("{0}");
	}

	EditorViewport::EditorViewport(SceneRenderTask* task, Editor::ViewportCamera* camera, bool useWidgets): RenderOutputViewport(task)
	{
		_mouseAccelerationScale = 0.1f;
		m_UseMouseFiltering = false;
		m_UseMouseAcceleration = false;
		m_Camera = camera;
		if (m_Camera != nullptr)
		{
			m_Camera->SetViewport(this);
		}

		AnchorPreset = AnchorPresets::StretchAll;
		Offsets = Margin::Zero;

		// Setup options
		{
			// Editor.Instance.Options.OptionsChanged += OnEditorOptionsChanged;
			SetupViewportOptions();
		}

		// Initialize camera values from cache
		/*if (_editor.ProjectCache.TryGetCustomData("CameraMovementSpeedValue", out var cachedState))
			MovementSpeed = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraMinMovementSpeedValue", out cachedState))
			_minMovementSpeed = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraMaxMovementSpeedValue", out cachedState))
			_maxMovementSpeed = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("UseCameraEasingState", out cachedState))
			_useCameraEasing = bool.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraPanningSpeedValue", out cachedState))
			_panningSpeed = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraInvertPanningState", out cachedState))
			_invertPanning = bool.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraRelativePanningState", out cachedState))
			_relativePanning = bool.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraOrthographicState", out cachedState))
			_isOrtho = bool.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraOrthographicSizeValue", out cachedState))
			_orthoSize = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraFieldOfViewValue", out cachedState))
			_fieldOfView = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraNearPlaneValue", out cachedState))
			_nearPlane = float.Parse(cachedState);
		if (_editor.ProjectCache.TryGetCustomData("CameraFarPlaneValue", out cachedState))
			_farPlane = float.Parse(cachedState)*/
		;

		OnCameraMovementProgressChanged();

		if (useWidgets)
		{
#pragma region Camera settings widget

			const Char* largestText = SE_TEXT("Relative Panning");
			Float2 textSize = Style::Current->FontMedium->MeasureText(largestText);
			float xLocationForExtras = textSize.x + 5;
			float cameraSpeedTextWidth = Style::Current->FontMedium->MeasureText(SE_TEXT("0.00")).x;

			// Camera Settings Widget
			_cameraWidget = New<ViewportWidgetsContainer>(ViewportWidgetLocation::UpperRight);

			// Camera Settings Menu
			ContextMenu* cameraCM = New<ContextMenu>();
			_cameraButton = New<ViewportWidgetButton>(String::Format(GetMovementSpeedTextFormat(), m_MovementSpeed), EditorIcons::Camera64, cameraCM, false, cameraSpeedTextWidth);
			_cameraButton->Tag = this;
			_cameraButton->TooltipText = "Camera Settings";
			_cameraButton->Parent = _cameraWidget;
			_cameraWidget->Parent = this;

			// Orthographic/Perspective Mode Widget
			_orthographicModeButton = New<ViewportWidgetButton>(String::Empty, EditorIcons::CamSpeed32, nullptr, true);
			_orthographicModeButton->Checked = !UseOrthographicProjection,
			_orthographicModeButton->TooltipText = "Toggle Orthographic/Perspective Mode",
			_orthographicModeButton->Parent = _cameraWidget;
			_orthographicModeButton->Toggled.BindUnique(CreateFunc([this](ViewportWidgetButton* button)
			{
				OnOrthographicModeToggled(button);
			}));

			// Camera Speed
			auto camSpeedButton = cameraCM->AddButton(SE_TEXT("Camera Speed"));
			camSpeedButton->CloseMenuOnClick = false;
			FloatValueBox* camSpeedValue = New<FloatValueBox>(m_MovementSpeed, xLocationForExtras, 2, 70.0f, m_MinMovementSpeed, m_MaxMovementSpeed, 0.5f);
			camSpeedValue->Parent = camSpeedButton;

			camSpeedValue->ValueChanged.BindUnique(CreateFunc([camSpeedValue, this]()
			{
				OnMovementSpeedChanged(camSpeedValue);
			}));
			cameraCM->VisibleChanged.BindUnique(CreateFunc([camSpeedValue, this](Control* control)
			{
				camSpeedValue->SetValue(m_MovementSpeed);
			}));

			// Minimum & Maximum Camera Speed
			auto minCamSpeedButton = cameraCM->AddButton(SE_TEXT("Min Cam Speed"));
			minCamSpeedButton->CloseMenuOnClick = false;
			FloatValueBox* minCamSpeedValue = New<FloatValueBox>(m_MinMovementSpeed, xLocationForExtras, 2, 70.0f, 0.05f, m_MaxMovementSpeed, 0.5f);
			minCamSpeedValue->Parent = minCamSpeedButton;

			auto maxCamSpeedButton = cameraCM->AddButton(SE_TEXT("Max Cam Speed"));
			maxCamSpeedButton->CloseMenuOnClick = false;
			FloatValueBox* maxCamSpeedValue = New<FloatValueBox>(m_MaxMovementSpeed, xLocationForExtras, 2, 70.0f, m_MinMovementSpeed, 1000.0f, 0.5f);
			maxCamSpeedValue->Parent = maxCamSpeedButton;

			minCamSpeedValue->ValueChanged.BindUnique(CreateFunc([minCamSpeedValue, maxCamSpeedValue, camSpeedValue, this]()
			{
				OnMinMovementSpeedChanged(minCamSpeedValue);

				maxCamSpeedValue->SetMinValue(minCamSpeedValue->GetValue());

				if (Math::Abs(camSpeedValue->GetMinValue() - minCamSpeedValue->GetValue()) > Math::EPSILON)
				{
					camSpeedValue->SetMinValue(minCamSpeedValue->GetValue());
				}
			}));

			cameraCM->VisibleChanged.BindUnique(CreateFunc([minCamSpeedValue, this](Control* control)
			{
				minCamSpeedValue->SetValue(m_MinMovementSpeed);
			}));

			maxCamSpeedValue->ValueChanged.BindUnique(CreateFunc([minCamSpeedValue, camSpeedValue, maxCamSpeedValue, this]()
			{
				OnMaxMovementSpeedChanged(maxCamSpeedValue);

				minCamSpeedValue->SetMaxValue(maxCamSpeedValue->GetValue());

				if (Math::Abs(camSpeedValue->GetMaxValue() - maxCamSpeedValue->GetValue()) > Math::EPSILON)
					camSpeedValue->SetMaxValue(maxCamSpeedValue->GetValue());
			}));
			cameraCM->VisibleChanged.BindUnique(CreateFunc([maxCamSpeedValue, this](Control* control)
			{
				maxCamSpeedValue->SetValue(m_MaxMovementSpeed);
			}));

			// Camera Easing
			{
				auto useCameraEasing = cameraCM->AddButton(SE_TEXT("Camera Easing"));
				useCameraEasing->CloseMenuOnClick = false;
				CheckBox* useCameraEasingValue = New<CheckBox>(xLocationForExtras, 2, _useCameraEasing);
				useCameraEasingValue->Parent = useCameraEasing;

				useCameraEasingValue->StateChanged.BindUnique([this](CheckBox* checkBox)
				{
					OnCameraEasingToggled(checkBox);
				});
				cameraCM->VisibleChanged.BindUnique(CreateFunc([useCameraEasingValue, this](Control* control)
				{
					useCameraEasingValue->Checked = _useCameraEasing;
				}));

				// Panning Speed
				{
					auto panningSpeed = cameraCM->AddButton(SE_TEXT("Panning Speed"));
					panningSpeed->CloseMenuOnClick = false;
					FloatValueBox* panningSpeedValue = New<FloatValueBox>(PanningSpeed, xLocationForExtras, 2, 70.0f, 0.01f, 128.0f, 0.1f);
					panningSpeedValue->Parent = panningSpeed;

					panningSpeedValue->ValueChanged.BindUnique(CreateFunc([panningSpeedValue, this]()
					{
						OnPanningSpeedChanged(panningSpeedValue);
					}));
					cameraCM->VisibleChanged.BindUnique([panningSpeed, panningSpeedValue, this](Control* control)
					{
						panningSpeed->Visible = !RelativePanning;
						panningSpeedValue->SetValue(PanningSpeed);
					});
				}

				// Relative Panning
				{
					auto relativePanning = cameraCM->AddButton(SE_TEXT("Relative Panning"));
					relativePanning->CloseMenuOnClick = false;
					CheckBox* relativePanningValue = New<CheckBox>(xLocationForExtras, 2, RelativePanning);
					relativePanningValue->Parent = relativePanning;

					relativePanningValue->StateChanged.BindUnique(CreateFunc([cameraCM, this](CheckBox* checkBox)
					{
						if (checkBox->Checked != RelativePanning)
						{
							OnRelativePanningToggled(checkBox);
							cameraCM->Hide();
						}
					}));
					cameraCM->VisibleChanged.BindUnique(CreateFunc([relativePanningValue, this](Control* control)
					{
						relativePanningValue->Checked = RelativePanning;
					}));
				}

				// Invert Panning
				{
					auto invertPanning = cameraCM->AddButton(SE_TEXT("Invert Panning"));
					invertPanning->CloseMenuOnClick = false;
					CheckBox* invertPanningValue = New<CheckBox>(xLocationForExtras, 2, InvertPanning);
					invertPanningValue->Parent = invertPanning;

					invertPanningValue->StateChanged.BindUnique(CreateFunc([this](CheckBox* checkBox)
					{
						OnInvertPanningToggled(checkBox);
					}));
					cameraCM->VisibleChanged.BindUnique(CreateFunc([invertPanningValue, this](Control* control)
					{
						invertPanningValue->Checked = InvertPanning;
					}));
				}

				cameraCM->AddSeparator();

				// Camera Viewpoints
				/*{
					auto cameraView = cameraCM->AddChildMenu(SE_TEXT("Viewpoints"))->ContextMenu;
					for (int i = 0; i < CameraViewpointValues.Length; i++)
					{
						var co = CameraViewpointValues[i];
						var button = cameraView->AddButton(co.Name);
						button.Tag = co.Orientation;
					}

					cameraView->ButtonClicked += OnViewpointChanged;
				}*/

				// Orthographic Mode
				{
					auto ortho = cameraCM->AddButton(SE_TEXT("Orthographic"));
					ortho->CloseMenuOnClick = false;
					CheckBox* orthoValue = New<CheckBox>(xLocationForExtras, 2, UseOrthographicProjection);
					orthoValue->Parent = ortho;

					orthoValue->StateChanged.BindUnique(CreateFunc([cameraCM, this](CheckBox* checkBox)
					{
						if (checkBox->Checked != UseOrthographicProjection)
						{
							OnOrthographicModeToggled(checkBox);
							cameraCM->Hide();
						}
					}));
					cameraCM->VisibleChanged.BindUnique(CreateFunc([orthoValue, this](Control* control)
					{
						orthoValue->Checked = UseOrthographicProjection;
					}));
				}

				// Field of View
				{
					auto fov = cameraCM->AddButton(SE_TEXT("Field Of View"));
					fov->CloseMenuOnClick = false;
					FloatValueBox* fovValue = New<FloatValueBox>(FieldOfView, xLocationForExtras, 2, 70.0f, 35.0f, 160.0f, 0.1f);
					fovValue->Parent = fov;

					fovValue->ValueChanged.BindUnique(CreateFunc([fovValue, this]()
					{
						OnFieldOfViewChanged(fovValue);
					}));
					cameraCM->VisibleChanged.BindUnique([fov, fovValue, this](Control* control)
					{
						fov->Visible = !UseOrthographicProjection;
						fovValue->SetValue(FieldOfView);
					});
				}

				// Orthographic Scale
				{
					auto orthoSize = cameraCM->AddButton(SE_TEXT("Ortho Scale"));
					orthoSize->CloseMenuOnClick = false;
					FloatValueBox* orthoSizeValue = New<FloatValueBox>(OrthographicScale, xLocationForExtras, 2, 70.0f, 0.001f, 100000.0f, 0.01f);
					orthoSizeValue->Parent = orthoSize;

					orthoSizeValue->ValueChanged.BindUnique(CreateFunc([orthoSizeValue, this]()
					{
						OnOrthographicSizeChanged(orthoSizeValue);
					}));
					cameraCM->VisibleChanged.BindUnique([orthoSize, orthoSizeValue, this](Control* control)
					{
						orthoSize->Visible = UseOrthographicProjection;
						orthoSizeValue->SetValue(OrthographicScale);
					});
				}

				// Near Plane
				{
					auto nearPlane = cameraCM->AddButton(SE_TEXT("Near Plane"));
					nearPlane->CloseMenuOnClick = false;
					FloatValueBox* nearPlaneValue = New<FloatValueBox>(NearPlane, xLocationForExtras, 2, 70.0f, 0.001f, 1000.0f);
					nearPlaneValue->Parent = nearPlane;

					nearPlaneValue->ValueChanged.BindUnique(CreateFunc([nearPlaneValue, this]()
					{
						OnNearPlaneChanged(nearPlaneValue);
					}));
					cameraCM->VisibleChanged.BindUnique([nearPlane, nearPlaneValue, this](Control* control)
					{
						nearPlane->Visible = UseOrthographicProjection;
						nearPlaneValue->SetValue(NearPlane);
					});
				}

				// Far Plane
				{
					auto farPlane = cameraCM->AddButton(SE_TEXT("Far Plane"));
					farPlane->CloseMenuOnClick = false;
					FloatValueBox* farPlaneValue = New<FloatValueBox>(FarPlane, xLocationForExtras, 2, 70.0f, 10.0f);
					farPlaneValue->Parent = farPlane;

					farPlaneValue->ValueChanged.BindUnique(CreateFunc([farPlaneValue, this]()
					{
						OnFarPlaneChanged(farPlaneValue);
					}));
					cameraCM->VisibleChanged.BindUnique([farPlaneValue, this](Control* control)
					{
						farPlaneValue->SetValue(NearPlane);
					});
				}

				cameraCM->AddSeparator();

				// Reset Button
				{
					auto reset = cameraCM->AddButton(SE_TEXT("Reset to default"));
					reset->ButtonClicked.BindUnique(CreateFunc([minCamSpeedValue, camSpeedValue, maxCamSpeedValue, this](ContextMenuButton* button)
					{
						SetupViewportOptions();

						// if the context menu is opened without triggering the value changes beforehand,
						// the movement speed will not be correctly reset to its default value in certain cases
						// therefore, a UI update needs to be triggered here
						minCamSpeedValue->SetValue(m_MinMovementSpeed);
						camSpeedValue->SetValue(m_MovementSpeed);
						maxCamSpeedValue->SetValue(m_MaxMovementSpeed);
					}));
				}

#pragma endregion Camera settings widget

#pragma region View mode widget

				largestText = SE_TEXT("Brightness");
				textSize = Style::Current->FontMedium->MeasureText(largestText);
				xLocationForExtras = textSize.x + 5;

				/*var viewMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
				ViewWidgetButtonMenu = new ContextMenu();
				var viewModeButton = new ViewportWidgetButton("View", SpriteHandle.Invalid, ViewWidgetButtonMenu)
				{
					TooltipText = "View properties",
						Parent = viewMode
				};
				viewMode.Parent = this;

				// Show
				{
					ViewWidgetShowMenu = ViewWidgetButtonMenu.AddChildMenu("Show").ContextMenu;

					// Show FPS
					{
						InitFpsCounter();
						_showFpsButton = ViewWidgetShowMenu.AddButton("FPS Counter", ()=> ShowFpsCounter = !ShowFpsCounter);
						_showFpsButton.CloseMenuOnClick = false;
					}
				}

				// View Layers
				{
					var viewLayers = ViewWidgetButtonMenu.AddChildMenu("View Layers").ContextMenu;
					viewLayers.AddButton("Copy layers", ()=> Clipboard.Text = JsonSerializer.Serialize(Task.View.RenderLayersMask));
					viewLayers.AddButton("Paste layers",
						()=>
						{
					try
					{
						Task.ViewLayersMask = JsonSerializer.Deserialize<LayersMask>(Clipboard.Text);

					}
			catch
					{
					}
				});
					viewLayers.AddButton("Reset layers", ()=> Task.ViewLayersMask = LayersMask.Default).Icon = Editor.Instance.Icons.Rotate32;
					viewLayers.AddButton("Disable layers", ()=> Task.ViewLayersMask = new LayersMask(0)).Icon = Editor.Instance.Icons.Rotate32;
					viewLayers.AddSeparator();
					var layers = LayersAndTagsSettings.GetCurrentLayers();
					if (layers != null && layers.Length > 0)
					{
						for (int i = 0; i < layers.Length; i++)
						{
							var layer = layers[i];
							var button = viewLayers.AddButton(layer);
							button.CloseMenuOnClick = false;
							button.Tag = 1 << i;
						}
					}
					viewLayers.ButtonClicked += button =  >
					{
						if (button.Tag != null)
						{
							int layerIndex = (int)button.Tag;
							LayersMask mask = new LayersMask(layerIndex);
							Task.ViewLayersMask ^= mask;
							button.Icon = (Task.ViewLayersMask & mask) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
						}
					};
					viewLayers.VisibleChanged += WidgetViewLayersShowHide;
				}

				// View Flags
				{
					var viewFlags = ViewWidgetButtonMenu.AddChildMenu("View Flags").ContextMenu;
					viewFlags.AddButton("Copy flags", ()=> Clipboard.Text = JsonSerializer.Serialize(Task.ViewFlags));
					viewFlags.AddButton("Paste flags",
						()=>
						{
					try
					{
						Task.ViewFlags = JsonSerializer.Deserialize<ViewFlags>(Clipboard.Text);

					}
			catch
					{
					}
				});
					viewFlags.AddButton("Reset flags", ()=> Task.ViewFlags = ViewFlags.DefaultEditor).Icon = Editor.Instance.Icons.Rotate32;
					viewFlags.AddButton("Disable flags", ()=> Task.ViewFlags = ViewFlags.None).Icon = Editor.Instance.Icons.Rotate32;
					viewFlags.AddSeparator();
					for (int i = 0; i < ViewFlagsValues.Length; i++)
					{
						var v = ViewFlagsValues[i];
						var button = viewFlags.AddButton(v.Name);
						button.CloseMenuOnClick = false;
						button.Tag = v.Mode;
					}
					viewFlags.ButtonClicked += button =  >
					{
						if (button.Tag != null)
						{
							var v = (ViewFlags)button.Tag;
							Task.ViewFlags ^= v;
							button.Icon = (Task.ViewFlags & v) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
						}
					};
					viewFlags.VisibleChanged += WidgetViewFlagsShowHide;
				}

				// Debug View
				{
					var debugView = ViewWidgetButtonMenu.AddChildMenu("Debug View").ContextMenu;
					debugView.AddButton("Copy view", ()=> Clipboard.Text = JsonSerializer.Serialize(Task.ViewMode));
					debugView.AddButton("Paste view",
						()=>
						{
					try
					{
						Task.ViewMode = JsonSerializer.Deserialize<ViewMode>(Clipboard.Text);

					}
			catch
					{
					}
				});
					debugView.AddSeparator();
					for (int i = 0; i < ViewModeValues.Length; i++)
					{
						ref var v = ref ViewModeValues[i];
						if (v.Options != null)
						{
							var childMenu = debugView.AddChildMenu(v.Name).ContextMenu;
							childMenu.ButtonClicked += WidgetViewModeShowHideClicked;
							childMenu.VisibleChanged += WidgetViewModeShowHide;
							for (int j = 0; j < v.Options.Length; j++)
							{
								ref var vv = ref
								v.Options[j];
								var button = childMenu.AddButton(vv.Name);
								button.CloseMenuOnClick = false;
								button.Tag = vv.Mode;
							}
						}
						else
						{
							var button = debugView.AddButton(v.Name);
							button.CloseMenuOnClick = false;
							button.Tag = v.Mode;
						}
					}
					debugView.ButtonClicked += WidgetViewModeShowHideClicked;
					debugView.VisibleChanged += WidgetViewModeShowHide;
				}

				ViewWidgetButtonMenu.AddSeparator();

				// Brightness
				{
					var brightness = ViewWidgetButtonMenu.AddButton("Brightness");
					brightness.CloseMenuOnClick = false;
					var brightnessValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.001f, 10.0f, 0.001f)
					{
						Parent = brightness
					};
					brightnessValue.ValueChanged += () =  > Brightness = brightnessValue.Value;
					ViewWidgetButtonMenu.VisibleChanged += control =  > brightnessValue.Value = Brightness;
				}

				// Resolution
				{
					var resolution = ViewWidgetButtonMenu.AddButton("Resolution");
					resolution.CloseMenuOnClick = false;
					var resolutionValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.1f, 4.0f, 0.001f)
					{
						Parent = resolution
					};
					resolutionValue.ValueChanged += () =  > ResolutionScale = resolutionValue.Value;
					ViewWidgetButtonMenu.VisibleChanged += control =  > resolutionValue.Value = ResolutionScale;
				}*/

#pragma endregion View mode widget
			}

			/*InputActions.Add(options => options.ViewpointTop, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Top").Orientation)));
			InputActions.Add(options => options.ViewpointBottom, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Bottom").Orientation)));
			InputActions.Add(options => options.ViewpointFront, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Front").Orientation)));
			InputActions.Add(options => options.ViewpointBack, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Back").Orientation)));
			InputActions.Add(options => options.ViewpointRight, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Right").Orientation)));
			InputActions.Add(options => options.ViewpointLeft, () => OrientViewport(Quaternion.Euler(CameraViewpointValues.First(vp => vp.Name == "Left").Orientation)));
			InputActions.Add(options => options.CameraToggleRotation, () => _isVirtualMouseRightDown = !_isVirtualMouseRightDown);
			InputActions.Add(options => options.CameraIncreaseMoveSpeed, () => AdjustCameraMoveSpeed(1));
			InputActions.Add(options => options.CameraDecreaseMoveSpeed, () => AdjustCameraMoveSpeed(-1));
			InputActions.Add(options => options.ToggleOrthographic, () => OnOrthographicModeToggled(null));*/

			// Link for task event
			task->Begin.BindUnique<EditorViewport, &EditorViewport::OnRenderBegin>(this);
		}
	}
	void EditorViewport::OnMovementSpeedChanged(FloatValueBox* control)
	{
		float value = Math::Clamp(control->GetValue(), m_MinMovementSpeed, m_MaxMovementSpeed);
		MovementSpeed = value;

		OnCameraMovementProgressChanged();
		// _editor.ProjectCache.SetCustomData("CameraMovementSpeedValue", _movementSpeed.ToString());
	}

	void EditorViewport::OnMinMovementSpeedChanged(FloatValueBox* control)
	{
		float value = Math::Clamp(control->GetValue(), 0.05f, m_MaxMovementSpeed);
		m_MinMovementSpeed = value;

		if (m_MovementSpeed < value)
			MovementSpeed = value;

		OnCameraMovementProgressChanged();
		// _editor.ProjectCache.SetCustomData("CameraMinMovementSpeedValue", _minMovementSpeed.ToString());
	}

	void EditorViewport::OnMaxMovementSpeedChanged(FloatValueBox* control)
	{
		float value = Math::Clamp(control->GetValue(), m_MinMovementSpeed, 1000.0f);
		m_MaxMovementSpeed = value;

		if (m_MovementSpeed > value)
			MovementSpeed = value;

		OnCameraMovementProgressChanged();
		// _editor.ProjectCache.SetCustomData("CameraMaxMovementSpeedValue", _maxMovementSpeed.ToString());
	}

	void EditorViewport::OnCameraEasingToggled(Control* control)
	{
		_useCameraEasing = !_useCameraEasing;

		OnCameraMovementProgressChanged();
		// _editor.ProjectCache.SetCustomData("UseCameraEasingState", _useCameraEasing.ToString());
	}

	void EditorViewport::OnPanningSpeedChanged(FloatValueBox* control)
	{
		PanningSpeed = control->GetValue();
		// _editor.ProjectCache.SetCustomData("CameraPanningSpeedValue", _panningSpeed.ToString());
	}

	void EditorViewport::__SetViewportCamera(::SE::Editor::ViewportCamera* value)
	{
		if (m_Camera != nullptr)
			m_Camera->SetViewport(nullptr);

		m_Camera = value;

		if (m_Camera != nullptr)
			m_Camera->SetViewport(this);
	}

	void EditorViewport::OnRelativePanningToggled(Control* control)
	{
		RelativePanning = !RelativePanning;
		// _editor.ProjectCache.SetCustomData("CameraRelativePanningState", RelativePanning.ToString());
	}

	void EditorViewport::OnInvertPanningToggled(Control* control)
	{
		InvertPanning = !InvertPanning;
		// _editor.ProjectCache.SetCustomData("CameraInvertPanningState", InvertPanning.ToString());
	}

	void EditorViewport::OnViewpointChanged(ContextMenuButton* button)
	{
		Quaternion orient = Quaternion::Euler(std::any_cast<Float3>(button->Tag));
		OrientViewport(orient);
	}

	void EditorViewport::OnFieldOfViewChanged(FloatValueBox* control)
	{
		FieldOfView = control->GetValue();
		// _editor.ProjectCache.SetCustomData("CameraFieldOfViewValue", FieldOfView.ToString());
	}

	void EditorViewport::OnOrthographicModeToggled(Control* control)
	{
		UseOrthographicProjection = !UseOrthographicProjection;

		/*if (_orthographicModeButton != nullptr)
			_orthographicModeButton.Checked = !UseOrthographicProjection;*/

		if (UseOrthographicProjection)
		{
			Quaternion orient = ViewOrientation;
			OrientViewport(orient);
		}

		// _editor.ProjectCache.SetCustomData("CameraOrthographicState", UseOrthographicProjection.ToString());
	}

	void EditorViewport::OnOrthographicSizeChanged(FloatValueBox* control)
	{
		OrthographicScale = control->GetValue();
		// _editor.ProjectCache.SetCustomData("CameraOrthographicSizeValue", OrthographicScale.ToString());
	}

	void EditorViewport::OnNearPlaneChanged(FloatValueBox* control)
	{
		NearPlane = control->GetValue();
		// _editor.ProjectCache.SetCustomData("CameraNearPlaneValue", NearPlane.ToString());
	}

	void EditorViewport::OnFarPlaneChanged(FloatValueBox* control)
	{
		FarPlane = control->GetValue();
		// _editor.ProjectCache.SetCustomData("CameraFarPlaneValue", _farPlane.ToString());
	}

	void EditorViewport::Update(float deltaTime)
	{
		RenderOutputViewport::Update(deltaTime);

		if (_disableInputUpdate)
			return;

		// Update camera
		bool useMovementSpeed = false;
		if (m_Camera != nullptr)
		{
			m_Camera->Update(deltaTime);
			useMovementSpeed = m_Camera->GetUseMovementSpeed();

			/*if (_cameraButton != nullptr)
			{
				_cameraButton.Parent.Visible = useMovementSpeed;
			}*/
		}

		// Get parent window
		WindowRootControl* win = (WindowRootControl*)Root.Get();

		// Get current mouse position in the view
		{
			// When the window is not focused, the position in window does not return sane values
			Float2 pos = PointFromWindow(win->GetMousePosition());
			/*if (!floatIN float.IsInfinity(pos.LengthSquared))
			{*/
				m_ViewMousePos = pos;
			// }
		}

		// Update input
		GraphicWindow* window = win->Window();
		bool canUseInput = window != nullptr && window->IsFocused() && window->IsForegroundWindow();
		{
			// Get input buttons and keys (skip if viewport has no focus or mouse is over a child control)
			bool isViewportControllingMouse = canUseInput && GetIsControllingMouse();
			if (isViewportControllingMouse != _isViewportControllingMouse)
			{
				_isViewportControllingMouse = isViewportControllingMouse;
				if (isViewportControllingMouse)
					StartMouseCapture();
				else
					EndMouseCapture();
			}
			bool useMouse = GetIsControllingMouse() || m_PrevInput.GetIsControllingMouse() ||
				(Math::RangeInclusive(m_ViewMousePos.x, 0.0f, Width.Get()) && Math::RangeInclusive(m_ViewMousePos.y, 0.0f, Height.Get()));
			m_PrevInput = m_Input;

			Control* hit = GetChildAt(m_ViewMousePos, [](Control* c)
			{
				return c->Visible/* && !(TypeIs<CanvasRootControl>(c) && !(TypeIs<UIEditorRoot>(c))*/;
			});

			if (canUseInput && ContainsFocus() && (hit == nullptr || m_PrevInput.GetIsControllingMouse()))
			{
				m_Input.Gather(win->Window(), useMouse, m_PrevInput);
			}
			else
			{
				m_Input.Clear();
			}

			// Track controlling mouse state change
			bool lastControllingMouse = m_PrevInput.GetIsControllingMouse();
			m_IsControllingMouse = m_Input.GetIsControllingMouse();

			// Simulate holding mouse right down for trackpad users
			if ((m_PrevInput.IsMouseRightDown && !m_Input.IsMouseRightDown) || win->GetKeyDown(KeyboardKeys::Escape))
				m_IsVirtualMouseRightDown = false; // Cancel when mouse right or escape is pressed
			if (m_WasVirtualMouseRightDown)
				lastControllingMouse = true;
			if (m_IsVirtualMouseRightDown)
				m_IsControllingMouse = m_IsVirtualMouseRightDown;

			if (lastControllingMouse != m_IsControllingMouse)
			{
				if (m_IsControllingMouse)
				{
					OnControlMouseBegin(win->Window());
				}
				else
				{
					OnControlMouseEnd(win->Window());
				}
			}

			// Track mouse buttons state change
			if (!m_PrevInput.IsMouseLeftDown && m_Input.IsMouseLeftDown)
				OnLeftMouseButtonDown();
			else if (m_PrevInput.IsMouseLeftDown && !m_Input.IsMouseLeftDown)
				OnLeftMouseButtonUp();

			if ((!m_PrevInput.IsMouseRightDown && m_Input.IsMouseRightDown) || (!m_WasVirtualMouseRightDown && m_IsVirtualMouseRightDown))
				OnRightMouseButtonDown();
			else if ((m_PrevInput.IsMouseRightDown && !m_Input.IsMouseRightDown) || (m_WasVirtualMouseRightDown && !m_IsVirtualMouseRightDown))
				OnRightMouseButtonUp();

			if (!m_PrevInput.IsMouseMiddleDown && m_Input.IsMouseMiddleDown)
				OnMiddleMouseButtonDown();
			else if (m_PrevInput.IsMouseMiddleDown && !m_Input.IsMouseMiddleDown)
				OnMiddleMouseButtonUp();

			m_WasVirtualMouseRightDown = m_IsVirtualMouseRightDown;
		}

		// Get clamped delta time (more stable during lags)
		float dt = Math::Min(Time::GetUnscaledDeltaTime(), 1.0f);

		// Check if update mouse
		Float2 size = Size;
		// var options = Editor.Instance.Options.Options;
		if (m_IsControllingMouse)
		{
			bool rmbWheel = false;

			// Gather input
			{
				bool isAltDown = m_Input.IsAltDown;
				bool lbDown = m_Input.IsMouseLeftDown;
				bool mbDown = m_Input.IsMouseMiddleDown;
				bool rbDown = m_Input.IsMouseRightDown || m_IsVirtualMouseRightDown;
				bool wheelInUse = Math::Abs(m_Input.MouseWheelDelta) > Math::EPSILON;

				m_Input.IsPanning = !isAltDown && mbDown && !rbDown;
				m_Input.IsRotating = !isAltDown && !mbDown && rbDown;
				m_Input.IsMoving = !isAltDown && mbDown && rbDown;
				m_Input.IsZooming = wheelInUse && !m_Input.IsShiftDown;
				m_Input.IsOrbiting = isAltDown && lbDown && !mbDown && !rbDown;

				// Control move speed with RMB+Wheel
				rmbWheel = useMovementSpeed && (m_Input.IsMouseRightDown || m_IsVirtualMouseRightDown) && wheelInUse;
				if (rmbWheel)
				{
					float step = m_Input.MouseWheelDelta;// * options.Viewport.MouseWheelSensitivity;
					AdjustCameraMoveSpeed(step > 0.0f ? 1 : -1);
				}
			}

			// Get input movement
			Float3 moveDelta = Float3::Zero;
			if (win->GetKey(KeyboardKeys::W/* options.Input.Forward.Key*/))
			{
				moveDelta += Float3::Forward;
			}
			if (win->GetKey(KeyboardKeys::S/* options.Input.Forward.Key*/))
			{
				moveDelta += Float3::Backward;
			}
			if (win->GetKey(KeyboardKeys::D/* options.Input.Forward.Key*/))
			{
				moveDelta += Float3::Right;
			}
			if (win->GetKey(KeyboardKeys::A/* options.Input.Forward.Key*/))
			{
				moveDelta += Float3::Left;
			}
			if (win->GetKey(KeyboardKeys::Q/* options.Input.Forward.Key*/))
			{
				moveDelta += Float3::Down;
			}
			if (win->GetKey(KeyboardKeys::E/* options.Input.Forward.Key*/))
			{
				moveDelta += Float3::Up;
			}
			moveDelta.Normalize(); // normalize direction
			moveDelta *= m_MovementSpeed;

			// Speed up or speed down
			if (m_Input.IsShiftDown)
				moveDelta *= 4.0f;
			if (m_Input.IsControlDown)
				moveDelta *= 0.3f;

			// Calculate smooth mouse delta not dependant on viewport size
			Float2 offset = m_ViewMousePos - m_LastViewMousePos;
			if (m_Input.IsZooming && !m_Input.IsMouseRightDown && !m_Input.IsMouseLeftDown && !m_Input.IsMouseMiddleDown && !UseOrthographicProjection && !rmbWheel && !m_IsVirtualMouseRightDown)
			{
				offset = Float2::Zero;
			}

			Float2 mouseDelta = Float2::Zero;
			if (m_UseMouseFiltering)
			{
				offset.x = offset.x > 0 ? Math::Floor(offset.x) : Math::Ceil(offset.x);
				offset.y = offset.y > 0 ? Math::Floor(offset.y) : Math::Ceil(offset.y);
				m_MouseDelta = offset;

				// Update delta filtering buffer
				_deltaFilteringBuffer[_deltaFilteringStep] = m_MouseDelta;
				_deltaFilteringStep++;

				// If the step is too far, zero
				if (_deltaFilteringStep == FpsCameraFilteringFrames)
					_deltaFilteringStep = 0;

				// Calculate filtered delta (avg)
				for (int i = 0; i < FpsCameraFilteringFrames; i++)
				{
					mouseDelta += _deltaFilteringBuffer[i];
				}

				mouseDelta /= FpsCameraFilteringFrames;
			}
			else
			{
				mouseDelta = m_MouseDelta = offset;
			}

			if (m_UseMouseAcceleration)
			{
				// Accelerate the delta
				Float2 currentDelta = mouseDelta;
				mouseDelta += m_MouseDeltaLast * _mouseAccelerationScale;
				m_MouseDeltaLast = currentDelta;
			}

			// Update
			moveDelta *= dt * 4.0;
			mouseDelta *= MouseSpeed * m_MouseSensitivity * deltaTime;
			bool centerMouse = false;
			UpdateView(dt, moveDelta, mouseDelta, centerMouse);

			// Change Ortho size on mouse scroll
			if (UseOrthographicProjection && !rmbWheel)
			{
				float scroll = m_Input.MouseWheelDelta;
				if (scroll > Math::EPSILON || scroll < -Math::EPSILON)
				{
					OrthographicScale -= scroll /** options.Viewport.MouseWheelSensitivity*/ * 0.2f * OrthographicScale;
				}
			}
		}
		else
		{
			if (m_Input.IsMouseLeftDown || m_Input.IsMouseRightDown || m_IsVirtualMouseRightDown)
			{
				// Calculate smooth mouse delta not dependant on viewport size
				Float2 offset = m_ViewMousePos - m_LastViewMousePos;
				offset.x = offset.x > 0 ? Math::Floor(offset.x) : Math::Ceil(offset.x);
				offset.y = offset.y > 0 ? Math::Floor(offset.y) : Math::Ceil(offset.y);
				m_MouseDelta = offset;
				m_LastViewMousePos = m_ViewMousePos;
			}
			else
			{
				m_MouseDelta = Float2::Zero;
			}
			m_MouseDeltaLast = Float2::Zero;

			/*if (ContainsFocus())
			{
				_input.IsPanning = false;
				_input.IsRotating = false;
				_input.IsMoving = true;
				_input.IsZooming = false;
				_input.IsOrbiting = false;

				// Get input movement
				Float3 moveDelta = Float3::Zero;
				Float2 mouseDelta = Float2::Zero;
				if (::SE::Input::Gamepads.Count() > 0)
				{
					// Gamepads handling
					moveDelta += Float3(::SE::Input::GetGamepadAxis(GamepadAxis::LeftStickX), 0, ::SE::Input::GetGamepadAxis(GamepadAxis::LeftStickY));
					mouseDelta += Float2(::SE::Input::GetGamepadAxis(GamepadAxis::RightStickX), -::SE::Input::GetGamepadAxis(GamepadAxis::RightStickY));
					_input.IsRotating |= !mouseDelta.IsZero;
				}
				if (win->GetKey(KeyboardKeys::ArrowRight))
				{
					moveDelta += Float3::Right;
				}
				if (win->GetKey(KeyboardKeys::ArrowLeft))
				{
					moveDelta += Float3::Left;
				}
				if (win->GetKey(KeyboardKeys::ArrowUp))
				{
					moveDelta += Float3::Up;
				}
				if (win->GetKey(KeyboardKeys::ArrowDown))
				{
					moveDelta += Float3::Down;
				}
				moveDelta.Normalize();
				moveDelta *= _movementSpeed;
				if (FlaxEngine.Input.GamepadsCount > 0)
					moveDelta *= Math::Remap(GetGamepadAxis(GamepadAxis.RightTrigger), 0, 1, 1, 4.0f);

				// Update
				moveDelta *= dt * (60.0f * 4.0f);
				UpdateView(dt, moveDelta, mouseDelta, out _);
			}*/
		}

		m_LastViewMousePos = m_ViewMousePos;
		m_Input.MouseWheelDelta = 0;
	}

	bool EditorViewport::OnMouseDown(Float2 location, MouseButton button)
	{
		Focus();

		RenderOutputViewport::OnMouseDown(location, button);
		return true;
	}

	bool EditorViewport::OnMouseWheel(Float2 location, float delta)
	{
		m_Input.MouseWheelDelta += delta;

		return RenderOutputViewport::OnMouseWheel(location, delta);
	}

	void EditorViewport::OnChildResized(Control* control)
	{
		RenderOutputViewport::OnChildResized(control);

		PerformLayout();
	}

	bool EditorViewport::OnKeyDown(KeyboardKeys key)
	{
		// Base
		if (RenderOutputViewport::OnKeyDown(key))
			return true;

		// Custom input events
		return false;//InputActions.Process(Editor.Instance, this, key);
	}

	void EditorViewport::Draw()
	{
		RenderOutputViewport::Draw();

		// Add overlay during debugger breakpoint hang
		/*if (Editor.Instance.Simulation.IsDuringBreakpointHang)
		{
			Rectangle bounds = Rectangle(Float2::Zero, Size);
			Render2D::FillRectangle(bounds, Color(0.0f, 0.0f, 0.0f, 0.2f));
			Render2D::RenderText(Style::Current->FontLarge, SE_TEXT("Debugger breakpoint hit..."), bounds, Colors::White, TextAlignment::Center, TextAlignment::Center);
		}*/
	}

	void EditorViewport::OnLostFocus()
	{
		RenderOutputViewport::OnLostFocus();

		if (m_IsControllingMouse)
		{
			OnControlMouseEnd(RootWindow()->Window());
			m_IsControllingMouse = false;
			m_IsVirtualMouseRightDown = false;
		}
	}

	void EditorViewport::OnDestroy()
	{
		// Editor.Instance.Options.OptionsChanged -= OnEditorOptionsChanged;

		RenderOutputViewport::OnDestroy();
	}

	void EditorViewport::CopyViewData(RenderView& view)
	{
		Float3 position = ViewPosition;
		// LargeWorlds.UpdateOrigin(ref view.Origin, position);
		view.Position = position - view.Origin;
		view.Direction = ViewDirection;
		view.Near = NearPlane;
		view.Far = FarPlane;

		CreateProjectionMatrix(view.Projection);
		CreateViewMatrix(view.Position, view.View);

		view.UpdateCachedData();
	}

	EditorViewport::Input& EditorViewport::GetInput()
	{
		return m_Input;
	}

	void EditorViewport::PerformLayoutBeforeChildren()
	{
		RenderOutputViewport::PerformLayoutBeforeChildren();

		// ViewportWidgetsContainer.ArrangeWidgets(this);
	}

	void EditorViewport::OrientViewport(Quaternion& orientation)
	{
		/*if (ViewportCamera is FPSCamera fpsCamera)
		{
			var pos = ViewPosition + Float3::Backward * orientation * 2000.0f;
			fpsCamera.MoveViewport(pos, orientation);
		}
		else*/
		{
			ViewportCamera->SetArcBallView(orientation, ViewPosition, 2000.0f);
		}
	}

	void EditorViewport::AdjustCameraMoveSpeed(int step)
	{
		_speedStep = Math::Clamp(_speedStep + step, 0, _maxSpeedSteps);

		// calculate new linear/eased progress
		float progress = _useCameraEasing
					   ? Math::Pow((float)_speedStep / _maxSpeedSteps, _cameraEasingDegree)
					   : (float)_speedStep / _maxSpeedSteps;

		float speed = Math::Lerp(m_MinMovementSpeed, m_MaxMovementSpeed, progress);
		MovementSpeed = speed;//(float)Math::Round(speed, 3.0f);
		// _editor.ProjectCache.SetCustomData("CameraMovementSpeedValue", _movementSpeed.ToString());
	}

	void EditorViewport::CreateProjectionMatrix(Matrix& result)
	{
		if (UseOrthographicProjection)
		{
			Matrix::Ortho(Width * UseOrthographicProjection, Height * OrthographicScale, NearPlane, FarPlane, result);
		}
		else
		{
			float aspect = Width / Height;
			Matrix::PerspectiveFov(FieldOfView * Math::DegreesToRadians, aspect, NearPlane, FarPlane, result);
		}
	}

	void EditorViewport::CreateViewMatrix(Float3 position, Matrix& result)
	{
		Float3 direction = ViewDirection;
		Float3 target = position + direction;
		Float3 right = Float3::Normalize(Float3::Cross(Float3::Up, direction));
		Float3 up = Float3::Normalize(Float3::Cross(direction, right));
		Matrix::LookAt(position, target, up, result);
	}

	void EditorViewport::OnControlMouseBegin(Window* win)
	{
		// Hide cursor and start tracking mouse movement
		win->StartTrackingMouse(false);
		win->SetCursor(CursorType::Hidden);

		// Center mouse position if it's too close to the edge
		Float2 size = Size;
		Float2 center = Float2::Round(size * 0.5f);
		if (Math::Abs(m_ViewMousePos.x - center.x) > center.x * 0.8f || Math::Abs(m_ViewMousePos.y - center.y) > center.y * 0.8f)
		{
			m_StartPos = center;
			m_ViewMousePos = center;
			m_LastViewMousePos = center;
			win->SetMousePosition(PointToWindow(m_ViewMousePos));
		}
	}

	void EditorViewport::OnControlMouseEnd(Window* win)
	{
		// Restore cursor and stop tracking mouse movement
		win->SetCursor(CursorType::Default);
		win->SetMousePosition(PointToWindow(m_StartPos));
		win->EndTrackingMouse();
	}

	void EditorViewport::UpdateView(float dt, const Float3& moveDelta, const Float2& mouseDelta, bool& centerMouse)
	{
		centerMouse = true;
		if (m_Camera != nullptr)
		{
			m_Camera->UpdateView(dt, moveDelta, mouseDelta, centerMouse);
		}
	}

	void EditorViewport::OnCameraMovementProgressChanged()
	{
		// prevent NaN
		if (Math::Abs(m_MinMovementSpeed - m_MaxMovementSpeed) < Math::EPSILON)
		{
			_speedStep = 0;
			return;
		}

		if (Math::Abs(m_MovementSpeed - m_MaxMovementSpeed) < Math::EPSILON)
		{
			_speedStep = _maxSpeedSteps;
			return;
		}
		else if (Math::Abs(m_MovementSpeed - m_MinMovementSpeed) < Math::EPSILON)
		{
			_speedStep = 0;
			return;
		}

		// calculate current linear/eased progress
		float progress = Math::Remap(m_MovementSpeed, m_MinMovementSpeed, m_MaxMovementSpeed, 0.0f, 1.0f);

		if (_useCameraEasing)
			progress = Math::Pow(progress, 1.0f / _cameraEasingDegree);

		_speedStep = Math::RoundToInt(progress * _maxSpeedSteps);
	}

	void EditorViewport::OnRenderBegin(RenderTask* task, GPUContext* context)
	{
		SceneRenderTask* sceneTask = (SceneRenderTask*)task;

		RenderView view = sceneTask->View;
		CopyViewData(view);
		sceneTask->View = view;
	}

	void EditorViewport::SetupViewportOptions()
	{
		m_MinMovementSpeed = 0.1f;
		m_MaxMovementSpeed = 100.0f;
		m_MouseSensitivity = 1;
		m_MovementSpeed = 1;

		FieldOfView = 60.0f;
		MovementSpeed = 1;
		/*var options = Editor.Instance.Options.Options;
		_minMovementSpeed = options.Viewport.MinMovementSpeed;
		MovementSpeed = options.Viewport.MovementSpeed;
		_maxMovementSpeed = options.Viewport.MaxMovementSpeed;
		_useCameraEasing = options.Viewport.UseCameraEasing;
		PanningSpeed = options.Viewport.PanningSpeed;
		InvertPanning = options.Viewport.InvertPanning;
		RelativePanning = options.Viewport.UseRelativePanning;

		UseOrthographicProjection = options.Viewport.UseOrthographicProjection;
		OrthographicScale = options.Viewport.OrthographicScale;
		FieldOfView = options.Viewport.FieldOfView;
		NearPlane = options.Viewport.NearPlane;
		FarPlane = options.Viewport.FarPlane;*/

		// OnEditorOptionsChanged(options);
	}

	void EditorViewport::__SetMovementSpeed(float value)
	{
		m_MovementSpeed = value;

		/*if (_cameraButton != nullptr)
			_cameraButton.Text = String::Format(GetMovementSpeedTextFormat(), _movementSpeed);*/
	}
}
