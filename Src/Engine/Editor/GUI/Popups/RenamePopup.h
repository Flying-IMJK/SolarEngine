#pragma once

#include "Editor/GUI/ContextMenu/ContextMenuBase.h"

namespace SE
{
    class TextBox;
}

namespace SE::Editor
{
    /// <summary>
    /// Popup menu useful for renaming objects via UI. Displays text box for renaming.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    class RenamePopup : public ContextMenuBase
    {
        /// <summary>
        /// Input value validation delegate.
        /// </summary>
        /// <param name="popup">The popup reference.</param>
        /// <param name="value">The input text value.</param>
        /// <returns>True if text is valid, otherwise false.</returns>
        using ValidateFunc = Function<bool(RenamePopup*, String&)>;

    private:
        TextBox* _inputField;

    public:
        /// <summary>
        /// Occurs when renaming is done.
        /// </summary>
        Function<void(RenamePopup*)> Renamed;

        /// <summary>
        /// Occurs when popup is closing (after renaming done or not).
        /// </summary>
        Function<void(RenamePopup*)> Closed;

        /// <summary>
        /// Occurs when input text validation should be performed.
        /// </summary>
        ValidateFunc Validate;

        /// <summary>
        /// Gets or sets the initial value.
        /// </summary>
        String InitialValue;

        /// <summary>
        /// Gets or sets the input field text.
        /// </summary>
        PRO_REF(Text, RenamePopup, String, __GetText, __SetText);

        /// <summary>
        /// Gets the text input field control.
        /// </summary>
        PRO_GET(InputField, RenamePopup, TextBox*, __GetInputField);

        /// <summary>
        /// Initializes a new instance of the <see cref="RenamePopup"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="size">The size.</param>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        RenamePopup(String& value, Float2 size, bool isMultiline);

        /// <inheritdoc />
        void Update(float deltaTime) override;


        /// <summary>
        /// Shows the rename popup.
        /// </summary>
        /// <param name="control">The target control.</param>
        /// <param name="area">The target control area to cover.</param>
        /// <param name="value">The initial value.</param>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <returns>Created popup.</returns>
        static RenamePopup* ShowPopup(Control* control, Rectangle area, String& value, bool isMultiline);

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnDestroy() override;
        
    protected:
        /// <inheritdoc />
        bool UseAutomaticDirectionFix() override {  return false; }

        /// <inheritdoc />
        void OnShow() override;

        /// <inheritdoc />
        void OnHide() override;

    private:
        void OnEnd();

        void OnTextChanged();

        bool GetIsInputValid();

        String& __GetText();
        void __SetText(String& value);

        TextBox* __GetInputField() { return _inputField; }

    };

} // SE
