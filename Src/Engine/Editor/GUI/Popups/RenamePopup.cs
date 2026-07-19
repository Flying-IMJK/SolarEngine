using System;
namespace SE.Editor.GUI
{
    public sealed class RenamePopup : ContextMenu
    {
        private readonly SearchBox _inputField;

        public RenamePopup(string value, GuiSize size, bool isMultiline)
        {
            InitialValue = value;
            _inputField = new SearchBox(0, 0, size.Width, isMultiline: isMultiline);
            _inputField.SetText(value);
        }

        public event Action<RenamePopup>? Renamed;
        public event Action<RenamePopup>? Closed;
        public Func<RenamePopup, string, bool>? Validate { get; set; }
        public string InitialValue { get; }
        public SearchBox InputField => _inputField;

        public string Text
        {
            get => _inputField.Text;
            set => _inputField.SetText(value);
        }

        public static RenamePopup ShowPopup(SE.GUI.Control control, GuiRect area, string value, bool isMultiline)
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
