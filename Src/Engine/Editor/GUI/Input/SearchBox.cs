using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class SearchBox : TextBox
    {
        public SearchBox(float x = 0, float y = 0, float width = 120, bool isMultiline = false)
            : base(new Rectangle(x, y, width, 22), isMultiline)
        {
            PlaceholderText = "Search...";
            ClearSearchButton = new Button(new Rectangle(width - 18, 4, 14, 14), "x");
            ClearSearchButton.Visible = false;
            ClearSearchButton.Clicked += _ => Clear();

            AddChild(ClearSearchButton);
        }

        public event Action<SearchBox>? TextChanged;

        public Button ClearSearchButton { get; }
        public string WatermarkText
        {
            get => PlaceholderText;
            set => PlaceholderText = value;
        }

        public void SetText(string text)
        {
            Text = text;
        }

        public void Clear()
        {
            SetText(string.Empty);
        }

        public override bool OnKeyDown(int key)
        {
            if (key == 46) // Delete
            {
                Clear();
                return true;
            }

            return base.OnKeyDown(key);
        }

        protected override void OnTextChanged()
        {
            base.OnTextChanged();
            ClearSearchButton.Visible = !string.IsNullOrEmpty(Text);
            ClearSearchButton.SetBounds(Width - 18, 4, 14, 14);
            TextChanged?.Invoke(this);
        }
    }
}
