using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// A control that owns and lays out child controls.
    /// </summary>
    public class ContainerControl : Control
    {
        private readonly List<Control> _children = new();

        public ContainerControl()
        {
        }

        public ContainerControl(Rectangle bounds)
            : base(bounds)
        {
        }

        /// <summary>
        /// Gets the children in visual order, from back to front.
        /// </summary>
        public IReadOnlyList<Control> Children => _children;

        /// <summary>
        /// Gets the logical offset applied to every immediate child. Scrolling containers override this.
        /// </summary>
        internal virtual Float2 ChildOffset => Float2.Zero;

        /// <summary>
        /// Adds a child and returns it for fluent tree construction.
        /// </summary>
        public T AddChild<T>(T control) where T : Control
        {
            ArgumentNullException.ThrowIfNull(control);
            if (ReferenceEquals(control, this))
                throw new InvalidOperationException("A control cannot parent itself.");
            if (IsDescendantOf(control))
                throw new InvalidOperationException("A control cannot be parented to one of its descendants.");
            if (ReferenceEquals(control.Parent, this))
                return control;

            control.Parent?.RemoveChild(control);
            _children.Add(control);
            control.SetParentCore(this);
            OnChildAdded(control);
            PerformLayout();
            return control;
        }

        /// <summary>
        /// Detaches a child without disposing it.
        /// </summary>
        public bool RemoveChild(Control control)
        {
            ArgumentNullException.ThrowIfNull(control);
            if (!_children.Remove(control))
                return false;

            control.SetParentCore(null);
            OnChildRemoved(control);
            PerformLayout();
            return true;
        }

        /// <summary>
        /// Disposes every child control.
        /// </summary>
        public void DisposeChildren()
        {
            while (_children.Count > 0)
                _children[_children.Count - 1].Dispose();
        }

        /// <summary>
        /// Finds the top-most child that contains the given root logical-coordinate point.
        /// </summary>
        public virtual Control? HitTest(Float2 location)
        {
            if (!ContainsPoint(location))
                return null;

            for (int index = _children.Count - 1; index >= 0; index--)
            {
                Control child = _children[index];
                if (!child.VisibleInHierarchy || !child.EnabledInHierarchy)
                    continue;

                if (child is ContainerControl container)
                {
                    Control? nested = container.HitTest(location);
                    if (nested != null)
                        return nested;
                }
                else if (child.ContainsPoint(location))
                {
                    return child;
                }
            }

            return this;
        }

        public override void PerformLayout(bool force = false)
        {
            base.PerformLayout(force);
            OnLayoutChildren();

            for (int index = 0; index < _children.Count; index++)
                _children[index].PerformLayout(force);
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (!VisibleInHierarchy || IsDisposed)
                return;

            var children = _children.ToArray();
            for (int index = 0; index < children.Length; index++)
            {
                if (ReferenceEquals(children[index].Parent, this))
                    children[index].Update(deltaTime);
            }
        }

        public override void Draw()
        {
            DrawSelf();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            DrawChildren();
        }

        /// <summary>
        /// Draws this container without any child controls.
        /// </summary>
        protected void DrawSelf()
        {
            base.Draw();
        }

        /// <summary>
        /// Draws immediate child controls in visual order.
        /// </summary>
        protected void DrawChildren()
        {
            var children = _children.ToArray();
            for (int index = 0; index < children.Length; index++)
            {
                if (ReferenceEquals(children[index].Parent, this))
                    children[index].Draw();
            }
        }

        public override void ClearState()
        {
            base.ClearState();
            for (int index = 0; index < _children.Count; index++)
                _children[index].ClearState();
        }

        protected override void OnDispose()
        {
            DisposeChildren();
            base.OnDispose();
        }

        protected virtual void OnChildAdded(Control control)
        {
            _ = control;
        }

        protected virtual void OnChildRemoved(Control control)
        {
            _ = control;
        }

        /// <summary>
        /// Lays out immediate child controls. Derived panels override this method.
        /// </summary>
        protected virtual void OnLayoutChildren()
        {
        }

        internal int IndexOf(Control control)
        {
            return _children.IndexOf(control);
        }

        internal void SetChildIndex(Control control, int index)
        {
            int currentIndex = _children.IndexOf(control);
            if (currentIndex < 0)
                throw new InvalidOperationException("The control is not a child of this container.");
            if ((uint)index >= (uint)_children.Count)
                throw new ArgumentOutOfRangeException(nameof(index));
            if (currentIndex == index)
                return;

            _children.RemoveAt(currentIndex);
            _children.Insert(index, control);
            PerformLayout();
        }

        internal override void SetRootCore(RootControl? root)
        {
            base.SetRootCore(root);
            for (int index = 0; index < _children.Count; index++)
                _children[index].SetRootCore(root);
        }

        private bool IsDescendantOf(Control control)
        {
            for (ContainerControl? current = this; current != null; current = current.Parent)
            {
                if (ReferenceEquals(current, control))
                    return true;
            }

            return false;
        }
    }
}
