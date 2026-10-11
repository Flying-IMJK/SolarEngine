#nullable disable annotations

using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class RenderOutputViewport : ContainerControl
    {
        /// <summary>
        /// The task.
        /// </summary>
        protected SceneRenderTask m_Task;

        /// <summary>
        /// The back buffer.
        /// </summary>
        protected GPUTexture m_BackBuffer;

        private GPUTexture m_BackBufferOld;
        private int _oldBackbufferLiveTimeLeft;
        private float m_ResizeTime;
        private Int2 _customResolution = new Int2(0, 0);

        /// <summary>
        /// The default back buffer format used by the GUI controls presenting rendered frames.
        /// </summary>
        public const PixelFormat BackBufferFormat = PixelFormat.R8G8B8A8_UNorm;

        /// <summary>
        /// The resize check timeout (in seconds).
        /// </summary>
        public const float ResizeCheckTime = 0.9f;

        /// <summary>
        /// Gets the task.
        /// </summary>
        public SceneRenderTask GetTask()
        {
            return m_Task;
        }

        /// <summary>
        /// Gets or sets a value indicating whether render to that output only if parent window exists, otherwise false.
        /// </summary>
        public bool RenderOnlyWithWindow = true;

        /// <summary>
        /// Gets or sets a value indicating whether use automatic task rendering skipping if output is too small or window is missing. Disable it to manually control <see cref="RenderTask.Enabled"/>.
        /// </summary>
        public bool UseAutomaticTaskManagement = true;

        /// <summary>
        /// Gets a value indicating whether keep aspect ratio of the backbuffer image, otherwise false.
        /// </summary>
        public bool KeepAspectRatio = false;

        /// <summary>
        /// Gets or sets the color of the tint used to color the backbuffer of the render output.
        /// </summary>
        public Color TintColor = Color.White;

        /// <summary>
        /// Gets or sets the brightness of the output.
        /// </summary>
        public float Brightness = 1.0f;

        /// <summary>
        /// Gets or sets the rendering resolution scale. Can be used to upscale image or to downscale the rendering to save the performance.
        /// </summary>
        public float ResolutionScale = 1.0f;

        /// <summary>
        /// Gets or sets the custom resolution to use for the rendering.
        /// </summary>
        public Int2 CustomResolution
        {
            get => _customResolution;
            set
            {
                if (_customResolution.X != value.X || _customResolution.Y != value.Y)
                {
                    _customResolution = value;
                    SyncBackbufferSize();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderOutputControl"/> class.
        /// </summary>
        /// <param name="task">The task. Cannot be null.</param>
        /// <exception cref="ArgumentNullException">Invalid task.</exception>
        public RenderOutputViewport(SceneRenderTask task)
        {
            SE.Debug.Assert(task != null, "Invalid task.");
            m_Task = task;

            m_BackBuffer = GPUDevice.instance.CreateTexture("RenderOutputControl.BackBuffer");

            m_ResizeTime = ResizeCheckTime;
            m_Task.Output = m_BackBuffer;
            m_Task.End += OnEnd;
            Engine.UpdateCall += OnUpdate;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            Rectangle bounds = new Rectangle(Float2.Zero, Size);

            // Draw background
            Color backgroundColor = BackgroundColor;
            if (backgroundColor.A > 0.0f)
            {
                Render2D.FillRectangle(bounds, backgroundColor);
            }

            // Draw backbuffer texture
            GPUTexture buffer;
            if (m_BackBufferOld != null)
            {
                buffer = m_BackBufferOld;
            }
            else
            {
                buffer = m_BackBuffer;
            }
            Color color = TintColor.RGBMultiplied(Brightness);
            if (KeepAspectRatio)
            {
                float ratioX = bounds.Width / buffer.Width;
                float ratioY = bounds.Height / buffer.Height;
                float ratio = ratioX < ratioY ? ratioX : ratioY;
                bounds = new Rectangle(
                    (bounds.Width - buffer.Width * ratio) / 2,
                    (bounds.Height - buffer.Height * ratio) / 2,
                    buffer.Width * ratio,
                    buffer.Height * ratio);
            }
            Render2D.DrawTexture(buffer, bounds, color);

            // Push clipping mask
            if (ClipChildren)
            {
                Rectangle clientArea = GetDesireClientArea();
                Render2D.PushClip(clientArea);
            }

            DrawChildren();

            // Pop clipping mask
            if (ClipChildren)
            {
                Render2D.PopClip();
            }
        }

        /// <summary>
        /// Synchronizes size of the back buffer with the size of the control.
        /// </summary>
        public void SyncBackbufferSize()
        {
            float scale = ResolutionScale * GetDpiScale();
            int width = (int)MathF.Ceiling(Width * scale);
            int height = (int)MathF.Ceiling(Height * scale);
            if (_customResolution.X != 0 || _customResolution.Y != 0)
            {
                width = _customResolution.X;
                height = _customResolution.Y;
            }
            if (m_BackBuffer == null || m_BackBuffer.Width == width && m_BackBuffer.Height == height)
            {
                return;
            }
            if (width < 1 || height < 1)
            {
                m_BackBuffer.ReleaseGPU();

                if (m_BackBufferOld != null)
                {
                    Object.DestroyNow(m_BackBufferOld);
                    m_BackBufferOld = null;
                }
                return;
            }

            // Cache old backbuffer to remove flickering effect
            if (m_BackBufferOld == null && m_BackBuffer.IsAllocated)
            {
                m_BackBufferOld = m_BackBuffer;
                m_BackBuffer = GPUDevice.instance.CreateTexture("RenderOutputControl.BackBuffer");
            }

            // Set timeout to remove old buffer
            _oldBackbufferLiveTimeLeft = 3;

            // Resize backbuffer
            GPUTextureDescription desc = GPUTextureDescription.New2D(width, height, BackBufferFormat);
            m_BackBuffer.Init(desc);
            m_Task.Output = m_BackBuffer;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
            {
                return;
            }

            Engine.UpdateCall -= OnUpdate;

            // Cleanup
            // Scripting.Update -= OnUpdate;
            if (m_Task != null)
            {
                m_Task.Enabled = false;
                // _task->ClearCustomActors();
                //_task.CustomPostFx.Clear();
            }
            m_BackBuffer.ReleaseGPU();
            m_BackBufferOld.ReleaseGPU();
            Object.DestroyNow(m_BackBuffer);
            Object.DestroyNow(m_BackBufferOld);
            Object.DestroyNow(m_Task);

            base.OnDestroy();
        }

        private bool WalkTree(Control c)
        {
            while (c != null)
            {
                if (c is RootControl)
                {
                    return false;
                }
                if (c.Visible == false)
                {
                    break;
                }
                c = c.Parent;
            }
            return true;
        }

        private void OnUpdate()
        {
            if (m_Task == null || !UseAutomaticTaskManagement)
            {
                return;
            }
            float deltaTime = Time.GetUnscaledDeltaTime();

            // Check if need to resize the output
            m_ResizeTime += deltaTime;
            if (m_ResizeTime >= ResizeCheckTime && Visible && Enabled)
            {
                m_ResizeTime = 0;
                SyncBackbufferSize();
            }

            // Check if skip rendering
            bool wasEnabled = m_Task.Enabled;
            m_Task.Enabled = !CanSkipRendering();
            if (!wasEnabled && m_Task.Enabled)
            {
                SyncBackbufferSize();
            }
        }

        /// <summary>
        /// Performs a check if rendering a current frame can be skipped (if control size is too small, has missing data, etc.).
        /// </summary>
        /// <returns>True if skip rendering, otherwise false.</returns>
        protected virtual bool CanSkipRendering()
        {
            // Disable task rendering if control is very small
            const float MinRenderSize = 4;
            if (Width < MinRenderSize || Height < MinRenderSize)
            {
                return true;
            }

            // Disable task rendering if control is not used in a window (has using ParentWindow)
            if (RenderOnlyWithWindow)
            {
                return WalkTree(Parent);
            }

            return false;
        }

        /// <summary>
        /// Called when ask rendering ends.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="context">The GPU execution context.</param>
        protected virtual void OnEnd(RenderTask task, GPUContext context)
        {
            // Check if was using old backbuffer
            if (m_BackBufferOld != null)
            {
                _oldBackbufferLiveTimeLeft--;
                if (_oldBackbufferLiveTimeLeft < 0)
                {
                    m_BackBufferOld.ReleaseGPU();
                    Object.DestroyNow(m_BackBufferOld);
                    m_BackBufferOld = null;
                }
            }
        }

    }
}
