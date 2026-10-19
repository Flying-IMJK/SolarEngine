using System;
using System.Linq;
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Float3)), DefaultEditor]
    public class Float3Editor : FloatComponentVectorEditor
    {
        protected override int ComponentCount => 3;
        protected override float[] ReadComponents(object value)
        {
            if (value is Double3 d)
            {
                return new[] { (float)d.X, (float)d.Y, (float)d.Z };
            }
            Float3 v = (Float3)value;
            return new[] { v.X, v.Y, v.Z };
        }
        protected override object CreateValue(float[] v) => new Float3(v[0], v[1], v[2]);

        public bool LinkValues = false;
        public virtual bool AllowSlidingForDifferentValues => true;
        private float m_DefaultSlidingSpeed;
        private bool m_SlidingEnded;

        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            Utils.ValueCategory category = Values.GetAttributes().OfType<ValueCategoryAttribute>()
                .FirstOrDefault()?.Category
                ?? Utils.ValueCategory.None;
            foreach (ValueBox<float> component in Components)
            {
                ((FloatValueBox)component).Category = category;
            }
            m_DefaultSlidingSpeed = Components[0].SlideSpeed;
            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                                {
                                    menu.AddSeparator();
                                    ContextMenuButton button = menu.AddButton("Show formatted", item =>
                                    {
                                        foreach (ValueBox<float> component in Components)
                                        {
                                            ((FloatValueBox)component).Category = item.Checked ? category : Utils
                                                .ValueCategory.None;
                                        }
                                    });
                                    button.AutoCheck = true;
                                    button.Checked = ((FloatValueBox)Components[0]).Category != Utils.ValueCategory
                                        .None;
                                };
            }
        }

        protected override void OnSlidingEnd(int index)
        {
            m_SlidingEnded = true;
            ClearToken();
        }

        protected override void OnComponentChanged(int index)
        {
            if (IsSetBlocked || Values == null)
            {
                return;
            }
            float x = Components[0].GetValue();
            float y = Components[1].GetValue();
            float z = Components[2].GetValue();
            Float3 first = (Float3)Values[0]!;
            if (LinkValues)
            {
                float initial = index == 0 ? first.X : index == 1 ? first.Y : first.Z;
                float changed = index == 0 ? x : index == 1 ? y : z;
                float ratio = Mathf.NearEqual(initial, 0) ? 0 : changed / initial;
                if (Mathf.NearEqual(ratio, 0))
                {
                    Components[index].Enabled = false;
                    ratio = 1;
                }
                if (index != 0 && !Mathf.NearEqual(x, 0))
                {
                    x *= ratio;
                }
                if (index != 1 && !Mathf.NearEqual(y, 0))
                {
                    y *= ratio;
                }
                if (index != 2 && !Mathf.NearEqual(z, 0))
                {
                    z *= ratio;
                }
            }

            bool sliding = Components[0].IsSliding || Components[1].IsSliding || Components[2].IsSliding;
            object token = sliding ? this : null;
            Float3 value = new Float3(x, y, z);
            if (HasDifferentValues && Values.Count > 1)
            {
                object[] result = new object[Values.Count];
                if (AllowSlidingForDifferentValues && (sliding || m_SlidingEnded))
                {
                    Float3 average = Float3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        average += (Float3)Values[i]!;
                    }
                    average /= Values.Count;
                    Float3 delta = value - average;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        Float3 current = (Float3)Values[i]!;
                        result[i] = current + (LinkValues ? delta : new Float3(index == 0 ? delta.X : 0,
                            index == 1 ? delta.Y : 0, index == 2 ? delta.Z : 0));
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
                        Float3 current = (Float3)Values[i]!;
                        result[i] = LinkValues
                            ? current + value
                            : new Float3(index == 0 ? x : current.X,
                                index == 1 ? y : current.Y,
                                index == 2 ? z : current.Z);
                    }
                }
                SetValue(result, token);
            }
            else
            {
                SetValue(value, token);
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                Float3 first = (Float3)Values[0]!;
                Float3 average = Float3.Zero;
                bool[] different = new bool[3];
                for (int i = 0; i < Values.Count; i++)
                {
                    Float3 current = (Float3)Values[i]!;
                    average += current;
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
                        Components[i].SetValue(firstValues[i]);
                    }
                    else
                    {
                        if (AllowSlidingForDifferentValues)
                        {
                            Components[i].SetValue(averageValues[i]);
                        }
                        else
                        {
                            Components[i].SlideSpeed = 0;
                        }
                        Components[i].Text = "---";
                    }
                }
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    if (!Mathf.NearEqual(Components[i].SlideSpeed, m_DefaultSlidingSpeed))
                    {
                        Components[i].SlideSpeed = m_DefaultSlidingSpeed;
                    }
                }
            }
        }
    }

    [CustomEditor(typeof(Double3)), DefaultEditor]
    public class Double3Editor : DoubleComponentVectorEditor
    {
        protected override int ComponentCount => 3;
        protected override double[] ReadComponents(object value)
        {
            if (value is Float3 f)
            {
                return new[] { (double)f.X, f.Y, f.Z };
            }
            Double3 v = (Double3)value;
            return new[] { v.X, v.Y, v.Z };
        }
        protected override object CreateValue(double[] v) => new Double3(v[0], v[1], v[2]);

        public virtual bool AllowSlidingForDifferentValues => true;
        private float m_DefaultSlidingSpeed;
        private bool m_SlidingEnded;

        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            Utils.ValueCategory category = Values.GetAttributes().OfType<ValueCategoryAttribute>()
                .FirstOrDefault()?.Category
                ?? Utils.ValueCategory.None;
            foreach (ValueBox<double> component in Components)
            {
                ((DoubleValueBox)component).Category = category;
            }
            m_DefaultSlidingSpeed = Components[0].SlideSpeed;
            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                                {
                                    menu.AddSeparator();
                                    ContextMenuButton button = menu.AddButton("Show formatted", item =>
                                    {
                                        foreach (ValueBox<double> component in Components)
                                        {
                                            ((DoubleValueBox)component).Category = item.Checked ? category : Utils
                                                .ValueCategory.None;
                                        }
                                    });
                                    button.AutoCheck = true;
                                    button.Checked = ((DoubleValueBox)Components[0]).Category != Utils.ValueCategory
                                        .None;
                                };
            }
        }

        protected override void OnSlidingEnd(int index)
        {
            m_SlidingEnded = true;
            ClearToken();
        }

        protected override void OnComponentChanged(int index)
        {
            if (IsSetBlocked || Values == null)
            {
                return;
            }
            double x = Components[0].GetValue(), y = Components[1].GetValue(), z = Components[2].GetValue();
            bool sliding = Components[0].IsSliding || Components[1].IsSliding || Components[2].IsSliding;
            object token = sliding ? this : null;
            Double3 value = new Double3(x, y, z);
            if (HasDifferentValues && Values.Count > 1)
            {
                object[] result = new object[Values.Count];
                if (AllowSlidingForDifferentValues && (sliding || m_SlidingEnded))
                {
                    Double3 average = Double3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        average += (Double3)Values[i]!;
                    }
                    average /= Values.Count;
                    Double3 delta = value - average;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        Double3 current = (Double3)Values[i]!;
                        result[i] = current + new Double3(index == 0 ? delta.X : 0,
                            index == 1 ? delta.Y : 0, index == 2 ? delta.Z : 0);
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
                        Double3 current = (Double3)Values[i]!;
                        result[i] = new Double3(index == 0 ? x : current.X,
                            index == 1 ? y : current.Y, index == 2 ? z : current.Z);
                    }
                }
                SetValue(result, token);
            }
            else
            {
                SetValue(value, token);
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                Double3 first = (Double3)Values[0]!;
                Double3 average = Double3.Zero;
                bool[] different = new bool[3];
                for (int i = 0; i < Values.Count; i++)
                {
                    Double3 current = (Double3)Values[i]!;
                    average += current;
                    if (i == 0)
                    {
                        continue;
                    }
                    if (!Mathd.NearEqual(first.X, current.X))
                    {
                        different[0] = true;
                    }
                    if (!Mathd.NearEqual(first.Y, current.Y))
                    {
                        different[1] = true;
                    }
                    if (!Mathd.NearEqual(first.Z, current.Z))
                    {
                        different[2] = true;
                    }
                }
                average /= Values.Count;
                double[] firstValues = new[] { first.X, first.Y, first.Z };
                double[] averageValues = new[] { average.X, average.Y, average.Z };
                for (int i = 0; i < 3; i++)
                {
                    if (!different[i])
                    {
                        Components[i].SetValue(firstValues[i]);
                    }
                    else
                    {
                        if (AllowSlidingForDifferentValues)
                        {
                            Components[i].SetValue(averageValues[i]);
                        }
                        else
                        {
                            Components[i].SlideSpeed = 0;
                        }
                        Components[i].Text = "---";
                    }
                }
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    if (!Mathf.NearEqual(Components[i].SlideSpeed, m_DefaultSlidingSpeed))
                    {
                        Components[i].SlideSpeed = m_DefaultSlidingSpeed;
                    }
                }
            }
        }
    }
}
