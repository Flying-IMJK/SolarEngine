#pragma once

#include "Editor/GUI/Docking/DockWindow.h"

namespace SE::Editor
{
    class ContentItem;
    class EditorApp;

    /// <summary>
    ///  Base class for all windows in Editor.
    /// </summary>
    SE_CLASS(Reflect)
    class EditorWindow : public DockWindow
    {
        SE_DEFINE_CLASS_DEFAULT(EditorWindow, DockWindow)
    public:
        /// <summary>
        /// Gets the editor object.
        /// </summary>
        const EditorApp* editor;

    protected:
        /// <summary>
        /// Gets a value indicating whether this window can open content finder popup.
        /// </summary>
        virtual bool CanOpenContentFinder() { return true; }

        /// <summary>
        /// Gets a value indicating whether this window can use UI navigation (tab/enter).
        /// </summary>
        virtual bool CanUseNavigation() { return true; }

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="hideOnClose">True if hide window on closing, otherwise it will be destroyed.</param>
        /// <param name="scrollBars">The scroll bars.</param>
        EditorWindow(const EditorApp* editor, bool hideOnClose, ScrollBars scrollBars);


    public:
        /// <summary>
        /// Determines whether this window is holding reference to the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if window is editing the specified item; otherwise, <c>false</c>.</returns>
        virtual bool IsEditingItem(ContentItem* item)
        {
            return false;
        }


        /// <summary>
        /// Called before Editor will enter play mode.
        /// </summary>
        virtual void OnPlayBeginning()
        {
        }

        /// <summary>
        /// Called when Editor is entering play mode.
        /// </summary>
        virtual void OnPlayBegin()
        {
        }

        /// <summary>
        /// Called when Editor leaves the play mode.
        /// </summary>
        virtual void OnPlayEnd()
        {
        }

        /// <summary>
        /// Called when window should be initialized.
        /// At this point, main window, content database, default editor windows are ready.
        /// </summary>
        virtual void OnInit()
        {
        }

        /// <summary>
        /// Called when every engine update.
        /// Note: <see cref="Control.Update"/> may be called at the lower frequency than the engine updates.
        /// </summary>
        virtual void OnUpdate()
        {
        }

        /// <summary>
        /// Called when editor is being closed and window should perform release data operations.
        /// </summary>
        virtual void OnExit()
        {
        }

        /// <summary>
        /// Called when Editor state gets changed.
        /// </summary>
        virtual void OnEditorStateChanged()
        {
        }

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnDestroy() override;
    };
} // SE
