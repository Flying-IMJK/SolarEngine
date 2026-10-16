using SE.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public class SyncPointEditor : CustomEditor
    {
        protected bool m_IsDirty;
        protected object? m_SetValueToken;

        public override void Initialize(LayoutElementsContainer layout) => m_IsDirty = false;

        internal override void RefreshInternal()
        {
            bool dirty = m_IsDirty;
            m_IsDirty = false;
            Refresh();
            if (dirty)
            {
                OnModified();
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            foreach (var child in ChildrenEditors)
            {
                child.RefreshInternal();
            }
        }

        protected virtual void OnModified() => Presenter.OnModified();

        protected override bool OnDirty(CustomEditor editor, object? value, object? token = null)
        {
            m_SetValueToken = token;
            m_IsDirty = true;
            return true;
        }

        protected override void ClearToken() => m_SetValueToken = null;
    }
}
