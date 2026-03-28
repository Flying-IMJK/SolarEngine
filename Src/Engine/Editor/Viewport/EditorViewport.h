#pragma once

#include "Cameras/ViewportCamera.h"
#include "Core/Input/Input.h"
#include "Core/Math/Transform.h"

#include "Editor/GUI/Viewport/RenderOutputViewport.h"

namespace SE
{
	struct RenderView;
}

namespace SE::Editor
{
	class ViewportWidgetButton;
	class ViewportWidgetsContainer;
	class ContextMenuButton;
    class ViewportCamera;
    class FloatValueBox;
    class ContextMenu;

    class EditorViewport : public RenderOutputViewport
	{
	public:
        /// <summary>
        /// Gathered input data.
        /// </summary>
	    struct Input
        {
	    public:
            /// <summary>
            /// The is panning state.
            /// </summary>
            bool IsPanning;

            /// <summary>
            /// The is rotating state.
            /// </summary>
	        bool IsRotating;

            /// <summary>
            /// The is moving state.
            /// </summary>
	        bool IsMoving;

            /// <summary>
            /// The is zooming state.
            /// </summary>
	        bool IsZooming;

            /// <summary>
            /// The is orbiting state.
            /// </summary>
	        bool IsOrbiting;

            /// <summary>
            /// The is control down flag.
            /// </summary>
	        bool IsControlDown;

            /// <summary>
            /// The is shift down flag.
            /// </summary>
	        bool IsShiftDown;

            /// <summary>
            /// The is alt down flag.
            /// </summary>
	        bool IsAltDown;

            /// <summary>
            /// The is alt down flag cached from the previous input. Used to make <see cref="IsControllingMouse"/> consistent when user releases Alt while orbiting with Alt+LMB.
            /// </summary>
	        bool WasAltDownBefore;

            /// <summary>
            /// The is mouse right down flag.
            /// </summary>
	        bool IsMouseRightDown;

            /// <summary>
            /// The is mouse middle down flag.
            /// </summary>
	        bool IsMouseMiddleDown;

            /// <summary>
            /// The is mouse left down flag.
            /// </summary>
	        bool IsMouseLeftDown;

            /// <summary>
            /// The mouse wheel delta.
            /// </summary>
	        float MouseWheelDelta;

            /// <summary>
            /// Gets a value indicating whether use is controlling mouse.
            /// </summary>
	        bool GetIsControllingMouse() { return IsMouseMiddleDown || IsMouseRightDown || ((IsAltDown || WasAltDownBefore) && IsMouseLeftDown) || Math::Abs(MouseWheelDelta) > 0.1f; }

            /// <summary>
            /// Gathers input from the specified window.
            /// </summary>
            /// <param name="window">The window.</param>
            /// <param name="useMouse">True if use mouse input, otherwise will skip mouse.</param>
            /// <param name="prevInput">Previous input state.</param>
	        void Gather(Window* window, bool useMouse, Input& prevInput);

            /// <summary>
            /// Clears the data.
            /// </summary>
	        void Clear()
            {
                IsControlDown = false;
                IsShiftDown = false;
                IsAltDown = false;
                WasAltDownBefore = false;

                IsMouseRightDown = false;
                IsMouseMiddleDown = false;
                IsMouseLeftDown = false;
            }
        };

        /// <summary>
        /// The FPS camera filtering frames count (how much frames we want to keep in the buffer to calculate the avg. delta currently hardcoded).
        /// </summary>
	    constexpr static int FpsCameraFilteringFrames = 3;

	private:
        float m_MouseSensitivity;
        float m_MovementSpeed;
        float m_MinMovementSpeed;
        float m_MaxMovementSpeed;
        float _mouseAccelerationScale;
        bool m_UseMouseFiltering;
        bool m_UseMouseAcceleration;

        // Input
	    bool _disableInputUpdate;
        bool m_IsControllingMouse, _isViewportControllingMouse, m_WasVirtualMouseRightDown, m_IsVirtualMouseRightDown;
        int _deltaFilteringStep;
        Float2 m_StartPos;
        Float2 m_MouseDeltaLast;
    	Float2 m_LastViewMousePos;
        Float2 _deltaFilteringBuffer[FpsCameraFilteringFrames];

	    // Camera
	    ViewportCamera* m_Camera;
	    float _yaw;
	    float _pitch;
	    bool _useCameraEasing;
	    float _cameraEasingDegree;

