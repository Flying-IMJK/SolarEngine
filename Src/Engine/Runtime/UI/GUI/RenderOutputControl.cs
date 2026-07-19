using System;

namespace SE.GUI
{
    /// <summary>
    /// Base control for presenting the result of a render task in a managed GUI tree.
    /// The actual GPU output binding remains owned by the Runtime graphics layer.
    /// </summary>
    public class RenderOutputControl : ContainerControl
    {
        /// <summary>
        /// The resize check timeout in seconds.
        /// </summary>
        public const float ResizeCheckTime = 0.9f;

        private Int2? _customResolution;
        private Int2 _backBufferSize;

        public RenderOutputControl(object renderTask)
        {
            Task = renderTask ?? throw new ArgumentNullException(nameof(renderTask));
        }

        /// <summary>
        /// Gets the render task associated with this output.
        /// </summary>
        public object Task { get; }

        /// <summary>
        /// Gets or sets whether rendering is skipped while the control is detached from a root window.
        /// </summary>
        public bool RenderOnlyWithWindow { get; set; } = true;

        /// <summary>
        /// Gets or sets whether the owning render task should use automatic visibility and size management.
        /// </summary>
        public bool UseAutomaticTaskManagement { get; set; } = true;

        public bool KeepAspectRatio { get; set; }
        public Color TintColor { get; set; } = Color.White;
        public float Brightness { get; set; } = 1.0f;
        public float ResolutionScale { get; set; } = 1.0f;

        /// <summary>
        /// Gets the desired logical output-buffer size. The graphics layer owns the actual texture.
        /// </summary>
        public Int2 BackBufferSize => _backBufferSize;

        /// <summary>
        /// Gets or sets a fixed output-buffer size. A null value follows the control bounds.
        /// </summary>
        public Int2? CustomResolution
        {
            get => _customResolution;
            set
            {
                if (_customResolution == value)
                    return;

                _customResolution = value;
                SyncBackbufferSize();
            }
        }

        /// <summary>
        /// Updates the desired output-buffer size from the control bounds and resolution scale.
        /// </summary>
        public void SyncBackbufferSize()
        {
            if (_customResolution.HasValue)
            {
                _backBufferSize = _customResolution.Value;
                return;
            }

            float scale = Math.Max(0.01f, ResolutionScale);
            _backBufferSize = new Int2(
                Math.Max(1, (int)Math.Ceiling(Width * scale)),
                Math.Max(1, (int)Math.Ceiling(Height * scale)));
        }

        /// <summary>
        /// Determines whether automatic render-task management can skip this output.
        /// </summary>
        protected virtual bool CanSkipRendering()
        {
            if (!UseAutomaticTaskManagement || Width < 1.0f || Height < 1.0f)
                return true;

            if (!RenderOnlyWithWindow)
                return false;

            for (Control? control = this; control != null; control = control.Parent)
            {
                if (!control.Visible)
                    return true;
                if (control is RootControl)
                    return false;
            }

            return true;
        }

        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            if (sizeChanged)
                SyncBackbufferSize();
        }
    }
}
