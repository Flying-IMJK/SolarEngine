#pragma once

#include <any>

#include "Enums.h"
#include "Margin.h"
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Matrix3x3.h"
#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Platform/Base/WindowBase.h"
#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Property.h"
#include "Runtime/Core/TypeSystem/Info/TypeCompositeInfo.h"

namespace SE
{
    class Tooltip;
    class DragData;
    class ContainerControl;
    class RootControl;
    class WindowRootControl;

    typedef Delegate<float /*deltaTime*/> UpdateDelegate;

    /// <summary>Compare
    /// Base class for all GUI controls
    /// </summary>
    SE_CLASS(Reflect)
    class SE_API_RUNTIME Control : public IType// : IComparable,
    {
        SE_DEFINE_CLASS(Control, IType)
    protected:
        struct AnchorPresetData
        {
            AnchorPresets Preset;
            Float2 Min;
            Float2 Max;

            AnchorPresetData(AnchorPresets preset, Float2 min, Float2 max)
            {
                Preset = preset;
                Min = min;
                Max = max;
            }
        };

        static AnchorPresetData anchorPresetsData[16];

        ContainerControl* m_Parent = nullptr;
        RootControl* m_Root = nullptr;
        bool m_IsDisposing, m_IsFocused, m_IsNavFocused;
        String m_Name;

        // State

        // TODO: convert to flags

        bool _isMouseOver = false, _isDragOver = false;
        bool _isVisible = true;
        bool _isEnabled = true;

        bool _pivotRelativeSizing = false;
        List<int> _touchOvers;
        UpdateDelegate m_TooltipUpdate;
        UpdateDelegate m_OnUpdateTooltip;

        // Transform

        Rectangle m_Bounds = Rectangle::Empty;
        Margin m_Offsets = Margin(0.0f, 100.0f, 0.0f, 30.0f);
        Float2 m_AnchorMin = Float2(0.0f);
        Float2 m_AnchorMax = Float2(0.0f);
        Float2 m_Scale = Float2::One;
        Float2 m_Pivot = Float2(0.5f);
        Float2 m_Shear = Float2(0.0f);
        float m_Rotation = 0;
        Matrix3x3 m_CachedTransform = Matrix3x3::Zero;
        Matrix3x3 m_CachedTransformInv = Matrix3x3::Zero;

    public:

        // Tooltip
        String TooltipText = String::Empty;
        /// <summary>
        /// Gets or sets the custom tooltip control linked. Use null to show default shared tooltip from the current <see cref="Style"/>.
        /// </summary>
        Tooltip* Tooltip = nullptr;

        // State

        /// <summary>
        /// If checked, the control can receive automatic focus (eg. on user click or UI navigation).
        /// </summary>
        bool AutoFocus = true;

        // Style

        /// <summary>
        /// Gets or sets control background color (transparent color (alpha=0) means no background rendering)
        /// </summary>
        Color BackgroundColor = Colors::Transparent;

        /// <summary>
        /// Gets or sets a value indicating whether this control is scrollable (scroll bars affect it).
        /// </summary>
        bool IsScrollable = true;


        /// <summary>
        /// Action is invoked, when location is changed
        /// </summary>
        Delegate<Control*> LocationChanged;

        /// <summary>
        /// Action is invoked, when size is changed
        /// </summary>
        Delegate<Control*> SizeChanged;

        /// <summary>
        /// Action is invoked, when parent is changed
        /// </summary>
        Delegate<Control*> ParentChanged;

        /// <summary>
        /// Action is invoked, when visibility is changed
        /// </summary>
        Delegate<Control*> VisibleChanged;


        #pragma region Properties

        PRO_REF(Name, Control, String, __GetName, __SetName);

        /// <summary>
        /// Parent control (the one above in the tree hierarchy)
        /// </summary>
        PRO(Parent, Control, ContainerControl*, __GetParent, __SetParent);


        /// <summary>
        /// Checks if control has parent container control
        /// </summary>
        bool HasParent();


        /// <summary>
        /// Gets or sets zero-based index of the control inside the parent container list.
        /// </summary>
        PRO(IndexInParent, Control, int, __GetIndexInParent, __SetIndexInParent);

