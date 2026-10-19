using System;
using SE.Editor.GUI.Dialogs;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>Displays a color swatch and opens an interactive color picker when submitted.</summary>
    [HideInEditor]
    public class ColorValueBox : Control
    {
        /// <summary>Reports a selected color. Sliding is true while the picker is being dragged.</summary>
        public delegate void ColorPickerEvent(Color color, bool sliding);
        /// <summary>Reports that the picker has closed.</summary>
        public delegate void ColorPickerClosedEvent();
        /// <summary>Creates a picker owned by the caller, which must invoke pickerClosed on close.</summary>
        public delegate IColorPickerDialog ShowPickColorDialogDelegate(Control targetControl, Color initialValue,
            ColorPickerEvent colorChanged, ColorPickerClosedEvent pickerClosed, bool useDynamicEditing);
        /// <summary>Optional picker factory. If null, the built-in dialog is used.</summary>
        public static ShowPickColorDialogDelegate ShowPickColorDialog;

        private bool m_IsMouseDown;
        private IColorPickerDialog m_CurrentDialog;
        private Color m_Value;
        private bool m_IsSliding;
        public bool UseDynamicEditing = true;
        public event Action ValueChanged;
        public event Action<ColorValueBox> ColorValueChanged;

        public Color Value
        {
            get => m_Value;
            set
            {
                if (m_Value == value)
                {
                    return;
                }
                m_Value = value;
                ValueChanged?.Invoke();
                ColorValueChanged?.Invoke(this);
            }
        }

        public bool IsSliding => m_IsSliding;

        public ColorValueBox() : base(0, 0, 32, 18) { }
        public ColorValueBox(Color value, float x, float y) : base(x, y, 32, 18) => m_Value = value;

        public override void Draw()
        {
            base.Draw();
            Color color = m_Value.ToSRgb();
            bool transparent = color.A < 1;
            Rectangle swatch = new Rectangle(0, 0, transparent ? Width * 0.7f : Width, Height);
            if (transparent)
            {
                Rectangle alpha = new Rectangle(swatch.Right, 0, Width - swatch.Right, Height);
                Render2D.FillRectangle(alpha, Color.White);
                const float checkerSize = 7.9f;
                int columns = Mathf.CeilToInt(alpha.Width / checkerSize);
                int rows = Mathf.CeilToInt(alpha.Height / checkerSize);
                Render2D.PushClip(alpha);
                for (int x = 0; x < columns; x++)
                {
                    for (int y = 0; y < rows; y++)
                    {
                        if ((x + y) % 2 == 0)
                        {
                            Render2D.FillRectangle(new Rectangle(alpha.X + checkerSize * x,
                                                            alpha.Y + checkerSize * y, checkerSize, checkerSize),
                                                                Color.Gray);
                        }
                    }
                }
                Render2D.PopClip();
                Render2D.FillRectangle(alpha, color);
            }
            color.A = 1;
            Render2D.FillRectangle(swatch, color);
            Render2D.DrawRectangle(new Rectangle(0, 0, Width, Height),
                IsMouseOver || IsNavFocused ? Style.Current.BackgroundSelected : Color.Black);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            m_IsMouseDown = true;
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (m_IsMouseDown)
            {
                m_IsMouseDown = false;
                Focus();
                OnSubmit();
            }
            return true;
        }

        public override void OnSubmit()
        {
            base.OnSubmit();
            ShowPickColorDialogDelegate show = ShowPickColorDialog ?? DefaultShowPicker;
            m_CurrentDialog = show(this, m_Value, OnColorChanged, OnPickerClosed, UseDynamicEditing);
        }

        private static IColorPickerDialog DefaultShowPicker(Control target, Color initial,
            ColorPickerEvent changed, ColorPickerClosedEvent closed, bool dynamic)
        {
            ColorPickerDialog dialog = new ColorPickerDialog(initial, changed, closed, dynamic);
            dialog.Show(target);
            return dialog;
        }

        private void OnColorChanged(Color color, bool sliding)
        {
            if (m_IsSliding != sliding)
            {
                m_IsSliding = sliding;
                m_Value = color;
                ValueChanged?.Invoke();
                ColorValueChanged?.Invoke(this);
            }
            else
            {
                Value = color;
            }
        }

        private void OnPickerClosed() => m_CurrentDialog = null;

        public override void OnDestroy()
        {
            m_CurrentDialog?.ClosePicker();
            m_CurrentDialog = null;
            base.OnDestroy();
        }
    }
}
