using System;
using SE.GUI;

namespace SE.Editor.GUI.Dialogs
{
    public interface IColorPickerDialog
    {
        void ClosePicker();
    }

    public sealed class ColorPickerDialog : Dialog, IColorPickerDialog
    {
        private readonly Color _initialValue;
        private readonly bool _useDynamicEditing;
        private readonly ColorValueBox.ColorPickerEvent _onChanged;
        private readonly ColorValueBox.ColorPickerClosedEvent _onClosed;
        private readonly ColorSelectorWithSliders _selector;
        private readonly Panel _rgbPanel;
        private readonly Panel _hsvPanel;
        private readonly FloatValueBox[] _rgb = new FloatValueBox[3];
        private readonly FloatValueBox[] _hsv = new FloatValueBox[3];
        private readonly FloatValueBox _alpha;
        private readonly TextBox _hex;
        private Color _value;
        private bool _disableEvents;
        private bool _canPassLastChangeEvent = true;

        public Color SelectedColor
        {
            get => _value;
            private set
            {
                if (_disableEvents || value == _value) return;
                _disableEvents = true;
                _value = value;
                _selector.Color = value;
                _rgb[0].SetValue(value.R);
                _rgb[1].SetValue(value.G);
                _rgb[2].SetValue(value.B);
                _alpha.SetValue(value.A);
                Float3 hsv = value.ToHSV();
                _hsv[0].SetValue(hsv.X);
                _hsv[1].SetValue(hsv.Y * 100);
                _hsv[2].SetValue(hsv.Z * 100);
                _hex.Text = Color.Min(Color.White, value).ToHexString();
                if (_useDynamicEditing) _onChanged(value, true);
                _disableEvents = false;
            }
        }

        public ColorPickerDialog(Color initialValue, ColorValueBox.ColorPickerEvent onChanged,
            ColorValueBox.ColorPickerClosedEvent onClosed, bool useDynamicEditing)
            : base("Pick a color!")
        {
            _initialValue = initialValue;
            _value = Color.Transparent;
            _onChanged = onChanged;
            _onClosed = onClosed;
            _useDynamicEditing = useDynamicEditing;

            _selector = new ColorSelectorWithSliders(180, 21)
            {
                Parent = this,
                Location = new Float2(6, 6),
            };
            _selector.ColorChanged += color => SelectedColor = color;

            float panelX = _selector.Right + 16;
            var rgbTab = new Button { Text = "RGB", Parent = this,
                Bounds = new Rectangle(panelX, 6, 64, 20) };
            var hsvTab = new Button { Text = "HSV", Parent = this,
                Bounds = new Rectangle(panelX + 66, 6, 64, 20) };
            _rgbPanel = new Panel { Parent = this, Bounds = new Rectangle(panelX, 28, 130, 84) };
            _hsvPanel = new Panel { Parent = this, Bounds = new Rectangle(panelX, 28, 130, 84), Visible = false };
            rgbTab.Clicked += () => { _rgbPanel.Visible = true; _hsvPanel.Visible = false; };
            hsvTab.Clicked += () => { _rgbPanel.Visible = false; _hsvPanel.Visible = true; };

            string[] rgbNames = { "R", "G", "B" };
            string[] hsvNames = { "H", "S", "V" };
            for (int i = 0; i < 3; i++)
            {
                float y = 3 + i * 26;
                new Label { Text = rgbNames[i], Parent = _rgbPanel,
                    Bounds = new Rectangle(0, y, 18, 18) };
                var rgbBox = new FloatValueBox(0, 24, y, 102, 0, float.MaxValue, 0.001f)
                { Parent = _rgbPanel };
                rgbBox.ValueChanged += OnRGBAChanged;
                _rgb[i] = rgbBox;

                new Label { Text = hsvNames[i], Parent = _hsvPanel,
                    Bounds = new Rectangle(0, y, 18, 18) };
                var hsvBox = new FloatValueBox(0, 24, y, 102,
                    0, i == 0 ? 360 : i == 1 ? 100 : float.MaxValue, 0.1f)
                { Parent = _hsvPanel };
                if (i == 0) hsvBox.Category = Utils.ValueCategory.Angle;
                hsvBox.ValueChanged += OnHSVChanged;
                _hsv[i] = hsvBox;
            }

            new Label { Text = "A", Parent = this,
                Bounds = new Rectangle(panelX, 126, 18, 18) };
            _alpha = new FloatValueBox(0, panelX + 24, 126, 102, 0, float.MaxValue, 0.001f)
            { Parent = this };
            _alpha.ValueChanged += OnRGBAChanged;
            new Label { Text = "Hex", Parent = this,
                Bounds = new Rectangle(panelX, 154, 24, 18) };
            _hex = new TextBox(false, panelX + 30, 154, 96) { Parent = this };
            _hex.EditEnd += OnHexChanged;

            var accept = new Button { Text = "OK", Parent = this,
                Bounds = new Rectangle(panelX + 32, 245, 46, 22) };
            accept.Clicked += OnSubmit;
            var cancel = new Button { Text = "Cancel", Parent = this,
                Bounds = new Rectangle(panelX + 82, 245, 54, 22) };
            cancel.Clicked += OnCancel;
            m_DialogSize = Size = new Float2(panelX + 142, 275);
            SelectedColor = initialValue;
        }

        private void OnRGBAChanged()
        {
            if (_disableEvents) return;
            SelectedColor = new Color(_rgb[0].GetValue(), _rgb[1].GetValue(),
                _rgb[2].GetValue(), _alpha.GetValue());
        }

        private void OnHSVChanged()
        {
            if (_disableEvents) return;
            float hue = Mathf.Wrap(_hsv[0].GetValue(), 0, 360);
            SelectedColor = Color.FromHSV(hue, _hsv[1].GetValue() / 100,
                _hsv[2].GetValue() / 100, _alpha.GetValue());
        }

        private void OnHexChanged()
        {
            if (!_disableEvents && Color.TryParseHex(_hex.Text, out Color parsed))
                SelectedColor = parsed;
        }

        public override void Draw()
        {
            base.Draw();
            var preview = new Rectangle(_selector.X, 206, _selector.Width, 50);
            Render2D.FillRectangle(preview, Color.White);
            Render2D.FillRectangle(preview, _value.ToSRgb());
            Render2D.DrawRectangle(preview, Color.Black);
        }

        public override void OnSubmit()
        {
            if (_disableEvents) return;
            _disableEvents = true;
            if (_value != _initialValue) _onChanged(_value, false);
            base.OnSubmit();
        }

        public override void OnCancel()
        {
            if (_disableEvents) return;
            _disableEvents = true;
            if (_useDynamicEditing && _initialValue != _value && _canPassLastChangeEvent)
            {
                _canPassLastChangeEvent = false;
                _onChanged(_initialValue, false);
            }
            base.OnCancel();
        }

        public override void OnDestroy()
        {
            _onClosed();
            base.OnDestroy();
        }

        public void ClosePicker() => OnCancel();
    }
}