	    int _speedStep;
	    int _maxSpeedSteps;

	protected:
    	/// <summary>
    	/// The camera settings widget.
    	/// </summary>
    	ViewportWidgetsContainer* _cameraWidget;

    	/// <summary>
    	/// The camera settings widget button.
    	/// </summary>
    	ViewportWidgetButton* _cameraButton;

    	/// <summary>
    	/// The orthographic mode widget button.
    	/// </summary>
    	ViewportWidgetButton* _orthographicModeButton;

        /// <summary>
        /// The previous input (from the previous update).
        /// </summary>
	    Input m_PrevInput;

        /// <summary>
        /// The input data (from the current frame).
        /// </summary>
	    Input m_Input;

        /// <summary>
        /// The view mouse position.
        /// </summary>
	    Float2 m_ViewMousePos;

        /// <summary>
        /// The mouse position delta.
        /// </summary>
	    Float2 m_MouseDelta;

	    /// <summary>
	    /// Format of the text for the camera move speed.
	    /// </summary>
	    const Char* GetMovementSpeedTextFormat();

	public:
        /// <summary>
        /// Speed of the mouse.
        /// </summary>
	    float MouseSpeed = 5;

        /// <summary>
        /// Speed of the mouse wheel zooming.
        /// </summary>
	    float MouseWheelZoomSpeedFactor = 1;

        /// <summary>
        /// Gets or sets the camera movement speed.
        /// </summary>
        PRO(MovementSpeed, EditorViewport, float, __GetMovementSpeed, __SetMovementSpeed);

        /// <summary>
        /// Gets or sets the minimum camera movement speed.
        /// </summary>
	    PRO(MinMovementSpeed, EditorViewport, float, __GetMinMovementSpeed, __SetMinMovementSpeed);

        /// <summary>
        /// Gets or sets the maximum camera movement speed.
        /// </summary>
	    PRO(MaxMovementSpeed, EditorViewport, float, __GetMaxMovementSpeed, __SetMaxMovementSpeed);

        /// <summary>
        /// Gets or sets the camera easing mode.
        /// </summary>
	    PRO(UseCameraEasing, EditorViewport, float, __GetUseCameraEasing, __SetUseCameraEasing);

        /// <summary>
        /// Gets the mouse movement position delta (user press and move).
        /// </summary>
	    PRO_GET(MousePositionDelta, EditorViewport, Float2, __GetMousePositionDelta);

        /// <summary>
        /// Camera's pitch angle clamp range (in degrees).
        /// </summary>
	    Float2 CamPitchAngles = Float2(-88, 88);

        /// <summary>
        /// Gets the view transform.
        /// </summary>
	    PRO(ViewTransform, EditorViewport, Transform, __GetViewTransform, __SetViewTransform);

        /// <summary>
        /// Gets or sets the view position.
        /// </summary>
	    Float3 ViewPosition;

        /// <summary>
        /// Gets or sets the view orientation.
        /// </summary>
	    PRO(ViewOrientation, EditorViewport, Quaternion, __GetViewOrientation, __SetViewOrientation);

        /// <summary>
        /// Gets or sets the view direction vector.
        /// </summary>
	    PRO(ViewDirection, EditorViewport, Float3, __GetViewDirection, __SetViewDirection);
	    Float3 __GetViewDirection() { return ViewOrientation.Get() * Float3::Forward; }
	    void __SetViewDirection(Float3 value)
	    {
	        Float3 right = Float3::Cross(value, Float3::Up);
	        Float3 up = Float3::Cross(right, value);
	        ViewOrientation = Quaternion::LookRotation(value, up);
	    }

        /// <summary>
        /// Gets or sets the view ray (position and direction).
        /// </summary>
        /*public Ray ViewRay
        {
            get => new Ray(ViewPosition, ViewDirection);
            set
            {
                ViewPosition = value.Position;
                ViewDirection = value.Direction;
            }
        }*/

        /// <summary>
        /// Gets the bounding frustum of the current viewport camera.
        /// </summary>
        /*public BoundingFrustum ViewFrustum
        {
            get
            {
                Vector3 viewOrigin = Task.View.Origin;
                Float3 position = ViewPosition - viewOrigin;
                CreateViewMatrix(position, out var view);
                CreateProjectionMatrix(out var projection);
                Matrix.Multiply(ref view, ref projection, out var viewProjection);
                return new BoundingFrustum(ref viewProjection);
            }
        }*/

