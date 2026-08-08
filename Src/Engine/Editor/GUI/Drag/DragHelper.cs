using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public abstract class DragHelperBase
    {
        public abstract bool HasValidDrag { get; }
        public abstract DragDropEffect Effect { get; }
        public abstract bool OnDragEnter(DragData data);
        public abstract void OnDragLeave();
        public abstract void OnDragDrop();
        public abstract void OnDragDrop(DragEventArgs dragEventArgs);
    }

    public abstract class DragHelper<T, TEventArgs> : DragHelperBase
        where TEventArgs : DragEventArgs
    {
        private readonly Func<T, bool> m_ValidateFunction;

        protected DragHelper(Func<T, bool> validateFunction)
        {
            m_ValidateFunction = validateFunction;
        }

        public List<T> Objects { get; } = new List<T>();
        public override bool HasValidDrag => Objects.Count > 0;
        public override DragDropEffect Effect => HasValidDrag ? DragDropEffect.Move : DragDropEffect.None;

        public abstract DragData ToDragData(T item);
        public abstract DragData ToDragData(IEnumerable<T> items);
        public abstract IEnumerable<T> FromDragData(DragData data);

        public virtual void DragDrop(TEventArgs? dragEventArgs, IReadOnlyList<T> items)
        {
        }

        public void InvalidDrag()
        {
            Objects.Clear();
        }

        public override bool OnDragEnter(DragData data)
        {
            Objects.Clear();
            foreach (T item in FromDragData(data))
            {
                if (m_ValidateFunction(item))
                    Objects.Add(item);
            }

            return HasValidDrag;
        }

        public override void OnDragLeave()
        {
            Objects.Clear();
        }

        public override void OnDragDrop()
        {
            if (HasValidDrag)
                DragDrop(null, Objects);
            Objects.Clear();
        }

        public override void OnDragDrop(DragEventArgs dragEventArgs)
        {
            if (HasValidDrag)
                DragDrop(dragEventArgs as TEventArgs, Objects);
            Objects.Clear();
        }
    }
}
