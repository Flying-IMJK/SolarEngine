
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Editor rendering viewport.
    /// </summary>
    public class EditorViewport : RenderOutputViewport
    {
        /// <summary>
        /// Gathered input data.
        /// </summary>
        public struct Input
        {
            /// <summary>The is panning state.</summary>
            public bool IsPanning;

            /// <summary>The is rotating state.</summary>
            public bool IsRotating;

            /// <summary>The is moving state.</summary>
            public bool IsMoving;

            /// <summary>The is zooming state.</summary>
            public bool IsZooming;

            /// <summary>The is orbiting state.</summary>
            public bool IsOrbiting;

            /// <summary>The is control down flag.</summary>
            public bool IsControlDown;

            /// <summary>The is shift down flag.</summary>
            public bool IsShiftDown;

            /// <summary>The is alt down flag.</summary>
            public bool IsAltDown;

            /// <summary>
            /// The alt down flag cached from the previous input. Used to make
            /// <see cref="GetIsControllingMouse"/> consistent when Alt is released
            /// while orbiting with Alt and the left mouse button.
            /// </summary>
            public bool WasAltDownBefore;

            /// <summary>The is mouse right down flag.</summary>
            public bool IsMouseRightDown;

            /// <summary>The is mouse middle down flag.</summary>
            public bool IsMouseMiddleDown;

            /// <summary>The is mouse left down flag.</summary>
            public bool IsMouseLeftDown;

            /// <summary>The mouse wheel delta.</summary>
            public float MouseWheelDelta;

            /// <summary>Gets a value indicating whether use is controlling mouse.</summary>
            public bool GetIsControllingMouse()
            {
                return IsMouseMiddleDown || IsMouseRightDown ||
                       ((IsAltDown || WasAltDownBefore) && IsMouseLeftDown) ||
                       Mathf.Abs(MouseWheelDelta) > 0.1f;
            }

            /// <summary>Gathers input from the specified window.</summary>
            /// <param name="window">The window.</param>
            /// <param name="useMouse">True to use mouse input; otherwise skip mouse.</param>
            /// <param name="prevInput">Previous input state.</param>
            public void Gather(Window window, bool useMouse, Input prevInput)
            {
                IsControlDown = window.GetKey(KeyboardKeys.Control);
                IsShiftDown = window.GetKey(KeyboardKeys.Shift);
                IsAltDown = window.GetKey(KeyboardKeys.Alt);
                WasAltDownBefore = prevInput.WasAltDownBefore || prevInput.IsAltDown;

                IsMouseRightDown = useMouse && window.GetMouseButton(MouseButton.Right);
                IsMouseMiddleDown = useMouse && window.GetMouseButton(MouseButton.Middle);
                IsMouseLeftDown = useMouse && window.GetMouseButton(MouseButton.Left);
            }

            /// <summary>Clears the data.</summary>
            public void Clear()
            {
                IsControlDown = false;
                IsShiftDown = false;
                IsAltDown = false;
                WasAltDownBefore = false;
                IsMouseRightDown = false;
                IsMouseMiddleDown = false;
                IsMouseLeftDown = false;
            }
        }

        /// <summary>
        /// The FPS camera filtering frames count, used to calculate the average delta.
        /// </summary>
        public const int FpsCameraFilteringFrames = 3;

        private float m_MouseSensitivity;
        private float m_MovementSpeed;
        private float m_MinMovementSpeed;
        private float m_MaxMovementSpeed;
        private float _mouseAccelerationScale;
        private bool m_UseMouseFiltering;
        private bool m_UseMouseAcceleration;

        private bool _disableInputUpdate;
        private bool m_IsControllingMouse;
        private bool _isViewportControllingMouse;
        private bool _waitForMouseRelease;
        private bool m_WasVirtualMouseRightDown;
        private bool m_IsVirtualMouseRightDown;
        private int _deltaFilteringStep;
        private Float2 m_StartPos;
        private Float2 m_MouseDeltaLast;
        private Float2 m_LastViewMousePos;
        private readonly Float2[] _deltaFilteringBuffer = new Float2[FpsCameraFilteringFrames];

        private ViewportCamera m_Camera;
        private float _yaw;
        private float _pitch;
        private bool _useCameraEasing;
        private float _cameraEasingDegree;
        private int _speedStep;
        private int _maxSpeedSteps;

        /// <summary>The camera settings widget.</summary>
        protected ViewportWidgetsContainer _cameraWidget;

        /// <summary>The camera settings widget button.</summary>
        protected ViewportWidgetButton _cameraButton;

        /// <summary>The orthographic mode widget button.</summary>
        protected ViewportWidgetButton _orthographicModeButton;

        /// <summary>The previous input.</summary>
        protected Input m_PrevInput;

        /// <summary>The input data from the current frame.</summary>
        protected Input m_Input;

        /// <summary>The view mouse position.</summary>
        protected Float2 m_ViewMousePos;

        /// <summary>The mouse position delta.</summary>
        protected Float2 m_MouseDelta;

        /// <summary>Speed of the mouse.</summary>
        public float MouseSpeed = 5;

        /// <summary>Speed of the mouse wheel zooming.</summary>
        public float MouseWheelZoomSpeedFactor = 1;

        /// <summary>Gets or sets the camera movement speed.</summary>
        public float MovementSpeed
        {
            get => m_MovementSpeed;
            set
            {
                m_MovementSpeed = value;
                /*if (_cameraButton != null)
                    _cameraButton.Text = string.Format(GetMovementSpeedTextFormat(), m_MovementSpeed);*/
            }
        }

        /// <summary>Gets or sets the minimum camera movement speed.</summary>
        public float MinMovementSpeed
        {
            get => m_MinMovementSpeed;
            set => m_MinMovementSpeed = value;
        }

        /// <summary>Gets or sets the maximum camera movement speed.</summary>
        public float MaxMovementSpeed
        {
            get => m_MaxMovementSpeed;
            set => m_MaxMovementSpeed = value;
        }

        /// <summary>Gets or sets the camera easing mode.</summary>
        public bool UseCameraEasing
        {
            get => _useCameraEasing;
            set => _useCameraEasing = value;
        }

        /// <summary>Gets the mouse movement position delta.</summary>
        public Float2 MousePositionDelta => m_MouseDelta;

        /// <summary>Camera's pitch angle clamp range in degrees.</summary>
        public Float2 CamPitchAngles = new Float2(-88, 88);

        /// <summary>Gets the view transform.</summary>
        public Transform ViewTransform
        {
            get => new Transform(ViewPosition, ViewOrientation);
            set
            {
                ViewPosition = value.Translation;
                ViewOrientation = value.Orientation;
            }
        }

        /// <summary>Gets or sets the view position.</summary>
        public Float3 ViewPosition;

        /// <summary>Gets or sets the view orientation.</summary>
        public Quaternion ViewOrientation
        {
            get => Quaternion.RotationYawPitchRoll(_yaw * Mathf.DegreesToRadians, _pitch * Mathf.DegreesToRadians, 0);
            set => EulerAngles = value.EulerAngles;
        }

        /// <summary>Gets or sets the view direction vector.</summary>
        public Float3 ViewDirection
        {
            get => Float3.Forward * ViewOrientation;
            set
            {
                Float3 right = Float3.Cross(value, Float3.Up);
                Float3 up = Float3.Cross(right, value);
                ViewOrientation = Quaternion.LookRotation(value, up);
            }
        }

        /*public Ray ViewRay
        {
            get => new Ray(ViewPosition, ViewDirection);
            set
            {
                ViewPosition = value.Position;
                ViewDirection = value.Direction;
            }
        }

        public BoundingFrustum ViewFrustum
        {
            get
            {
                Float3 viewOrigin = Task.View.Origin;
                Float3 position = ViewPosition - viewOrigin;
                CreateViewMatrix(position, out var view);
                CreateProjectionMatrix(out var projection);
                Matrix.Multiply(ref view, ref projection, out var viewProjection);
                return new BoundingFrustum(ref viewProjection);
            }
        }*/

        /// <summary>Gets or sets the yaw angle in degrees.</summary>
        public float Yaw
        {
            get => _yaw;
            set => _yaw = value;
        }

        /// <summary>Gets or sets the pitch angle in degrees.</summary>
        public float Pitch
        {
            get => _pitch;
            set => _pitch = Mathf.Clamp(value, CamPitchAngles.X, CamPitchAngles.Y);
        }

        /// <summary>
        /// Gets or sets the absolute mouse position, normalized rather than in pixels.
        /// Yaw is X and Pitch is Y.
        /// </summary>
        public Float2 YawPitch
        {
            get => new Float2(_yaw, _pitch);
            set
            {
                Yaw = value.X;
                Pitch = value.Y;
            }
        }

        /// <summary>Gets or sets the euler angles.</summary>
        public Float3 EulerAngles
        {
            get => new Float3(_pitch, _yaw, 0);
            set
            {
                Pitch = value.X;
                Yaw = value.Y;
            }
        }

        /// <summary>Gets a value indicating whether this viewport has loaded dependant assets.</summary>
        public virtual bool GetHasLoadedAssets()
        {
            return true;
        }

        /// <summary>The View widget button context menu.</summary>
        public ContextMenu ViewWidgetButtonMenu;

        /// <summary>The View widget Show category context menu.</summary>
        public ContextMenu ViewWidgetShowMenu;

        /// <summary>Gets or sets the viewport camera controller.</summary>
        public ViewportCamera ViewportCamera
        {
            get => m_Camera;
            set
            {
                if (m_Camera != null)
                {
                    m_Camera.SetViewport(null);
                }
                m_Camera = value;
                if (m_Camera != null)
                {
                    m_Camera.SetViewport(this);
                }
            }
        }
        /// <summary>Gets or sets the camera near clipping plane.</summary>
        public float NearPlane;

        /// <summary>Gets or sets the camera far clipping plane.</summary>
        public float FarPlane;

        /// <summary>Gets or sets the camera field of view in degrees.</summary>
        public float FieldOfView;

        /// <summary>
        /// Gets or sets the camera orthographic size scale when orthographic mode is used.
        /// </summary>
        public float OrthographicScale;

        /// <summary>Gets or sets the camera orthographic mode.</summary>
        public bool UseOrthographicProjection;

        /// <summary>Gets or sets if the panning speed should be relative to the camera target.</summary>
        public bool RelativePanning;

        /// <summary>Gets or sets if the panning direction is inverted.</summary>
        public bool InvertPanning;

        /// <summary>Gets or sets the camera panning speed.</summary>
        public float PanningSpeed;

        /// <summary>Initializes a new instance of the <see cref="EditorViewport"/> class.</summary>
        /// <param name="task">The task.</param>
        /// <param name="camera">The camera controller.</param>
        /// <param name="useWidgets">Enable or disable viewport widgets.</param>
        public EditorViewport(SceneRenderTask task, ViewportCamera camera, bool useWidgets) : base(task)
        {
            _mouseAccelerationScale = 0.1f;
            m_UseMouseFiltering = false;
            m_UseMouseAcceleration = false;
            m_Camera = camera;
            if (m_Camera != null)
            {
                m_Camera.SetViewport(this);
            }

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;

            // Setup options
            // Editor.Instance.Options.OptionsChanged += OnEditorOptionsChanged;
            SetupViewportOptions();

            // Initialize camera values from cache
            /*if (_editor.ProjectCache.TryGetCustomData("CameraMovementSpeedValue", out var cachedState))
                MovementSpeed = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraMinMovementSpeedValue", out cachedState))
                m_MinMovementSpeed = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraMaxMovementSpeedValue", out cachedState))
                m_MaxMovementSpeed = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("UseCameraEasingState", out cachedState))
                _useCameraEasing = bool.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraPanningSpeedValue", out cachedState))
                PanningSpeed = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraInvertPanningState", out cachedState))
                InvertPanning = bool.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraRelativePanningState", out cachedState))
                RelativePanning = bool.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraOrthographicState", out cachedState))
                UseOrthographicProjection = bool.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraOrthographicSizeValue", out cachedState))
                OrthographicScale = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraFieldOfViewValue", out cachedState))
                FieldOfView = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraNearPlaneValue", out cachedState))
                NearPlane = float.Parse(cachedState);
            if (_editor.ProjectCache.TryGetCustomData("CameraFarPlaneValue", out cachedState))
                FarPlane = float.Parse(cachedState);*/
            OnCameraMovementProgressChanged();

            if (useWidgets)
            {
                #region Camera settings widget
                const string largestText = "Relative Panning";
                Float2 textSize = Style.Current.FontMedium.MeasureText(largestText, TextLayoutOptions.Default);
                float xLocationForExtras = textSize.X + 5;
                float cameraSpeedTextWidth = Style.Current.FontMedium.MeasureText("0.00", TextLayoutOptions.Default).X;

                _cameraWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                ContextMenu cameraCM = new ContextMenu();
                _cameraButton = new ViewportWidgetButton(
                    string.Format(GetMovementSpeedTextFormat(), m_MovementSpeed),
                    EditorIcons.Camera64, cameraCM, false, cameraSpeedTextWidth);
                _cameraButton.Tag = this;
                _cameraButton.TooltipText = "Camera Settings";
                _cameraButton.Parent = _cameraWidget;
                _cameraWidget.Parent = this;

                _orthographicModeButton = new ViewportWidgetButton(
                    string.Empty, EditorIcons.CamSpeed32, null, true);
                _orthographicModeButton.Checked = !UseOrthographicProjection;
                _orthographicModeButton.TooltipText = "Toggle Orthographic/Perspective Mode";
                _orthographicModeButton.Parent = _cameraWidget;
                _orthographicModeButton.Toggled += button => OnOrthographicModeToggled(button);

                ContextMenuButton camSpeedButton = cameraCM.AddButton("Camera Speed");
                camSpeedButton.CloseMenuOnClick = false;
                FloatValueBox camSpeedValue = new FloatValueBox(
                    m_MovementSpeed, xLocationForExtras, 2, 70.0f,
                    m_MinMovementSpeed, m_MaxMovementSpeed, 0.5f);
                camSpeedValue.Parent = camSpeedButton;
                camSpeedValue.ValueChanged += () => OnMovementSpeedChanged(camSpeedValue);
                cameraCM.VisibleChanged += control => camSpeedValue.SetValue(m_MovementSpeed);

                ContextMenuButton minCamSpeedButton = cameraCM.AddButton("Min Cam Speed");
                minCamSpeedButton.CloseMenuOnClick = false;
                FloatValueBox minCamSpeedValue = new FloatValueBox(
                    m_MinMovementSpeed, xLocationForExtras, 2, 70.0f,
                    0.05f, m_MaxMovementSpeed, 0.5f);
                minCamSpeedValue.Parent = minCamSpeedButton;
                ContextMenuButton maxCamSpeedButton = cameraCM.AddButton("Max Cam Speed");
                maxCamSpeedButton.CloseMenuOnClick = false;
                FloatValueBox maxCamSpeedValue = new FloatValueBox(
                    m_MaxMovementSpeed, xLocationForExtras, 2, 70.0f,
                    m_MinMovementSpeed, 1000.0f, 0.5f);
                maxCamSpeedValue.Parent = maxCamSpeedButton;
                minCamSpeedValue.ValueChanged += () =>
                {
                    OnMinMovementSpeedChanged(minCamSpeedValue);
                    maxCamSpeedValue.SetMinValue(minCamSpeedValue.GetValue());
                    if (Mathf.Abs(camSpeedValue.GetMinValue() - minCamSpeedValue.GetValue()) > Mathf.Epsilon)
                    {
                        camSpeedValue.SetMinValue(minCamSpeedValue.GetValue());
                    }
                };
                cameraCM.VisibleChanged += control => minCamSpeedValue.SetValue(m_MinMovementSpeed);
                maxCamSpeedValue.ValueChanged += () =>
                {
                    OnMaxMovementSpeedChanged(maxCamSpeedValue);
                    minCamSpeedValue.SetMaxValue(maxCamSpeedValue.GetValue());
                    if (Mathf.Abs(camSpeedValue.GetMaxValue() - maxCamSpeedValue.GetValue()) > Mathf.Epsilon)
                    {
                        camSpeedValue.SetMaxValue(maxCamSpeedValue.GetValue());
                    }
                };
                cameraCM.VisibleChanged += control => maxCamSpeedValue.SetValue(m_MaxMovementSpeed);

                ContextMenuButton useCameraEasing = cameraCM.AddButton("Camera Easing");
                useCameraEasing.CloseMenuOnClick = false;
                CheckBox useCameraEasingValue = new CheckBox(xLocationForExtras, 2, _useCameraEasing);
                useCameraEasingValue.Parent = useCameraEasing;
                useCameraEasingValue.StateChanged += checkBox => OnCameraEasingToggled(checkBox);
                cameraCM.VisibleChanged += control => useCameraEasingValue.Checked = _useCameraEasing;

                ContextMenuButton panningSpeed = cameraCM.AddButton("Panning Speed");
                panningSpeed.CloseMenuOnClick = false;
                FloatValueBox panningSpeedValue = new FloatValueBox(
                    PanningSpeed, xLocationForExtras, 2, 70.0f, 0.01f, 128.0f, 0.1f);
                panningSpeedValue.Parent = panningSpeed;
                panningSpeedValue.ValueChanged += () => OnPanningSpeedChanged(panningSpeedValue);
                cameraCM.VisibleChanged += control =>
                {
                    panningSpeed.Visible = !RelativePanning;
                    panningSpeedValue.SetValue(PanningSpeed);
                };

                ContextMenuButton relativePanning = cameraCM.AddButton("Relative Panning");
                relativePanning.CloseMenuOnClick = false;
                CheckBox relativePanningValue = new CheckBox(xLocationForExtras, 2, RelativePanning);
                relativePanningValue.Parent = relativePanning;
                relativePanningValue.StateChanged += checkBox =>
                {
                    if (checkBox.Checked != RelativePanning)
                    {
                        OnRelativePanningToggled(checkBox);
                        cameraCM.Hide();
                    }
                };
                cameraCM.VisibleChanged += control => relativePanningValue.Checked = RelativePanning;

                ContextMenuButton invertPanning = cameraCM.AddButton("Invert Panning");
                invertPanning.CloseMenuOnClick = false;
                CheckBox invertPanningValue = new CheckBox(xLocationForExtras, 2, InvertPanning);
                invertPanningValue.Parent = invertPanning;
                invertPanningValue.StateChanged += checkBox => OnInvertPanningToggled(checkBox);
                cameraCM.VisibleChanged += control => invertPanningValue.Checked = InvertPanning;

                cameraCM.AddSeparator();
                /* Camera Viewpoints
                ContextMenu cameraView = cameraCM.AddChildMenu("Viewpoints").ContextMenu;
                for (int i = 0; i < CameraViewpointValues.Length; i++)
                {
                    var co = CameraViewpointValues[i];
                    var button = cameraView.AddButton(co.Name);
                    button.Tag = co.Orientation;
                }
                cameraView.ButtonClicked += OnViewpointChanged;
                */

                ContextMenuButton ortho = cameraCM.AddButton("Orthographic");
                ortho.CloseMenuOnClick = false;
                CheckBox orthoValue = new CheckBox(xLocationForExtras, 2, UseOrthographicProjection);
                orthoValue.Parent = ortho;
                orthoValue.StateChanged += checkBox =>
                {
                    if (checkBox.Checked != UseOrthographicProjection)
                    {
                        OnOrthographicModeToggled(checkBox);
                        cameraCM.Hide();
                    }
                };
                cameraCM.VisibleChanged += control => orthoValue.Checked = UseOrthographicProjection;

                ContextMenuButton fov = cameraCM.AddButton("Field Of View");
                fov.CloseMenuOnClick = false;
                FloatValueBox fovValue = new FloatValueBox(
                    FieldOfView, xLocationForExtras, 2, 70.0f, 35.0f, 160.0f, 0.1f);
                fovValue.Parent = fov;
                fovValue.ValueChanged += () => OnFieldOfViewChanged(fovValue);
                cameraCM.VisibleChanged += control =>
                {
                    fov.Visible = !UseOrthographicProjection;
                    fovValue.SetValue(FieldOfView);
                };

                ContextMenuButton orthoSize = cameraCM.AddButton("Ortho Scale");
                orthoSize.CloseMenuOnClick = false;
                FloatValueBox orthoSizeValue = new FloatValueBox(
                    OrthographicScale, xLocationForExtras, 2, 70.0f,
                    0.001f, 100000.0f, 0.01f);
                orthoSizeValue.Parent = orthoSize;
                orthoSizeValue.ValueChanged += () => OnOrthographicSizeChanged(orthoSizeValue);
                cameraCM.VisibleChanged += control =>
                {
                    orthoSize.Visible = UseOrthographicProjection;
                    orthoSizeValue.SetValue(OrthographicScale);
                };

                ContextMenuButton nearPlane = cameraCM.AddButton("Near Plane");
                nearPlane.CloseMenuOnClick = false;
                FloatValueBox nearPlaneValue = new FloatValueBox(
                    NearPlane, xLocationForExtras, 2, 70.0f, 0.001f, 1000.0f);
                nearPlaneValue.Parent = nearPlane;
                nearPlaneValue.ValueChanged += () => OnNearPlaneChanged(nearPlaneValue);
                cameraCM.VisibleChanged += control =>
                {
                    nearPlane.Visible = UseOrthographicProjection;
                    nearPlaneValue.SetValue(NearPlane);
                };

                ContextMenuButton farPlane = cameraCM.AddButton("Far Plane");
                farPlane.CloseMenuOnClick = false;
                FloatValueBox farPlaneValue = new FloatValueBox(
                    FarPlane, xLocationForExtras, 2, 70.0f, 10.0f);
                farPlaneValue.Parent = farPlane;
                farPlaneValue.ValueChanged += () => OnFarPlaneChanged(farPlaneValue);
                cameraCM.VisibleChanged += control => farPlaneValue.SetValue(NearPlane);

                cameraCM.AddSeparator();
                ContextMenuButton reset = cameraCM.AddButton("Reset to default");
                reset.ButtonClicked += button =>
                {
                    SetupViewportOptions();
                    minCamSpeedValue.SetValue(m_MinMovementSpeed);
                    camSpeedValue.SetValue(m_MovementSpeed);
                    maxCamSpeedValue.SetValue(m_MaxMovementSpeed);
                };
                #endregion

                #region View mode widget
                // The View mode widget is disabled in the native constructor too.
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
					viewLayers.ButtonClicked += button =>
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
					viewFlags.ButtonClicked += button =>
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
					brightnessValue.ValueChanged += () => Brightness = brightnessValue.Value;
					ViewWidgetButtonMenu.VisibleChanged += control => brightnessValue.Value = Brightness;
				}

				// Resolution
				{
					var resolution = ViewWidgetButtonMenu.AddButton("Resolution");
					resolution.CloseMenuOnClick = false;
					var resolutionValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.1f, 4.0f, 0.001f)
					{
						Parent = resolution
					};
					resolutionValue.ValueChanged += () => ResolutionScale = resolutionValue.Value;
					ViewWidgetButtonMenu.VisibleChanged += control => resolutionValue.Value = ResolutionScale;
                }*/
                #endregion
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
            task.Begin += OnRenderBegin;
        }

        /// <summary>Format of the text for the camera move speed.</summary>
        protected string GetMovementSpeedTextFormat()
        {
            if (Mathf.Abs(m_MovementSpeed - m_MaxMovementSpeed) < Mathf.Epsilon ||
                Mathf.Abs(m_MovementSpeed - m_MinMovementSpeed) < Mathf.Epsilon)
            {
                return "{0}";
            }
            if (m_MovementSpeed < 10.0f)
            {
                return "{0:2f}";
            }
            if (m_MovementSpeed < 100.0f)
            {
                return "{0:1f}";
            }
            return "{0}";
        }

        public void OnMovementSpeedChanged(FloatValueBox control)
        {
            float value = Mathf.Clamp(control.GetValue(), m_MinMovementSpeed, m_MaxMovementSpeed);
            MovementSpeed = value;
            OnCameraMovementProgressChanged();
            // _editor.ProjectCache.SetCustomData("CameraMovementSpeedValue", m_MovementSpeed.ToString());
        }

        public void OnMinMovementSpeedChanged(FloatValueBox control)
        {
            float value = Mathf.Clamp(control.GetValue(), 0.05f, m_MaxMovementSpeed);
            m_MinMovementSpeed = value;
            if (m_MovementSpeed < value)
            {
                MovementSpeed = value;
            }
            OnCameraMovementProgressChanged();
            // _editor.ProjectCache.SetCustomData("CameraMinMovementSpeedValue", m_MinMovementSpeed.ToString());
        }

        public void OnMaxMovementSpeedChanged(FloatValueBox control)
        {
            float value = Mathf.Clamp(control.GetValue(), m_MinMovementSpeed, 1000.0f);
            m_MaxMovementSpeed = value;
            if (m_MovementSpeed > value)
            {
                MovementSpeed = value;
            }
            OnCameraMovementProgressChanged();
            // _editor.ProjectCache.SetCustomData("CameraMaxMovementSpeedValue", m_MaxMovementSpeed.ToString());
        }

        public void OnCameraEasingToggled(Control control)
        {
            _useCameraEasing = !_useCameraEasing;
            OnCameraMovementProgressChanged();
            // _editor.ProjectCache.SetCustomData("UseCameraEasingState", _useCameraEasing.ToString());
        }

        public void OnPanningSpeedChanged(FloatValueBox control)
        {
            PanningSpeed = control.GetValue();
            // _editor.ProjectCache.SetCustomData("CameraPanningSpeedValue", PanningSpeed.ToString());
        }

        public void OnRelativePanningToggled(Control control)
        {
            RelativePanning = !RelativePanning;
            // _editor.ProjectCache.SetCustomData("CameraRelativePanningState", RelativePanning.ToString());
        }

        public void OnInvertPanningToggled(Control control)
        {
            InvertPanning = !InvertPanning;
            // _editor.ProjectCache.SetCustomData("CameraInvertPanningState", InvertPanning.ToString());
        }

        public void OnViewpointChanged(ContextMenuButton button)
        {
            Quaternion orient = Quaternion.Euler((Float3)button.Tag);
            OrientViewport(ref orient);
        }

        public void OnFieldOfViewChanged(FloatValueBox control)
        {
            FieldOfView = control.GetValue();
            // _editor.ProjectCache.SetCustomData("CameraFieldOfViewValue", FieldOfView.ToString());
        }

        public void OnOrthographicModeToggled(Control control)
        {
            UseOrthographicProjection = !UseOrthographicProjection;
            // _orthographicModeButton.Checked = !UseOrthographicProjection;
            if (UseOrthographicProjection)
            {
                Quaternion orient = ViewOrientation;
                OrientViewport(ref orient);
            }
            // _editor.ProjectCache.SetCustomData("CameraOrthographicState", UseOrthographicProjection.ToString());
        }

        public void OnOrthographicSizeChanged(FloatValueBox control)
        {
            OrthographicScale = control.GetValue();
            // _editor.ProjectCache.SetCustomData("CameraOrthographicSizeValue", OrthographicScale.ToString());
        }

        public void OnNearPlaneChanged(FloatValueBox control)
        {
            NearPlane = control.GetValue();
            // _editor.ProjectCache.SetCustomData("CameraNearPlaneValue", NearPlane.ToString());
        }

        public void OnFarPlaneChanged(FloatValueBox control)
        {
            FarPlane = control.GetValue();
            // _editor.ProjectCache.SetCustomData("CameraFarPlaneValue", FarPlane.ToString());
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (_disableInputUpdate)
            {
                return;
            }

            bool useMovementSpeed = false;
            if (m_Camera != null)
            {
                m_Camera.Update(deltaTime);
                useMovementSpeed = m_Camera.GetUseMovementSpeed();
                /*if (_cameraButton != null)
                {
                    _cameraButton.Parent.Visible = useMovementSpeed;
                }*/
            }

            WindowRootControl win = (WindowRootControl)Root;
            // When the window is not focused, the position in window does not return sane values.
            Float2 pos = PointFromWindow(win.GetMousePosition());
            /*if (!float.IsInfinity(pos.LengthSquared))
            {*/
            m_ViewMousePos = pos;
            // }

            Window window = win.Window;
            bool canUseInput = window != null && window.IsFocused && window.IsForegroundWindow;
            if (!canUseInput)
            {
                ReleaseMouseControl();
            }
            else if (_waitForMouseRelease)
            {
                _waitForMouseRelease = window.GetMouseButton(MouseButton.Left) ||
                                       window.GetMouseButton(MouseButton.Middle) ||
                                       window.GetMouseButton(MouseButton.Right);
            }

            m_PrevInput = m_Input;
            bool useMouse = GetIsControllingMouse() || m_IsControllingMouse ||
                            (m_ViewMousePos.X >= 0.0f && m_ViewMousePos.X <= Width &&
                             m_ViewMousePos.Y >= 0.0f && m_ViewMousePos.Y <= Height);
            Control hit = GetChildAt(m_ViewMousePos, control => control.Visible /* && !(control is CanvasRootControl && control is not UIEditorRoot) */);
            if (canUseInput && !_waitForMouseRelease && ContainsFocus && (hit == null || m_PrevInput.GetIsControllingMouse()))
            {
                m_Input.Gather(window, useMouse, m_PrevInput);
            }
            else
            {
                m_Input.Clear();
            }

            bool lastControllingMouse = m_PrevInput.GetIsControllingMouse();
            m_IsControllingMouse = m_Input.GetIsControllingMouse();
            if ((m_PrevInput.IsMouseRightDown && !m_Input.IsMouseRightDown) ||
                win.GetKeyDown(KeyboardKeys.Escape))
            {
                m_IsVirtualMouseRightDown = false;
            }
            if (m_WasVirtualMouseRightDown)
            {
                lastControllingMouse = true;
            }
            if (m_IsVirtualMouseRightDown)
            {
                m_IsControllingMouse = m_IsVirtualMouseRightDown;
            }

            if (lastControllingMouse != m_IsControllingMouse)
            {
                if (m_IsControllingMouse)
                {
                    OnControlMouseBegin(window);
                }
                else
                {
                    OnControlMouseEnd(window);
                }
            }

            if (!m_PrevInput.IsMouseLeftDown && m_Input.IsMouseLeftDown)
            {
                OnLeftMouseButtonDown();
            }
            else if (m_PrevInput.IsMouseLeftDown && !m_Input.IsMouseLeftDown)
            {
                OnLeftMouseButtonUp();
            }

            if ((!m_PrevInput.IsMouseRightDown && m_Input.IsMouseRightDown) ||
                (!m_WasVirtualMouseRightDown && m_IsVirtualMouseRightDown))
            {
                OnRightMouseButtonDown();
            }
            else if ((m_PrevInput.IsMouseRightDown && !m_Input.IsMouseRightDown) ||
                     (m_WasVirtualMouseRightDown && !m_IsVirtualMouseRightDown))
            {
                OnRightMouseButtonUp();
            }

            if (!m_PrevInput.IsMouseMiddleDown && m_Input.IsMouseMiddleDown)
            {
                OnMiddleMouseButtonDown();
            }
            else if (m_PrevInput.IsMouseMiddleDown && !m_Input.IsMouseMiddleDown)
            {
                OnMiddleMouseButtonUp();
            }
            m_WasVirtualMouseRightDown = m_IsVirtualMouseRightDown;

            // Capture through the GUI root so movement cannot reach adjacent controls.
            bool shouldCapture = canUseInput && !_waitForMouseRelease &&
                                 (m_IsControllingMouse || GetIsControllingMouse());
            if (shouldCapture != _isViewportControllingMouse)
            {
                _isViewportControllingMouse = shouldCapture;
                if (shouldCapture)
                    StartMouseCapture();
                else
                    EndMouseCapture();
            }

            float dt = Mathf.Min(Time.GetUnscaledDeltaTime(), 1.0f);
            Float2 size = Size;
            if (m_IsControllingMouse)
            {
                bool rmbWheel;
                bool isAltDown = m_Input.IsAltDown;
                bool lbDown = m_Input.IsMouseLeftDown;
                bool mbDown = m_Input.IsMouseMiddleDown;
                bool rbDown = m_Input.IsMouseRightDown || m_IsVirtualMouseRightDown;
                bool wheelInUse = Mathf.Abs(m_Input.MouseWheelDelta) > Mathf.Epsilon;

                m_Input.IsPanning = !isAltDown && mbDown && !rbDown;
                m_Input.IsRotating = !isAltDown && !mbDown && rbDown;
                m_Input.IsMoving = !isAltDown && mbDown && rbDown;
                m_Input.IsZooming = wheelInUse && !m_Input.IsShiftDown;
                m_Input.IsOrbiting = isAltDown && lbDown && !mbDown && !rbDown;

                rmbWheel = useMovementSpeed &&
                           (m_Input.IsMouseRightDown || m_IsVirtualMouseRightDown) &&
                           wheelInUse;
                if (rmbWheel)
                {
                    float step = m_Input.MouseWheelDelta;
                    AdjustCameraMoveSpeed(step > 0.0f ? 1 : -1);
                }

                Float3 moveDelta = Float3.Zero;
                if (win.GetKey(KeyboardKeys.W /* options.Input.Forward.Key */))
                {
                    moveDelta += Float3.Forward;
                }
                if (win.GetKey(KeyboardKeys.S /* options.Input.Forward.Key */))
                {
                    moveDelta += Float3.Backward;
                }
                if (win.GetKey(KeyboardKeys.D /* options.Input.Forward.Key */))
                {
                    moveDelta += Float3.Right;
                }
                if (win.GetKey(KeyboardKeys.A /* options.Input.Forward.Key */))
                {
                    moveDelta += Float3.Left;
                }
                if (win.GetKey(KeyboardKeys.Q /* options.Input.Forward.Key */))
                {
                    moveDelta += Float3.Down;
                }
                if (win.GetKey(KeyboardKeys.E /* options.Input.Forward.Key */))
                {
                    moveDelta += Float3.Up;
                }
                moveDelta.Normalize();
                moveDelta *= m_MovementSpeed;
                if (m_Input.IsShiftDown)
                {
                    moveDelta *= 4.0f;
                }
                if (m_Input.IsControlDown)
                {
                    moveDelta *= 0.3f;
                }

                Float2 offset = m_ViewMousePos - m_LastViewMousePos;
                if (m_Input.IsZooming && !m_Input.IsMouseRightDown &&
                    !m_Input.IsMouseLeftDown && !m_Input.IsMouseMiddleDown &&
                    !UseOrthographicProjection && !rmbWheel && !m_IsVirtualMouseRightDown)
                {
                    offset = Float2.Zero;
                }

                Float2 mouseDelta = Float2.Zero;
                if (m_UseMouseFiltering)
                {
                    offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                    offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                    m_MouseDelta = offset;
                    _deltaFilteringBuffer[_deltaFilteringStep] = m_MouseDelta;
                    _deltaFilteringStep++;
                    if (_deltaFilteringStep == FpsCameraFilteringFrames)
                    {
                        _deltaFilteringStep = 0;
                    }
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
                    Float2 currentDelta = mouseDelta;
                    mouseDelta += m_MouseDeltaLast * _mouseAccelerationScale;
                    m_MouseDeltaLast = currentDelta;
                }

                moveDelta *= dt * 4.0f;
                mouseDelta *= MouseSpeed * m_MouseSensitivity * deltaTime;
                bool centerMouse = false;
                UpdateView(dt, in moveDelta, in mouseDelta, ref centerMouse);

                if (centerMouse)
                {
                    Float2 center = Float2.Round(Size * 0.5f);
                    win.SetMousePosition(PointToWindow(center));
                    m_ViewMousePos = center;
                    m_LastViewMousePos = center;
                }

                if (UseOrthographicProjection && !rmbWheel)
                {
                    float scroll = m_Input.MouseWheelDelta;
                    if (scroll > Mathf.Epsilon || scroll < -Mathf.Epsilon)
                    {
                        OrthographicScale -= scroll /* options.Viewport.MouseWheelSensitivity */ * 0.2f * OrthographicScale;
                    }
                }
            }
            else
            {
                if (m_Input.IsMouseLeftDown || m_Input.IsMouseRightDown || m_IsVirtualMouseRightDown)
                {
                    Float2 offset = m_ViewMousePos - m_LastViewMousePos;
                    offset.X = offset.X > 0 ? Mathf.Floor(offset.X) : Mathf.Ceil(offset.X);
                    offset.Y = offset.Y > 0 ? Mathf.Floor(offset.Y) : Mathf.Ceil(offset.Y);
                    m_MouseDelta = offset;
                    m_LastViewMousePos = m_ViewMousePos;
                }
                else
                {
                    m_MouseDelta = Float2.Zero;
                }
                m_MouseDeltaLast = Float2.Zero;

                /*if (ContainsFocus)
                {
                    m_Input.IsPanning = false;
                    m_Input.IsRotating = false;
                    m_Input.IsMoving = true;
                    m_Input.IsZooming = false;
                    m_Input.IsOrbiting = false;

                    Float3 moveDelta = Float3.Zero;
                    Float2 mouseDelta = Float2.Zero;
                    if (SE.Input.Gamepads.Count > 0)
                    {
                        moveDelta += new Float3(SE.Input.GetGamepadAxis(GamepadAxis.LeftStickX), 0,
                            SE.Input.GetGamepadAxis(GamepadAxis.LeftStickY));
                        mouseDelta += new Float2(SE.Input.GetGamepadAxis(GamepadAxis.RightStickX),
                            -SE.Input.GetGamepadAxis(GamepadAxis.RightStickY));
                        m_Input.IsRotating |= !mouseDelta.IsZero;
                    }
                    if (win.GetKey(KeyboardKeys.ArrowRight)) moveDelta += Float3.Right;
                    if (win.GetKey(KeyboardKeys.ArrowLeft)) moveDelta += Float3.Left;
                    if (win.GetKey(KeyboardKeys.ArrowUp)) moveDelta += Float3.Up;
                    if (win.GetKey(KeyboardKeys.ArrowDown)) moveDelta += Float3.Down;
                    moveDelta.Normalize();
                    moveDelta *= m_MovementSpeed;
                    if (SE.Input.GamepadsCount > 0)
                        moveDelta *= Mathf.Remap(SE.Input.GetGamepadAxis(GamepadAxis.RightTrigger), 0, 1, 1, 4.0f);
                    moveDelta *= dt * (60.0f * 4.0f);
                    UpdateView(dt, in moveDelta, in mouseDelta, ref centerMouse);
                }*/
            }

            m_LastViewMousePos = m_ViewMousePos;
            m_Input.MouseWheelDelta = 0;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();
            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            m_Input.MouseWheelDelta += delta;
            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);
            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
            {
                return true;
            }
            return false; // InputActions.Process(Editor.Instance, this, key);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();
            /*if (Editor.Instance.Simulation.IsDuringBreakpointHang)
            {
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, new Color(0.0f, 0.0f, 0.0f, 0.2f));
                Render2D.RenderText(Style.Current.FontLarge, "Debugger breakpoint hit...", bounds,
                    Color.White, TextAlignment.Center, TextAlignment.Center);
            }*/
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();
            ReleaseMouseControl();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // The root already removed the capture owner. Do not release recursively.
            ResetMouseControl();
            _isViewportControllingMouse = false;
            base.OnEndMouseCapture();
        }

        private void ResetMouseControl()
        {
            _waitForMouseRelease |= m_IsControllingMouse || _isViewportControllingMouse;
            if (m_IsControllingMouse)
            {
                Window window = RootWindow()?.Window;
                if (window != null)
                    window.Cursor = CursorType.Default;
            }
            m_IsControllingMouse = false;
            m_IsVirtualMouseRightDown = false;
            m_WasVirtualMouseRightDown = false;
            m_Input = default;
            m_PrevInput = default;
            m_MouseDelta = Float2.Zero;
            m_MouseDeltaLast = Float2.Zero;
            System.Array.Clear(_deltaFilteringBuffer, 0, _deltaFilteringBuffer.Length);
            _deltaFilteringStep = 0;
        }

        private void ReleaseMouseControl()
        {
            ResetMouseControl();
            if (_isViewportControllingMouse)
            {
                _isViewportControllingMouse = false;
                EndMouseCapture();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            ReleaseMouseControl();
            // Editor.Instance.Options.OptionsChanged -= OnEditorOptionsChanged;
            base.OnDestroy();
        }

        /// <summary>Copies the render view data to the <see cref="RenderView"/> structure.</summary>
        /// <param name="view">The view.</param>
        public void CopyViewData(ref RenderView view)
        {
            Float3 position = ViewPosition;
            // LargeWorlds.UpdateOrigin(ref view.Origin, position);
            view.Position = position - view.Origin;
            view.Direction = ViewDirection;
            view.Near = NearPlane;
            view.Far = FarPlane;
            CreateProjectionMatrix(out view.Projection);
            CreateViewMatrix(view.Position, out view.View);
            view.UpdateCachedData();
        }

        /// <summary>Gets the input state data.</summary>
        public ref Input GetInput()
        {
            return ref m_Input;
        }

        /// <summary>Gets the input state data from the previous update.</summary>
        public ref Input GetPrevInput()
        {
            return ref m_PrevInput;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
            // ViewportWidgetsContainer.ArrangeWidgets(this);
        }

        /// <summary>Gets a value indicating whether this viewport is using mouse currently.</summary>
        protected virtual bool GetIsControllingMouse()
        {
            return false;
        }

        /// <summary>Orients the viewport.</summary>
        /// <param name="orientation">The orientation.</param>
        protected virtual void OrientViewport(ref Quaternion orientation)
        {
            /*if (ViewportCamera is FPSCamera fpsCamera)
            {
                var pos = ViewPosition + Float3.Backward * orientation * 2000.0f;
                fpsCamera.MoveViewport(pos, orientation);
            }
            else*/
            ViewportCamera.SetArcBallView(orientation, ViewPosition, 2000.0f);
        }

        /// <summary>Increases or decreases the camera movement speed.</summary>
        /// <param name="step">The stepping direction for speed adjustment.</param>
        protected void AdjustCameraMoveSpeed(int step)
        {
            _speedStep = Mathf.Clamp(_speedStep + step, 0, _maxSpeedSteps);
            float progress = _useCameraEasing
                ? Mathf.Pow((float)_speedStep / _maxSpeedSteps, _cameraEasingDegree)
                : (float)_speedStep / _maxSpeedSteps;
            float speed = Mathf.Lerp(m_MinMovementSpeed, m_MaxMovementSpeed, progress);
            MovementSpeed = speed;
        }

        /// <summary>Creates the projection matrix.</summary>
        /// <param name="result">The result.</param>
        protected virtual void CreateProjectionMatrix(out Matrix result)
        {
            if (UseOrthographicProjection)
            {
                Matrix.Ortho(Width * (UseOrthographicProjection ? 1 : 0),
                    Height * OrthographicScale, NearPlane, FarPlane, out result);
            }
            else
            {
                float aspect = Width / Height;
                Matrix.PerspectiveFov(FieldOfView * Mathf.DegreesToRadians, aspect, NearPlane, FarPlane, out result);
            }
        }

        /// <summary>Creates the view matrix.</summary>
        /// <param name="position">The view position.</param>
        /// <param name="result">The result.</param>
        protected virtual void CreateViewMatrix(Float3 position, out Matrix result)
        {
            Float3 direction = ViewDirection;
            Float3 target = position + direction;
            Float3 right = Float3.Normalize(Float3.Cross(Float3.Up, direction));
            Float3 up = Float3.Normalize(Float3.Cross(direction, right));
            Matrix.LookAt(ref position, ref target, ref up, out result);
        }

        /// <summary>
        /// Called when mouse control begins.
        /// </summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseBegin(Window win)
        {
            m_StartPos = m_ViewMousePos;
            win.Cursor = CursorType.Hidden;

            Float2 center = Float2.Round(Size * 0.5f);
            RootWindow().SetMousePosition(PointToWindow(center));
            m_ViewMousePos = center;
            m_LastViewMousePos = center;
        }

        /// <summary>Called when mouse control ends.</summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseEnd(Window win)
        {
            win.Cursor = CursorType.Default;
            if (win.IsFocused && win.IsForegroundWindow)
                RootWindow().SetMousePosition(PointToWindow(m_StartPos));
            m_ViewMousePos = m_StartPos;
            m_LastViewMousePos = m_StartPos;
        }

        /// <summary>Called when left mouse button goes down.</summary>
        protected virtual void OnLeftMouseButtonDown()
        {
        }

        /// <summary>Called when left mouse button goes up.</summary>
        protected virtual void OnLeftMouseButtonUp()
        {
        }

        /// <summary>Called when right mouse button goes down.</summary>
        protected virtual void OnRightMouseButtonDown()
        {
        }

        /// <summary>Called when right mouse button goes up.</summary>
        protected virtual void OnRightMouseButtonUp()
        {
        }

        /// <summary>Called when middle mouse button goes down.</summary>
        protected virtual void OnMiddleMouseButtonDown()
        {
        }

        /// <summary>Called when middle mouse button goes up.</summary>
        protected virtual void OnMiddleMouseButtonUp()
        {
        }

        /// <summary>Updates the view.</summary>
        /// <param name="dt">The delta time in seconds.</param>
        /// <param name="moveDelta">The scaled move delta.</param>
        /// <param name="mouseDelta">The scaled mouse delta.</param>
        /// <param name="centerMouse">Whether to center the mouse after the update.</param>
        protected virtual void UpdateView(float dt, in Float3 moveDelta, in Float2 mouseDelta, ref bool centerMouse)
        {
            centerMouse = true;
            if (m_Camera != null)
            {
                m_Camera.UpdateView(dt, in moveDelta, in mouseDelta, ref centerMouse);
            }
        }

        private void OnCameraMovementProgressChanged()
        {
            if (Mathf.Abs(m_MinMovementSpeed - m_MaxMovementSpeed) < Mathf.Epsilon)
            {
                _speedStep = 0;
                return;
            }
            if (Mathf.Abs(m_MovementSpeed - m_MaxMovementSpeed) < Mathf.Epsilon)
            {
                _speedStep = _maxSpeedSteps;
                return;
            }
            if (Mathf.Abs(m_MovementSpeed - m_MinMovementSpeed) < Mathf.Epsilon)
            {
                _speedStep = 0;
                return;
            }

            float progress = Mathf.Remap(m_MovementSpeed, m_MinMovementSpeed, m_MaxMovementSpeed, 0.0f, 1.0f);
            if (_useCameraEasing)
            {
                progress = Mathf.Pow(progress, 1.0f / _cameraEasingDegree);
            }
            _speedStep = Mathf.RoundToInt(progress * _maxSpeedSteps);
        }

        private void OnRenderBegin(RenderTask task, GPUContext context)
        {
            SceneRenderTask sceneTask = (SceneRenderTask)task;
            RenderView view = sceneTask.View;
            CopyViewData(ref view);
            sceneTask.View = view;
        }

        private void SetupViewportOptions()
        {
            m_MinMovementSpeed = 0.1f;
            m_MaxMovementSpeed = 100.0f;
            m_MouseSensitivity = 1;
            m_MovementSpeed = 1;
            FieldOfView = 60.0f;
            MovementSpeed = 1;
            /*var options = Editor.Instance.Options.Options;
            m_MinMovementSpeed = options.Viewport.MinMovementSpeed;
            MovementSpeed = options.Viewport.MovementSpeed;
            m_MaxMovementSpeed = options.Viewport.MaxMovementSpeed;
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
    }
}
