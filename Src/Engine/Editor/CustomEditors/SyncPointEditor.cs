using SE.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public class SyncPointEditor : CustomEditor
    {
        protected bool _isDirty;
        protected object? _setValueToken;

        public override void Initialize(LayoutElementsContainer layout) => _isDirty = false;

        internal override void RefreshInternal()
        {
            bool dirty = _isDirty;
            _isDirty = false;
            Refresh();
            if (dirty)
                OnModified();
        }

        public override void Refresh()
        {
            base.Refresh();
            foreach (var child in ChildrenEditors)
                child.RefreshInternal();
        }

        protected virtual void OnModified() => Presenter.OnModified();

        protected override bool OnDirty(CustomEditor editor, object? value, object? token = null)
        {
            _setValueToken = token;
            _isDirty = true;
            return true;
        }

        protected override void ClearToken() => _setValueToken = null;
    }
}
