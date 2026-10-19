using System;
using System.Text.Json;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Quaternion)), DefaultEditor]
    public class QuaternionEditor : CustomEditor
    {
        private readonly FloatValueBox[] m_Boxes = new FloatValueBox[3];
        private Float3 m_CachedAngles = Float3.Zero;
        private object m_CachedToken;
        private int m_ChangedComponent;
        private float m_DefaultSlidingSpeed;
        private bool m_SlidingEnded;

        public virtual bool AllowSlidingForDifferentValues => true;
        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            UniformGridPanel grid = new UniformGridPanel
            {
                Parent = layout.ContainerControl,
                ClipChildren = false,
                Height = TextBox.DefaultHeight,
                SlotsHorizontally = 3,
                SlotsVertically = 1,
            };
            for (int i = 0; i < 3; i++)
            {
                int index = i;
                FloatValueBox box = new FloatValueBox(0) { Parent = grid };
                box.Category = Utils.ValueCategory.Angle;
                box.ValueChanged += () => OnComponentChanged(index);
                box.SlidingEnd += () => { m_SlidingEnded = true; ClearToken(); };
                m_Boxes[i] = box;
            }
            m_DefaultSlidingSpeed = m_Boxes[0].SlideSpeed;
            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                {
                    menu.AddSeparator();
                    Float3 angles = ((Quaternion)Values[0]!).EulerAngles;
                    menu.AddButton("Copy Euler", () => Clipboard.Text = JsonSerializer.Serialize(angles)).TooltipText =
                        "Copy the Euler Angles in Degrees";
                };
            }
        }

        private void OnComponentChanged(int index)
        {
            if (IsSetBlocked)
            {
                return;
            }
            m_ChangedComponent = index;
            OnValueChanged();
        }

        private void OnValueChanged()
        {
            bool sliding = m_Boxes[0].IsSliding || m_Boxes[1].IsSliding || m_Boxes[2].IsSliding;
            object token = sliding ? this : null;
            bool useCached = sliding && ReferenceEquals(token, m_CachedToken);
            if (HasDifferentValues && Values.Count > 1)
            {
                float x = Mathf.UnwindDegrees(m_Boxes[0].GetValue());
                float y = Mathf.UnwindDegrees(m_Boxes[1].GetValue());
                float z = Mathf.UnwindDegrees(m_Boxes[2].GetValue());
                Float3 value = new Float3(x, y, z);
                m_CachedToken = token;
                object[] results = new object[Values.Count];
                if (AllowSlidingForDifferentValues && (sliding || m_SlidingEnded))
                {
                    Float3 average = Float3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        average += ((Quaternion)Values[0]!).EulerAngles;
                    }
                    average /= Values.Count;
                    Float3 delta = value - average;
                    Quaternion.Euler(m_ChangedComponent == 0 ? delta.X : 0,
                        m_ChangedComponent == 1 ? delta.Y : 0,
                        m_ChangedComponent == 2 ? delta.Z : 0, out Quaternion rotation);
                    for (int i = 0; i < Values.Count; i++)
                    {
                        results[i] = (Quaternion)Values[i]! * rotation;
                    }
                    if (m_SlidingEnded)
                    {
                        m_SlidingEnded = false;
                    }
                }
                else
                {
                    for (int i = 0; i < Values.Count; i++)
                    {
                        Quaternion current = (Quaternion)Values[i]!;
                        Float3 angles = current.EulerAngles;
                        Quaternion.Euler(m_ChangedComponent == 0 ? x : angles.X,
                            m_ChangedComponent == 1 ? y : angles.Y,
                            m_ChangedComponent == 2 ? z : angles.Z, out Quaternion rotation);
                        results[i] = current * rotation;
                    }
                }
                SetValue(results, token);
            }
            else
            {
                float x = useCached && !m_Boxes[0].IsSliding ? m_CachedAngles.X : m_Boxes[0].GetValue();
                float y = useCached && !m_Boxes[1].IsSliding ? m_CachedAngles.Y : m_Boxes[1].GetValue();
                float z = useCached && !m_Boxes[2].IsSliding ? m_CachedAngles.Z : m_Boxes[2].GetValue();
                x = Mathf.UnwindDegrees(x);
                y = Mathf.UnwindDegrees(y);
                z = Mathf.UnwindDegrees(z);
                if (!useCached)
                {
                    m_CachedAngles = new Float3(x, y, z);
                }
                m_CachedToken = token;
                Quaternion.Euler(x, y, z, out Quaternion value);
                SetValue(value, token);
            }
        }

        protected override void ClearToken()
        {
            m_CachedToken = null;
            base.ClearToken();
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                Float3 first = ((Quaternion)Values[0]!).EulerAngles;
                Float3 average = Float3.Zero;
                bool[] different = new bool[3];
                for (int i = 0; i < Values.Count; i++)
                {
                    Quaternion current = (Quaternion)Values[i]!;
                    Float3 angles = current.EulerAngles;
                    average += angles;
                    if (i == 0)
                    {
                        continue;
                    }
                    if (!Mathf.NearEqual(first.X, current.X))
                    {
                        different[0] = true;
                    }
                    if (!Mathf.NearEqual(first.Y, current.Y))
                    {
                        different[1] = true;
                    }
                    if (!Mathf.NearEqual(first.Z, current.Z))
                    {
                        different[2] = true;
                    }
                }
                average /= Values.Count;
                float[] firstValues = new[] { first.X, first.Y, first.Z };
                float[] averageValues = new[] { average.X, average.Y, average.Z };
                for (int i = 0; i < 3; i++)
                {
                    if (!different[i])
                    {
                        m_Boxes[i].SetValue(firstValues[i]);
                    }
                    else
                    {
                        if (AllowSlidingForDifferentValues)
                        {
                            m_Boxes[i].SetValue(averageValues[i]);
                        }
                        else
                        {
                            m_Boxes[i].SlideSpeed = 0;
                        }
                        m_Boxes[i].Text = "---";
                    }
                }
            }
            else
            {
                Float3 angles = ((Quaternion)Values[0]!).EulerAngles;
                m_Boxes[0].SetValue(angles.X);
                m_Boxes[1].SetValue(angles.Y);
                m_Boxes[2].SetValue(angles.Z);
                for (int i = 0; i < 3; i++)
                {
                    if (!Mathf.NearEqual(m_Boxes[i].SlideSpeed, m_DefaultSlidingSpeed))
                    {
                        m_Boxes[i].SlideSpeed = m_DefaultSlidingSpeed;
                    }
                }
            }
        }
    }
}
