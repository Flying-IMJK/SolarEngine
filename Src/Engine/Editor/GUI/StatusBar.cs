// Managed editor GUI feature implementation.
using SE.GUI;
namespace SE.Editor.GUI
{
    public sealed class StatusBar : ContainerControl
    {
        public const float DefaultHeight = 22.0f;

        private readonly Label _label;
        private string _text = string.Empty;

        public StatusBar(float y, float width)
            : base(new Rectangle(0, y, width, DefaultHeight))
        {
            _label = new Label(new Rectangle(4, 0, width - 20, DefaultHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
            };
            SetBounds(0, y, width, DefaultHeight);
            AddChild(_label);
        }

        public string Text
        {
            get => _text;
            set
            {
                _text = value;
                _label.Text = value;
            }
        }

        protected override void OnLayoutChildren()
        {
            _label.SetBounds(4, 0, Width - 20, Height);
        }
    }
}