        /// <summary>
        /// Gets or sets the anchor preset used by the control anchors (based on <see cref="AnchorMin"/> and <see cref="AnchorMax"/>).
        /// </summary>
        /// <remarks>To change anchor preset with current control bounds preservation use <see cref="SetAnchorPreset"/>.</remarks>
        PRO(AnchorPreset, Control, AnchorPresets, __GetAnchorPreset, __SetAnchorPreset);


        /// <summary>
        /// Gets or sets a value indicating whether the control can respond to user interaction
        /// </summary>
        PRO(Enabled, Control, bool, __GetEnabled, __SetEnabled);


        /// <summary>
        /// Gets a value indicating whether the control is enabled in the hierarchy (it's enabled and all it's parents are enabled as well).
        /// </summary>
        bool EnabledInHierarchy();

        /// <summary>
        /// Gets or sets a value indicating whether the control is visible
        /// </summary>
        PRO(Visible, Control, bool, __GetVisible, __SetVisible);


        /// <summary>
        /// Gets a value indicating whether the control is visible in the hierarchy (it's visible and all it's parents are visible as well).
        /// </summary>
        bool VisibleInHierarchy();

        /// <summary>
        /// Returns true if control is during disposing state (on destroy)
        /// </summary>
        PRO_GET(IsDisposing, Control, bool, __GetIsDisposing);

        /// <summary>
        /// Gets the GUI tree root control which contains that control (or null if not linked to any)
        /// </summary>
        PRO_GET(Root, Control, RootControl*, __GetRoot);

        /// <summary>
        /// Gets the GUI window root control which contains that control (or null if not linked to any).
        /// </summary>
        virtual WindowRootControl* RootWindow();

        /// <summary>
        /// Gets the control DPI scale factor (1 is default). Includes custom DPI scale.
        /// </summary>
        float GetDpiScale();

        /// <summary>
        /// Gets screen position of the control (upper left corner).
        /// </summary>
        Float2 ScreenPos();

        Matrix3x3 GetCachedTransform() const { return m_CachedTransform; }

        #pragma endregion


        /// <summary>
        /// Gets or sets the cursor (per window). Control should restore cursor to the default value eg. when mouse leaves it's area.
        /// </summary>
        PRO(Cursor, Control, CursorType, __GetCursor, __SetCursor);

        /// <summary>
        /// The custom tag object value linked to the control.
        /// </summary>
        std::any Tag;

        Control();

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        /// <param name="x">X coordinate</param>
        /// <param name="y">Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        Control(float x, float y, float width, float height);

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        /// <param name="location">Upper left corner location.</param>
        /// <param name="size">Bounds size.</param>
        Control(Float2 location, Float2 size);

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="bounds">Window bounds</param>
        explicit Control(Rectangle bounds);

        /// <summary>
        /// Delete control (will unlink from the parent and start to dispose)
        /// </summary>
        void Dispose();

        /// <summary>
        /// Perform control update and all its children
        /// </summary>
        /// <param name="deltaTime">Delta time in seconds</param>
        virtual void Update(float deltaTime)
        {
            // TODO: move all controls to use UpdateDelegate and remove this generic Update
        }

        void OnUpdateTooltip(float deltaTime)
        {
            // Tooltip.OnMouseOverControl(this, deltaTime);
        }

        /// <summary>
        /// Draw control
        /// </summary>
        virtual void Draw();

        /// <summary>
        /// Update control layout
        /// </summary>
        /// <param name="force">True if perform layout by force even if cached state wants to skip it due to optimization.</param>
        virtual void PerformLayout(bool force = false)
        {
        }

        /// <summary>
        /// Called to clear UI state. For example, removes mouse over state or drag and drop when control gets disabled or hidden (including hierarchy).
        /// </summary>
        virtual void ClearState();

        #pragma region Bounds
    public:

        /// <summary>
        /// Gets or sets X coordinate of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO(X, Control, float, __GetX, __SetX);

        /// <summary>
        /// Gets or sets Y coordinate of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO(Y, Control, float, __GetY, __SetY);

        /// <summary>
        /// Gets or sets the local X coordinate of the pivot of the control relative to the anchor in parent of its container.
        /// </summary>
        PRO(LocalX, Control, float, __GetLocalX, __SetLocalX);

        /// <summary>
        /// Gets or sets the local Y coordinate of the pivot of the control relative to the anchor in parent of its container.
        /// </summary>
        PRO(LocalY, Control, float, __GetLocalY, __SetLocalY);

