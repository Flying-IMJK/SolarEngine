using System;

namespace SE.GUI
{
    /// <summary>
    /// Managed GUI root hosted by a native <see cref="SE.Window"/>.
    /// </summary>
    public sealed class WindowRootControl : RootControl
    {
        internal WindowRootControl(SE.Window window)
        {
            Window = window ?? throw new ArgumentNullException(nameof(window));
        }

        /// <summary>
        /// Gets the native window that hosts this managed GUI tree.
        /// </summary>
        public SE.Window Window { get; }

        /// <summary>
        /// Gets whether the native window host has completed GUI initialization.
        /// </summary>
        public bool IsInitialized { get; private set; }

        internal void Initialize(Float2 logicalSize, float dpiScale)
        {
            if (IsDisposed)
                return;

            IsInitialized = true;
            SetWindowMetrics(logicalSize, dpiScale);
        }

        internal void Resize(Float2 logicalSize, float dpiScale)
        {
            if (!IsInitialized || IsDisposed)
                return;

            SetWindowMetrics(logicalSize, dpiScale);
        }

        internal void OnWindowFocusChanged(bool focused)
        {
            if (!focused)
                ClearState();
        }

        protected override void OnDispose()
        {
            IsInitialized = false;
            base.OnDispose();
        }
    }
}
