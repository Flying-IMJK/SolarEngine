using System;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Version)), DefaultEditor]
    public class VersionEditor : CustomEditor
    {
        protected IntValueBox Major = null!;
        protected IntValueBox Minor = null!;
        protected IntValueBox Build = null!;
        protected IntValueBox Revision = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            UniformGridPanel grid = new UniformGridPanel
            {
                Height = TextBox.DefaultHeight,
                SlotsHorizontally = 4,
                SlotsVertically = 1,
                ClipChildren = false,
                Parent = layout.ContainerControl,
            };
            Major = CreateBox(grid, 0, 100000000);
            Minor = CreateBox(grid, 0, 100000000);
            Build = CreateBox(grid, -1, 100000000);
            Revision = CreateBox(grid, -1, 100000000);
        }

        private IntValueBox CreateBox(UniformGridPanel grid, int min, int max)
        {
            IntValueBox box = new IntValueBox { Parent = grid };
            box.SetLimits(min, max);
            box.ValueChanged += OnValueChanged;
            box.SlidingEnd += ClearToken;
            return box;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
            {
                return;
            }
            bool sliding = Major.IsSliding || Minor.IsSliding || Build.IsSliding || Revision.IsSliding;
            int major = Major.GetValue();
            int minor = Minor.GetValue();
            int build = Build.GetValue();
            int revision = Revision.GetValue();
            Version value = build > -1
                ? revision > 0 ? new Version(major, minor, build, revision) : new Version(major, minor, build)
                : new Version(major, minor);
            SetValue(value, sliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues && Values[0] is Version value)
            {
                Major.SetValue(value.Major);
                Minor.SetValue(value.Minor);
                Build.SetValue(value.Build);
                Revision.SetValue(value.Revision);
            }
        }
    }
}