        /// <summary>
        /// Gets or sets the normalized position in the parent control that the upper left corner is anchored to (range 0-1).
        /// </summary>
        PRO_REF(AnchorMin, Control, Float2, __GetAnchorMin, __SetAnchorMin);

        /// <summary>
        /// Gets or sets the normalized position in the parent control that the bottom right corner is anchored to (range 0-1).
        /// </summary>
        PRO_REF(AnchorMax, Control, Float2, __GetAnchorMax, __SetAnchorMax);

        /// <summary>
        /// Gets or sets the offsets of the corners of the control relative to its anchors.
        /// </summary>
        PRO_REF(Offsets, Control, Margin, __GetOffsets, __SetOffsets);


#if SE_EDITOR
        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        /*[NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Left
        {
            get => _offsets.Left;
            set => Offsets = new Margin(value, _offsets.Right, _offsets.Top, _offsets.Bottom);
        }

        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Right
        {
            get => _offsets.Right;
            set => Offsets = new Margin(_offsets.Left, value, _offsets.Top, _offsets.Bottom);
        }

        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Top
        {
            get => _offsets.Top;
            set => Offsets = new Margin(_offsets.Left, _offsets.Right, value, _offsets.Bottom);
        }

        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Bottom
        {
            get => _offsets.Bottom;
            set => Offsets = new Margin(_offsets.Left, _offsets.Right, _offsets.Top, value);
        }*/
#endif


        /// <summary>
        /// Gets or sets coordinates of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO(Location, Control, Float2, __GetLocation, __SetLocation);

        /// <summary>
        /// Gets or sets the local position of the pivot of the control relative to the anchor in parent of its container.
        /// </summary>
        PRO(LocalLocation, Control, Float2, __GetLocalLocation, __SetLocalLocation);

        /// <summary>
        /// Whether to resize the UI Control based on where the pivot is rather than just the top-left.
        /// </summary>
        PRO(PivotRelative, Control, bool, __GetPivotRelative, __SetPivotRelative);

        /// <summary>
        /// Gets or sets width of the control.
        /// </summary>
        PRO(Width, Control, float, __GetWidth, __SetWidth);

        /// <summary>
        /// Gets or sets height of the control.
        /// </summary>
        PRO(Height, Control, float, __GetHeight, __SetHeight);

        /// <summary>
        /// Gets or sets control's size.
        /// </summary>
        PRO_REF(Size, Control, Float2, __GetSize, __SetSize);

        /// <summary>
        /// Gets Y coordinate of the top edge of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(Top, Control, float, __GetTop);

        /// <summary>
        /// Gets Y coordinate of the bottom edge of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(Bottom, Control, float, __GetBottom);

        /// <summary>
        /// Gets X coordinate of the left edge of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(Left, Control, float, __GetLeft);

        /// <summary>
        /// Gets X coordinate of the right edge of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(Right, Control, float, __GetRight);

        /// <summary>
        /// Gets position of the upper left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(UpperLeft, Control, Float2, __GetUpperLeft);

        /// <summary>
        /// Gets position of the upper right corner of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(UpperRight, Control, Float2, __GetUpperRight);

        /// <summary>
        /// Gets position of the bottom right corner of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(BottomRight, Control, Float2, __GetBottomRight);

        /// <summary>
        /// Gets position of the bottom left of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO_GET(BottomLeft, Control, Float2, __GetBottomLeft);

        /// <summary>
        /// Gets center position of the control relative to the upper-left corner of its container.
        /// </summary>
        PRO(Center, Control, Float2, __GetCenter, __SetCenter);

        /// <summary>
        /// Gets or sets control's bounds rectangle.
        /// </summary>
        PRO_REF(Bounds, Control, Rectangle, __GetBounds, __SetBounds);

        /// <summary>
        /// Gets or sets the scale. Scales control according to its Pivot which by default is (0.5,0.5) (middle of the control). If you set pivot to (0,0) it will scale the control based on it's upper-left corner.
        /// </summary>
        PRO_REF(Scale, Control, Float2, __GetScale, __SetScale);

        /// <summary>
        /// Gets or sets the normalized pivot location (used to transform control around it). Point (0,0) is upper left corner, (0.5,0.5) is center, (1,1) is bottom right corner.
        /// </summary>
        PRO_REF(Pivot, Control, Float2, __GetPivot, __SetPivot);

