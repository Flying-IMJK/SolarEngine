using System;
namespace SE.Editor.GUI
{
    public sealed class RenamePopup : ContextMenu
    {
        private readonly SearchBox m_InputField;

        public RenamePopup(string value, Float2 size, bool isMultiline)
        {
            InitialValue = value;
            m_InputField = new SearchBox(0, 0, size.X, isMultiline: isMultiline);
            m_InputField.SetText(value);
        }

        public event Action<RenamePopup>? Renamed;
        public event Action<RenamePopup>? Closed;
        public Func<RenamePopup, string, bool>? Validate { get; set; }
        public string InitialValue { get; }
        public SearchBox InputField => m_InputField;

        public string Text
        {
            get => m_InputField.Text;
            set => m_InputField.SetText(value);
        }

        public static RenamePopup ShowPopup(SE.GUI.Control control, Rectangle area, string value, bool isMultiline)
        {
            RenamePopup popup = new RenamePopup(value, area.Size, isMultiline);
            popup.Show(control, area.X, area.Y);
            return popup;
        }

        public bool OnKeyDown(ConsoleKey key)
        {
            switch (key)
            {
                case ConsoleKey.Enter:
                    End();
                    return true;
                case ConsoleKey.Escape:
                    Hide();
                    return true;
                default:
                    return false;
            }
        }

        public override void Hide()
        {
            base.Hide();
            Closed?.Invoke(this);
        }

        private void End()
        {
            if (!IsInputValid())
                return;

            Renamed?.Invoke(this);
            Hide();
        }

        private bool IsInputValid()
        {
            return Validate?.Invoke(this, Text) ?? !string.IsNullOrWhiteSpace(Text);
        }
    }
}
