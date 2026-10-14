using System;
using System.Text.Json;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Quaternion)), DefaultEditor]
    public class QuaternionEditor : CustomEditor
    {
        private readonly FloatValueBox[] _boxes = new FloatValueBox[3];
        private Float3 _cachedAngles = Float3.Zero;
        private object? _cachedToken;
        private int _changedComponent;
        private float _defaultSlidingSpeed;
        private bool _slidingEnded;

        public virtual bool AllowSlidingForDifferentValues => true;
        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = new UniformGridPanel
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
                var box = new FloatValueBox(0) { Parent = grid };
                box.Category = Utils.ValueCategory.Angle;
                box.ValueChanged += () => OnComponentChanged(index);
                box.SlidingEnd += () => { _slidingEnded = true; ClearToken(); };
                _boxes[i] = box;
            }
            _defaultSlidingSpeed = _boxes[0].SlideSpeed;
            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                {
                    menu.AddSeparator();
                    var angles = ((Quaternion)Values[0]!).EulerAngles;
                    menu.AddButton("Copy Euler", () => Clipboard.Text = JsonSerializer.Serialize(angles)).TooltipText =
                        "Copy the Euler Angles in Degrees";
                };
            }
        }

        private void OnComponentChanged(int index)
        {
            if (IsSetBlocked) return;
            _changedComponent = index;
            OnValueChanged();
        }

        private void OnValueChanged()
        {
            bool sliding = _boxes[0].IsSliding || _boxes[1].IsSliding || _boxes[2].IsSliding;
            object? token = sliding ? this : null;
            bool useCached = sliding && ReferenceEquals(token, _cachedToken);
            if (HasDifferentValues && Values.Count > 1)
            {
                float x = Mathf.UnwindDegrees(_boxes[0].GetValue());
                float y = Mathf.UnwindDegrees(_boxes[1].GetValue());
                float z = Mathf.UnwindDegrees(_boxes[2].GetValue());
                var value = new Float3(x, y, z);
                _cachedToken = token;
                var results = new object[Values.Count];
                if (AllowSlidingForDifferentValues && (sliding || _slidingEnded))
                {
                    Float3 average = Float3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                        average += ((Quaternion)Values[0]!).EulerAngles;
                    average /= Values.Count;
                    Float3 delta = value - average;
                    Quaternion.Euler(_changedComponent == 0 ? delta.X : 0,
                        _changedComponent == 1 ? delta.Y : 0,
                        _changedComponent == 2 ? delta.Z : 0, out Quaternion rotation);
                    for (int i = 0; i < Values.Count; i++)
                        results[i] = (Quaternion)Values[i]! * rotation;
                    if (_slidingEnded) _slidingEnded = false;
                }
                else
                {
                    for (int i = 0; i < Values.Count; i++)
                    {
                        var current = (Quaternion)Values[i]!;
                        var angles = current.EulerAngles;
                        Quaternion.Euler(_changedComponent == 0 ? x : angles.X,
                            _changedComponent == 1 ? y : angles.Y,
                            _changedComponent == 2 ? z : angles.Z, out Quaternion rotation);
                        results[i] = current * rotation;
                    }
                }
                SetValue(results, token);
            }
            else
            {
                float x = useCached && !_boxes[0].IsSliding ? _cachedAngles.X : _boxes[0].GetValue();
                float y = useCached && !_boxes[1].IsSliding ? _cachedAngles.Y : _boxes[1].GetValue();
                float z = useCached && !_boxes[2].IsSliding ? _cachedAngles.Z : _boxes[2].GetValue();
                x = Mathf.UnwindDegrees(x);
                y = Mathf.UnwindDegrees(y);
                z = Mathf.UnwindDegrees(z);
                if (!useCached) _cachedAngles = new Float3(x, y, z);
                _cachedToken = token;
                Quaternion.Euler(x, y, z, out Quaternion value);
                SetValue(value, token);
            }
        }

        protected override void ClearToken()
        {
            _cachedToken = null;
            base.ClearToken();
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                var first = ((Quaternion)Values[0]!).EulerAngles;
                Float3 average = Float3.Zero;
                bool[] different = new bool[3];
                for (int i = 0; i < Values.Count; i++)
                {
                    var current = (Quaternion)Values[i]!;
                    var angles = current.EulerAngles;
                    average += angles;
                    if (i == 0) continue;
                    if (!Mathf.NearEqual(first.X, current.X)) different[0] = true;
                    if (!Mathf.NearEqual(first.Y, current.Y)) different[1] = true;
                    if (!Mathf.NearEqual(first.Z, current.Z)) different[2] = true;
                }
                average /= Values.Count;
                var firstValues = new[] { first.X, first.Y, first.Z };
                var averageValues = new[] { average.X, average.Y, average.Z };
                for (int i = 0; i < 3; i++)
                {
                    if (!different[i]) _boxes[i].SetValue(firstValues[i]);
                    else
                    {
                        if (AllowSlidingForDifferentValues) _boxes[i].SetValue(averageValues[i]);
                        else _boxes[i].SlideSpeed = 0;
                        _boxes[i].Text = "---";
                    }
                }
            }
            else
            {
                var angles = ((Quaternion)Values[0]!).EulerAngles;
                _boxes[0].SetValue(angles.X);
                _boxes[1].SetValue(angles.Y);
                _boxes[2].SetValue(angles.Z);
                for (int i = 0; i < 3; i++)
                    if (!Mathf.NearEqual(_boxes[i].SlideSpeed, _defaultSlidingSpeed))
                        _boxes[i].SlideSpeed = _defaultSlidingSpeed;
            }
        }
    }
}