        /// <summary>
        /// Gets or sets the shear transform angles (x, y). Defined in degrees. Shearing happens relative to the control pivot point.
        /// </summary>
        PRO_REF(Shear, Control, Float2, __GetShear, __SetShear);

        /// <summary>
        /// Gets or sets the rotation angle (in degrees). Control is rotated around it's pivot point (middle of the control by default).
        /// </summary>
        PRO(Rotation, Control, float, __GetRotation, __SetRotation);

        /// <summary>
        /// Updates the control cached bounds (based on anchors and offsets).
        /// </summary>
        void UpdateBounds();

        /// <summary>
        /// Updates the control cached transformation matrix (based on bounds).
        /// </summary>
        void UpdateTransform();

        /// <summary>
        /// Sets the anchor preset for the control. Can be use to auto-place the control for a given preset or can preserve the current control bounds.
        /// </summary>
        /// <param name="anchorPreset">The anchor preset to set.</param>
        /// <param name="preserveBounds">True if preserve current control bounds, otherwise will align control position accordingly to the anchor location.</param>
        /// <param name="setPivotToo">Whether or not we should set the pivot too, eg left-top 0,0, bottom-right 1,1</param>
        void SetAnchorPreset(AnchorPresets anchorPreset, bool preserveBounds, bool setPivotToo = false);

        #pragma endregion

        #pragma region Focus


        /// <summary>
        /// Gets a value indicating whether the control, currently has the input focus
        /// </summary>
        virtual bool ContainsFocus() { return m_IsFocused; }

        /// <summary>
        /// Gets a value indicating whether the control has input focus
        /// </summary>
        PRO_GET(IsFocused, Control, bool, __GetIsFocused);

        /// <summary>
        /// Gets a value indicating whether the control has UI navigation focus.
        /// </summary>
        PRO_GET(IsNavFocused, Control, bool, __GetIsNavFocused);

        /// <summary>
        /// Sets input focus to the control
        /// </summary>
        virtual void Focus()
        {
            if (!IsFocused)
            {
                Focus(this);
            }
        }

        /// <summary>
        /// Removes input focus from the control
        /// </summary>
        virtual void Defocus();

        /// <summary>
        /// When control gets input focus
        /// </summary>
        virtual void OnGetFocus()
        {
            // Cache flag
            m_IsFocused = true;
            m_IsNavFocused = false;
        }

        /// <summary>
        /// When control losts input focus
        /// </summary>
        virtual void OnLostFocus()
        {
            // Clear flag
            m_IsFocused = false;
            m_IsNavFocused = false;
        }

        /// <summary>
        /// Action fired when control gets 'Contains Focus' state
        /// </summary>
        virtual void OnStartContainsFocus()
        {
        }

        /// <summary>
        /// Action fired when control lost 'Contains Focus' state
        /// </summary>
        virtual void OnEndContainsFocus()
        {
        }


        /// <summary>
        /// Starts the mouse tracking. Used by the scrollbars, splitters, etc.
        /// </summary>
        /// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
        void StartMouseCapture(bool useMouseScreenOffset = false);

        /// <summary>
        /// Ends the mouse tracking.
        /// </summary>
        void EndMouseCapture();

        /// <summary>
        /// When mouse goes up/down not over the control but it has user focus so remove that focus from it (used by scroll
        /// bars, sliders etc.)
        /// </summary>
        virtual void OnEndMouseCapture()
        {
        }

    protected:
        /// <summary>
        /// Focus that control
        /// </summary>
        /// <param name="c">Control to focus</param>
        /// <returns>True if control got a focus</returns>
        virtual bool Focus(Control* c);

        #pragma endregion

        #pragma region Navigation

    public:
        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Up"/> direction.
        /// </summary>
        Control* NavTargetUp;

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Down"/> direction.
        /// </summary>
        Control* NavTargetDown;

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Left"/> direction.
        /// </summary>
        Control* NavTargetLeft;

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Right"/> direction.
        /// </summary>
        Control* NavTargetRight;

        /// <summary>
        /// Gets the next navigation control to focus for the given direction. Returns null for automated direction resolving.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <returns>The target navigation control or null to use automatic navigation.</returns>
        virtual Control* GetNavTarget(NavDirection direction);