        /// <summary>
        /// Gets or sets the yaw angle (in degrees).
        /// </summary>
	    PRO(Yaw, EditorViewport, float, __GetYaw, __SetYaw);

        /// <summary>
        /// Gets or sets the pitch angle (in degrees).
        /// </summary>
	    PRO(Pitch, EditorViewport, float, __GetPitch, __SetPitch);

        /// <summary>
        /// Gets or sets the absolute mouse position (normalized, not in pixels). Yaw is X, Pitch is Y.
        /// </summary>
	    PRO(YawPitch, EditorViewport, Float2, __GetYawPitch, __SetYawPitch);

        /// <summary>
        /// Gets or sets the euler angles (pitch, yaw, roll).
        /// </summary>
	    PRO(EulerAngles, EditorViewport, Float3, __GetEulerAngles, __SetEulerAngles);

        /// <summary>
        /// Gets a value indicating whether this viewport has loaded dependant assets.
        /// </summary>
	    virtual bool GetHasLoadedAssets() { return true; }

        /// <summary>
        /// The 'View' widget button context menu.
        /// </summary>
	    ContextMenu* ViewWidgetButtonMenu;

        /// <summary>
        /// The 'View' widget 'Show' category context menu.
        /// </summary>
	    ContextMenu* ViewWidgetShowMenu;

        /// <summary>
        /// Gets or sets the viewport camera controller.
        /// </summary>
        PRO(ViewportCamera, EditorViewport, ViewportCamera*, __GetViewportCamera, __SetViewportCamera);

        /// <summary>
        /// Gets or sets the camera near clipping plane.
        /// </summary>
	    float NearPlane;

        /// <summary>
        /// Gets or sets the camera far clipping plane.
        /// </summary>
	    float FarPlane;

        /// <summary>
        /// Gets or sets the camera field of view (in degrees).
        /// </summary>
	    float FieldOfView;

        /// <summary>
        /// Gets or sets the camera orthographic size scale (if camera uses orthographic mode).
        /// </summary>
	    float OrthographicScale;

        /// <summary>
        /// Gets or sets the camera orthographic mode (otherwise uses perspective projection).
        /// </summary>
	    bool UseOrthographicProjection;

        /// <summary>
        /// Gets or sets if the panning speed should be relative to the camera target.
        /// </summary>
	    bool RelativePanning;

        /// <summary>
        /// Gets or sets if the panning direction is inverted.
        /// </summary>
	    bool InvertPanning;

        /// <summary>
        /// Gets or sets the camera panning speed.
        /// </summary>
	    float PanningSpeed;

        /// <summary>
        /// The input actions collection to processed during user input.
        /// </summary>
        // public InputActionsContainer InputActions = new InputActionsContainer();

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorViewport"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="camera">The camera controller.</param>
        /// <param name="useWidgets">Enable/disable viewport widgets.</param>
	    EditorViewport(SceneRenderTask* task, Editor::ViewportCamera* camera, bool useWidgets);

        void OnMovementSpeedChanged(FloatValueBox* control);

        void OnMinMovementSpeedChanged(FloatValueBox* control);

        void OnMaxMovementSpeedChanged(FloatValueBox* control);

        void OnCameraEasingToggled(Control* control);

        void OnPanningSpeedChanged(FloatValueBox* control);

        void OnRelativePanningToggled(Control* control);

        void OnInvertPanningToggled(Control* control);

        void OnViewpointChanged(ContextMenuButton* button);

        void OnFieldOfViewChanged(FloatValueBox* control);

        void OnOrthographicModeToggled(Control* control);

        void OnOrthographicSizeChanged(FloatValueBox* control);

        void OnNearPlaneChanged(FloatValueBox* control);

        void OnFarPlaneChanged(FloatValueBox* control);

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
    	void OnChildResized(Control* control) override;

        /// <inheritdoc />
    	bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
    	void OnLostFocus() override;

        /// <inheritdoc />
    	void OnDestroy() override;

        /// <summary>
        /// Takes the screenshot of the current viewport.
        /// </summary>
        /// <param name="path">The output file path. Set null to use default value.</param>
        /*public void TakeScreenshot(string path = null)
        {
            Screenshot.Capture(Task, path);
        }*/

        /// <summary>
        /// Copies the render view data to <see cref="RenderView"/> structure.
        /// </summary>
        /// <param name="view">The view.</param>
    	void CopyViewData(RenderView& view);

