#pragma once
#include "ContextMenuItem.h"
#include "Runtime/Render/2D/SpriteAtlas.h"

namespace SE::Editor
{
	class ContextMenuButton : public ContextMenuItem
	{
		SE_DEFINE_CLASS_DEFAULT(ContextMenuButton, ContextMenuItem)
	private:
		bool _isMouseDown = false;

	public:
		/// <summary>
		/// The amount to adjust the short keys and arrow image by in x coordinates.
		/// </summary>
		float ExtraAdjustmentAmount = 0;

		/// <summary>
		/// Event fired when user clicks on the button.
		/// </summary>
		Action Clicked;

		/// <summary>
		/// Event fired when user clicks on the button.
		/// </summary>
		Delegate<ContextMenuButton*> ButtonClicked;

		/// <summary>
		/// The button text.
		/// </summary>
		String Text;

		/// <summary>
		/// The button short keys information (eg. 'Ctrl+C').
		/// </summary>
		String ShortKeys;

		/// <summary>
		/// Item icon (best is 16x16).
		/// </summary>
		SpriteHandle Icon;

		/// <summary>
		/// The checked state.
		/// </summary>
		bool Checked = false;

		/// <summary>
		/// The automatic check mode.
		/// </summary>
		bool AutoCheck = false;

		/// <summary>
		/// Closes the context menu after clicking the button, otherwise menu will stay open.
		/// </summary>
		bool CloseMenuOnClick = true;

		/// <summary>
		/// Initializes a new instance of the <see cref="ContextMenuButton"/> class.
		/// </summary>
		/// <param name="parent">The parent context menu.</param>
		/// <param name="text">The text.</param>
		/// <param name="shortKeys">The short keys tip.</param>
		ContextMenuButton(ContextMenu* parent, String text, String shortKeys = SE_TEXT(""));

		/// <summary>
		/// Sets the automatic check mode. In auto check mode the button sets the check sprite as an icon when user clicks it.
		/// </summary>
		/// <param name="value">True if use auto check, otherwise false.</param>
		/// <returns>This button.</returns>
		ContextMenuButton* SetAutoCheck(bool value);

		/// <summary>
		/// Sets the checked state.
		/// </summary>
		/// <param name="value">True if check it, otherwise false.</param>
		/// <returns>This button.</returns>
		ContextMenuButton* SetChecked(bool value);

		/// <summary>
		/// Clicks this button.
		/// </summary>
		void Click();

		/// <inheritdoc />
		void Draw() override;

		/// <inheritdoc />
		void OnMouseLeave() override;

		/// <inheritdoc />
		bool OnMouseDown(Float2 location, MouseButton button) override;

		/// <inheritdoc />
		bool OnMouseUp(Float2 location, MouseButton button) override;

		/// <inheritdoc />
		bool OnKeyDown(KeyboardKeys key) override;

		/// <inheritdoc />
		void OnLostFocus() override;

	protected:

		float __GetMinimumWidth() override;
	};
} // SE

