#pragma once
#include "Runtime/Render/2D/FontReference.h"
#include "Runtime/UI/GUI/Control.h"

namespace SE
{
    class IBrush;
}

namespace SE::Editor
{
    class ContextMenuButton;
    class ContextMenu;

    /// <summary>
    /// Combo box control allows to choose one item or set of items from the provided collection of options.
    /// </summary>
    /// <remarks>
    /// Difference between <see cref="ComboBox"/> and <see cref="Dropdown"/> is that ComboBox uses native window to show the items list while Dropdown uses a custom panel added to parent window.
    /// This means that Dropdown will work on all platforms that don't support multiple native windows (eg. Android, PS4, Xbox One).
    /// </remarks>
    SE_CLASS(Reflect)
    class ComboBox : public Control
    {
        SE_DEFINE_CLASS(ComboBox, Control)
    public:
        /// <summary>
        /// The default height of the control.
        /// </summary>
        constexpr static float DefaultHeight = 18.0f;

    protected:
        
        /// <summary>
        /// The popup menu. May be null if has not been used yet.
        /// </summary>
        ContextMenu* _popupMenu;

        /// <summary>
        /// The mouse down flag.
        /// </summary>
        bool _mouseDown;

        /// <summary>
        /// The block popup flag.
        /// </summary>
        bool _blockPopup;

        /// <summary>
        /// The selected indices.
        /// </summary>
        List<int> _selectedIndices = List<int>(4);

    public:
        /// <summary>
        /// The items.
        /// </summary>
        List<String> items = List<String>();
        
        /// <summary>
        /// The item tooltips (optional).
        /// </summary>
        List<String> tooltips;

        /// <summary>
        /// True if sort items before showing the list, otherwise present them in the unchanged order.
        /// </summary>
        bool Sorted;

        /// <summary>
        /// Gets or sets a value indicating whether support multi items selection.
        /// </summary>
        bool SupportMultiSelect;

        /// <summary>
        /// Gets or sets the maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.
        /// </summary>
        int MaximumItemsInViewCount;

        /// <summary>
        /// Gets or sets the selected item (returns <see cref="string.Empty"/> if no item is being selected or more than one item is selected).
        /// </summary>
        PRO_REF(SelectedItem, ComboBox, String, __GetSelectedItem, __SetSelectedItem);

        /// <summary>
        /// Gets a value indicating whether this combobox has any item selected.
        /// </summary>
        PRO_GET(HasSelection, ComboBox, bool, __GetHasSelection);

        /// <summary>
        /// Gets or sets the index of the selected. If combobox has more than 1 item selected then it returns invalid index (value -1).
        /// </summary>
        PRO(SelectedIndex, ComboBox, int, __GetSelectedIndex, __SetSelectedIndex);

        /// <summary>
        /// Gets or sets the selection.
        /// </summary>
        PRO_REF(Selection, ComboBox, List<int>, __GetSelection, __SetSelection);

        /// <summary>
        /// Event fired when selected index gets changed.
        /// </summary>
        Delegate<ComboBox*> SelectedIndexChanged;

        /// <summary>
        /// Occurs when popup is showing (before event). Can be used to update items collection before showing it to the user.
        /// </summary>
        Delegate<ComboBox*> PopupShowing;

        /// <summary>
        /// Custom popup creation function.
        /// </summary>
        Function<ContextMenu*(ComboBox*)> PopupCreate;

        /// <summary>
        /// Gets the popup menu (it may be null if not used - lazy init).
        /// </summary>
        PRO_GET(Popup, ComboBox, ContextMenu*, __GetPopup);

        /// <summary>
        /// Gets a value indicating whether this popup menu is opened.
        /// </summary>
        PRO_GET(IsPopupOpened, ComboBox, bool, __GetIsPopupOpened);

        /// <summary>
        /// Gets or sets the font used to draw text.
        /// </summary>
        FontReference Font;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        Color TextColor;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        Color BorderColor;

        /// <summary>
        /// Gets or sets the background color when combobox popup is opened.
        /// </summary>
        Color BackgroundColorSelected;