        /// <summary>
        /// Gets the input state data.
        /// </summary>
    	Input& GetInput();

        /// <summary>
        /// Gets the input state data (from the previous update).
        /// </summary>
        /// <param name="input">The input.</param>
    	Input& GetPrevInput()
        {
            return m_PrevInput;
        }


    protected:
        /// <summary>
        /// Gets a value indicating whether this viewport is using mouse currently (eg. user moving objects).
        /// </summary>
    	virtual bool GetIsControllingMouse()
    	{
    		return false;
    	};

        /// <inheritdoc />
    	void PerformLayoutBeforeChildren() override;

        /// <summary>
        /// Orients the viewport.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        virtual void OrientViewport(Quaternion& orientation);

    	/// <summary>
    	/// Increases or decreases the camera movement speed.
    	/// </summary>
    	/// <param name="step">The stepping direction for speed adjustment.</param>
    	void AdjustCameraMoveSpeed(int step);


        /*#region FPS Counter

        private class FpsCounter : Control
        {
            public FpsCounter(float x, float y)
            : base(x, y, 64, 32)
            {
            }

            public override void Draw()
            {
                EditorViewport::Draw();

                int fps = Engine.FramesPerSecond;
                Color color = Color.Green;
                if (fps < 13)
                    color = Color.Red;
                else if (fps < 22)
                    color = Color.Yellow;
                var text = string.Format("FPS: {0}", fps);
                var font = Style.Current.FontMedium;
                Render2D.DrawText(font, text, new Rectangle(Float2::One, Size), Color.Black);
                Render2D.DrawText(font, text, new Rectangle(Float2::Zero, Size), color);
            }
        }

        private FpsCounter _fpsCounter;
        private ContextMenuButton _showFpsButton;

        /// <summary>
        /// Gets or sets a value indicating whether show or hide FPS counter.
        /// </summary>
        public bool ShowFpsCounter
        {
            get => _fpsCounter.Visible;
            set
            {
                _fpsCounter.Visible = value;
                _fpsCounter.Enabled = value;
                _showFpsButton.Icon = value ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
            }
        }

        private void InitFpsCounter()
        {
            _fpsCounter = new FpsCounter(10, ViewportWidgetsContainer.WidgetsHeight + 14)
            {
                Visible = false,
                Enabled = false,
                Parent = this
            };
        }

        #endregion*/

        /// <summary>
        /// Creates the projection matrix.
        /// </summary>
        /// <param name="result">The result.</param>
        virtual void CreateProjectionMatrix(Matrix& result);

        /// <summary>
        /// Creates the view matrix.
        /// </summary>
        /// <param name="position">The view position.</param>
        /// <param name="result">The result.</param>
        virtual void CreateViewMatrix(Float3 position, Matrix& result);

        /// <summary>
        /// Gets the mouse ray.
        /// </summary>
        /*public Ray MouseRay
        {
            get
            {
                if (IsMouseOver)
                    return ConvertMouseToRay(ref _viewMousePos);
                return new Ray(Float3::Maximum, Float3::Up);
            }
        }*/

        /// <summary>
        /// Converts the mouse position to the ray (in world space of the viewport).
        /// </summary>
        /// <param name="mousePosition">The mouse position.</param>
        /// <returns>The result ray.</returns>
        /*
        public Ray ConvertMouseToRay(ref Float2 mousePosition)
        {
            // Prepare
            var viewport = new FlaxEngine.Viewport(0, 0, Width, Height);
            CreateProjectionMatrix(out var p);
            Vector3 viewOrigin = Task.View.Origin;
            Float3 position = ViewPosition - viewOrigin;
            CreateViewMatrix(position, out var v);
            Matrix.Multiply(ref v, ref p, out var ivp);
            ivp.Invert();

            // Create near and far points
            var nearPoint = new Vector3(mousePosition, _nearPlane);
            var farPoint = new Vector3(mousePosition, _farPlane);
            viewport.Unproject(ref nearPoint, ref ivp, out nearPoint);
            viewport.Unproject(ref farPoint, ref ivp, out farPoint);

            // Create direction vector
            Vector3 direction = farPoint - nearPoint;
            direction.Normalize();

            return new Ray(nearPoint + viewOrigin, direction);
        }
        */

        /// <summary>
        /// Called when mouse control begins.
        /// </summary>
        /// <param name="win">The parent window.</param>
    	virtual void OnControlMouseBegin(Window* win);

