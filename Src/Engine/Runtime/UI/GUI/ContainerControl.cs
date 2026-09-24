using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// A control that owns and routes events to child controls.
    /// </summary>
    public class ContainerControl : Control
    {
        private readonly List<Control> m_Children = new();
        private bool m_ContainsFocus;
        protected bool m_IsLayoutLocked;
        public bool ClipChildren { get; set; } = true;
        public bool CullChildren { get; set; } = true;
        public bool IsLayoutLocked { get => m_IsLayoutLocked; set => m_IsLayoutLocked = value; }

        public ContainerControl() { m_IsLayoutLocked = true; }
        public ContainerControl(float x, float y, float width, float height) : base(x, y, width, height) { m_IsLayoutLocked = true; }
        public ContainerControl(Float2 location, Float2 size) : base(location, size) { m_IsLayoutLocked = true; }
        public ContainerControl(Rectangle bounds) : base(bounds) { m_IsLayoutLocked = true; }

        public IReadOnlyList<Control> Children => m_Children;
        public int ChildrenCount => m_Children.Count;
        public bool HasChildren => m_Children.Count > 0;
        public override bool ContainsFocus => m_ContainsFocus;
        internal virtual Float2 ChildOffset => Float2.Zero;

        public T AddChild<T>(T control) where T : Control { control.Parent = this; return control; }
        public bool RemoveChild(Control control) { if (!ReferenceEquals(control.Parent, this)) return false; control.Parent = null; return true; }
        public Control GetChild(int index) => m_Children[index];
        public T? GetChild<T>() where T : Control
        {
            Type type = typeof(T);
            for (int i = 0; i < m_Children.Count; i++) if (type.IsAssignableFrom(m_Children[i].GetType())) return (T)m_Children[i];
            return null;
        }
        public int GetChildIndex(Control child) => m_Children.IndexOf(child);
        public void ChangeChildIndex(Control child, int newIndex) => SetChildIndex(child, newIndex);

        public int GetChildIndexAt(Float2 point)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--) if (IntersectsChildContent(m_Children[i], point, out _)) return i;
            return -1;
        }
        public Control? GetChildAt(Float2 point) => GetChildAt(point, static _ => true);
        public Control? GetChildAt(Float2 point, Func<Control, bool> isValid)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--) if (isValid(m_Children[i]) && IntersectsChildContent(m_Children[i], point, out _)) return m_Children[i];
            return null;
        }
        public Control? GetChildAtRecursive(Float2 point)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                Control child = m_Children[i];
                if (!child.Visible || !IntersectsChildContent(child, point, out Float2 childLocation)) continue;
                if (child is ContainerControl container)
                {
                    Control? nested = container.GetChildAtRecursive(childLocation);
                    if (nested != null && nested.Visible) child = nested;
                }
                return child;
            }
            return null;
        }

        public Rectangle GetClientArea() => GetDesireClientArea();
        public virtual Rectangle GetDesireClientArea() => new(Float2.Zero, Size);
        public virtual bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            childSpaceLocation = child.PointFromParent(location);
            return child.ContainsPoint(childSpaceLocation);
        }
        public virtual void SortChildren() { m_Children.Sort(static (left, right) => left.Compare(right)); PerformLayout(); }
        public void SortChildrenRecursive() { SortChildren(); for (int i = 0; i < m_Children.Count; i++) if (m_Children[i] is ContainerControl child) child.SortChildrenRecursive(); }
        public void LockChildrenRecursive() { m_IsLayoutLocked = true; for (int i = 0; i < m_Children.Count; i++) if (m_Children[i] is ContainerControl child) child.LockChildrenRecursive(); }
        public void UnlockChildrenRecursive() { m_IsLayoutLocked = false; for (int i = 0; i < m_Children.Count; i++) if (m_Children[i] is ContainerControl child) child.UnlockChildrenRecursive(); }
        public virtual void RemoveChildren() { bool wasLocked = m_IsLayoutLocked; m_IsLayoutLocked = true; while (m_Children.Count > 0) m_Children[0].Parent = null; m_IsLayoutLocked = wasLocked; PerformLayout(); }
        // Dispose removes each child through the managed parent seam; no snapshot or duplicate removal is used.
        public virtual void DisposeChildren() { bool wasLocked = m_IsLayoutLocked; m_IsLayoutLocked = true; while (m_Children.Count > 0) m_Children[m_Children.Count - 1].Dispose(); m_IsLayoutLocked = wasLocked; PerformLayout(); }

        public virtual Control? HitTest(Float2 location)
        {
            if (!ContainsPoint(location)) return null;
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                Control child = m_Children[i];
                if (!child.Visible || !child.Enabled || !IntersectsChildContent(child, location, out Float2 childLocation)) continue;
                if (child is ContainerControl container) { Control? nested = container.HitTest(childLocation); if (nested != null) return nested; }
                else return child;
            }
            return this;
        }

        public override void PerformLayout(bool force = false)
        {
            if (m_IsLayoutLocked && !force) return;
            bool wasLocked = m_IsLayoutLocked;
            if (!wasLocked) LockChildrenRecursive();
            PerformLayoutBeforeChildren();
            for (int i = 0; i < m_Children.Count; i++) m_Children[i].PerformLayout(true);
            PerformLayoutAfterChildren();
            if (!wasLocked) UnlockChildrenRecursive();
        }
        protected virtual void PerformLayoutBeforeChildren() { UpdateChildrenBounds(); OnLayoutChildren(); }
        protected virtual void PerformLayoutAfterChildren() { }
        protected virtual void OnLayoutChildren() { }
        protected virtual void OnChildAdded(Control control) { }
        protected virtual void OnChildRemoved(Control control) { }
        public virtual void OnChildResized(Control control) { }
        protected virtual void OnChildrenChanged()
        {
            if (!IsDisposing)
            {
                PerformLayout();
            }
        }
        protected virtual void UpdateChildrenBounds()
        {
            for (int i = 0; i < m_Children.Count; i++)
                m_Children[i].UpdateBounds();
        }
        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            if (!sizeChanged) return;
            bool wasLocked = m_IsLayoutLocked;
            m_IsLayoutLocked = true;
            for (int i = 0; i < m_Children.Count; i++) m_Children[i].OnParentResized();
            m_IsLayoutLocked = wasLocked;
            PerformLayout();
        }

        public override void Update(float deltaTime) { base.Update(deltaTime); for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].Enabled) m_Children[i].Update(deltaTime); }
        public override void Draw() { DrawSelf(); if (ClipChildren) { Render2D.PushClip(GetDesireClientArea()); DrawChildren(); Render2D.PopClip(); } else DrawChildren(); }
        protected virtual void DrawSelf() => base.Draw();
        // Use the live child list, matching native mutation behavior during traversal.
        protected virtual void DrawChildren()
        {
            Rectangle globalClip = default; Matrix3x3 globalTransform = default; Render2D.PeekClip(ref globalClip); Render2D.PeekTransform(ref globalTransform);
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i]; if (!child.Visible) continue;
                if (CullChildren) { Matrix3x3 childCachedTransform = child.CachedTransform; Matrix3x3.Multiply(ref childCachedTransform, ref globalTransform, out Matrix3x3 childTransform); Rectangle childRect = new(childTransform.M31, childTransform.M32, child.Width * childTransform.M11, child.Height * childTransform.M22); if (!globalClip.Intersects(childRect)) continue; }
                Render2D.PushTransform(child.CachedTransform); child.Draw(); Render2D.PopTransform();
            }
        }

        public override bool RayCast(ref Float2 location, out Control? hit) { if (RayCastChildren(location, out hit)) return true; return base.RayCast(ref location, out hit); }
        public bool RayCastChildren(Float2 location, out Control? hit)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--) if (m_Children[i].Visible) { IntersectsChildContent(m_Children[i], location, out Float2 childLocation); if (m_Children[i].RayCast(ref childLocation, out hit)) return true; }
            hit = null; return false;
        }
        public override void OnMouseEnter(Float2 location) { for (int i = m_Children.Count - 1; i >= 0; i--) if (m_Children[i].Visible && m_Children[i].Enabled && IntersectsChildContent(m_Children[i], location, out Float2 childLocation)) m_Children[i].OnMouseEnter(childLocation); base.OnMouseEnter(location); }
        public override void OnMouseMove(Float2 location)
        {
            for (int i = m_Children.Count - 1; i >= 0; i--)
            {
                Control child = m_Children[i]; if (!child.Visible || !child.Enabled) continue;
                if (IntersectsChildContent(child, location, out Float2 childLocation)) { if (child.IsMouseOver) child.OnMouseMove(childLocation); else child.OnMouseEnter(childLocation); }
                else if (child.IsMouseOver) child.OnMouseLeave();
            }
            base.OnMouseMove(location);
        }
        public override void OnMouseLeave() { for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].Visible && m_Children[i].Enabled && m_Children[i].IsMouseOver) m_Children[i].OnMouseLeave(); base.OnMouseLeave(); }
        private bool RouteMouse(Float2 location, Func<Control, Float2, bool> handler) { for (int i = m_Children.Count - 1; i >= 0; i--) if (m_Children[i].Visible && m_Children[i].Enabled && IntersectsChildContent(m_Children[i], location, out Float2 childLocation) && handler(m_Children[i], childLocation)) return true; return false; }
        public override bool OnMouseWheel(Float2 location, float delta) => RouteMouse(location, (child, childLocation) => child.OnMouseWheel(childLocation, delta));
        public override bool OnMouseDown(Float2 location, MouseButton button) => RouteMouse(location, (child, childLocation) => child.OnMouseDown(childLocation, button));
        public override bool OnMouseUp(Float2 location, MouseButton button) => RouteMouse(location, (child, childLocation) => child.OnMouseUp(childLocation, button));
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button) => RouteMouse(location, (child, childLocation) => child.OnMouseDoubleClick(childLocation, button));

        public override bool IsTouchOver() { if (base.IsTouchOver()) return true; for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].IsTouchOver()) return true; return false; }
        public override bool IsTouchPointerOver(int pointerIndex) { if (base.IsTouchPointerOver(pointerIndex)) return true; for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].IsTouchPointerOver(pointerIndex)) return true; return false; }
        public override void OnTouchEnter(Float2 location, int pointerIndex) { for (int i = m_Children.Count - 1; i >= 0; i--) if (m_Children[i].Visible && m_Children[i].Enabled && !m_Children[i].IsTouchPointerOver(pointerIndex) && IntersectsChildContent(m_Children[i], location, out Float2 childLocation)) m_Children[i].OnTouchEnter(childLocation, pointerIndex); base.OnTouchEnter(location, pointerIndex); }
        // The first enter on touch-down intentionally receives the parent-space location, as in native.
        public override bool OnTouchDown(Float2 location, int pointerIndex) { for (int i = m_Children.Count - 1; i >= 0; i--) { Control child = m_Children[i]; if (child.Visible && child.Enabled && IntersectsChildContent(child, location, out Float2 childLocation)) { if (!child.IsTouchPointerOver(pointerIndex)) child.OnTouchEnter(location, pointerIndex); if (child.OnTouchDown(childLocation, pointerIndex)) return true; } } return base.OnTouchDown(location, pointerIndex); }
        public override void OnTouchMove(Float2 location, int pointerIndex) { for (int i = m_Children.Count - 1; i >= 0; i--) { Control child = m_Children[i]; if (!child.Visible || !child.Enabled) continue; if (IntersectsChildContent(child, location, out Float2 childLocation)) { if (child.IsTouchPointerOver(pointerIndex)) child.OnTouchMove(childLocation, pointerIndex); else child.OnTouchEnter(childLocation, pointerIndex); } else if (child.IsTouchPointerOver(pointerIndex)) child.OnTouchLeave(pointerIndex); } base.OnTouchMove(location, pointerIndex); }
        public override bool OnTouchUp(Float2 location, int pointerIndex) { for (int i = m_Children.Count - 1; i >= 0; i--) { Control child = m_Children[i]; if (child.Visible && child.Enabled && child.IsTouchPointerOver(pointerIndex) && IntersectsChildContent(child, location, out Float2 childLocation) && child.OnTouchUp(childLocation, pointerIndex)) return true; } return base.OnTouchUp(location, pointerIndex); }
        public override void OnTouchLeave(int pointerIndex) { for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].Visible && m_Children[i].Enabled && m_Children[i].IsTouchPointerOver(pointerIndex)) m_Children[i].OnTouchLeave(pointerIndex); base.OnTouchLeave(pointerIndex); }
        public override void OnTouchLeave() => base.OnTouchLeave();

        public override bool OnCharInput(char character) { for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].Enabled && m_Children[i].ContainsFocus) return m_Children[i].OnCharInput(character); return false; }
        public override bool OnKeyDown(KeyboardKeys key) { for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].Enabled && m_Children[i].ContainsFocus) return m_Children[i].OnKeyDown(key); return false; }
        public override bool OnKeyUp(KeyboardKeys key) { for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].Enabled && m_Children[i].ContainsFocus) return m_Children[i].OnKeyUp(key); return false; }
        private DragDropEffect RouteDrag(Float2 location, DragData data, bool move, bool drop) { DragDropEffect result = drop ? base.OnDragDrop(ref location, data) : move ? base.OnDragMove(ref location, data) : base.OnDragEnter(ref location, data); for (int i = m_Children.Count - 1; i >= 0; i--) { Control child = m_Children[i]; if (!child.Visible || !child.Enabled) continue; if (IntersectsChildContent(child, location, out Float2 childLocation)) { DragDropEffect childResult = drop ? child.OnDragDrop(ref childLocation, data) : move ? (child.IsDragOver() ? child.OnDragMove(ref childLocation, data) : child.OnDragEnter(ref childLocation, data)) : child.OnDragEnter(ref childLocation, data); if (childResult != DragDropEffect.None) { result = childResult; if (!move || drop) break; } } else if (move && child.IsDragOver()) child.OnDragLeave(); } return result; }
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data) => RouteDrag(location, data, false, false);
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data) => RouteDrag(location, data, true, false);
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data) => RouteDrag(location, data, false, true);
        public override void OnDragLeave() { base.OnDragLeave(); for (int i = 0; i < m_Children.Count; i++) if (m_Children[i].IsDragOver()) m_Children[i].OnDragLeave(); }

        public override Control? OnNavigate(NavDirection direction, Float2 location, Control? caller, List<Control> visited) { if (AutoFocus && !ContainsFocus) return this; if (m_Children.Count != 0 && !visited.Contains(this)) { visited.Add(this); Control? result = NavigationRaycast(direction, location, visited); Float2 rightMostLocation = location; if (result == null && (direction == NavDirection.Next || direction == NavDirection.Previous)) result = NavigationWrap(direction, location, visited, out rightMostLocation); if (result != null) { Float2 useLocation = direction == NavDirection.Previous ? rightMostLocation : location; result = result.OnNavigate(direction, result.PointFromParent(useLocation), this, visited); if (result != null) return result; } } if ((AutoFocus && !IsFocused) || caller == this) return this; if (AutoFocus && Visible) location = GetNavOrigin(direction); return Parent?.OnNavigate(direction, PointToParent(location), caller, visited); }
        protected virtual bool CanNavigateChild(Control child) => !child.IsFocused && child.Enabled && child.Visible && CanGetAutoFocus(child);
        private static bool CanGetAutoFocus(Control control) { if (control.AutoFocus) return true; if (control is ContainerControl container) for (int i = 0; i < container.m_Children.Count; i++) if (container.CanNavigateChild(container.m_Children[i])) return true; return false; }
        protected virtual Control? NavigationWrap(NavDirection direction, Float2 location, List<Control> visited, out Float2 rightMostLocation) { visited.Add(this); Float2 layoutSize = Root?.FocusedControl?.Size ?? Float2.Zero; Float2 predicted = direction == NavDirection.Next ? new Float2(0, location.Y + layoutSize.Y) : new Float2(Size.X, location.Y - layoutSize.Y); if (new Rectangle(Float2.Zero, Size).Contains(predicted)) { Control? result = NavigationRaycast(direction, predicted, visited); if (result != null) { rightMostLocation = predicted; return result; } } rightMostLocation = location; return Parent?.NavigationWrap(direction, PointToParent(location), visited, out rightMostLocation); }
        private Control? NavigationRaycast(NavDirection direction, Float2 location, List<Control> visited) { Float2 dir1 = Float2.Zero, dir2 = Float2.Zero; switch (direction) { case NavDirection.Up: dir1 = dir2 = new Float2(0, -1); break; case NavDirection.Down: dir1 = dir2 = new Float2(0, 1); break; case NavDirection.Left: dir1 = dir2 = new Float2(-1, 0); break; case NavDirection.Right: dir1 = dir2 = new Float2(1, 0); break; case NavDirection.Next: dir1 = new Float2(1, 0); dir2 = new Float2(0, 1); break; case NavDirection.Previous: dir1 = new Float2(-1, 0); dir2 = new Float2(0, -1); break; } Control? result = null; float minDistance = float.MaxValue; for (int i = 0; i < m_Children.Count; i++) { Control child = m_Children[i]; if (!CanNavigateChild(child) || visited.Contains(child)) continue; Float2 childDirection = Float2.Normalize(child.Center - location); float c1 = Float2.Dot(dir1, childDirection); float c2 = Float2.Dot(dir2, childDirection); float distance = Rectangle.Distance(child.Bounds, location); if (c1 > Mathf.Epsilon && c2 > Mathf.Epsilon && distance < minDistance) { minDistance = distance; result = child; } } return result; }

        protected override void OnDispose() { base.OnDispose(); }
        public override void ClearState() { base.ClearState(); for (int i = 0; i < m_Children.Count; i++) m_Children[i].ClearState(); }
        public override void OnDestroy() { if (ContainsFocus) Focus(); if (!m_IsLayoutLocked) LockChildrenRecursive(); base.OnDestroy(); for (int i = 0; i < m_Children.Count; i++) m_Children[i].OnDestroy(); m_Children.Clear(); }

        internal virtual void AddChildInternal(Control child) { m_Children.Add(child); OnChildAdded(child); OnChildrenChanged(); }
        internal virtual void RemoveChildInternal(Control child) { m_Children.Remove(child); OnChildRemoved(child); OnChildrenChanged(); }
        internal int IndexOf(Control control) => GetChildIndex(control);
        internal void SetChildIndex(Control control, int index) { int currentIndex = m_Children.IndexOf(control); if (currentIndex < 0 || currentIndex == index) return; m_Children.RemoveAt(currentIndex); if (index < 0 || index >= m_Children.Count) m_Children.Add(control); else m_Children.Insert(index, control); PerformLayout(); }
        internal override void SetRootCore(RootControl? root) { base.SetRootCore(root); for (int i = 0; i < m_Children.Count; i++) m_Children[i].SetRootCore(root); UpdateContainsFocus(); }
        internal override void CacheRootHandle() { base.CacheRootHandle(); for (int i = 0; i < m_Children.Count; i++) m_Children[i].CacheRootHandle(); }
        internal void UpdateContainsFocusUpwards() { ContainerControl? current = this; while (current != null) { current.UpdateContainsFocus(); current = current.Parent; } }
        private void UpdateContainsFocus() { bool result = base.ContainsFocus; for (int i = 0; i < m_Children.Count; i++) { if (m_Children[i] is ContainerControl container) container.UpdateContainsFocus(); if (m_Children[i].ContainsFocus) result = true; } if (result == m_ContainsFocus) return; m_ContainsFocus = result; if (result) OnStartContainsFocus(); else OnEndContainsFocus(); }
    }
}