        /// <summary>
        /// Gets the navigation origin location for this control. It's the starting anchor point for searching navigable controls in the nearby area. By default the origin points are located on the control bounds edges.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <returns>The navigation origin for the automatic navigation.</returns>
        virtual Float2 GetNavOrigin(NavDirection direction);

        /// <summary>
        /// Performs the UI navigation for this control.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <param name="location">The navigation start location (in the control-space).</param>
        /// <param name="caller">The control that calls the event.</param>
        /// <param name="visited">The list with visited controls. Used to skip recursive navigation calls when doing traversal across the UI hierarchy.</param>
        /// <returns>The target navigation control or null if didn't performed any navigation.</returns>
        virtual Control* OnNavigate(NavDirection direction, Float2 location, Control* caller, List<Control*>& visited);

        /// <summary>
        /// Focuses the control by the UI navigation system. Called during navigating around UI with gamepad/keyboard navigation. Focuses the control and sets the <see cref="IsNavFocused"/> flag.
        /// </summary>
        virtual void NavigationFocus();

        /// <summary>
        /// Generic user interaction event for a control used by UI navigation (eg. user submits on the currently focused control).
        /// </summary>
        virtual void OnSubmit()
        {
        }

        #pragma endregion

        #pragma region Mouse

        PRO_GET(IsMouseOver, Control, bool, __GetIsMouseOver);

        /// <summary>
        /// When mouse enters control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        virtual void OnMouseEnter(Float2 location);

        /// <summary>
        /// When mouse moves over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        virtual void OnMouseMove(Float2 location);

        /// <summary>
        /// When mouse leaves control's area
        /// </summary>
        virtual void OnMouseLeave();

        /// <summary>
        /// When mouse wheel moves
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="delta">Mouse wheel move delta. A positive value indicates that the wheel was rotated forward, away from the user; a negative value indicates that the wheel was rotated backward, toward the user. Normalized to [-1;1] range.</param>
        /// <returns>True if event has been handled</returns>
        virtual bool OnMouseWheel(Float2 location, float delta)
        {
            return false;
        }

        /// <summary>
        /// When mouse goes down over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="button">Mouse buttons state (flags)</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        virtual bool OnMouseDown(Float2 location, MouseButton button)
        {
            return false;
        }

        /// <summary>
        /// When mouse goes up over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="button">Mouse buttons state (flags)</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        virtual bool OnMouseUp(Float2 location, MouseButton button)
        {
            return false;
        }

        /// <summary>
        /// When mouse double clicks over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="button">Mouse buttons state (flags)</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        virtual bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            return false;
        }

        #pragma endregion

        #pragma region Keyboard

        /// <summary>
        /// On input character
        /// </summary>
        /// <param name="c">Input character</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        virtual bool OnCharInput(Char c)
        {
            return false;
        }

        /// <summary>
        /// When key goes down
        /// </summary>
        /// <param name="key">Key value</param>
        /// <returns>True if event has been handled, otherwise false</returns>
        virtual bool OnKeyDown(KeyboardKeys key)
        {
            return false;
        }

        /// <summary>
        /// When key goes up
        /// </summary>
        /// <param name="key">Key value</param>
        virtual void OnKeyUp(KeyboardKeys key)
        {
        }

        #pragma endregion

        #pragma region Touch

        /// <summary>
        /// Check if touch is over that item or its child items
        /// </summary>
        virtual bool IsTouchOver();

        /// <summary>
        /// Determines whether the given touch pointer is over the control.
        /// </summary>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <returns>True if given touch pointer is over the control, otherwise false.</returns>
        virtual bool IsTouchPointerOver(int pointerId);

        /// <summary>
        /// When touch enters control's area
        /// </summary>
        /// <param name="location">Touch location in Control Space</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        virtual void OnTouchEnter(Float2 location, int pointerId);

        /// <summary>
        /// When touch enters control's area.
        /// </summary>
        /// <param name="location">Touch location in Control Space.</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        virtual bool OnTouchDown(Float2 location, int pointerId)
        {
            return false;
        }

        /// <summary>
        /// When touch moves over control's area.
        /// </summary>
        /// <param name="location">Touch location in Control Space.</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        virtual void OnTouchMove(Float2 location, int pointerId)
        {
        }

        /// <summary>
        /// When touch goes up over control's area.
        /// </summary>
        /// <param name="location">Touch location in Control Space</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        virtual bool OnTouchUp(Float2 location, int pointerId)
        {
            return false;
        }

