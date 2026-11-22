
using System;
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

    internal sealed class SoftAssetRefEditor<T> : CustomEditor where T : Asset
    {
        private AssetPicker m_Picker;
        private bool m_IsRefreshing;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
            {
                return;
            }

            m_Picker = layout.Custom<AssetPicker>().CustomControl;
            m_Picker.Validator.UseAssetID = true;
            m_Picker.Validator.AssetType = typeof(T);
            m_Picker.SelectedItemChanged += OnSelectedItemChanged;
            Editor.Instance.AssetDatabase.WorkspaceModified += m_Picker.Validator.RefreshSelectedAssetID;
        }

        private void OnSelectedItemChanged()
        {
            if (m_IsRefreshing)
            {
                return;
            }

            Guid id = m_Picker.Validator.SelectedID;
            if (HasDifferentValues || ((SoftAssetRef<T>)Values[0]).ID != id)
            {
                SetValue(new SoftAssetRef<T>(id));
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            if (m_Picker == null)
            {
                return;
            }

            bool differentValues = HasDifferentValues;
            m_Picker.DifferentValues = differentValues;
            Guid id = differentValues ? Guid.Empty : ((SoftAssetRef<T>)Values[0]).ID;
            if (m_Picker.Validator.SelectedID == id)
            {
                return;
            }

            m_IsRefreshing = true;
            try
            {
                m_Picker.Validator.SelectedID = id;
            }
            finally
            {
                m_IsRefreshing = false;
            }
        }

        protected override void Deinitialize()
        {
            if (m_Picker != null)
            {
                Editor.Instance.AssetDatabase.WorkspaceModified -= m_Picker.Validator.RefreshSelectedAssetID;
                m_Picker.SelectedItemChanged -= OnSelectedItemChanged;
                m_Picker = null;
            }
            base.Deinitialize();
        }
    }
}
