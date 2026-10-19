
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Asset)), DefaultEditor]
    public class AssetRefEditor : CustomEditor
    {
        public AssetPicker Picker;

        private bool m_IsRefreshing;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            Picker = layout.Custom<AssetPicker>().CustomControl;

            if (Values.Type == typeof(object) && Values[0] != null)
            {
                Picker.Validator.AssetType = Values[0].GetType();
            }
            else
            {
                Picker.Validator.AssetType = Values.Type;
            }


            Picker.SelectedItemChanged += OnSelectedItemChanged;
        }

        private void OnSelectedItemChanged()
        {
            if (!m_IsRefreshing)
            {
                SetValue(Picker.Validator.SelectedAsset);
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            if (Picker == null)
            {
                return;
            }

            Picker.DifferentValues = HasDifferentValues;

            if (HasDifferentValues)
            {
                return;
            }
            m_IsRefreshing = true;

            try
            {
                Picker.Validator.SelectedAsset = Values[0] as Asset;
            }
            finally
            {
                m_IsRefreshing = false;
            }
        }

        protected override void Deinitialize()
        {
            if (Picker != null)
            {
                Picker.SelectedItemChanged -= OnSelectedItemChanged;
            }

            Picker = null;
            base.Deinitialize();
        }
    }
}