        /// <summary>
        /// Called when mouse control ends.
        /// </summary>
        /// <param name="win">The parent window.</param>
        virtual void OnControlMouseEnd(Window* win);

        /// <summary>
        /// Called when left mouse button goes down (on press).
        /// </summary>
        virtual void OnLeftMouseButtonDown()
        {

        }

        /// <summary>
        /// Called when left mouse button goes up (on release).
        /// </summary>
    	virtual void OnLeftMouseButtonUp()
        {
        }

        /// <summary>
        /// Called when right mouse button goes down (on press).
        /// </summary>
    	virtual void OnRightMouseButtonDown()
        {

        }

        /// <summary>
        /// Called when right mouse button goes up (on release).
        /// </summary>
    	virtual void OnRightMouseButtonUp()
        {
        }

        /// <summary>
        /// Called when middle mouse button goes down (on press).
        /// </summary>
    	virtual void OnMiddleMouseButtonDown()
        {

        }

        /// <summary>
        /// Called when middle mouse button goes up (on release).
        /// </summary>
    	virtual void OnMiddleMouseButtonUp()
        {
        }

        /// <summary>
        /// Updates the view.
        /// </summary>
        /// <param name="dt">The delta time (in seconds).</param>
        /// <param name="moveDelta">The move delta (scaled).</param>
        /// <param name="mouseDelta">The mouse delta (scaled).</param>
        /// <param name="centerMouse">True if center mouse after the update.</param>
        virtual void UpdateView(float dt, const Float3& moveDelta, const Float2& mouseDelta, bool& centerMouse);

	private:

    	void OnCameraMovementProgressChanged();

    	/*void OnEditorOptionsChanged(EditorOptions options)
    	{
    		_mouseSensitivity = options.Viewport.MouseSensitivity;
    		_maxSpeedSteps = options.Viewport.TotalCameraSpeedSteps;
    		_cameraEasingDegree = options.Viewport.CameraEasingDegree;
    		OnCameraMovementProgressChanged();
    	}*/

    	void OnRenderBegin(RenderTask* task, GPUContext* context);

        /// <summary>
        /// Sets the viewport options to the default values.
        /// </summary>
        void SetupViewportOptions();


	    float __GetMovementSpeed() { return m_MovementSpeed; }
	    void __SetMovementSpeed(float value);
	    float __GetMinMovementSpeed() { return m_MinMovementSpeed; }
	    void __SetMinMovementSpeed(float value) { m_MinMovementSpeed = value; }
	    float __GetMaxMovementSpeed() { return m_MaxMovementSpeed; }
	    void __SetMaxMovementSpeed(float value) { m_MaxMovementSpeed = value; }
	    float __GetUseCameraEasing() { return _useCameraEasing; }
	    void __SetUseCameraEasing(float value) { _useCameraEasing = value; }
	    Float2 __GetMousePositionDelta() { return m_MouseDelta; }
	    ::SE::Transform __GetViewTransform() { return ::SE::Transform(ViewPosition, ViewOrientation); }
	    void __SetViewTransform(::SE::Transform value)
	    {
	        ViewPosition = value.Translation;
	        ViewOrientation = value.Orientation;;
	    }
	    Quaternion __GetViewOrientation() { return Quaternion::RotationYawPitchRoll(_yaw * Math::DegreesToRadians, _pitch * Math::DegreesToRadians, 0); }
	    void __SetViewOrientation(Quaternion value)
	    {
	        EulerAngles = value.GetEuler();
	    }

	    float __GetYaw() { return _yaw; }
	    void __SetYaw(float value)
	    {
	        _yaw = value;
	    }
	    float __GetPitch() { return _pitch; }
	    void __SetPitch(float value)
	    {
	        _pitch = Math::Clamp(value, CamPitchAngles.x, CamPitchAngles.y);
	    }
	    Float2 __GetYawPitch() { return Float2(_yaw, _pitch); }
	    void __SetYawPitch(Float2 value)
	    {
	        Yaw = value.x;
	        Pitch = value.y;
	    }
	    Float3 __GetEulerAngles() { return Float3(_pitch, _yaw, 0); }
	    void __SetEulerAngles(Float3 value)
	    {
	        Pitch = value.x;
	        Yaw = value.y;
	    }

        ::SE::Editor::ViewportCamera* __GetViewportCamera() { return m_Camera; }
        void __SetViewportCamera(::SE::Editor::ViewportCamera* value);
	};

} // SE

