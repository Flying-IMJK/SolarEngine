namespace SE.GUI
{
    /// <summary>Caches this control and its children in a texture until their contents are invalidated.</summary>
    [HideInEditor]
    public class RenderToTextureControl : ContainerControl
    {
        private bool m_Invalid;
        private bool m_IsDuringTextureDraw;
        private bool m_AutoSize = true;
        private GPUTexture m_Texture;
        private RenderTask m_Task;
        private Float2 m_TextureSize;

        /// <summary>The control-owned texture, or null before allocation. It is released with this control.</summary>
        public GPUTexture Texture => m_Texture;
        public bool AutomaticInvalidate { get; set; } = true;

        public bool AutomaticTextureSize
        {
            get => m_AutoSize;
            set
            {
                m_AutoSize = value;
                if (value)
                {
                    TextureSize = Size;
                }
            }
        }

        public Float2 TextureSize
        {
            get => m_TextureSize;
            set
            {
                if (m_TextureSize != value)
                {
                    m_TextureSize = value;
                    Invalidate();
                }
            }
        }

        /// <summary>Requests a redraw on the GUI/render owning thread; never call from a worker.</summary>
        public void Invalidate()
        {
            if (IsDisposing)
            {
                return;
            }
            m_Invalid = true;
            if (m_Task == null)
            {
                // Solar dispatches render work through RenderTask instead of Flax Scripting.Draw.
                m_Task = Object.New<RenderTask>();
                m_Task.Order = -100;
                m_Task.Render += OnDraw;
            }
            m_Task.Enabled = true;
        }

        private void OnDraw(RenderTask task, GPUContext context)
        {
            task.Enabled = false;
            if (!m_Invalid || m_TextureSize.X < 1.0f || m_TextureSize.Y < 1.0f || Width <= 0.0f || Height <= 0.0f)
            {
                return;
            }
            if (m_Texture == null)
            {
                m_Texture = GPUDevice.instance.CreateTexture("RenderToTextureControl");
            }
            if (!m_Texture.IsAllocated || m_Texture.Size != m_TextureSize)
            {
                GPUTextureDescription desc = GPUTextureDescription.New2D((int)m_TextureSize.X, (int)m_TextureSize.Y, PixelFormat.R8G8B8A8_UNorm);
                if (m_Texture.Init(desc))
                {
                    Debug.LogWarning("Failed to allocate RenderToTextureControl texture.");
                    return;
                }
            }
            m_Invalid = false;
            m_IsDuringTextureDraw = true;
            context.Clear(m_Texture.View(), Color.Transparent);
            try
            {
                Render2D.Begin(context, m_Texture);
                Float2 scale = m_TextureSize / Size;
                Render2D.PushTransform(Matrix3x3.Scaling(scale.X, scale.Y, 1.0f));
                try
                {
                    Draw();
                }
                finally
                {
                    Render2D.PopTransform();
                    Render2D.End();
                }
            }
            finally
            {
                m_IsDuringTextureDraw = false;
            }
        }

        public override void Draw()
        {
            if (m_Texture != null && m_Texture.IsAllocated && !m_Invalid && !m_IsDuringTextureDraw)
            {
                Render2D.DrawTexture(m_Texture, new Rectangle(Float2.Zero, Size));
                return;
            }
            base.Draw();
        }

        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();
            if (m_AutoSize)
            {
                TextureSize = Size;
            }
        }

        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);
            if (AutomaticInvalidate)
            {
                Invalidate();
            }
        }

        public override void OnChildrenChanged()
        {
            base.OnChildrenChanged();
            if (AutomaticInvalidate)
            {
                Invalidate();
            }
        }

        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
            if (AutomaticInvalidate)
            {
                Invalidate();
            }
        }

        public override void OnDestroy()
        {
            if (IsDisposing)
            {
                return;
            }
            if (m_Task != null)
            {
                m_Task.Enabled = false;
                m_Task.Render -= OnDraw;
                Object.DestroyNow(m_Task);
                m_Task = null;
            }
            Object.DestroyNow(m_Texture);
            m_Texture = null;
            base.OnDestroy();
        }
    }
}