        /// <summary>
        /// Gets or sets the border color when combobox popup is opened.
        /// </summary>
        Color BorderColorSelected;

        /// <summary>
        /// Gets or sets the background color when combobox is highlighted.
        /// </summary>
        Color BackgroundColorHighlighted;

        /// <summary>
        /// Gets or sets the border color when combobox is highlighted.
        /// </summary>
        Color BorderColorHighlighted;

        /// <summary>
        /// Gets or sets the image used to render combobox drop arrow icon.
        /// </summary>
        IBrush* ArrowImage;

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon.
        /// </summary>
        Color ArrowColor;

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon (menu is opened).
        /// </summary>
        Color ArrowColorSelected;

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon (menu is highlighted).
        /// </summary>
        Color ArrowColorHighlighted;

        ComboBox();

        /// <summary>
        /// Initializes a new instance of the <see cref="ComboBox"/> class.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="width">The width.</param>
        ComboBox(float x, float y, float width = 120.0f);

        /// <summary>
        /// Clears the items.
        /// </summary>
        void ClearItems();

        /// <summary>
        /// Adds the item.
        /// </summary>
        /// <param name="item">The item.</param>
        void AddItem(String& item);

        /// <summary>
        /// Adds the items.
        /// </summary>
        /// <param name="items">The items.</param>
        void AddItems(List<String>& items);

        /// <summary>
        /// Sets the items.
        /// </summary>
        /// <param name="items">The items.</param>
        void SetItems(List<String>& items);

        /// <summary>
        /// Determines whether the specified item is selected.
        /// </summary>
        /// <param name="item">The item to check.</param>
        /// <returns><c>true</c> if the item is selected; otherwise, <c>false</c>.</returns>
        bool IsSelected(String& item);

        /// <summary>
        /// Determines whether the item at the specified index is selected.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns><c>true</c> if the item is selected; otherwise, <c>false</c>.</returns>
        bool IsSelected(int index);

        /// <inheritdoc />
        void OnDestroy() override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void OnLostFocus() override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnSubmit() override;

    protected:

        /// <summary>
        /// Called when selected item index gets changed.
        /// </summary>
        virtual void OnSelectedIndexChanged();

        /// <summary>
        /// Called when item is clicked.
        /// </summary>
        /// <param name="index">The index.</param>
        virtual void OnItemClicked(int index)
        {
            if (SupportMultiSelect)
            {
                if (_selectedIndices.Contains(index))
                    _selectedIndices.Remove(index);
                else
                    _selectedIndices.Add(index);
                OnSelectedIndexChanged();
            }
            else
            {
                SelectedIndex = index;
            }
        }

        /// <summary>
        /// Shows the context menu popup.
        /// </summary>
        void ShowPopup();

        /// <summary>
        /// Called when button is created or updated. Can be used to customize the visuals.
        /// </summary>
        /// <param name="button">The button.</param>
        /// <param name="index">The item index.</param>
        /// <param name="construct">true if button is created else it is repainting the button</param>
        virtual void OnLayoutMenuButton(ContextMenuButton* button, int index, bool construct = false);

        /// <summary>
        /// Creates the popup menu.
        /// </summary>
        virtual ContextMenu* OnCreatePopup();

    private:
        /// <summary>
        /// Updates buttons layout.
        /// </summary>
        void UpdateButtons();

        String& __GetSelectedItem()
        {
            return _selectedIndices.Count() == 1 ? items[_selectedIndices[0]] : String::Empty;
        }
        void __SetSelectedItem(String& value)
        {
            SelectedIndex = items.Find(value);
        }

        bool __GetHasSelection() { return _selectedIndices.Count() != 0; }

        int __GetSelectedIndex()
        {
            return _selectedIndices.Count() == 1 ? _selectedIndices[0] : -1;
        }
        void __SetSelectedIndex(int value);

        List<int>& __GetSelection()
        {
            return _selectedIndices;
        }

        void __SetSelection(List<int>& value);

        bool __GetIsPopupOpened();

        ContextMenu* __GetPopup() { return _popupMenu; }
        
    };
} // SE

