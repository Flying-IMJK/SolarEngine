using System.Collections.Generic;

namespace SE.Editor.GUI
{
    public sealed class DragHandlers : List<DragHelperBase>
    {
        public DragDropEffect OnDragEnter(DragData data)
        {
            DragDropEffect result = DragDropEffect.None;
            foreach (DragHelperBase helper in this)
            {
                if (helper.OnDragEnter(data) && result == DragDropEffect.None)
                    result = helper.Effect;
            }

            return result;
        }

        public void OnDragLeave()
        {
            foreach (DragHelperBase helper in this)
            {
                helper.OnDragLeave();
            }
        }

        public void OnDragDrop(DragEventArgs dragEventArgs)
        {
            foreach (DragHelperBase helper in this)
            {
                helper.OnDragDrop(dragEventArgs);
            }
        }

        public bool HasValidDrag
        {
            get
            {
                return WithValidDrag() != null;
            }
        }

        public DragHelperBase? WithValidDrag()
        {
            foreach (DragHelperBase helper in this)
            {
                if (helper.HasValidDrag)
                    return helper;
            }

            return null;
        }

        public DragDropEffect Effect => WithValidDrag()?.Effect ?? DragDropEffect.None;
    }
}
