// Managed editor GUI feature implementation.
using SE.GUI;
namespace SE.Editor.GUI
{
    public sealed class StatusBar : ContainerControl
    {
        public const float DefaultHeight = 22.0f;

        private readonly Label m_Label;
        private string m_Text = string.Empty;

        public StatusBar(float y, float width)
            : base(new Rectangle(0, y, width, DefaultHeight))
        {
            AutoFocus = false;
            m_Label = new Label(new Rectangle(4, 0, width - 20, DefaultHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
            };
            SetBounds(0, y, width, DefaultHeight);
            AddChild(m_Label);
        }

        public string Text
        {
            get => m_Text;
            set
            {
                m_Text = value;
                m_Label.Text = value;
            }
        }

        public Color StatusColor
        {
            get => BackgroundColor;
            set => BackgroundColor = value;
        }

        public Color TextColor
        {
            get => m_Label.TextColor;
            set => m_Label.TextColor = value;
        }

        protected override void OnLayoutChildren()
        {
            m_Label.SetBounds(4, 0, Width - 20, Height);
        }
    }
}