        /// <summary>
        /// When touch leaves control's area
        /// </summary>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        virtual void OnTouchLeave(int pointerId);

        /// <summary>
        /// When all touch leaves control's area
        /// </summary>
        virtual void OnTouchLeave()
        {
        }

        #pragma endregion

        #pragma region Drag&Drop

        /// <summary>
        /// Check if mouse dragging is over that item or its child items.
        /// </summary>
        virtual bool IsDragOver() { return _isDragOver; }

        /// <summary>
        /// When mouse dragging enters control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="data">The data. See <see cref="DragDataText"/> and <see cref="DragDataFiles"/>.</param>
        /// <returns>The drag event result effect.</returns>
        virtual DragDropEffect OnDragEnter(const Float2& location, DragData* data)
        {
            // Set flag
            _isDragOver = true;
            return DragDropEffect::None;
        }

        /// <summary>
        /// When mouse dragging moves over control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="data">The data. See <see cref="DragDataText"/> and <see cref="DragDataFiles"/>.</param>
        /// <returns>The drag event result effect.</returns>
        virtual DragDropEffect OnDragMove(const Float2& location, DragData* data)
        {
            return DragDropEffect::None;
        }

        /// <summary>
        /// When mouse dragging drops on control's area
        /// </summary>
        /// <param name="location">Mouse location in Control Space</param>
        /// <param name="data">The data. See <see cref="DragDataText"/> and <see cref="DragDataFiles"/>.</param>
        /// <returns>The drag event result effect.</returns>
        virtual DragDropEffect OnDragDrop(const Float2& location, DragData* data)
        {
            // Clear flag
            _isDragOver = false;
            return DragDropEffect::None;
        }

        /// <summary>
        /// When mouse dragging leaves control's area
        /// </summary>
        virtual void OnDragLeave()
        {
            // Clear flag
            _isDragOver = false;
        }

        /// <summary>
        /// Starts the drag and drop operation.
        /// </summary>
        /// <param name="data">The data.</param>
        virtual void DoDragDrop(DragData* data);

        #pragma endregion

        #pragma region Tooltip

        /// <summary>
        /// Gets the tooltip used by this control (custom or shared one).
        /// </summary>
        ::SE::Tooltip* GetTooltip();

        /// <summary>
        /// Gets a value indicating whether show control tooltip (control is in a proper state, tooltip text is valid, etc.). Can be used to implement custom conditions for showing tooltips (eg. based on current mouse location within the control bounds).
        /// </summary>
        /// <remarks>Tooltip can be only visible if mouse is over the control area (see <see cref="IsMouseOver"/>).</remarks>
    protected:
        virtual bool ShowTooltip();

    public:
        /// <summary>
        /// Links the tooltip.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="customTooltip">The custom tooltip.</param>
        /// <returns>This control pointer. Useful for creating controls in code.</returns>
        Control* LinkTooltip(String text, ::SE::Tooltip* customTooltip = nullptr);

        /// <summary>
        /// Unlinks the tooltip.
        /// </summary>
        void UnlinkTooltip();

        /// <summary>
        /// Called when tooltip wants to be shown. Allows modifying its appearance.
        /// </summary>
        /// <param name="text">The tooltip text to show.</param>
        /// <param name="location">The popup start location (in this control local space).</param>
        /// <param name="area">The allowed area of mouse movement to show tooltip (in this control local space).</param>
        /// <returns>True if can show tooltip, otherwise false to skip.</returns>
        virtual bool OnShowTooltip(String text, Float2& location, Rectangle& area);

        /// <summary>
        /// Called when tooltip gets created and shown for this control. Can be used to customize tooltip UI.
        /// </summary>
        /// <param name="tooltip">The tooltip.</param>
        virtual void OnTooltipShown(::SE::Tooltip* tooltip)
        {
        }

        /// <summary>
        /// Called when tooltip is visible and tests if the given mouse location (in control space) is valid (is over the content).
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>True if tooltip can be still visible, otherwise false.</returns>
        virtual bool OnTestTooltipOverControl(Float2 location);

        #pragma endregion

        #pragma region Helper Functions

