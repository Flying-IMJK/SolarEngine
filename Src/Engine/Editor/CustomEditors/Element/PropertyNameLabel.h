#pragma once
#include "Runtime/UI/GUI/Control.h"
#include "Core/Math/Color.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/String.h"

namespace SE::Editor
{
	class CustomEditor;
	class ContextMenu;

	/// <summary>
	/// Displays custom editor property name with support for context menu and visual feedback.
	/// </summary>
	class PropertyNameLabel : public Control
	{
		SE_CLASS(PropertyNameLabel, Control)

	public:
		/// <summary>
		/// Helper value used by the PropertiesList to draw property names in a proper area.
		/// </summary>
		int FirstChildControlIndex = 0;

		/// <summary>
		/// The linked custom editor (shows the label property).
		/// </summary>
		CustomEditor* LinkedEditor = nullptr;

		/// <summary>
		/// The highlight strip color drawn on a side (transparent if skip rendering).
		/// </summary>
		Color HighlightStripColor = Colors::Transparent;

		/// <summary>
		/// Event fired when right-click context menu is requested.
		/// </summary>
		Delegate<PropertyNameLabel*, ContextMenu*, CustomEditor*> SetupContextMenu;

		/// <summary>
		/// Initializes a new instance of the PropertyNameLabel class.
		/// </summary>
		/// <param name="name">The property name to display.</param>
		PropertyNameLabel(const String& name);

		~PropertyNameLabel() override = default;

		/// <summary>
		/// Links this label to a custom editor.
		/// </summary>
		/// <param name="editor">The editor to link.</param>
		void LinkEditor(CustomEditor* editor);

		// Control overrides
		void Draw() override;
		void OnMouseLeave() override;
		bool OnMouseDown(const Float2& location, MouseButton button) override;
		bool OnMouseUp(const Float2& location, MouseButton button) override;
		void OnLostFocus() override;

	protected:
		/// <summary>
		/// Shows the context menu at the specified location.
		/// </summary>
		/// <param name="location">The menu location.</param>
		virtual void ShowContextMenu(const Float2& location);

	private:
		bool m_mouseDown = false;
	};

	/// <summary>
	/// Clickable property name label that fires mouse events.
	/// </summary>
	class ClickablePropertyNameLabel : public PropertyNameLabel
	{
		SE_CLASS(ClickablePropertyNameLabel, PropertyNameLabel)

	public:
		/// <summary>
		/// Mouse action delegate.
		/// </summary>
		using MouseDelegate = Delegate<ClickablePropertyNameLabel*, const Float2&>;

		/// <summary>
		/// The mouse left button clicks on the label.
		/// </summary>
		MouseDelegate MouseLeftClick;

		/// <summary>
		/// The mouse right button clicks on the label.
		/// </summary>
		MouseDelegate MouseRightClick;

		/// <summary>
		/// The mouse left button double clicks on the label.
		/// </summary>
		MouseDelegate MouseLeftDoubleClick;

		/// <summary>
		/// The mouse right button double clicks on the label.
		/// </summary>
		MouseDelegate MouseRightDoubleClick;

		ClickablePropertyNameLabel(const String& name);
		~ClickablePropertyNameLabel() override = default;

		// Control overrides
		bool OnMouseUp(const Float2& location, MouseButton button) override;
		bool OnMouseDoubleClick(const Float2& location, MouseButton button) override;
	};

	/// <summary>
	/// Draggable property name label with drag events.
	/// </summary>
	class DraggablePropertyNameLabel : public PropertyNameLabel
	{
		SE_CLASS(DraggablePropertyNameLabel, PropertyNameLabel)

	public:
		/// <summary>
		/// Drag event delegate.
		/// </summary>
		using DragDelegate = Delegate<DraggablePropertyNameLabel*, const Float2&>;

		/// <summary>
		/// Event fired when drag starts.
		/// </summary>
		DragDelegate DragStart;

		/// <summary>
		/// Event fired when drag moves.
		/// </summary>
		DragDelegate DragMove;

		/// <summary>
		/// Event fired when drag ends.
		/// </summary>
		Delegate<DraggablePropertyNameLabel*> DragEnd;

		DraggablePropertyNameLabel(const String& name);
		~DraggablePropertyNameLabel() override = default;

		// Control overrides
		bool OnMouseDown(const Float2& location, MouseButton button) override;
		void OnMouseMove(const Float2& mousePos) override;
		bool OnMouseUp(const Float2& location, MouseButton button) override;

	private:
		bool m_isDragging = false;
		Float2 m_dragStartPos;
	};

	/// <summary>
	/// Checkable property name label with checkbox functionality.
	/// </summary>
	class CheckablePropertyNameLabel : public PropertyNameLabel
	{
		SE_CLASS(CheckablePropertyNameLabel, PropertyNameLabel)

	public:
		/// <summary>
		/// Gets or sets whether the label is checked.
		/// </summary>
		bool Checked = false;

		/// <summary>
		/// Event fired when checked state changes.
		/// </summary>
		Delegate<CheckablePropertyNameLabel*, bool> CheckedChanged;

		CheckablePropertyNameLabel(const String& name);
		~CheckablePropertyNameLabel() override = default;

		// Control overrides
		void Draw() override;
		bool OnMouseUp(const Float2& location, MouseButton button) override;
	};

} // namespace SE::Editor
