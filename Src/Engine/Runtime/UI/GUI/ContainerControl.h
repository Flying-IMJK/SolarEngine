#pragma once

#include "Control.h"
#include "Core/TypeSystem/Types.h"

namespace SE
{
	class SE_API_RUNTIME ContainerControl : public Control
	{
	    SE_DEFINE_CLASS(ContainerControl, Control)
	private:
		/// <summary>
		/// The layout locking flag.
		/// </summary>
		bool m_IsLayoutLocked;

	protected:
        /// <summary>
        /// The children collection.
        /// </summary>
	    List<Control*> m_Children;

        /// <summary>
        /// The contains focus cached flag.
        /// </summary>
        bool _containsFocus = false;

	public:
	    /// <summary>
	    /// Gets or sets a value indicating whether apply clipping mask on children during rendering.
	    /// </summary>
	    bool ClipChildren = true;
	    /// <summary>
	    /// True if automatic updates for control layout are locked (useful when creating a lot of GUI control to prevent lags).
	    /// </summary>
	    bool CullChildren = true;

		ContainerControl();

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        ContainerControl(float x, float y, float width, float height) : Control(x, y, width, height)
        {
        	m_IsLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
	    ContainerControl(Float2 location, Float2 size) : Control(location, size)
        {
        	m_IsLayoutLocked = true;
        }

        /// <inheritdoc />
        explicit ContainerControl(Rectangle bounds) : Control(bounds)
	    {
        	m_IsLayoutLocked = true;
	    }

		FORCE_INLINE void SetIsLayoutLocked(bool isLayoutLocked) { m_IsLayoutLocked = isLayoutLocked; };
		FORCE_INLINE bool GetIsLayoutLocked() const { return m_IsLayoutLocked; };

        /// <summary>
        /// Gets child controls list
        /// </summary>
	    List<Control*>& Children() { return m_Children; };

        /// <summary>
        /// Gets amount of the children controls
        /// </summary>
	    int ChildrenCount() const { return m_Children.Count(); };

        /// <summary>
        /// Checks if container has any child controls
        /// </summary>
	    bool HasChildren() const { return m_Children.Count() > 0; }

        /// <summary>
        /// Gets a value indicating whether the control, or one of its child controls, currently has the input focus.
        /// </summary>
	    bool ContainsFocus() override { return _containsFocus; }
	    

        /// <summary>
        /// Locks all child controls layout and itself.
        /// </summary>
	    void LockChildrenRecursive();

        /// <summary>
        /// Unlocks all the child controls layout and itself.
        /// </summary>
        void UnlockChildrenRecursive();

        /// <summary>
        /// Unlinks all the child controls.
        /// </summary>
        virtual void RemoveChildren();

        /// <summary>
        /// Removes and disposes all the child controls
        /// </summary>
        virtual void DisposeChildren();

        /// <summary>
        /// Creates a new control and adds it to the container.
        /// </summary>
        /// <returns>The added control.</returns>
        template<class T>
        T* AddChild()
        {
            T* child = New<T>();
            child.SetParent(this);
            return child;
        }

        /// <summary>
        /// Adds the control to the container.
        /// </summary>
        /// <param name="child">The control to add.</param>
        /// <returns>The added control.</returns>
	    template<class T>
        T* AddChild(T* child)
        {
            static_assert(TIsBaseOf<Control, T>::Value, "T must be a subclass of Control");

	        ENGINE_ASSERT(child != nullptr);
	        ENGINE_ASSERT_M(!(child->Parent == this && m_Children.Contains(child)), SE_TEXT("Argument child cannot be added, if current container is already its parent."));

            // Set child new parent
            child->Parent = this;

            return child;
        }

        /// <summary>
        /// Removes control from the container.
        /// </summary>
        /// <param name="child">The control to remove.</param>
	    void RemoveChild(Control* child) const;

        /// <summary>
        /// Gets child control at given index.
        /// </summary>
        /// <param name="index">The control index.</param>
        /// <returns>The child control.</returns>
	    Control* GetChild(int index)
        {
            return m_Children[index];
        }

        /// <summary>
        /// Searches for a child control of a specific type. If there are multiple controls matching the type, only the first one found is returned.
        /// </summary>
        /// <typeparam name="T">The type of the control to search for. Includes any controls derived from the type.</typeparam>
        /// <returns>The control instance if found, otherwise null.</returns>
	    template<class T>
        T* GetChild()
        {
            TypeID type = Typeof<T>();
            for (int i = 0; i < m_Children.Count(); i++)
            {
                Control* ct = m_Children[i];
                if (Types::IsTypeDerivedFrom(ct->GetType(), type))
                {
                    return (T*)m_Children[i];
                }
            }
            return nullptr;
        }

        /// <summary>
        /// Gets zero-based index in the list of control children.
        /// </summary>
        /// <param name="child">The child control.</param>
        /// <returns>The zero-based index in the list of control children.</returns>
	    int GetChildIndex(const Control* child) const;

        void ChangeChildIndex(Control* child, int newIndex);

        /// <summary>
        /// Tries to find any child control at given point in control local coordinates.
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <returns>The found control index or -1 if failed.</returns>
	    int GetChildIndexAt(Float2 point);

        /// <summary>
        /// Tries to find any child control at given point in control local coordinates
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <returns>The found control or null.</returns>
        Control* GetChildAt(Float2 point);

        /// <summary>
        /// Tries to find valid child control at given point in control local coordinates. Uses custom callback method to test controls to pick.
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <param name="isValid">The control validation callback.</param>
        /// <returns>The found control or null.</returns>
        Control* GetChildAt(Float2 point, const Function<bool(Control*)>& isValid);

        /// <summary>
        /// Tries to find lowest child control at given point in control local coordinates.
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <returns>The found control or null.</returns>
        Control* GetChildAtRecursive(Float2 point);

        /// <summary>
        /// Gets rectangle in local control coordinates with area for controls (without scroll bars, anchored controls, etc.).
        /// </summary>
        /// <returns>The rectangle in local control coordinates with area for controls (without scroll bars etc.).</returns>
        Rectangle GetClientArea();

        /// <summary>
        /// Sort child controls list
        /// </summary>
        void SortChildren();

        /// <summary>
        /// Sort children using recursion
        /// </summary>
	    void SortChildrenRecursive();

        /// <summary>
        /// Called when child control gets resized.
        /// </summary>
        /// <param name="control">The resized control.</param>
        virtual void OnChildResized(Control* control)
        {
        }

        /// <summary>
        /// Called when children collection gets changed (child added or removed).
        /// </summary>
        virtual void OnChildrenChanged();

	public:
        /// <inheritdoc />
	    void CacheRootHandle() override;

        /// <summary>
        /// Adds a child control to the container.
        /// </summary>
        /// <param name="child">The control to add.</param>
	    virtual void AddChildInternal(Control* child);

        /// <summary>
        /// Removes a child control from this container.
        /// </summary>
        /// <param name="child">The control to remove.</param>
	    virtual void RemoveChildInternal(Control* child);

        /// <summary>
        /// Gets the desire client area rectangle for all the controls.
        /// </summary>
	    virtual Rectangle GetDesireClientArea();

        /// <summary>
        /// Checks if given point in this container control space intersects with the child control content.
        /// Also calculates result location in child control space which can be used to feed control with event at that point.
        /// </summary>
        /// <param name="child">The child control to check.</param>
        /// <param name="location">The location in this container control space.</param>
        /// <param name="childSpaceLocation">The output location in child control space.</param>
        /// <returns>True if point is over the control content, otherwise false.</returns>
        virtual bool IntersectsChildContent(Control* child, Float2 location, Float2& childSpaceLocation);

        #pragma region Navigation

        /// <inheritdoc />
	public:
	    Control* OnNavigate(NavDirection direction, Float2 location, Control* caller, List<Control*>& visited) override;

        /// <summary>
        /// Checks if this container control can more with focus navigation into the given child control.
        /// </summary>
        /// <param name="child">The child.</param>
        /// <returns>True if can navigate to it, otherwise false.</returns>
	protected:
	    virtual bool CanNavigateChild(Control* child);

        /// <summary>
        /// Wraps the navigation over the layout.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <param name="location">The navigation start location (in the control-space).</param>
        /// <param name="visited">The list with visited controls. Used to skip recursive navigation calls when doing traversal across the UI hierarchy.</param>
        /// <param name="rightMostLocation">Returns the rightmost location of the parent container for the raycast used by the child container</param>
        /// <returns>The target navigation control or null if didn't performed any navigation.</returns>
        virtual Control* NavigationWrap(NavDirection direction, Float2 location, List<Control*> visited, Float2& rightMostLocation);

	private:
        static bool CanGetAutoFocus(Control* c);

        Control* NavigationRaycast(NavDirection direction, Float2 location, List<Control*> visited);

        #pragma endregion

        /// <summary>
        /// Update contain focus state and all it's children
        /// </summary>
	protected:
	    void UpdateContainsFocus();

        /// <summary>
        /// Updates child controls bounds.
        /// </summary>
        void UpdateChildrenBounds();

        /// <summary>
        /// Perform layout for that container control before performing it for child controls.
        /// </summary>
        virtual void PerformLayoutBeforeChildren()
        {
            UpdateChildrenBounds();
        }

        /// <summary>
        /// Perform layout for that container control after performing it for child controls.
        /// </summary>
        virtual void PerformLayoutAfterChildren()
        {
        }

        #pragma region Control

	public:

	    void OnDestroy() override;

	    bool IsTouchOver() override;

	    void Update(float deltaTime) override;

	    void ClearState() override;

        /// <summary>
        /// Draw the control and the children.
        /// </summary>
        void Draw() override;

        /// <summary>
        /// Draws the control.
        /// </summary>
	    virtual void DrawSelf()
        {
            Control::Draw();
        }

	protected:
        /// <summary>
        /// Draws the children. Can be overridden to provide some customizations. Draw is performed with applied clipping mask for the client area.
        /// </summary>
        virtual void DrawChildren();

	public:
        /// <inheritdoc />
	    void PerformLayout(bool force = false) override;

        /// <inheritdoc />
        bool RayCast(Float2& location, Control*& hit) override;

        bool RayCastChildren(Float2 location, Control*& hit);

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
	    bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool IsTouchPointerOver(int pointerId) override;

        /// <inheritdoc />
        void OnTouchEnter(Float2 location, int pointerId) override;

        /// <inheritdoc />
        bool OnTouchDown(Float2 location, int pointerId) override;

        /// <inheritdoc />
        void OnTouchMove(Float2 location, int pointerId) override;

        /// <inheritdoc />
        bool OnTouchUp(Float2 location, int pointerId) override;

        /// <inheritdoc />
        void OnTouchLeave(int pointerId) override;

		void OnTouchLeave() override;

        /// <inheritdoc />
        bool OnCharInput(Char c) override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnKeyUp(KeyboardKeys key) override;

        /// <inheritdoc />
        DragDropEffect OnDragEnter(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        void OnDragLeave() override;

        /// <inheritdoc />
	    DragDropEffect OnDragDrop(const Float2& location, DragData* data) override;

	protected:
	    void OnSizeChanged() override;

        #pragma endregion
	};

} // SE