        /// <summary>
        /// Performs a raycast against UI controls hierarchy to find any intersecting control content. Uses <see cref="ContainsPoint"/> with precise check (skips transparent surfaces and empty panels).
        /// </summary>
        /// <param name="location">The position to intersect UI with.</param>
        /// <param name="hit">The result control that intersects with the raycast.</param>
        /// <returns>True if ray hits any matching control, otherwise false.</returns>
        virtual bool RayCast(Float2& location, Control*& hit);

        /// <summary>
        /// Checks if given location point in Parent Space intersects with the control content and calculates local position.
        /// </summary>
        /// <param name="locationParent">The location in Parent Space.</param>
        /// <param name="location">The location of intersection in Control Space.</param>
        /// <returns>True if given point in Parent Space intersects with this control content, otherwise false.</returns>
        virtual bool IntersectsContent(Float2& locationParent, Float2& location);

        /// <summary>
        /// Checks if this control contains given point in local Control Space.
        /// </summary>
        /// <param name="location">Point location in Control Space to check</param>
        /// <param name="precise">True if perform precise intersection test against the control content (eg. with hit mask or transparency threshold). Otherwise, only simple bounds-check will be performed.</param>
        /// <returns>True if point is inside control's area, otherwise false.</returns>
        virtual bool ContainsPoint(Float2& location, bool precise = false);

        /// <summary>
        /// Converts point in local control's space into one of the parent control coordinates
        /// </summary>
        /// <param name="parent">This control parent of any other parent.</param>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in parent control coordinates</returns>
        Float2 PointToParent(ContainerControl* parent, Float2 location);

        /// <summary>
        /// Converts point in local control's space into parent control coordinates.
        /// </summary>
        /// <param name="location">The input location of the point to convert.</param>
        /// <returns>The converted point location in parent control coordinates.</returns>
        virtual Float2 PointToParent(Float2 location);


        /// <summary>
        /// Converts point in parent control coordinates into local control's space.
        /// </summary>
        /// <param name="locationParent">The input location of the point to convert.</param>
        /// <returns>The converted point location in control's space.</returns>
        virtual Float2 PointFromParent(Float2 locationParent);


        /// <summary>
        /// Converts point in one of the parent control coordinates into local control's space.
        /// </summary>
        /// <param name="parent">This control parent of any other parent.</param>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>The converted point location in control's space.</returns>
        Float2 PointFromParent(ContainerControl* parent, Float2 location);

        /// <summary>
        /// Converts point in local control's space into window coordinates
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in window coordinates</returns>
        Float2 PointToWindow(Float2 location);

        /// <summary>
        /// Converts point in the window coordinates into control's space
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in control's space</returns>
        Float2 PointFromWindow(Float2 location);

        /// <summary>
        /// Converts point in the local control's space into screen coordinates
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in screen coordinates</returns>
        virtual Float2 PointToScreen(Float2 location);

        /// <summary>
        /// Converts point in screen coordinates into the local control's space
        /// </summary>
        /// <param name="location">Input location of the point to convert</param>
        /// <returns>Converted point location in local control's space</returns>
        virtual Float2 PointFromScreen(Float2 location);

#if FLAX_EDITOR
        /// <summary>
        /// Bounds rectangle for editor UI.
        /// </summary>
        virtual Rectangle EditorBounds => new Rectangle(Float2.Zero, _bounds.Size);
#endif

        #pragma endregion

        #pragma region Control Action

    protected:
        /// <summary>
        /// Called when control location gets changed.
        /// </summary>
        virtual void OnLocationChanged();

        /// <summary>
        /// Called when control size gets changed.
        /// </summary>
        virtual void OnSizeChanged();

        /// <summary>
        /// Sets the scale and updates the transform.
        /// </summary>
        /// <param name="scale">The scale.</param>
        virtual void SetScaleInternal(Float2 scale);

        /// <summary>
        /// Sets the pivot and updates the transform.
        /// </summary>
        /// <param name="pivot">The pivot.</param>
        virtual void SetPivotInternal(Float2 pivot);

        /// <summary>
        /// Sets the shear and updates the transform.
        /// </summary>
        /// <param name="shear">The shear.</param>
        virtual void SetShearInternal(Float2 shear);

        /// <summary>
        /// Sets the rotation angle and updates the transform.
        /// </summary>
        /// <param name="rotation">The rotation (in degrees).</param>
        virtual void SetRotationInternal(float rotation);

