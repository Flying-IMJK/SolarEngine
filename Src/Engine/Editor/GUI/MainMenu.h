#pragma once

#include "Core/Types/Delegate.h"
#include "Runtime/UI/GUI/ContainerControl.h"


namespace SE
{
	class Image;
	class GraphicWindow;
    class Button;
	class Label;
}

namespace SE::Editor
{
    class MainMenuButton;

    class MainMenu : public ContainerControl
    {
	    SE_CLASS_DEFAULT(MainMenu, ContainerControl)
	private:
#if PLATFORM_WINDOWS
    	Function<void(const Float2&, WindowHitCodes&, bool&)> m_hitTestCall;
        bool _useCustomWindowSystem = false;
        Image* _icon = nullptr;
        Label* _title = nullptr;
        Button* _closeButton = nullptr;
        Button* _minimizeButton = nullptr;
        Button* _maximizeButton = nullptr;
        GraphicWindow* _window = nullptr;
#endif
        MainMenuButton* _selected = nullptr;

	    void OnSelectedContextMenuVisibleChanged(Control* control);

	    /// <summary>
	    /// Return the rightmost button.
	    /// </summary>
	    /// <returns>Rightmost button, null if there is no <see cref="MainMenuButton"/></returns>
	    MainMenuButton* GetRightButton();

#if PLATFORM_WINDOWS
	    void OnWindowClosed();

	    WindowHitCodes OnHitTest(Float2 mouse);
#endif
	public:

    	/// <summary>
    	/// Gets or sets the selected button (with opened context menu).
    	/// </summary>
    	PRO(Selected, MainMenu, MainMenuButton*, __GetSelected, __SetSelected);

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenu"/> class.
        /// </summary>
        /// <param name="mainWindow">The main window.</param>
	    MainMenu(RootControl* mainWindow);

#if PLATFORM_WINDOWS
        /// <inheritdoc />
        void Update(float deltaTime) override;
#endif

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>Created button control.</returns>
	    MainMenuButton* AddButton(String text);

        /// <summary>
        /// Gets the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>The button or null if missing.</returns>
	    MainMenuButton* GetButton(String text);

        /// <inheritdoc />
	    bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
	    bool OnKeyDown(KeyboardKeys key) override;

#if PLATFORM_WINDOWS
        /// <inheritdoc />
	    void OnDestroy() override;
#endif

	protected:
	    /// <inheritdoc />
		void PerformLayoutAfterChildren() override;

    	MainMenuButton* __GetSelected() { return _selected; }
    	void __SetSelected(MainMenuButton* value);

	};

} // SE
