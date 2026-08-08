using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// A control that owns and lays out child controls.
    /// </summary>
    public class ContainerControl : Control
    {
        private readonly List<Control> m_Children = new();
        private bool m_ContainsFocus;

        /// <summary>
        /// The layout locking flag.
        /// </summary>
        protected bool m_IsLayoutLocked;

        public ContainerControl()
        {
            m_IsLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl(float x, float y, float width, float height)  : base(x, y, width, height)
        {
            m_IsLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl(Float2 location, Float2 size) : base(location, size)
        {
            m_IsLayoutLocked = true;
        }

        public ContainerControl(Rectangle bounds) : base(bounds)
        {
        }

        /// <summary>
        /// Gets the children in visual order, from back to front.
        /// </summary>
        public IReadOnlyList<Control> Children => m_Children;

        /// <summary>
        /// Gets amount of the children controls.
        /// </summary>
        public int ChildrenCount => m_Children.Count;

        /// <summary>
        /// Checks if container has any child controls.
        /// </summary>
        public bool HasChildren => m_Children.Count > 0;

        /// <summary>
        /// Gets a value indicating whether the control, or one of its child controls, currently has the input focus.
        /// </summary>
        public override bool ContainsFocus => m_ContainsFocus;

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
            m_Children.Add(control);
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
            if (!m_Children.Remove(control))
                return false;

            control.SetParentCore(null);
            OnChildRemoved(control);
            PerformLayout();
            return true;
        }

        /// <summary>
        /// Gets child control at given index.
        /// </summary>
        public Control GetChild(int index)
        {
            return m_Children[index];
        }

        /// <summary>
        /// Searches for a child control of a specific type.
        /// </summary>
        public T? GetChild<T>() where T : Control
        {
            Type type = typeof(T);
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (type.IsAssignableFrom(m_Children[i].GetType()))
                    return (T)m_Children[i];
            }

            return null;
        }

        /// <summary>
        /// Gets zero-based index in the list of control children.
        /// </summary>
        public int GetChildIndex(Control child)
        {
            return m_Children.IndexOf(child);
        }

        /// <summary>
        /// Unlocks all the child controls layout and itself.
        /// </summary>
        public void UnlockChildrenRecursive()
        {
            m_IsLayoutLocked = false;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is ContainerControl child)
                    child.UnlockChildrenRecursive();
            }
        }

        /// <summary>
        /// Unlinks all the child controls.
        /// </summary>
        public virtual void RemoveChildren()
        {
            bool wasLayoutLocked = m_IsLayoutLocked;
            m_IsLayoutLocked = true;

            // Delete children
            while (m_Children.Count > 0)
            {
                m_Children[0].Parent = null;
            }

            m_IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Disposes every child control.
        /// </summary>
        public void DisposeChildren()
        {
            while (m_Children.Count > 0)
            {
                m_Children[m_Children.Count - 1].Dispose();
            }
        }

        /// <summary>
        /// Finds the top-most child that contains the given root logical-coordinate point.
        /// </summary>
        public virtual Control? HitTest(Float2 location)
        {
            if (!ContainsPoint(location))
                return null;

            for (int index = m_Children.Count - 1; index >= 0; index--)
            {
                Control child = m_Children[index];
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

            for (int index = 0; index < m_Children.Count; index++)
                m_Children[index].PerformLayout(force);
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (!VisibleInHierarchy || IsDisposed)
                return;

            var children = m_Children.ToArray();
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
            var children = m_Children.ToArray();
            for (int index = 0; index < children.Length; index++)
            {
                if (ReferenceEquals(children[index].Parent, this))
                    children[index].Draw();
            }
        }

        public override void ClearState()
        {
            base.ClearState();
            for (int index = 0; index < m_Children.Count; index++)
                m_Children[index].ClearState();
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
            return GetChildIndex(control);
        }

        internal void SetChildIndex(Control control, int index)
        {
            int currentIndex = m_Children.IndexOf(control);
            if (currentIndex < 0)
                throw new InvalidOperationException("The control is not a child of this container.");
            if ((uint)index >= (uint)m_Children.Count)
                throw new ArgumentOutOfRangeException(nameof(index));
            if (currentIndex == index)
                return;

            m_Children.RemoveAt(currentIndex);
            m_Children.Insert(index, control);
            PerformLayout();
        }

        internal override void SetRootCore(RootControl? root)
        {
            base.SetRootCore(root);
            for (int index = 0; index < m_Children.Count; index++)
                m_Children[index].SetRootCore(root);
            UpdateContainsFocus();
        }

        internal void UpdateContainsFocusUpwards()
        {
            ContainerControl? control = this;
            while (control != null)
            {
                control.UpdateContainsFocus();
                control = control.Parent;
            }
        }

        private void UpdateContainsFocus()
        {
            bool result = base.ContainsFocus;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i].ContainsFocus)
                {
                    result = true;
                    break;
                }
            }

            if (result == m_ContainsFocus)
                return;

            m_ContainsFocus = result;
            if (result)
                OnStartContainsFocus();
            else
                OnEndContainsFocus();
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
