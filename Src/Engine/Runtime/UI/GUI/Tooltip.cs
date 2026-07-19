using System;

namespace SE.GUI
{
    /// <summary>
    /// A delayed, non-interactive tooltip overlay hosted by a managed root control.
    /// </summary>
    public sealed class Tooltip : ContainerControl
    {
        private const float HorizontalPadding = 6.0f;
        private const float VerticalPadding = 4.0f;
        private const float CharacterWidth = 7.0f;
        private const float LineHeight = 18.0f;

        private readonly Label _textLabel;
        private float _timeToPopupLeft;
        private Control? _lastTarget;
        private Control? _showTarget;

        public Tooltip()
        {
            AutoFocus = false;
            Enabled = false;
            IsScrollable = false;
            Visible = false;
            BackgroundColor = Style.Current.BackgroundNormal;
            _textLabel = new Label
            {
                AutoFocus = false,
                IsScrollable = false,
                TextWrapping = TextWrapping.WrapWords,
                TextColor = Style.Current.Foreground,
            };
            AddChild(_textLabel);
        }

        /// <summary>
        /// Gets or sets the pointer dwell time, in seconds, before the tooltip opens.
        /// </summary>
        public float TimeToShow { get; set; } = 0.3f;

        /// <summary>
        /// Gets or sets the maximum tooltip width in logical pixels.
        /// </summary>
        public float MaxWidth { get; set; } = 500.0f;

        /// <summary>
        /// Gets whether the tooltip is currently attached to a root and visible.
        /// </summary>
        public bool IsShowing => _showTarget != null && Visible;

        /// <summary>
        /// Shows the tooltip for the supplied target using its <see cref="Control.TooltipText"/>.
        /// </summary>
        public void Show(Control target, Float2 location, Rectangle targetArea)
        {
            ArgumentNullException.ThrowIfNull(target);
            _ = targetArea;
            if (string.IsNullOrWhiteSpace(target.TooltipText) || target.Root == null)
            {
                Hide();
                return;
            }

            RootControl root = target.Root;
            _textLabel.Text = target.TooltipText;
            float availableWidth = MathF.Max(32.0f, MaxWidth);
            float rawWidth = target.TooltipText.Length * CharacterWidth + HorizontalPadding * 2.0f;
            float width = MathF.Min(availableWidth, MathF.Max(32.0f, rawWidth));
            int lineCount = Math.Max(1, (int)MathF.Ceiling(rawWidth / width));
            float height = lineCount * LineHeight + VerticalPadding * 2.0f;
            Float2 desiredPosition = target.PointToRoot(location) + new Float2(12.0f, 18.0f);
            float x = Math.Clamp(desiredPosition.X, 0.0f, MathF.Max(0.0f, root.Width - width));
            float y = Math.Clamp(desiredPosition.Y, 0.0f, MathF.Max(0.0f, root.Height - height));

            SetBounds(x, y, width, height);
            _textLabel.SetBounds(HorizontalPadding, VerticalPadding, width - HorizontalPadding * 2.0f, height - VerticalPadding * 2.0f);
            if (!ReferenceEquals(Parent, root))
                root.AddChild(this);
            Visible = true;
            _showTarget = target;
        }

        /// <summary>
        /// Hides the tooltip and detaches it from its root.
        /// </summary>
        public void Hide()
        {
            _showTarget = null;
            Visible = false;
            Parent?.RemoveChild(this);
        }

        /// <summary>
        /// Starts delayed tooltip tracking for the supplied control.
        /// </summary>
        public void OnMouseEnterControl(Control target)
        {
            ArgumentNullException.ThrowIfNull(target);
            if (ReferenceEquals(target, _lastTarget))
                return;

            Hide();
            _lastTarget = target;
            _timeToPopupLeft = MathF.Max(0.0f, TimeToShow);
        }

        /// <summary>
        /// Updates delayed tooltip tracking while the pointer remains over the supplied control.
        /// </summary>
        public void OnMouseOverControl(Control target, float deltaTime)
        {
            ArgumentNullException.ThrowIfNull(target);
            if (!ReferenceEquals(target, _lastTarget))
                OnMouseEnterControl(target);
            UpdateTooltip(deltaTime);
        }

        /// <summary>
        /// Stops tracking a control after its pointer leaves.
        /// </summary>
        public void OnMouseLeaveControl(Control target)
        {
            if (!ReferenceEquals(target, _lastTarget) && !ReferenceEquals(target, _showTarget))
                return;

            _lastTarget = null;
            _timeToPopupLeft = 0.0f;
            Hide();
        }

        public override void Update(float deltaTime)
        {
            UpdateTooltip(deltaTime);
            base.Update(deltaTime);
        }

        private void UpdateTooltip(float deltaTime)
        {
            if (_showTarget != null)
            {
                if (!_showTarget.IsMouseOver || !_showTarget.VisibleInHierarchy)
                    Hide();
                return;
            }

            if (_lastTarget == null || !_lastTarget.IsMouseOver || string.IsNullOrWhiteSpace(_lastTarget.TooltipText))
                return;

            _timeToPopupLeft -= MathF.Max(0.0f, deltaTime);
            if (_timeToPopupLeft > 0.0f)
                return;

            Show(_lastTarget, _lastTarget.PointFromRoot(_lastTarget.Root?.MousePosition ?? Float2.Zero), _lastTarget.Bounds);
        }
    }
}
