namespace SE.GUI
{
    /// <summary>
    /// Helper control class for other panels.
    /// </summary>
    public class PanelWithMargins : ContainerControl
    {
        /// <summary>
        /// The panel area margins.
        /// </summary>
        protected Margin _margin = new SE.Margin(2.0f);

        /// <summary>
        /// The space between the items.
        /// </summary>
        protected float _spacing = 2.0f;

        /// <summary>
        /// The auto size flag.
        /// </summary>
        protected bool _autoSize = true;

        /// <summary>
        /// The control offset.
        /// </summary>
        protected Float2 _offset = new Float2(0.0f);

        /// <summary>
        /// Initializes a new instance of the <see cref="PanelWithMargins"/> class.
        /// </summary>
        protected PanelWithMargins()
            : base(0.0f, 0.0f, 64.0f, 64.0f)
        {
            AutoFocus = false;
        }

        /// <summary>
        /// Gets or sets the left margin.
        /// </summary>
        public float LeftMargin
        {
            get => _margin.Right;
            set
            {
                if (!Mathf.WithinEpsilon((double)_margin.Left, value, Mathf.Epsilon))
                {
                    _margin.Left = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the right margin.
        /// </summary>
        public float RightMargin
        {
            get => _margin.Left;
            set
            {
                if (!Mathf.WithinEpsilon((double)_margin.Right, value, Mathf.Epsilon))
                {
                    _margin.Right = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the top margin.
        /// </summary>
        public float TopMargin
        {
            get => _margin.Top;
            set
            {
                if (!Mathf.WithinEpsilon((double)_margin.Top, value, Mathf.Epsilon))
                {
                    _margin.Top = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the bottom margin.
        /// </summary>
        public float BottomMargin
        {
            get => _margin.Bottom;
            set
            {
                if (!Mathf.WithinEpsilon((double)_margin.Bottom, value, Mathf.Epsilon))
                {
                    _margin.Bottom = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the child controls spacing.
        /// </summary>
        // [Tooltip("The child controls spacing (the space between controls).")]
        public float Spacing
        {
            get => _spacing;
            set
            {
                if (!Mathf.WithinEpsilon((double)_spacing, value, Mathf.Epsilon))
                {
                    _spacing = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the child controls offset (additive).
        /// </summary>
        // [Tooltip("The child controls offset (additive).")]
        public Float2 Offset
        {
            get => _offset;
            set
            {
                if (!(Mathf.WithinEpsilon((double)_offset.X, value.X, Mathf.Epsilon) &&
                      Mathf.WithinEpsilon((double)_offset.Y, value.Y, Mathf.Epsilon)))
                {
                    _offset = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the value indicating whenever the panel size will be based on a children dimensions.
        /// </summary>
        // [Tooltip("If checked, the panel size will be based on a children dimensions.")]
        public bool AutoSize
        {
            get => _autoSize;
            set
            {
                if (_autoSize != value)
                {
                    _autoSize = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the panel area margin.
        /// </summary>
        // [Tooltip("The panel area margin.")]
        public Margin Margin
        {
            get => _margin;
            set
            {
                if (!(Mathf.WithinEpsilon((double)value.Left, _margin.Left, Mathf.Epsilon) &&
                      Mathf.WithinEpsilon((double)value.Right, _margin.Right, Mathf.Epsilon) &&
                      Mathf.WithinEpsilon((double)value.Top, _margin.Top, Mathf.Epsilon) &&
                      Mathf.WithinEpsilon((double)value.Bottom, _margin.Bottom, Mathf.Epsilon)))
                {
                    _margin = value;
                    PerformLayout();
                }
            }
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        public override bool ContainsPoint(Float2 location, bool precise)
        {
            if (precise && BackgroundColor.A <= 0.0f) // Go through transparency
            {
                return false;
            }

            return base.ContainsPoint(location, precise);
        }
    }
}
