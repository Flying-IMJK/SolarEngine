#nullable disable annotations

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
        private bool m_WasVirtualMouseRightDown;
        private bool m_IsVirtualMouseRightDown;
        private int _deltaFilteringStep;
        private Float2 m_StartPos;
        private Float2 m_MouseDeltaLast;
        private Float2 m_LastViewMousePos;
        private readonly Float2[] _deltaFilteringBuffer = new Float2[FpsCameraFilteringFrames];

        //private ViewportCamera m_Camera;
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
                // Native camera button text update is commented out.
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
/*        public ViewportCamera ViewportCamera
        {
            get => m_Camera;
            set
            {
                if (m_Camera != null)
                {
                    // m_Camera.SetViewport(null);
                }
                m_Camera = value;
                if (m_Camera != null)
                {
                    // m_Camera.SetViewport(this);
                }
            }
        }
*/
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
        public EditorViewport(SceneRenderTask task/*, ViewportCamera camera*/, bool useWidgets) : base(task)
        {
            _mouseAccelerationScale = 0.1f;
            m_UseMouseFiltering = false;
            m_UseMouseAcceleration = false;
            //m_Camera = camera;
            //if (m_Camera != null)
            //{
            //    // m_Camera.SetViewport(this);
            //}

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;

            // Editor options and project cache bindings are commented out in the native source.
            SetupViewportOptions();
            OnCameraMovementProgressChanged();

            if (useWidgets)
            {
                #region Camera settings widget
                // The native ViewportWidgetsContainer and ViewportWidgetButton calls are
                // retained in EditorViewport.cpp until their C# types are migrated.
                // _cameraWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                // _cameraButton = new ViewportWidgetButton(...);
                // _orthographicModeButton = new ViewportWidgetButton(...);
                #endregion

                #region View mode widget
                // Native ViewWidgetButtonMenu and ViewWidgetShowMenu setup calls are
                // commented pending the viewport widget types.
                #endregion
            }

            // task.Begin += OnRenderBegin; // Native render-task Begin is not exported.
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
        }

        public void OnCameraEasingToggled(Control control)
        {
            _useCameraEasing = !_useCameraEasing;
            OnCameraMovementProgressChanged();
        }

        public void OnPanningSpeedChanged(FloatValueBox control)
        {
            PanningSpeed = control.GetValue();
        }

        public void OnRelativePanningToggled(Control control)
        {
            RelativePanning = !RelativePanning;
        }

        public void OnInvertPanningToggled(Control control)
        {
            InvertPanning = !InvertPanning;
        }

        public void OnViewpointChanged(ContextMenuButton button)
        {
            Quaternion orient = Quaternion.Euler((Float3)button.Tag);
            OrientViewport(ref orient);
        }

        public void OnFieldOfViewChanged(FloatValueBox control)
        {
            FieldOfView = control.GetValue();
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
        }

        public void OnOrthographicSizeChanged(FloatValueBox control)
        {
            OrthographicScale = control.GetValue();
        }

        public void OnNearPlaneChanged(FloatValueBox control)
        {
            NearPlane = control.GetValue();
        }

        public void OnFarPlaneChanged(FloatValueBox control)
        {
            FarPlane = control.GetValue();
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
/*            if (m_Camera != null)
            {
                // m_Camera.Update(deltaTime);
                // useMovementSpeed = m_Camera.GetUseMovementSpeed();
            }*/

            WindowRootControl win = (WindowRootControl)Root;
            m_ViewMousePos = PointFromWindow(win.GetMousePosition());

            Window window = win.Window;
            bool canUseInput = window != null && window.IsFocused && window.IsForegroundWindow;
            bool isViewportControllingMouse = canUseInput && GetIsControllingMouse();
            if (isViewportControllingMouse != _isViewportControllingMouse)
            {
                _isViewportControllingMouse = isViewportControllingMouse;
                if (isViewportControllingMouse)
                {
                    StartMouseCapture();
                }
                else
                {
                    EndMouseCapture();
                }
            }

            bool useMouse = GetIsControllingMouse() || m_PrevInput.GetIsControllingMouse() ||
                            (m_ViewMousePos.X >= 0.0f && m_ViewMousePos.X <= Width &&
                             m_ViewMousePos.Y >= 0.0f && m_ViewMousePos.Y <= Height);
            m_PrevInput = m_Input;
            Control hit = GetChildAt(m_ViewMousePos, control => control.Visible);
            if (canUseInput && ContainsFocus && (hit == null || m_PrevInput.GetIsControllingMouse()))
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
                if (win.GetKey(KeyboardKeys.W))
                {
                    moveDelta += Float3.Forward;
                }
                if (win.GetKey(KeyboardKeys.S))
                {
                    moveDelta += Float3.Backward;
                }
                if (win.GetKey(KeyboardKeys.D))
                {
                    moveDelta += Float3.Right;
                }
                if (win.GetKey(KeyboardKeys.A))
                {
                    moveDelta += Float3.Left;
                }
                if (win.GetKey(KeyboardKeys.Q))
                {
                    moveDelta += Float3.Down;
                }
                if (win.GetKey(KeyboardKeys.E))
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

                if (UseOrthographicProjection && !rmbWheel)
                {
                    float scroll = m_Input.MouseWheelDelta;
                    if (scroll > Mathf.Epsilon || scroll < -Mathf.Epsilon)
                    {
                        OrthographicScale -= scroll * 0.2f * OrthographicScale;
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
            return false; // Native InputActions.Process is commented out.
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();
            // Native debugger-breakpoint overlay is commented out.
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();
            if (m_IsControllingMouse)
            {
                OnControlMouseEnd(RootWindow().Window);
                m_IsControllingMouse = false;
                m_IsVirtualMouseRightDown = false;
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
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
            //view.UpdateCachedData();
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
            // ViewportCamera.SetArcBallView(orientation, ViewPosition, 2000.0f);
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

        /// <summary>Called when mouse control begins.</summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseBegin(Window win)
        {
            win.StartTrackingMouse(false);
            win.Cursor = CursorType.Hidden;

            Float2 size = Size;
            Float2 center = Float2.Round(size * 0.5f);
            if (Mathf.Abs(m_ViewMousePos.X - center.X) > center.X * 0.8f ||
                Mathf.Abs(m_ViewMousePos.Y - center.Y) > center.Y * 0.8f)
            {
                m_StartPos = center;
                m_ViewMousePos = center;
                m_LastViewMousePos = center;
                win.MousePosition = PointToWindow(m_ViewMousePos);
            }
        }

        /// <summary>Called when mouse control ends.</summary>
        /// <param name="win">The parent window.</param>
        protected virtual void OnControlMouseEnd(Window win)
        {
            win.Cursor = CursorType.Default;
            win.MousePosition = PointToWindow(m_StartPos);
            win.EndTrackingMouse();
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
            //if (m_Camera != null)
            //{
            //    // m_Camera.UpdateView(dt, moveDelta, mouseDelta, ref centerMouse);
            //}
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
            // SceneRenderTask sceneTask = (SceneRenderTask)task;
            // RenderView view = sceneTask.View;
            // CopyViewData(ref view);
            // sceneTask.View = view;
        }

        private void SetupViewportOptions()
        {
            m_MinMovementSpeed = 0.1f;
            m_MaxMovementSpeed = 100.0f;
            m_MouseSensitivity = 1;
            m_MovementSpeed = 1;
            FieldOfView = 60.0f;
            MovementSpeed = 1;
            // Native Editor.Options assignments are commented out.
        }
    }
}
