#pragma once

#include "DockPanel.h"
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/UI/GUI/Panels/Panel.h"

namespace SE::Editor
{
    class ContextMenu;
    class DockPanel;
    class MasterDockPanel;

    /// <summary>
    /// Dockable window UI control.
    /// </summary>
    class DockWindow : public Panel
    {
        friend class MasterDockPanel;
        SE_CLASS_DEFAULT(DockWindow, Panel)
    private:
        String _title;
        Float2 _titleSize;

    protected:
        /// <summary>
        /// The master panel.
        /// </summary>
        MasterDockPanel* _masterPanel;

        /// <summary>
        /// The parent panel.
        /// </summary>
        DockPanel* _dockedTo;

        /// <summary>
        /// Called when window is unlinked from the master panel.
        /// </summary>
        virtual void OnUnlink();

        /// <summary>
        /// Undocks this window.
        /// </summary>
        virtual void Undock();

        /// <summary>
        /// Called when window is closing. Operation can be cancelled.
        /// </summary>
        /// <param name="reason">The reason.</param>
        /// <returns>True if cancel, otherwise false to allow.</returns>
        virtual bool OnClosing(ClosingReason reason);

        /// <summary>
        /// Called when window is closed.
        /// </summary>
        virtual void OnClose()
        {
            // Nothing to do
        }

        /// <summary>
        /// Called when window shows.
        /// </summary>
        virtual void OnShow()
        {
            // Nothing to do
        }

        /*/// <summary>
        /// Serializes splitter panel value into the saved layout.
        /// </summary>
        /// <param name="writer">The Xml writer.</param>
        /// <param name="name">The Xml attribute name to use for value.</param>
        /// <param name="splitter">The splitter panel.</param>
        protected void LayoutSerializeSplitter(XmlWriter writer, String name, SplitPanel* splitter)
        {
            writer.WriteAttributeString(name, splitter.SplitterValue.ToString(CultureInfo.InvariantCulture));
        }*/

        /*/// <summary>
        /// Deserializes splitter panel value from the saved layout.
        /// </summary>
        /// <param name="node">The Xml document node.</param>
        /// <param name="name">The Xml attribute name to use for value.</param>
        /// <param name="splitter">The splitter panel.</param>
        protected void LayoutDeserializeSplitter(XmlElement node, String name, SplitPanel* splitter)
        {
            if (float.TryParse(node.GetAttribute(name), CultureInfo.InvariantCulture, out float value) && value > 0.01f && value < 0.99f)
                splitter.SplitterValue = value;
        }*/

        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;

        virtual bool __GetUseLayoutData() { return false; }

    public:

        /// <summary>
        /// Gets or sets a value indicating whether hide window on close.
        /// </summary>
        bool HideOnClose;

        /// <summary>
        /// Gets the master panel.
        /// </summary>
        PRO_GET(MasterPanel, DockWindow, MasterDockPanel*, __GetMasterPanel);

        /// <summary>
        /// Gets the parent dock panel.
        /// </summary>
        PRO(ParentDockPanel, DockWindow, DockPanel*, __GetParentDockPanel, __SetParentDockPanel);


        /// <summary>
        /// Gets a value indicating whether this window is docked.
        /// </summary>
        PRO_GET(IsDocked, DockWindow, bool, __GetIsDocked);

        /// <summary>
        /// Gets a value indicating whether this window is selected.
        /// </summary>
        PRO_GET(IsSelected, DockWindow, bool, __GetIsSelected);


        /// <summary>
        /// Gets a value indicating whether this window is hidden from the user (eg. not shown or hidden on closed).
        /// </summary>
        PRO_GET(IsHidden, DockWindow, bool, __GetIsHidden);
        
        /// <summary>
        /// Gets the default window size (in UI units, unscaled by DPI which is handled by windowing system).
        /// </summary>
        PRO_GET(DefaultSize, DockWindow, Float2, __GetDefaultSize);


        /// <summary>
        /// Gets the serialization typename.
        /// </summary>
        PRO_GET(SerializationTypename, DockWindow, String, __GetSerializationTypename);
        
        /// <summary>
        /// Gets or sets the window title.
        /// </summary>
        PRO_REF(Title, DockWindow, String, __GetTitle, __SetTitle);
        
        /// <summary>
        /// Gets or sets the window icon
        /// </summary>
        SpriteHandle Icon;

        /// <summary>
        /// Gets the size of the title.
        /// </summary>
        PRO_GET(TitleSize, DockWindow, Float2, __GetTitleSize);
        
        /// <summary>
        /// The input actions collection to processed during user input.
        /// </summary>
        // InputActionsContainer InputActions = new InputActionsContainer();

