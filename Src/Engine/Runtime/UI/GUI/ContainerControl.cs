#nullable disable annotations

using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// A control that owns and routes events to child controls.
    /// </summary>
    public class ContainerControl : Control
    {
        private bool m_IsLayoutLocked;

        protected readonly List<Control> m_Children = new();

        protected bool m_ContainsFocus;

        public bool ClipChildren { get; set; } = true;

        public bool CullChildren { get; set; } = true;

        public ContainerControl()
        {
            m_IsLayoutLocked = true;
        }

        public ContainerControl(float x, float y, float width, float height) : base(x, y, width, height)
        {
            m_IsLayoutLocked = true;
        }

        public ContainerControl(Float2 location, Float2 size) : base(location, size)
        {
            m_IsLayoutLocked = true;
        }

        public ContainerControl(Rectangle bounds) : base(bounds)
        {
            m_IsLayoutLocked = true;
        }

        public bool IsLayoutLocked
        {
            get => m_IsLayoutLocked;
            set => m_IsLayoutLocked = value;
        }

        public List<Control> Children => m_Children;

        public int ChildrenCount => m_Children.Count;

        public bool HasChildren => m_Children.Count > 0;

        public override bool ContainsFocus => m_ContainsFocus;

        public void LockChildrenRecursive()
        {
            m_IsLayoutLocked = true;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is ContainerControl child)
                {
                    child.LockChildrenRecursive();
                }
            }
        }

        public void UnlockChildrenRecursive()
        {
            m_IsLayoutLocked = false;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is ContainerControl child)
                {
                    child.UnlockChildrenRecursive();
                }
            }
        }

        public virtual void RemoveChildren()
        {
            bool wasLocked = m_IsLayoutLocked;
            m_IsLayoutLocked = true;
            while (m_Children.Count > 0)
            {
                m_Children[0].Parent = null;
            }

            m_IsLayoutLocked = wasLocked;
            PerformLayout();
        }

        public virtual void DisposeChildren()
        {
            bool wasLocked = m_IsLayoutLocked;
            m_IsLayoutLocked = true;
            while (m_Children.Count > 0)
            {
                m_Children[m_Children.Count - 1].Dispose();
                m_Children.RemoveAt(m_Children.Count - 1);
            }

            m_IsLayoutLocked = wasLocked;
            PerformLayout();
        }

        public T AddChild<T>() where T : Control, new()
        {
            T child = new T();
            child.Parent = this;
            return child;
        }

        public T AddChild<T>(T control) where T : Control
        {
            if (control == null)
            {
                throw new ArgumentNullException(nameof(control));
            }

            Debug.Assert(!(control.Parent == this && m_Children.Contains(control)),
                "Argument child cannot be added, if current container is already its parent.");

            control.Parent = this;
            return control;
        }

        public void RemoveChild(Control control)
        {
            if (control == null)
            {
                throw new ArgumentNullException(nameof(control));
            }
            // Preserve the native assertion condition, including its inequality.
            Debug.Assert(control.Parent != this,
                "Argument child cannot be removed, if current container is not its parent.");
            control.Parent = null;
        }

        public Control GetChild(int index)
        {
            return m_Children[index];
        }

        public T GetChild<T>() where T : Control
        {
            Type type = typeof(T);
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (type.IsAssignableFrom(m_Children[i].GetType()))
                {
                    return (T)m_Children[i];
                }
            }
            return null;
        }

        public int GetChildIndex(Control child)
        {
            return m_Children.IndexOf(child);
        }

        public void ChangeChildIndex(Control control, int index)
        {
            int currentIndex = m_Children.IndexOf(control);
            if (currentIndex < 0 || currentIndex == index)
            {
                return;
            }

            m_Children.RemoveAt(currentIndex);
            if (index < 0 || index >= m_Children.Count)
            {
                m_Children.Add(control);
            }
            else
            {
                m_Children.Insert(index, control);
            }

            PerformLayout();
        }

        public int GetChildIndexAt(Float2 point)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                if (IntersectsChildContent(m_Children[i], point, out _))
                {
                    return i;
                }
            }

            return -1;
        }

        public Control GetChildAt(Float2 point)
        {
            Control result = null;
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                Control child = m_Children[i];
                if (IntersectsChildContent(child, point, out Float2 childLocation))
                {
                    result = child;
                    break;
                }
            }
            return result;
        }

        public Control GetChildAt(Float2 point, Func<Control, bool> isValid)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                if (isValid(m_Children[i]) && IntersectsChildContent(m_Children[i], point, out _))
                {
                    return m_Children[i];
                }
            }

            return null;
        }

        public Control GetChildAtRecursive(Float2 point)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                Control child = m_Children[i];
                if (!child.Visible || !IntersectsChildContent(child, point, out Float2 childLocation))
                {
                    continue;
                }

                if (child is ContainerControl container)
                {
                    Control nested = container.GetChildAtRecursive(childLocation);
                    if (nested != null && nested.Visible)
                    {
                        child = nested;
                    }
                }
                return child;
            }
            return null;
        }

        public Rectangle GetClientArea()
        {
            return GetDesireClientArea();
        }

        public void SortChildren()
        {
            m_Children.Sort();
            PerformLayout();
        }

        public void SortChildrenRecursive()
        {
            SortChildren();
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is ContainerControl child)
                {
                    child.SortChildrenRecursive();
                }
            }
        }

        public virtual void OnChildResized(Control control)
        {
        }

        public virtual void OnChildrenChanged()
        {
            if (!IsDisposing)
            {
                PerformLayout();
            }
        }

        public override void CacheRootHandle()
        {
            base.CacheRootHandle();
            for (int i = 0; i < m_Children.Count; i++)
            {
                m_Children[i].CacheRootHandle();
            }
        }

        public virtual void AddChildInternal(Control child)
        {
            if (Parent == child)
            {
                throw new ArgumentException(
"Child control cannot be added if it is already a child of this container.");
            }

            Debug.Assert(child != null, "Invalid control.");

            m_Children.Add(child);
            OnChildrenChanged();
        }

        public virtual void RemoveChildInternal(Control child)
        {
            SE.Debug.Assert(child != null, "Invalid control.");
            m_Children.Remove(child);
            OnChildrenChanged();
        }

        public virtual Rectangle GetDesireClientArea()
        {
            return new(Float2.Zero, Size);
        }

        public virtual bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            return child.IntersectsContent(location, out childSpaceLocation);
        }

        #region Navigation

        public override Control OnNavigate(NavDirection direction, Float2 location, Control caller,
            List<Control> visited)
        {
            if (AutoFocus && !ContainsFocus)
            {
                return this;
            }

            if (m_Children.Count != 0 && !visited.Contains(this))
            {
                visited.Add(this);
                Control result = NavigationRaycast(direction, location, visited);
                Float2 rightMostLocation = location;
                if (result == null && (direction == NavDirection.Next || direction == NavDirection.Previous))
                {
                    result = NavigationWrap(direction, location, visited, out rightMostLocation);
                }

                if (result != null)
                {
                    Float2 useLocation = direction == NavDirection.Previous ? rightMostLocation : location;
                    result = result.OnNavigate(direction, result.PointFromParent(useLocation), this, visited);
                    if (result != null)
                    {
                        return result;
                    }
                }
            }
            if ((AutoFocus && !IsFocused) || caller == this)
            {
                return this;
            }

            if (AutoFocus && Visible)
            {
                location = GetNavOrigin(direction);
            }

            return Parent?.OnNavigate(direction, PointToParent(location), caller, visited);
        }

        protected virtual bool CanNavigateChild(Control child)
        {
            return !child.IsFocused && child.Enabled && child.Visible && CanGetAutoFocus(child);
        }

        protected virtual Control NavigationWrap(
            NavDirection direction,
            Float2 location,
            List<Control> visited,
            out Float2 rightMostLocation)
        {
            // The native List parameter is passed by value.
            visited = new List<Control>(visited);
            Control currentChild = RootWindow()!.GetFocusedControl();
            visited.Add(this);
            if (currentChild != null)
            {
                Float2 layoutSize = currentChild.Size;
                Float2 predictedLocation = Float2.Minimum;
                switch (direction)
                {
                    case NavDirection.Next:
                        predictedLocation = new Float2(0, location.Y + layoutSize.Y);
                        break;
                    case NavDirection.Previous:
                        predictedLocation = new Float2(Size.X, location.Y - layoutSize.Y);
                        break;
                }
                if (new Rectangle(Float2.Zero, Size).Contains(predictedLocation))
                {
                    Control result = NavigationRaycast(direction, predictedLocation, visited);
                    if (result != null)
                    {
                        rightMostLocation = predictedLocation;
                        return result;
                    }
                }
            }
            rightMostLocation = location;
            ContainerControl parent = Parent;
            if (parent != null)
            {
                return parent.NavigationWrap(direction, PointToParent(location), visited, out rightMostLocation);
            }
            return null;
        }

        private static bool CanGetAutoFocus(Control control)
        {
            if (control.AutoFocus)
            {
                return true;
            }

            if (control is ContainerControl container)
            {
                for (int i = 0; i < container.m_Children.Count; i++)
                {
                    if (container.CanNavigateChild(container.GetChildAt(new Float2(i))!))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        private Control NavigationRaycast(NavDirection direction, Float2 location, List<Control> visited)
        {
            visited = new List<Control>(visited);
            Float2 dir1 = Float2.Zero, dir2 = Float2.Zero;
            switch (direction)
            {
                case NavDirection.Up:
                    dir1 = dir2 = new Float2(0, -1);
                    break;
                case NavDirection.Down:
                    dir1 = dir2 = new Float2(0, 1);
                    break;
                case NavDirection.Left:
                    dir1 = dir2 = new Float2(-1, 0);
                    break;
                case NavDirection.Right:
                    dir1 = dir2 = new Float2(1, 0);
                    break;
                case NavDirection.Next:
                    dir1 = new Float2(1, 0);
                    dir2 = new Float2(0, 1);
                    break;
                case NavDirection.Previous:
                    dir1 = new Float2(-1, 0);
                    dir2 = new Float2(0, -1);
                    break;
            }
            Control result = null;
            float minDistance = float.MaxValue;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i];
                if (!CanNavigateChild(child) || visited.Contains(child))
                {
                    continue;
                }

                Float2 childDirection = Float2.Normalize(child.Center - location);
                float c1 = Float2.Dot(dir1, childDirection);
                float c2 = Float2.Dot(dir2, childDirection);
                float distance = Rectangle.Distance(child.Bounds, location);
                if (c1 > Mathf.Epsilon && c2 > Mathf.Epsilon && distance < minDistance)
                {
                    minDistance = distance;
                    result = child;
                }
            }
            return result;
        }

        #endregion

        protected void UpdateContainsFocus()
        {
            bool result = base.ContainsFocus;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is ContainerControl container)
                {
                    container.UpdateContainsFocus();
                }

                if (m_Children[i].ContainsFocus)
                {
                    result = true;
                }
            }
            if (result == m_ContainsFocus)
            {
                return;
            }

            m_ContainsFocus = result;
            if (result)
            {
                OnStartContainsFocus();
            }
            else
            {
                OnEndContainsFocus();
            }
        }

        protected void UpdateChildrenBounds()
        {
            for (int i = 0; i < m_Children.Count; i++)
            {
                m_Children[i].UpdateBounds();
            }
        }

        protected virtual void PerformLayoutBeforeChildren()
        {
            UpdateChildrenBounds();
        }

        protected virtual void PerformLayoutAfterChildren()
        {
        }

        #region Control

        public override void OnDestroy()
        {
            if (ContainsFocus)
            {
                Focus();
            }

            if (!m_IsLayoutLocked)
            {
                LockChildrenRecursive();
            }

            base.OnDestroy();
            for (int i = 0; i < m_Children.Count; i++)
            {
                m_Children[i].OnDestroy();
            }

            m_Children.Clear();
        }

        public override bool IsTouchOver()
        {
            if (base.IsTouchOver())
            {
                return true;
            }

            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                if (m_Children[i].IsTouchOver())
                {
                    return true;
                }
            }
            return false;
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i].Enabled)
                {
                    m_Children[i].Update(deltaTime);
                }
            }
        }

        public override void ClearState()
        {
            base.ClearState();
            for (int i = 0; i < m_Children.Count; i++)
            {
                m_Children[i].ClearState();
            }
        }

        public override void Draw()
        {
            DrawSelf();
            if (ClipChildren)
            {
                Render2D.PushClip(GetDesireClientArea());
                DrawChildren();
                Render2D.PopClip();
            }
            else
            {
                DrawChildren();
            }
        }

        public virtual void DrawSelf()
        {
            base.Draw();
        }

        protected virtual void DrawChildren()
        {
            if (CullChildren)
            {
                Rectangle globalClipping = default;
                Matrix3x3 globalTransform = default;
                Render2D.PeekClip(ref globalClipping);
                Render2D.PeekTransform(ref globalTransform);
                for (int i = 0; i < m_Children.Count; i++)
                {
                    Control child = m_Children[i];
                    if (child.Visible)
                    {
                        Matrix3x3 cachedTransform = child.GetCachedTransform();
                        Matrix3x3.Multiply(ref cachedTransform, ref globalTransform,
                            out Matrix3x3 globalChildTransform);
                        Rectangle childGlobalRect = new Rectangle(
                            globalChildTransform.M31, globalChildTransform.M32,
                            child.Width * globalChildTransform.M11, child.Height * globalChildTransform.M22);
                        if (globalClipping.Intersects(childGlobalRect))
                        {
                            Render2D.PushTransform(child.GetCachedTransform());
                            child.Draw();
                            Render2D.PopTransform();
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < m_Children.Count; i++)
                {
                    Control child = m_Children[i];
                    if (child.Visible)
                    {
                        Render2D.PushTransform(child.GetCachedTransform());
                        child.Draw();
                        Render2D.PopTransform();
                    }
                }
            }
        }

        public override void PerformLayout(bool force = false)
        {
            if (m_IsLayoutLocked && !force)
            {
                return;
            }

            bool wasLocked = m_IsLayoutLocked;
            if (!wasLocked)
            {
                LockChildrenRecursive();
            }

            PerformLayoutBeforeChildren();
            for (int i = 0; i < m_Children.Count; i++)
            {
                m_Children[i].PerformLayout(true);
            }

            PerformLayoutAfterChildren();
            if (!wasLocked)
            {
                UnlockChildrenRecursive();
            }
        }

        public override bool RayCast(ref Float2 location, out Control hit)
        {
            if (RayCastChildren(location, out hit))
            {
                return true;
            }

            return base.RayCast(ref location, out hit);
        }

        public bool RayCastChildren(Float2 location, out Control hit)
        {
            Float2 childLocation;
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible)
                {
                    IntersectsChildContent(child, location, out childLocation);
                    if (child.RayCast(ref childLocation, out hit))
                    {
                        return true;
                    }
                }
            }
            hit = null;
            return false;
        }

        public override void OnMouseEnter(Float2 location)
        {
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        // Enter
                        child.OnMouseEnter(childLocation);
                    }
                }
            }

            base.OnMouseEnter(location);
        }

        public override void OnMouseMove(Float2 location)
        {
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child != null && child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.IsMouseOver)
                        {
                            // Move
                            child.OnMouseMove(childLocation);
                        }
                        else
                        {
                            // Enter
                            child.OnMouseEnter(childLocation);
                        }
                    }
                    else if (child.IsMouseOver)
                    {
                        // Leave
                        child.OnMouseLeave();
                    }
                }
            }

            base.OnMouseMove(location);
        }

        public override void OnMouseLeave()
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled && child.IsMouseOver)
                {
                    // Leave
                    child.OnMouseLeave();
                }
            }

            base.OnMouseLeave();
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.OnMouseWheel(childLocation, delta))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.OnMouseDown(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.OnMouseUp(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.OnMouseDoubleClick(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public override bool IsTouchPointerOver(int pointerIndex)
        {
            if (base.IsTouchPointerOver(pointerIndex))
            {
                return true;
            }

            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                if (m_Children[i].IsTouchPointerOver(pointerIndex))
                {
                    return true;
                }
            }

            return false;
        }

        public override void OnTouchEnter(Float2 location, int pointerIndex)
        {
            Float2 childLocation;
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled && !child.IsTouchPointerOver(pointerIndex))
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        child.OnTouchEnter(childLocation, pointerIndex);
                    }
                }
            }

            base.OnTouchEnter(location, pointerIndex);
        }

        public override bool OnTouchDown(Float2 location, int pointerIndex)
        {
            Float2 childLocation;
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (!child.IsTouchPointerOver(pointerIndex))
                        {
                            child.OnTouchEnter(location, pointerIndex);
                        }
                        if (child.OnTouchDown(childLocation, pointerIndex))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnTouchDown(location, pointerIndex);
        }

        public override void OnTouchMove(Float2 location, int pointerIndex)
        {
            Float2 childLocation;
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.IsTouchPointerOver(pointerIndex))
                        {
                            child.OnTouchMove(childLocation, pointerIndex);
                        }
                        else
                        {
                            child.OnTouchEnter(childLocation, pointerIndex);
                        }
                    }
                    else if (child.IsTouchPointerOver(pointerIndex))
                    {
                        child.OnTouchLeave(pointerIndex);
                    }
                }
            }

            base.OnTouchMove(location, pointerIndex);
        }

        public override bool OnTouchUp(Float2 location, int pointerIndex)
        {
            Float2 childLocation;
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled && child.IsTouchPointerOver(pointerIndex))
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.OnTouchUp(childLocation, pointerIndex))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnTouchUp(location, pointerIndex);
        }

        public override void OnTouchLeave(int pointerIndex)
        {
            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled && child.IsTouchPointerOver(pointerIndex))
                {
                    child.OnTouchLeave(pointerIndex);
                }
            }

            base.OnTouchLeave(pointerIndex);
        }

        public override void OnTouchLeave()
        {
            base.OnTouchLeave();
        }

        public override bool OnCharInput(char character)
        {
            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                Control child = m_Children[i];
                if (child.Enabled && child.ContainsFocus)
                {
                    return child.OnCharInput(character);
                }
            }
            return false;
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                Control child = m_Children[i];
                if (child.Enabled && child.ContainsFocus)
                {
                    return child.OnKeyDown(key);
                }
            }
            return false;
        }

        public override void OnKeyUp(KeyboardKeys key)
        {
            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                Control child = m_Children[i];
                if (child.Enabled && child.ContainsFocus)
                {
                    child.OnKeyUp(key);
                    break;
                }
            }
        }

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            // Base
            DragDropEffect result = base.OnDragEnter(ref location, data);

            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        // Enter
                        result = child.OnDragEnter(ref childLocation, data);
                        if (result != DragDropEffect.None)
                        {
                            break;
                        }
                    }
                }
            }

            return result;
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            // Base
            DragDropEffect result = base.OnDragMove(ref location, data);

            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        if (child.IsDragOver())
                        {
                            // Move
                            DragDropEffect tmpResult = child.OnDragMove(ref childLocation, data);
                            if (tmpResult != DragDropEffect.None)
                            {
                                result = tmpResult;
                            }
                        }
                        else
                        {
                            // Enter
                            DragDropEffect tmpResult = child.OnDragEnter(ref childLocation, data);
                            if (tmpResult != DragDropEffect.None)
                            {
                                result = tmpResult;
                            }
                        }
                    }
                    else if (child.IsDragOver())
                    {
                        // Leave
                        child.OnDragLeave();
                    }
                }
            }

            return result;
        }

        public override void OnDragLeave()
        {
            // Base
            base.OnDragLeave();

            // Check all children collisions with mouse and fire events for them
            for (int i = 0; i < m_Children.Count && m_Children.Count > 0; i++)
            {
                Control child = m_Children[i];
                if (child.IsDragOver())
                {
                    // Leave
                    child.OnDragLeave();
                }
            }
        }

        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            // Base
            DragDropEffect result = base.OnDragDrop(ref location, data);
            Float2 childLocation;
            // Check all children collisions with mouse and fire events for them
            for (int i = m_Children.Count - 1; i >= 0 && m_Children.Count > 0; i--)
            {
                Control child = m_Children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out childLocation))
                    {
                        // Enter
                        result = child.OnDragDrop(ref childLocation, data);
                        if (result != DragDropEffect.None)
                        {
                            break;
                        }
                    }
                }
            }

            return result;
        }

        protected override void OnSizeChanged()
        {
            bool wasLocked = m_IsLayoutLocked;
            m_IsLayoutLocked = true;
            base.OnSizeChanged();
            for (int i = 0; i < m_Children.Count; i++)
            {
                m_Children[i].OnParentResized();
            }

            m_IsLayoutLocked = wasLocked;
            PerformLayout();
        }

        #endregion

    }
}
