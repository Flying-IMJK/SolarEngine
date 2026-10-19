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
        private readonly Color m_InitialValue;
        private readonly bool m_UseDynamicEditing;
        private readonly ColorValueBox.ColorPickerEvent m_OnChanged;
        private readonly ColorValueBox.ColorPickerClosedEvent m_OnClosed;
        private readonly ColorSelectorWithSliders m_Selector;
        private readonly Panel m_RgbPanel;
        private readonly Panel m_HsvPanel;
        private readonly FloatValueBox[] m_Rgb = new FloatValueBox[3];
        private readonly FloatValueBox[] m_Hsv = new FloatValueBox[3];
        private readonly FloatValueBox m_Alpha;
        private readonly TextBox m_Hex;
        private Color m_Value;
        private bool m_DisableEvents;
        private bool m_CanPassLastChangeEvent = true;

        public Color SelectedColor
        {
            get => m_Value;
            private set
            {
                if (m_DisableEvents || value == m_Value)
                {
                    return;
                }
                m_DisableEvents = true;
                m_Value = value;
                m_Selector.Color = value;
                m_Rgb[0].SetValue(value.R);
                m_Rgb[1].SetValue(value.G);
                m_Rgb[2].SetValue(value.B);
                m_Alpha.SetValue(value.A);
                Float3 hsv = value.ToHSV();
                m_Hsv[0].SetValue(hsv.X);
                m_Hsv[1].SetValue(hsv.Y * 100);
                m_Hsv[2].SetValue(hsv.Z * 100);
                m_Hex.Text = Color.Min(Color.White, value).ToHexString();
                if (m_UseDynamicEditing)
                {
                    m_OnChanged(value, true);
                }
                m_DisableEvents = false;
            }
        }

        public ColorPickerDialog(Color initialValue, ColorValueBox.ColorPickerEvent onChanged,
            ColorValueBox.ColorPickerClosedEvent onClosed, bool useDynamicEditing)
            : base("Pick a color!")
        {
            m_InitialValue = initialValue;
            m_Value = Color.Transparent;
            m_OnChanged = onChanged;
            m_OnClosed = onClosed;
            m_UseDynamicEditing = useDynamicEditing;

            m_Selector = new ColorSelectorWithSliders(180, 21)
            {
                Parent = this,
                Location = new Float2(6, 6),
            };
            m_Selector.ColorChanged += color => SelectedColor = color;

            float panelX = m_Selector.Right + 16;
            Button rgbTab = new Button
            {
                Text = "RGB",
                Parent = this,
                Bounds = new Rectangle(panelX, 6, 64, 20)
            };
            Button hsvTab = new Button
            {
                Text = "HSV",
                Parent = this,
                Bounds = new Rectangle(panelX + 66, 6, 64, 20)
            };
            m_RgbPanel = new Panel { Parent = this, Bounds = new Rectangle(panelX, 28, 130, 84) };
            m_HsvPanel = new Panel { Parent = this, Bounds = new Rectangle(panelX, 28, 130, 84), Visible = false };
            rgbTab.Clicked += () => { m_RgbPanel.Visible = true; m_HsvPanel.Visible = false; };
            hsvTab.Clicked += () => { m_RgbPanel.Visible = false; m_HsvPanel.Visible = true; };

            string[] rgbNames = { "R", "G", "B" };
            string[] hsvNames = { "H", "S", "V" };
            for (int i = 0; i < 3; i++)
            {
                float y = 3 + i * 26;
                new Label
                {
                    Text = rgbNames[i],
                    Parent = m_RgbPanel,
                    Bounds = new Rectangle(0, y, 18, 18)
                };
                FloatValueBox rgbBox = new FloatValueBox(0, 24, y, 102, 0, float.MaxValue, 0.001f)
                { Parent = m_RgbPanel };
                rgbBox.ValueChanged += OnRGBAChanged;
                m_Rgb[i] = rgbBox;

                new Label
                {
                    Text = hsvNames[i],
                    Parent = m_HsvPanel,
                    Bounds = new Rectangle(0, y, 18, 18)
                };
                FloatValueBox hsvBox = new FloatValueBox(0, 24, y, 102,
                    0, i == 0 ? 360 : i == 1 ? 100 : float.MaxValue, 0.1f)
                { Parent = m_HsvPanel };
                if (i == 0)
                {
                    hsvBox.Category = Utils.ValueCategory.Angle;
                }
                hsvBox.ValueChanged += OnHSVChanged;
                m_Hsv[i] = hsvBox;
            }

            new Label
            {
                Text = "A",
                Parent = this,
                Bounds = new Rectangle(panelX, 126, 18, 18)
            };
            m_Alpha = new FloatValueBox(0, panelX + 24, 126, 102, 0, float.MaxValue, 0.001f)
            { Parent = this };
            m_Alpha.ValueChanged += OnRGBAChanged;
            new Label
            {
                Text = "Hex",
                Parent = this,
                Bounds = new Rectangle(panelX, 154, 24, 18)
            };
            m_Hex = new TextBox(false, panelX + 30, 154, 96) { Parent = this };
            m_Hex.EditEnd += OnHexChanged;

            Button accept = new Button
            {
                Text = "OK",
                Parent = this,
                Bounds = new Rectangle(panelX + 32, 245, 46, 22)
            };
            accept.Clicked += OnSubmit;
            Button cancel = new Button
            {
                Text = "Cancel",
                Parent = this,
                Bounds = new Rectangle(panelX + 82, 245, 54, 22)
            };
            cancel.Clicked += OnCancel;
            m_DialogSize = Size = new Float2(panelX + 142, 275);
            SelectedColor = initialValue;
        }

        private void OnRGBAChanged()
        {
            if (m_DisableEvents)
            {
                return;
            }
            SelectedColor = new Color(m_Rgb[0].GetValue(), m_Rgb[1].GetValue(),
                m_Rgb[2].GetValue(), m_Alpha.GetValue());
        }

        private void OnHSVChanged()
        {
            if (m_DisableEvents)
            {
                return;
            }
            float hue = Mathf.Wrap(m_Hsv[0].GetValue(), 0, 360);
            SelectedColor = Color.FromHSV(hue, m_Hsv[1].GetValue() / 100,
                m_Hsv[2].GetValue() / 100, m_Alpha.GetValue());
        }

        private void OnHexChanged()
        {
            if (!m_DisableEvents && Color.TryParseHex(m_Hex.Text, out Color parsed))
            {
                SelectedColor = parsed;
            }
        }

        public override void Draw()
        {
            base.Draw();
            Rectangle preview = new Rectangle(m_Selector.X, 206, m_Selector.Width, 50);
            Render2D.FillRectangle(preview, Color.White);
            Render2D.FillRectangle(preview, m_Value.ToSRgb());
            Render2D.DrawRectangle(preview, Color.Black);
        }

        public override void OnSubmit()
        {
            if (m_DisableEvents)
            {
                return;
            }
            m_DisableEvents = true;
            if (m_Value != m_InitialValue)
            {
                m_OnChanged(m_Value, false);
            }
            base.OnSubmit();
        }

        public override void OnCancel()
        {
            if (m_DisableEvents)
            {
                return;
            }
            m_DisableEvents = true;
            if (m_UseDynamicEditing && m_InitialValue != m_Value && m_CanPassLastChangeEvent)
            {
                m_CanPassLastChangeEvent = false;
                m_OnChanged(m_InitialValue, false);
            }
            base.OnCancel();
        }

        public override void OnDestroy()
        {
            m_OnClosed();
            base.OnDestroy();
        }

        public void ClosePicker() => OnCancel();
    }
}