        /// <summary>
        /// Initializes a new instance of the <see cref="DockWindow"/> class.
        /// </summary>
        /// <param name="masterPanel">The master docking panel.</param>
        /// <param name="hideOnClose">True if hide window on closing, otherwise it will be destroyed.</param>
        /// <param name="scrollBars">The scroll bars.</param>
        DockWindow(MasterDockPanel* masterPanel, bool hideOnClose, ScrollBars scrollBars);

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        void ShowFloating()
        {
            ShowFloating(Float2::Zero);
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        /// <param name="position">Window location.</param>
        void ShowFloating(WindowStartPosition position)
        {
            ShowFloating(Float2::Zero, position);
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        /// <param name="size">Window size, set <see cref="Float2.Zero"/> to use default.</param>
        /// <param name="position">Window location.</param>
        void ShowFloating(Float2 size, WindowStartPosition position = WindowStartPosition::CenterParent)
        {
            ShowFloating(Float2(200, 200), size, position);
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        /// <param name="location">Window location.</param>
        /// <param name="size">Window size, set <see cref="Float2.Zero"/> to use default.</param>
        /// <param name="position">Window location.</param>
        void ShowFloating(Float2 location, Float2 size, WindowStartPosition position = WindowStartPosition::CenterParent);

        /// <summary>
        /// Shows the window.
        /// </summary>
        /// <param name="state">Initial window state.</param>
        /// <param name="toDock">Panel to dock to it.</param>
        /// <param name="autoSelect">Only used if <paramref name="toDock"/> is set. If true the window will be selected after docking it.</param>
        /// <param name="splitterValue">Only used if <paramref name="toDock"/> is set. The splitter value to use. If not specified, a default value will be used.</param>
        void Show(DockState state = DockState::Float, DockPanel* toDock = nullptr, bool autoSelect = true, float splitterValue = 0);

        /// <summary>
        /// Shows the window.
        /// </summary>
        /// <param name="state">Initial window state.</param>
        /// <param name="toDock">Window to dock to it.</param>
        void Show(DockState state, DockWindow* toDock);

        /// <summary>
        /// Focuses or shows the window.
        /// </summary>
        void FocusOrShow();

        /// <summary>
        /// Focuses or shows the window.
        /// </summary>
        /// <param name="state">The state.</param>
        void FocusOrShow(DockState state);

        /// <summary>
        /// Hides the window.
        /// </summary>
        void Hide();

        /// <summary>
        /// Closes the window.
        /// </summary>
        /// <param name="reason">Window closing reason.</param>
        /// <returns>True if action has been cancelled (due to window internal logic).</returns>
        bool Close(ClosingReason reason = ClosingReason::CloseEvent);

        /// <summary>
        /// Selects this tab page.
        /// </summary>
        /// <param name="autoFocus">True if focus tab after selection change.</param>
        void SelectTab(bool autoFocus = true);

        /// <summary>
        /// Brings the window to the front of the Z order.
        /// </summary>
        void BringToFront();

        
        /// <summary>
        /// Gets a value indicating whether window uses custom layout data.
        /// </summary>
        PRO_GET(UseLayoutData, DockWindow, bool, __GetUseLayoutData);


        /// <summary>
        /// Called when during windows layout serialization. Each window can use it to store custom interface data (eg. splitter position).
        /// </summary>
        /// <param name="writer">The Xml writer.</param>
        /*virtual void OnLayoutSerialize(XmlWriter writer)
        {
        }*/

        /// <summary>
        /// Called when during windows layout deserialization. Each window can use it to load custom interface data (eg. splitter position).
        /// </summary>
        /// <param name="node">The Xml document node.</param>
        /*virtual void OnLayoutDeserialize(XmlElement node)
        {
        }*/

        /// <summary>
        /// Called when during windows layout deserialization if window has no layout data to load. Can be used to restore default UI layout.
        /// </summary>
        virtual void OnLayoutDeserialize()
        {
        }

        /// <inheritdoc />
        void OnDestroy() override;

        /// <inheritdoc />
        void Focus() override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <summary>
        /// Called when dock panel wants to show the context menu for this window. Can be used to inject custom buttons and items to the context menu (on top).
        /// </summary>
        /// <param name="menu">The menu.</param>
        virtual void OnShowContextMenu(ContextMenu* menu)
        {
        }

    private:

        MasterDockPanel* __GetMasterPanel() { return  _masterPanel; }

        DockPanel* __GetParentDockPanel() { return _dockedTo; }
        void __SetParentDockPanel(DockPanel* value) { _dockedTo = value; }

        bool __GetIsDocked() { return _dockedTo != nullptr; }
        bool __GetIsSelected();
        bool __GetIsHidden() { return !Visible || _dockedTo == nullptr; }
        virtual Float2 __GetDefaultSize() { return Float2(900, 580); }
        virtual String __GetSerializationTypename();
        String& __GetTitle() { return _title; }
        void __SetTitle(String &value);
        Float2 __GetTitleSize() { return _titleSize; }
    };

} // SE

