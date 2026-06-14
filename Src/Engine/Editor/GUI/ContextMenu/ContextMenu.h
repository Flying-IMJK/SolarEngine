#pragma once
#include <set>

#include "ContextMenuBase.h"
#include "fmt/compile.h"
#include "fmt/ranges.h"
#include "Runtime/UI/GUI/Panels/Panel.h"

namespace SE::Editor
{
	class ContextMenuSeparator;
	class ContextMenuChildMenu;
	class ContextMenuItem;
    class ContextMenuButton;

	class ContextMenu : public ContextMenuBase
    {
		SE_DEFINE_CLASS(ContextMenu, ContextMenuBase)
	protected:
	    /// <summary>
	    /// The items container.
	    /// </summary>
	    /// <seealso cref="FlaxEngine.GUI.Panel" />
	    class ItemsPanel : public Panel
        {
	    private:
	        ContextMenu* _menu;

	    public:
            /// <summary>
            /// Initializes a new instance of the <see cref="ItemsPanel"/> class.
            /// </summary>
            /// <param name="menu">The menu.</param>
	        ItemsPanel(ContextMenu* menu);

	    protected:
            /// <inheritdoc />
	        void Arrange() override;
        };

        /// <summary>
        /// The items area margin.
        /// </summary>
        Margin _itemsAreaMargin = Margin(0, 0, 3, 3);

        /// <summary>
        /// The items margin.
        /// </summary>
        Margin _itemsMargin = Margin(16, 0, 2, 0);

        /// <summary>
        /// The items panel.
        /// </summary>
        ItemsPanel* _panel;

	public:
        /// <summary>
        /// Gets or sets the items area margin (items container area margin).
        /// </summary>
        PRO_REF(ItemsAreaMargin, ContextMenu, Margin, __GetItemsAreaMargin, __SetItemsAreaMargin);

        /// <summary>
        /// Gets or sets the items margin.
        /// </summary>
	    PRO_REF(ItemsMargin, ContextMenu, Margin, __GetItemsMargin, __SetItemsMargin);

        /// <summary>
        /// Gets or sets the minimum popup width.
        /// </summary>
        float MinimumWidth;

        /// <summary>
        /// Gets or sets the maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.
        /// </summary>
        int MaximumItemsInViewCount;

        /// <summary>
        /// Gets the items (readonly).
        /// </summary>
	    PRO_GET(Items, ContextMenu, List<ContextMenuItem*>, __GetItems);

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        Delegate<ContextMenuButton*> ButtonClicked;

        /// <summary>
        /// Gets the context menu items container control.
        /// </summary>
	    PRO_GET(ItemsContainer, ContextMenu, Panel*, __GetItemsContainer);

        /// <summary>
        /// The auto sort property.
        /// </summary>
	    PRO(AutoSort, ContextMenu, bool, __GetAutoSort, __SetAutoSort);

        /// <summary>
        /// Sorts all <see cref="ContextMenuButton"/> alphabetically.
        /// </summary>
        /// <param name="force">Overrides <see cref="AutoSort"/> property.</param>
        void SortButtons(bool force = false);

        /// <summary>
        /// Removes all the added items (buttons, separators, etc.).
        /// </summary>
        void DisposeAllItems();

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuButton* AddButton(String text);


        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The short keys.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuButton* AddButton(String text, String shortKeys);

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuButton* AddButton(String text, Function<void()> clicked);

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
	    ContextMenuButton* AddButton(String text, Function<void(ContextMenuButton*)> clicked);

	    /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The shortKeys.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuButton* AddButton(String text, String shortKeys, Function<void()> clicked);

        /*/// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="binding">The input binding.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuButton* AddButton(String text, InputBinding binding, Action clicked)
        {
            var item = new ContextMenuButton(this, text, binding.ToString())
            {
                Parent = _panel
            };
            item.Clicked += clicked;
            SortButtons();
            return item;
        }*/

        /// <summary>
        /// Gets the child menu (with that name).
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control or null if missing.</returns>
        ContextMenuChildMenu* GetChildMenu(String text);

        /// <summary>
        /// Adds the child menu or gets it if already created (with that name).
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuChildMenu* GetOrAddChildMenu(String text);

        /// <summary>
        /// Adds the child menu.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        ContextMenuChildMenu* AddChildMenu(String text);

        /// <summary>
        /// Adds the separator.
        /// </summary>
        /// <returns>Created context menu item control.</returns>
        ContextMenuSeparator* AddSeparator();

        /// <summary>
        /// Called when button get clicked.
        /// </summary>
        /// <param name="button">The button.</param>
        virtual void OnButtonClicked(ContextMenuButton* button);

        /// <inheritdoc />
	    void Show(Control* parent, Float2 location) override;

        /// <inheritdoc />
		bool ContainsPoint(Float2& location, bool precise) override;

        /// <inheritdoc />
	    bool OnCharInput(Char c) override;

        /// <inheritdoc />
	    bool OnKeyDown(KeyboardKeys key) override;

		ContextMenu();

	protected:
	    /// <inheritdoc />
	    void PerformLayoutAfterChildren() override;

	private:
	    /// <summary>
	    /// The auto sort.
	    /// </summary>
	    bool _autosort;

	    Margin& __GetItemsAreaMargin() { return _itemsMargin; }
	    void __SetItemsAreaMargin(Margin &value);

	    Margin& __GetItemsMargin() { return _itemsMargin; }
	    void __SetItemsMargin(Margin &value);

	    List<ContextMenuItem*> __GetItems();

	    bool __GetAutoSort() { return _autosort; }
	    void __SetAutoSort(bool value);

	    Panel* __GetItemsContainer() { return _panel; }
    };

} // SE