        /// <summary>
        /// Called when visible state gets changed.
        /// </summary>
        virtual void OnVisibleChanged();

        /// <summary>
        /// Action fired when parent control gets changed.
        /// </summary>
        virtual void OnParentChangedInternal();

        /// <summary>
        /// Adds the custom control logic update callbacks to the root.
        /// </summary>
        /// <param name="root">The root.</param>
        virtual void AddUpdateCallbacks(RootControl* root);

        /// <summary>
        /// Removes the custom control logic update callbacks from the root.
        /// </summary>
        /// <param name="root">The root.</param>
        virtual void RemoveUpdateCallbacks(RootControl* root);

        /// <summary>
        /// Helper utility function to sets the update callback to the root. Does nothing if value has not been modified. Handles if control has no root or parent.
        /// </summary>
        /// <param name="onUpdate">The cached update callback delegate (field in the custom control implementation).</param>
        /// <param name="value">The value to assign.</param>
        void SetUpdate(Delegate<float>* onUpdate, Delegate<float>* value);

    public:

        /// <summary>
        /// Caches the root control handle.
        /// </summary>
        virtual void CacheRootHandle();

        /// <summary>
        /// Action fired when parent control gets resized (also when control gets non-null parent).
        /// </summary>
        virtual void OnParentResized();

        /// <summary>
        /// Method called when managed instance should be destroyed
        /// </summary>
        virtual void OnDestroy();

        #pragma endregion

        /// <inheritdoc />
        /*int CompareTo(object obj)
        {
            if (obj is Control c)
                return Compare(c);
            return 0;
        }*/

        /// <summary> 
        /// Compares this control with the other control.
        /// </summary>
        /// <param name="other">The other.</param>
        virtual int Compare(const Control* other) const
        {
            return (int)(Y - other->Y);
        }

    protected:
        virtual CursorType __GetCursor();
        virtual void __SetCursor(CursorType type);

        ContainerControl* __GetParent();
        void __SetParent(ContainerControl* control);

        int __GetIndexInParent();
        void __SetIndexInParent(int index);

        AnchorPresets __GetAnchorPreset();
        void __SetAnchorPreset(AnchorPresets presets);

        bool __GetEnabled();
        void __SetEnabled(bool value);

        bool __GetVisible();
        void __SetVisible(bool value);

        bool __GetIsDisposing();

        virtual RootControl* __GetRoot();

        virtual bool __GetIsMouseOver();

        bool __GetIsFocused();

        bool __GetIsNavFocused();

        String& __GetName();
        void __SetName(String& value);

        #pragma region Bounds

        float __GetX();
        void __SetX(float value);

        float __GetY();
        void __SetY(float value);

        float __GetLocalX();
        void __SetLocalX(float value);

        float __GetLocalY();
        void __SetLocalY(float value);

        Float2& __GetAnchorMin();
        void __SetAnchorMin(Float2 &value);

        Float2& __GetAnchorMax();
        void __SetAnchorMax(Float2 &value);

        Margin& __GetOffsets();
        void __SetOffsets(Margin &value);

        Float2 __GetLocation();
        void __SetLocation(Float2 value);

        Float2 __GetLocalLocation();
        void __SetLocalLocation(Float2 value);

        bool __GetPivotRelative();
        void __SetPivotRelative(bool value);

        float __GetWidth();
        void __SetWidth(float value);

        float __GetHeight();
        void __SetHeight(float value);

        Float2& __GetSize();
        void __SetSize(Float2 &value);

        float __GetTop();

        float __GetBottom();

        float __GetLeft();

        float __GetRight();

        Float2 __GetUpperLeft();

        Float2 __GetUpperRight();

        Float2 __GetBottomRight();

        Float2 __GetBottomLeft();

        Float2 __GetCenter();
        void __SetCenter(Float2 value);

        Rectangle& __GetBounds() { return m_Bounds; };
        void __SetBounds(Rectangle &value);

        Float2& __GetScale() { return m_Scale; }
        void __SetScale(Float2 &value);

        Float2& __GetPivot() { return m_Pivot;}
        void __SetPivot(Float2 &value);

        Float2& __GetShear() { return m_Shear; };
        void __SetShear(Float2 &value);

        float __GetRotation() { return m_Rotation; }
        void __SetRotation(float value);

        #pragma endregion

    };
}
