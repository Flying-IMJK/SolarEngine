
#include "ContentWindow.h"

#include "Core/Types/Collections/Sorting.h"
#include "Core/Platform/MessageBox.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/TypeSystem/Info/TypeEnumInfo.h"
#include "Editor/EditorApp.h"
#include "Editor/EditorIcons.h"
#include "Editor/Core/QueryFilterHelper.h"
#include "Editor/GUI/NavigationBar.h"
#include "Editor/GUI/ToolStrip.h"
#include "editor/gui/toolstripbutton.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Editor/GUI/ContextMenu/ContextMenuChildMenu.h"
#include "Editor/GUI/Input/FloatValueBox.h"
#include "Editor/GUI/Input/SearchBox.h"
#include "Editor/GUI/Popups/RenamePopup.h"
#include "Editor/GUI/Tree/Tree.h"
#include "Editor/Modules/AssetDatabaseModule.h"
#include "Editor/Modules/AssetImportingModule.h"
#include "Editor/Modules/SceneModule.h"
#include "Editor/Resource/ContentNavigationButton.h"
#include "Editor/Resource/ContentView.h"
#include "Editor/Resource/Items/BinaryAssetItem.h"
#include "Editor/Resource/Items/ContentFolder.h"
#include "Editor/Resource/Items/ContentItem.h"
#include "Editor/Resource/Items/FileItem.h"
#include "Editor/Resource/Items/NewItem.h"
#include "Editor/Resource/Tree/ContentTreeNode.h"
#include "editor/resource/tree/projecttreenode.h"
#include "Runtime/Level/Scene/SceneAsset.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/UI/GUI/Brushes/IBrush.h"
#include "Runtime/UI/GUI/Panels/SplitPanel.h"

namespace SE::Editor
{
	List<ContentTreeNode*> ContentWindow::NavUpdateCache = List<ContentTreeNode*>(8);

	ContentWindow::ContentWindow()	: EditorWindow(&EditorApp::Ins(), true, ScrollBars::None)
	{
	    Title = SE_TEXT("Content");
	    Icon = EditorIcons::Folder32;
	    // FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);

	    // Content database events
	    editor->databaseModule->WorkspaceModified.Bind([this]
	    {
		    _isWorkspaceDirty = true;
	    });
	    editor->databaseModule->ItemRemoved.Bind<ContentWindow, &ContentWindow::OnContentDatabaseItemRemoved>(this);
	    editor->databaseModule->WorkspaceRebuilding.Bind([this]
	    {
	    	if (SelectedNode != nullptr)
	    	{
	    		_workspaceRebuildLocation = SelectedNode->GetPath();
	    	}
	    });
	    editor->databaseModule->WorkspaceRebuilt.Bind([this]
        {
            ContentItem* selected = editor->databaseModule->Find(_workspaceRebuildLocation);
	    	ContentFolder* selectedFolder;
            if (TypeTryCast<ContentFolder>(selected, selectedFolder))
            {
                m_NavigationUnlocked = false;
                RefreshView(selectedFolder->Node);
                m_Tree->Select(selectedFolder->Node);
                UpdateItemsSearch();
                m_NavigationUnlocked = true;
                UpdateUI();
            }
            else
                ShowRoot();
        });

	    /*var options = Editor.Options;
	    options.OptionsChanged += OnOptionsChanged;*/

	    // Toolstrip
	    m_ToolStrip = New<ToolStrip>(34.0f);
		m_ToolStrip->Parent = this;

	    m_ImportButton = m_ToolStrip->AddButton(EditorIcons::Import64, []()
	    {
		    //EditorApp::GetInstance().importingModule->I // Editor.ContentImporting.ShowImportFileDialog(CurrentViewFolder)).LinkTooltip("Import content");
	    });
	    m_ToolStrip->AddSeparator();
	    m_NavigateBackwardButton = m_ToolStrip->AddButton(EditorIcons::Left64, CreateFunc<ContentWindow, &ContentWindow::NavigateBackward>(this));
		m_NavigateBackwardButton->LinkTooltip(SE_TEXT("Navigate backward"));

	    m_NavigateForwardButton = m_ToolStrip->AddButton(EditorIcons::Right64, CreateFunc<ContentWindow, &ContentWindow::NavigateForward>(this));
		m_NavigateForwardButton->LinkTooltip(SE_TEXT("Navigate forward"));

	    m_NavigateUpButton = m_ToolStrip->AddButton(EditorIcons::Up64, CreateFunc<ContentWindow, &ContentWindow::NavigateUp>(this));
		m_NavigateUpButton->LinkTooltip(SE_TEXT("Navigate up"));

	    m_ToolStrip->AddSeparator();

	    // Navigation bar
	    m_NavigationBar = New<NavigationBar>();
		m_NavigationBar->Parent = m_ToolStrip;

	    // Split panel
	    m_Split = New<SplitPanel>(Orientation::Horizontal, ScrollBars::None, ScrollBars::None);
		m_Split->AnchorPreset = AnchorPresets::StretchAll;
		m_Split->Offsets = Margin(0, 0, m_ToolStrip->Bottom, 0);
		m_Split->SplitterValue = 0.2f;
		m_Split->Parent = this;

	    // Content structure tree searching query input box
	    ContainerControl* headerPanel = New<ContainerControl>();
		headerPanel->AnchorPreset = AnchorPresets::HorizontalStretchTop,
		headerPanel->BackgroundColor = Style::Current->Background,
		headerPanel->IsScrollable = false,
		headerPanel->Offsets = Margin(0, 0, 0, 18 + 6),

	    m_FoldersSearchBox = New<SearchBox>();
		m_FoldersSearchBox->AnchorPreset = AnchorPresets::HorizontalStretchMiddle,
		m_FoldersSearchBox->Parent = headerPanel,
		m_FoldersSearchBox->Bounds = Rectangle(4, 4, headerPanel->Width - 8, 18),
	    m_FoldersSearchBox->TextChanged.Bind<ContentWindow, &ContentWindow::OnFoldersSearchBoxTextChanged>(this);

	    // Content tree panel
	    m_ContentTreePanel = New<Panel>();
		m_ContentTreePanel->AnchorPreset = AnchorPresets::StretchAll,
		m_ContentTreePanel->Offsets = Margin(0, 0, headerPanel->Bottom, 0),
		m_ContentTreePanel->IsScrollable = true,
		m_ContentTreePanel->SetScrollBars(ScrollBars::Both);
		m_ContentTreePanel->Parent = m_Split->Panel1,

	    // Content structure tree
	    m_Tree = New<Tree>(false);
		m_Tree->Parent = m_ContentTreePanel;
	    m_Tree->SelectedChanged.Bind<ContentWindow, &ContentWindow::OnTreeSelectionChanged>(this);

	    headerPanel->Parent = m_Split->Panel1;

	    // Content items searching query input box and filters selector
	    ContainerControl* contentItemsSearchPanel = New<ContainerControl>();
		contentItemsSearchPanel->AnchorPreset = AnchorPresets::HorizontalStretchTop;
		contentItemsSearchPanel->IsScrollable = true;
		contentItemsSearchPanel->Offsets = Margin(0, 0, 0, 18 + 8);
		contentItemsSearchPanel->Parent = m_Split->Panel2;

	    const float viewDropdownWidth = 50.0f;
	    m_ItemsSearchBox = New<SearchBox>();
        m_ItemsSearchBox->AnchorPreset = AnchorPresets::HorizontalStretchMiddle,
        m_ItemsSearchBox->Parent = contentItemsSearchPanel,
        m_ItemsSearchBox->Bounds = Rectangle(viewDropdownWidth + 8, 4, contentItemsSearchPanel->Width - 12 - viewDropdownWidth, 18),

	    m_ItemsSearchBox->TextChanged.Bind<ContentWindow, &ContentWindow::UpdateItemsSearch>(this);

	    m_ViewDropdown = New<ViewDropdown>();
		m_ViewDropdown->AnchorPreset = AnchorPresets::MiddleLeft;
		m_ViewDropdown->SupportMultiSelect = true;
		m_ViewDropdown->TooltipText = SE_TEXT("Change content view and filter options");
		m_ViewDropdown->Parent = contentItemsSearchPanel;
		m_ViewDropdown->Offsets = Margin(4, viewDropdownWidth, -9, 18);
	    m_ViewDropdown->SelectedIndexChanged.Bind([this](ComboBox* comboBox){ UpdateItemsSearch(); });

		TypeEnumInfo const* enumInfo = Types::GetEnumInfo(Typeof<ContentItemSearchFilter>());

	    for (int i = 0; i <= (int)ContentItemSearchFilter::Other; i++)
	    {
	    	m_ViewDropdown->items.Add(enumInfo->constants[i].id.ToString());
	    }
	    m_ViewDropdown->PopupCreate.Bind<ContentWindow, &ContentWindow::OnViewDropdownPopupCreate>(this);

	    // Content view panel
	    m_ContentViewPanel = New<Panel>();
		m_ContentViewPanel->AnchorPreset = AnchorPresets::StretchAll;
		m_ContentViewPanel->Offsets = Margin(0, 0, contentItemsSearchPanel->Bottom + 4, 0);
		m_ContentViewPanel->IsScrollable = true;
		m_ContentViewPanel->SetScrollBars(ScrollBars::Vertical);
		m_ContentViewPanel->Parent = m_Split->Panel2;

	    // Content View
	    m_View = New<ContentView>();
		m_View->AnchorPreset = AnchorPresets::HorizontalStretchTop;
		m_View->Offsets = Margin(0, 0, 0, 0);
		m_View->IsScrollable = true;
		m_View->Parent = m_ContentViewPanel;
	    m_View->OnOpen.Bind<ContentWindow, &ContentWindow::Open>(this);
	    m_View->OnNavigateBack.Bind<ContentWindow, &ContentWindow::NavigateBackward>(this);
	    m_View->OnRename.Bind<ContentWindow, &ContentWindow::Rename>(this);
	    m_View->OnDelete.Bind<ContentWindow, &ContentWindow::Delete>(this);
	    m_View->OnDuplicate.Bind<ContentWindow, &ContentWindow::Duplicate>(this);
	    m_View->OnPaste.Bind<ContentWindow, &ContentWindow::Paste>(this);

	    /*_view.InputActions.Add(options => options.Search, () => _itemsSearchBox.Focus());
	    InputActions.Add(options => options.Search, () => _itemsSearchBox.Focus());*/
	}

	void ContentWindow::RefreshView()
	{
		if (m_View->IsSearching)
			UpdateItemsSearch();
		else
			RefreshView(SelectedNode);
	}

	void ContentWindow::RefreshView(ContentTreeNode* target)
	{
		m_View->IsSearching = false;
		if (target == _root)
		{
			// Special case for root folder
			List<ContentItem*> items = List<ContentItem*>(8);
			for (int i = 0; i < _root->ChildrenCount(); i++)
			{
				ContentTreeNode* node;
				if (TypeTryCast<ContentTreeNode>(_root->GetChild(i), node))
				{
					items.Add(node->GetFolder());
				}
			}
			m_View->ShowItems(items, _sortType, false, true);
		}
		else if (target != nullptr)
		{
			// Show folder contents
			List<ContentItem*>& items = target->GetFolder()->Children;
			if (!_showAllFiles)
			{
				List<ContentItem*> result;
				ListExtensions::Where(items, CreateFunc([](ContentItem* const & x)
				{
					return !TypeAs<FileItem>(x);
				}), result);

				items = MoveTemp(result);
			}
			if (!_showGeneratedFiles)
				// items = items.Where(x => !(x.Path.EndsWith(".Gen.cs", StringComparison.Ordinal) || x.Path.EndsWith(".Gen.h", StringComparison.Ordinal) || x.Path.EndsWith(".Gen.cpp", StringComparison.Ordinal) || x.Path.EndsWith(".csproj", StringComparison.Ordinal) || x.Path.Contains(".CSharp"))).ToList();
			m_View->ShowItems(items, _sortType, false, true);
		}
	}

	void ContentWindow::ScrollingOnTreeView(bool enabled)
	{
		if (m_ContentTreePanel->vScrollBar != nullptr)
			m_ContentTreePanel->vScrollBar->ThumbEnabled = enabled;
		if (m_ContentTreePanel->hScrollBar != nullptr)
			m_ContentTreePanel->hScrollBar->ThumbEnabled = enabled;
	}

	void ContentWindow::ScrollingOnContentView(bool enabled)
	{
		if (m_ContentViewPanel->vScrollBar != nullptr)
			m_ContentViewPanel->vScrollBar->ThumbEnabled = enabled;
		if (m_ContentViewPanel->hScrollBar != nullptr)
			m_ContentViewPanel->hScrollBar->ThumbEnabled = enabled;
	}

	void ContentWindow::Rename(ContentItem* item)
	{
		if (!item->CanRename)
			return;

		// Show element in the view
		Select(item, true);

		// Disable scrolling in content view
		if (m_ContentViewPanel->vScrollBar != nullptr)
			m_ContentViewPanel->vScrollBar->ThumbEnabled = false;
		if (m_ContentViewPanel->hScrollBar != nullptr)
			m_ContentViewPanel->hScrollBar->ThumbEnabled = false;
		ScrollingOnContentView(false);

		// Show rename popup
		RenamePopup* popup = RenamePopup::ShowPopup(item, item->TextRectangle, item->ShortName, true);
		popup->Tag = item;
		popup->Validate.Bind<ContentWindow, &ContentWindow::OnRenameValidate>(this);
		popup->Renamed.Bind([this](RenamePopup* renamePopup)
		{
			Rename(std::any_cast<ContentItem*>(renamePopup->Tag), renamePopup->Text);
		});
		popup->Closed.Bind<ContentWindow, &ContentWindow::OnRenameClosed>(this);

		// For new asset we want to mock the initial value so user can press just Enter to use default name
		if (m_NewElement != nullptr)
		{
			popup->InitialValue = SE_TEXT("?");
		}
	}
	
	void ContentWindow::Rename(ContentItem* item, String& newShortName)
	{
		ENGINE_ASSERT(item != nullptr)

		// Check if can rename this item
		if (!item->CanRename)
		{
			// Cannot
			MessageBox::Show(SE_TEXT("Cannot rename this item."), SE_TEXT("Cannot rename"), MessageBoxButtons::OK, MessageBoxIcon::Error);
			return;
		}

		String newNameExtension = FileSystem::GetExtension(newShortName);
		String NameExtension = FileSystem::GetExtension(item->Path);
		// Renaming a file to an extension it already has
		if (!item->IsFolder && FileSystem::NormalizeExtension(newNameExtension) == FileSystem::NormalizeExtension(NameExtension))
		{
			newShortName = FileSystem::GetPathWithoutExtension(newShortName);
		}

		// Check if name is valid
		/*if (!Editor.ContentEditing.IsValidAssetName(item, newShortName, out string hint))
		{
			// Invalid name
			MessageBox.Show("Given asset name is invalid. " + hint, "Invalid name", MessageBoxButtons.OK, MessageBoxIcon.Error);
			return;
		}*/

		// Ensure has parent
		if (item->ParentFolder == nullptr)
		{
			LOG_WARNING("Asset", "Cannot rename root items. {0}", item->Path);
			return;
		}

		newShortName = newShortName.TrimTrailing();

		// Cache data
		String extension = item->IsFolder ? SE_TEXT("") : FileSystem::GetExtension(item->Path);
		String newPath = FileSystem::CombinePaths(item->ParentFolder->Path, newShortName + extension);

		// Check if was renaming mock element
		// Note: we create `_newElement` and then rename it to create new asset
		ContentFolder* itemFolder = item->ParentFolder;
		Delegate<ContentItem*> endEvent;
		if (m_NewElement == item)
		{
			endEvent = std::any_cast<Delegate<ContentItem*>>(m_NewElement->Tag);

			// Create new asset
			ContentOperate* proxy = m_NewElement->Proxy;
			LOG_INFO("Asset", "Creating asset {0} in {1}", proxy->Name.operator->(), newPath);
			proxy->Create(newPath, m_NewElement->Argument);
		}
		else
		{
			// Validate state
			ENGINE_ASSERT(m_NewElement != nullptr);

			// Rename asset
			LOG_INFO("Asset", "Renaming asset {0} to {1}", item->Path, newShortName);
			editor->databaseModule->Move(item, newPath);
		}

		if (m_NewElement != nullptr)
		{
			/*// Trigger compilation if need to
			if (_newElement.Proxy is ScriptProxy && Editor.Instance.Options.Options.General.AutoReloadScriptsOnMainWindowFocus)
				ScriptsBuilder.MarkWorkspaceDirty();*/

			// Destroy mock control
			m_NewElement->ParentFolder = nullptr;
			m_NewElement->Dispose();
			m_NewElement = nullptr;

			// Focus content window
			Focus();
			if (RootWindow() != nullptr)
			{
				RootWindow()->Focus();
			}
		}

		// Refresh database and view now
		editor->databaseModule->RefreshFolder(itemFolder, true);
		RefreshView();
		ContentItem* newItem = itemFolder->FindChild(newPath);
		if (newItem == nullptr)
		{
			LOG_WARNING("Asset", "Failed to find the created new item.");
			return;
		}

		// Auto-select item
		Select(newItem, true);

		// Custom post-action
		endEvent(newItem);
	}

	void ContentWindow::Delete(ContentItem* item)
	{
		List<ContentItem*> items = m_View->GetSelection();
		if (items.Count() == 0)
		{
			items = { item };
		}
		Delete(items);
	}

	void ContentWindow::Delete(List<ContentItem*>& items)
	{
		if (items.Count() == 0)
			return;

		// Sort items to remove files first, then folders
		List<ContentItem*> toDelete = List<ContentItem*>(items);
		auto sortFunc = CreateFunc([](ContentItem* const & a, ContentItem* const& b)
		{
			return (a->IsFolder ? 1 : b->IsFolder ? -1 : a->Compare(b)) > 0;;
		});
		Sorting::QuickSort(toDelete, sortFunc);

		String msg = toDelete.Count() == 1
					 ? String::Format(SE_TEXT("Are you sure to delete \'{0}\'?\nThis action cannot be undone. Files will be deleted permanently."), items[0]->Path)
					 : String::Format(SE_TEXT("Are you sure to delete {0} selected items?\nThis action cannot be undone. Files will be deleted permanently."), items.Count());

		// Ask user
		if (MessageBox::Show(msg, SE_TEXT("Delete asset(s)"), MessageBoxButtons::OKCancel, MessageBoxIcon::Question) != DialogResult::OK)
			return;

		// Clear navigation
		// TODO: just remove invalid locations from the history (those are removed)
		NavigationClearHistory();

		// Delete items
		for (int i = 0; i < toDelete.Count(); i++)
			editor->databaseModule->Delete(toDelete[i], true);

		RefreshView();
	}

	void ContentWindow::NavigateBackward()
	{
		// Check if navigation is unlocked
		if (m_NavigationUnlocked && m_NavigationUndo.Count() > 0)
		{
			// Pop node
			ContentTreeNode* node = m_NavigationUndo.Pop();

			// Lock navigation
			m_NavigationUnlocked = false;

			// Add to Redo list
			m_NavigationRedo.Push(SelectedNode);

			// Select node
			RefreshView(node);
			m_Tree->Select(node);
			node->ExpandAllParents();

			// Set valid sizes for stacks
			//RedoList.SetSize(32);
			//UndoList.SetSize(32);

			// Update search
			UpdateItemsSearch();

			// Unlock navigation
			m_NavigationUnlocked = true;

			// Update UI
			UpdateUI();
			m_View->SelectFirstItem();
		}
	}

	void ContentWindow::NavigateForward()
	{
		// Check if navigation is unlocked
		if (m_NavigationUnlocked && m_NavigationRedo.Count() > 0)
		{
			// Pop node
			ContentTreeNode* node = m_NavigationRedo.Pop();

			// Lock navigation
			m_NavigationUnlocked = false;

			// Add to Undo list
			m_NavigationUndo.Push(SelectedNode);

			// Select node
			RefreshView(node);
			m_Tree->Select(node);
			node->ExpandAllParents();

			// Set valid sizes for stacks
			//RedoList.SetSize(32);
			//UndoList.SetSize(32);

			// Update search
			UpdateItemsSearch();

			// Unlock navigation
			m_NavigationUnlocked = true;

			// Update UI
			UpdateUI();
			m_View->SelectFirstItem();
		}
	}

	void ContentWindow::NavigateUp()
	{
		ContentTreeNode* target = _root;
		ContentTreeNode* current = SelectedNode;

		if (current != nullptr && current->GetFolder()->ParentFolder != nullptr)
		{
			target = current->GetFolder()->ParentFolder->Node;
		}

		Navigate(target);
	}

	void ContentWindow::NavigationClearHistory()
	{
		m_NavigationUndo.Clear();
		m_NavigationRedo.Clear();
		UpdateUI();
	}

	void ContentWindow::ShowRoot()
	{
		m_Tree->Select(_root);
	}

	void ContentWindow::Navigate(ContentTreeNode* target)
	{
		Navigate(SelectedNode, target);
	}

	void ContentWindow::Select(Asset* asset)
	{
		ENGINE_ASSERT(asset != nullptr)

		ContentItem* item = editor->databaseModule->Find(asset->GetID());
		if (item != nullptr)
			Select(item);
	}

	void ContentWindow::Select(ContentItem* item, bool fastScroll)
	{
		ENGINE_ASSERT(item != nullptr)


		if (!m_NavigationUnlocked)
			return;
		ContentFolder* parent = item->ParentFolder;
		if (parent == nullptr || !parent->Visible)
			return;

		// Ensure that window is visible
		FocusOrShow();

		// Navigate to the parent directory
		Navigate(parent->Node);

		// Select and scroll to cover in view
		m_View->Select(item);
		m_ContentViewPanel->ScrollViewTo(item, fastScroll);

		// Focus
		m_View->Focus();
	}

	void ContentWindow::Open(ContentItem* item)
	{
		ENGINE_ASSERT(item != nullptr)

		// Check if it's a folder
		if (item->IsFolder)
		{
			// Show folder
			ContentFolder* folder = static_cast<ContentFolder*>(item);
			if (folder != nullptr)
			{
				folder->Node->Expand();
				m_Tree->Select(folder->Node);
				m_View->SelectFirstItem();
			}
			return;
		}

		// Open it
		ContentOpen(item);
	}

	void ContentWindow::Duplicate(ContentItem* item)
	{
		// Skip null
		if (item == nullptr)
			return;

		// TODO: don't allow to duplicate items without ParentFolder - like root items (Content, Source, Engine and Editor dirs)

		// Clone item
		String targetPath = GetClonedAssetPath(item);
		editor->databaseModule->Copy(item, targetPath);

		// Refresh this folder now and try to find duplicated item
		editor->databaseModule->RefreshFolder(item->ParentFolder, true);
		RefreshView();
		ContentItem* targetItem = item->ParentFolder->FindChild(targetPath);

		// Start renaming it
		if (targetItem != nullptr)
		{
			Select(targetItem);
			Rename(targetItem);
		}
	}

	void ContentWindow::Duplicate(List<ContentItem*>& items)
	{
		// Skip empty or null case
		if (items.Count() <= 0)
			return;

		// TODO: don't allow to duplicate items without ParentFolder - like root items (Content, Source, Engine and Editor dirs)

		// Check if it's just a single item
		if (items.Count() == 1)
		{
			Duplicate(items[0]);
		}
		else
		{
			// TODO: remove items that depend on different items in the list: use wants to remove `folderA` and `folderA/asset.x`, we should just remove `folderA`
			List<ContentItem*> toDuplicate = List<ContentItem*>(items);

			// Duplicate every item
			for (int i = 0; i < toDuplicate.Count(); i++)
			{
				ContentItem* item = toDuplicate[i];
				editor->databaseModule->Copy(item, GetClonedAssetPath(item));
			}
		}
	}

	void ContentWindow::Paste(List<String>& files)
	{
		List<String> importFiles = List<String>();
		for (String& sourcePath : files)
		{
			ContentItem* item = editor->databaseModule->Find(sourcePath);
			if (item != nullptr)
			{
				String newPath = FileSystem::CombinePaths(CurrentViewFolder->Path, item->FileName);
				FileSystem::NormalizePath(newPath);
				if (sourcePath == newPath)
				{
					newPath = GetClonedAssetPath(item);
				}
				editor->databaseModule->Copy(item, newPath);
			}
			else
				importFiles.Add(sourcePath);
		}

		ENGINE_UNREACHABLE_CODE()
		// Editor.ContentImporting.Import(importFiles, CurrentViewFolder);
	}

	void ContentWindow::CreateItem(ContentOperate* proxy, Function<void(ContentItem*)> created, String initialName, bool withRenaming, void* argument)
	{
		// Setup name
		String& name = initialName;
		if (initialName.IsEmpty())
		{
			name = proxy->NewItemName;
		}

		if (!proxy->IsFileNameValid(name))
		{
			name = proxy->NewItemName;
		}

		// If the proxy can not be created in the current folder, then navigate to the content folder
		if (!proxy->CanCreate(CurrentViewFolder))
		{
			Navigate(editor->databaseModule->ProjectNode);
		}

		ContentFolder* parentFolder = CurrentViewFolder;
		String parentFolderPath = parentFolder->Path;

		// Create asset name
		String extension = String::Format(SE_TEXT(".{0}"), proxy->FileExtension.operator->());
		String path = FileSystem::CombinePaths(parentFolderPath, name + extension);
		if (parentFolder->FindChild(path) != nullptr)
		{
			int i = 0;
			do
			{
				path = FileSystem::CombinePaths(parentFolderPath, String::Format(SE_TEXT("{0} {1}"), name, i++) + extension);
			} while (parentFolder->FindChild(path) != nullptr);
		}

		if (withRenaming)
		{
			// Create new asset proxy, add to view and rename it
			m_NewElement = ::SE::New<NewItem>(path, proxy, argument);
			m_NewElement->ParentFolder = parentFolder;
			m_NewElement->Tag = created;

			RefreshView();
			Rename(m_NewElement);
		}
		else
		{
			// Create new asset
			LOG_INFO("Asset", "Creating asset {0} in {1}", proxy->Name.operator->(), path);
			proxy->Create(path, argument);

			// Focus content window
			Focus();
			WindowRootControl* rootControl = RootWindow();
			if (rootControl != nullptr)
			{
				rootControl->Focus();
			}

			// Refresh database and view now
			editor->databaseModule->RefreshFolder(parentFolder, false);
			RefreshView();
			auto newItem = parentFolder->FindChild(path);
			if (newItem == nullptr)
			{
				LOG_WARNING("Asset", "Failed to find the created new item.");
				return;
			}

			// Auto-select item
			Select(newItem, true);

			// Custom post-action
			if (created.IsBinded())
			{
				created(newItem);
			}
		}
	}


	void ContentWindow::ClearItemsSearch()
	{
		// Skip if already cleared
		if (m_ItemsSearchBox->TextLength == 0)
			return;

		SetIsLayoutLocked(true);

		m_ItemsSearchBox->Clear();
		m_ViewDropdown->SelectedIndex = -1;

		SetIsLayoutLocked(false);

		UpdateItemsSearch();
	}

	void ContentWindow::OnInit()
	{
		// Setup content root node
		_root = New<RootContentTreeNode>();
		_root->ChildrenIndent = 0;
		_root->Expand(true);
		_root->Name = SE_TEXT("ContentWindow-Root");

		// Add game project on top, plugins in the middle and engine at bottom
		_root->AddChild(editor->databaseModule->ProjectNode);
		Sorting::QuickSort(editor->databaseModule->Projects);
		for (ProjectTreeNode* project : editor->databaseModule->Projects)
		{
			project->SortChildrenRecursive();
			if (project == editor->databaseModule->ProjectNode)
				continue;
			project->Visible = _showPluginsFiles;
			project->GetFolder()->Visible = _showPluginsFiles;
			_root->AddChild(project);
		}
		/*editor->databaseModule->Engine.Visible = _showEngineFiles;
		editor->databaseModule->Engine.Folder.Visible = _showEngineFiles;
		_root->AddChild(editor->databaseModule->);*/

		editor->databaseModule->ProjectNode->Expand(true);
		m_Tree->Margin = Margin(0.0f, 0.0f, -16.0f, 2.0f); // Hide root node
		m_Tree->AddChild(_root);
		m_Tree->Name = SE_TEXT("ContentWindow-Tree");

		// Setup navigation
		m_NavigationUnlocked = true;
		m_Tree->Select(_root);
		NavigationClearHistory();

		// Update UI layout
		SetIsLayoutLocked(false);
		PerformLayout();

		// Load last viewed folder
		/*if (Editor.ProjectCache.TryGetCustomData(ProjectDataLastViewedFolder, out var lastViewedFolder))
		{
			if (Editor.ContentDatabase.Find(lastViewedFolder) is ContentFolder folder)
				_tree.Select(folder.Node);
		}*/

		_isWorkspaceDirty = true;
	}

	void ContentWindow::Update(float deltaTime)
	{
		// Handle workspace modification events but only once per frame
		if (_isWorkspaceDirty)
		{
			_isWorkspaceDirty = false;
			RefreshView();
		}

		EditorWindow::Update(deltaTime);
	}

	void ContentWindow::OnExit()
	{
		// Save last viewed folder
		ContentTreeNode* lastViewedFolder = m_Tree->Selection.Count() == 1 ? TypeTryCast<ContentTreeNode>(m_Tree->SelectedNode) : nullptr;
		// Editor.ProjectCache.SetCustomData(ProjectDataLastViewedFolder, lastViewedFolder?.Path ?? String::Empty);

		// Clear view
		m_View->ClearItems();

		// Unlink used directories
		if (_root != nullptr)
		{
			while (_root->HasChildren())
			{
				_root->RemoveChild(_root->GetChild(0));
			}
		}
	}

	bool ContentWindow::OnMouseDown(Float2 location, MouseButton button)
	{
		// Navigate through directories using the side mouse buttons
		if (button == MouseButton::Extended1)
			NavigateBackward();
		else if (button == MouseButton::Extended2)
			NavigateForward();

		return EditorWindow::OnMouseDown(location, button);
	}

	bool ContentWindow::OnMouseUp(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Right)
		{
			// Find control that is under the mouse
			Control* c = GetChildAtRecursive(location);

			ContentItem* item;
			ContentTreeNode* node;
			if (TypeTryCast<ContentItem>(c, item))
			{
				if (m_View->IsSelected(item) == false)
					m_View->Select(item);
				ShowContextMenuForItem(item, location, false);
			}
			else if (TypeIs<ContentView>(c))
			{
				ShowContextMenuForItem(nullptr, location, false);
			}
			else if (TypeTryCast<ContentTreeNode>(c, node))
			{
				m_Tree->Select(node);
				ShowContextMenuForItem(node->GetFolder(), location, true);
			}

			return true;
		}

		if (button == MouseButton::Left)
		{
			// Find control that is under the mouse
			Control* c = GetChildAtRecursive(location);
			if (TypeIs<ContentView>(c))
			{
				m_View->ClearSelection();
				return true;
			}
		}

		return EditorWindow::OnMouseUp(location, button);
	}

	EditorWindow* ContentWindow::ContentOpen(ContentItem* item, bool disableAutoShow)
	{
		if (item == nullptr)
		{
			return nullptr;
		}

		// Check if any window is already editing this item
		EditorWindow* window = nullptr; //Editor.Windows.FindEditor(item);
		if (window != nullptr)
		{
			window->Focus();
			return window;
		}

		// Find proxy object
		ContentOperate* contentOperate = editor->databaseModule->GetProxy(item);
		if (contentOperate == nullptr)
		{
			LOG_INFO("Asset", "Missing content operate object for {0}", item->GetTypeInfo()->friendlyName);
			return nullptr;
		}

		// Open
		window = contentOperate->Open(editor, item);

		if (window != nullptr && !disableAutoShow)
		{
			// editor->windowsModule->Open(window);
		}

		return window;
	}

	void ContentWindow::PerformLayoutBeforeChildren()
	{
		EditorWindow::PerformLayoutBeforeChildren();

		if (m_NavigationBar != nullptr)
		{
			m_NavigationBar->UpdateBounds(m_ToolStrip);
		}
	}

	void ContentWindow::UpdateUI()
	{
		// UpdateToolstrip();
		UpdateNavigationBar();
	}

	ContextMenu* ContentWindow::OnViewDropdownPopupCreate(ComboBox* comboBox)
	{
		ContextMenu* menu = New<ContextMenu>();

		ContextMenuButton* viewScale = menu->AddButton(SE_TEXT("View Scale"));
		viewScale->CloseMenuOnClick = false;
		FloatValueBox* scaleValue = New<FloatValueBox>(1, 75, 2, 50.0f, 0.3f, 3.0f, 0.01f);
		scaleValue->Parent = viewScale;

		scaleValue->ValueChanged.Bind([this, scaleValue]() { m_View->ViewScale = scaleValue->GetValue();});
		menu->VisibleChanged.Bind([this, scaleValue](Control* control) { scaleValue->SetValue(m_View->ViewScale); });

		ContextMenuChildMenu* viewType = menu->AddChildMenu(SE_TEXT("View Type"));
		auto viewFunc = CreateFunc<ContentWindow, &ContentWindow::OnViewTypeButtonClicked>(this);
		viewType->ContextMenu->AddButton(SE_TEXT("Tiles"), viewFunc)->Tag = ContentViewType::Tiles;
		viewType->ContextMenu->AddButton(SE_TEXT("List"), viewFunc)->Tag = ContentViewType::List;
		viewType->ContextMenu->VisibleChanged.Bind( [this](Control* control)
		{
			if (!control->Visible)
				return;

			List<ContextMenuItem*> items = ((ContextMenu*)control)->Items;
			for(ContextMenuItem* item : items)
			{
				ContextMenuButton* button;
				if (TypeTryCast<ContextMenuButton>(item, button))
				{
					button->Checked = m_View->ViewType == std::any_cast<ContentViewType>(button->Tag);
				}
			}
		});

		ContextMenuChildMenu* show = menu->AddChildMenu(SE_TEXT("Show"));
		{
			auto b = show->ContextMenu->AddButton(SE_TEXT("File extensions"), CreateFunc([this]()
			{
				m_View->ShowFileExtensions = !m_View->ShowFileExtensions;
			}));
			// b->TooltipText = SE_TEXT("Shows all files with extensions");
			b->Checked = m_View->ShowFileExtensions;
			b->CloseMenuOnClick = false;
			b->AutoCheck = true;

			b = show->ContextMenu->AddButton(SE_TEXT("Engine files"), CreateFunc([this]()
			{
				/*ShowEngineFiles = !ShowEngineFiles;*/
			}));
			// b->TooltipText = SE_TEXT("Shows in-built engine content");
			// b.Checked = ShowEngineFiles;
			b->CloseMenuOnClick = false;
			b->AutoCheck = true;

			b = show->ContextMenu->AddButton(SE_TEXT("Plugins files"), CreateFunc([this]() { /*ShowPluginsFiles = !ShowPluginsFiles*/ }));
			// b->TooltipText = SE_TEXT("Shows plugin projects content");
			// b.Checked = ShowPluginsFiles;
			b->CloseMenuOnClick = false;
			b->AutoCheck = true;

			b = show->ContextMenu->AddButton(SE_TEXT("Generated files"), CreateFunc([this]() { /*ShowGeneratedFiles = !ShowGeneratedFiles*/ }));
			// b->TooltipText = SE_TEXT("Shows generated files");
			// b.Checked = ShowGeneratedFiles;
			b->CloseMenuOnClick = false;
			b->AutoCheck = true;

			b = show->ContextMenu->AddButton(SE_TEXT("All files"), CreateFunc([this]() {/*ShowAllFiles = !ShowAllFiles*/ }));
			// b->TooltipText = SE_TEXT("Shows all files including other than assets and source code");
			// b.Checked = ShowAllFiles;
			b->CloseMenuOnClick = false;
			b->AutoCheck = true;
		}

		ContextMenuChildMenu* filters = menu->AddChildMenu(SE_TEXT("Filters"));
		for (int i = 0; i < m_ViewDropdown->items.Count(); i++)
		{
			auto filterButton = filters->ContextMenu->AddButton(m_ViewDropdown->items[i], CreateFunc<ContentWindow, &ContentWindow::OnFilterClicked>(this));
			filterButton->CloseMenuOnClick = false;
			filterButton->Tag = i;
		}
		filters->ContextMenu->ButtonClicked.Bind([this, filters](ContextMenuButton* button)
		{
			List<ContextMenuItem*> items = (filters->ContextMenu)->Items;
			for (auto item : items)
			{
				ContextMenuButton* filterButton;
				if (TypeTryCast<ContextMenuButton>(item, filterButton))
				{
					filterButton->Checked = m_ViewDropdown->IsSelected(filterButton->Text);
				}
			}
		});
		filters->ContextMenu->VisibleChanged.Bind([this] (Control* control)
		{
			if (!control->Visible)
				return;

			List<ContextMenuItem*> items = ((ContextMenu*)control)->Items;
			for (auto item : items)
			{
				ContextMenuButton* filterButton;
				if (TypeTryCast<ContextMenuButton>(item, filterButton))
				{
					filterButton->Checked = m_ViewDropdown->IsSelected(filterButton->Text);
				}
			}
		});

		auto sortBy = menu->AddChildMenu(SE_TEXT("Sort by"));
		auto sortFunc = CreateFunc<ContentWindow, & ContentWindow::OnSortByButtonClicked>(this);

		sortBy->ContextMenu->AddButton(SE_TEXT("Alphabetic Order"), sortFunc)->Tag = SortType::AlphabeticOrder;
		sortBy->ContextMenu->AddButton(SE_TEXT("Alphabetic Reverse"), sortFunc)->Tag = SortType::AlphabeticReverse;
		sortBy->ContextMenu->VisibleChanged.Bind([this](Control* control)
		{
			if (!control->Visible)
				return;

			List<ContextMenuItem*> items = ((ContextMenu*)control)->Items;
			for (auto item : items)
			{
				ContextMenuButton* button;
				if (TypeTryCast<ContextMenuButton>(item, button))
				{
					button->Checked = _sortType == std::any_cast<SortType>(button->Tag) ;
				}
			}
		});

		return menu;
	}

	void ContentWindow::UpdateItemsSearchFilter(ContentFolder* folder, List<ContentItem*> items, bool filters[], bool showAllFiles)
	{
		for (int i = 0; i < folder->Children.Count(); i++)
		{
			ContentItem* child = folder->Children[i];
			ContentFolder* childFolder;
			if (TypeTryCast<ContentFolder>(child,childFolder))
			{
				UpdateItemsSearchFilter(childFolder, items, filters, showAllFiles);
			}
			else if (filters[(int)child->SearchFilter.operator->()] && (showAllFiles || !TypeAs<FileItem>(child)))
			{
				items.Add(child);
			}
		}
	}

	void ContentWindow::UpdateItemsSearchFilter(ContentFolder* folder, List<ContentItem*> items, bool filters[], bool showAllFiles, String& filterText)
	{
		for (int i = 0; i < folder->Children.Count(); i++)
		{
			ContentItem* child = folder->Children[i];
			ContentFolder* childFolder;
			if (TypeTryCast<ContentFolder>(child,childFolder))
			{
				UpdateItemsSearchFilter(childFolder, items, filters, showAllFiles, filterText);
			}
			else if (filters[(int)child->SearchFilter.operator->()] && (showAllFiles || !TypeAs<FileItem>(child)) &&
				QueryFilterHelper::Match(filterText, child->ShortName))
			{
				items.Add(child);
			}
		}
	}

	bool ContentWindow::TryParseAssetId(String& text, AssetItem*& item)
	{
		item = nullptr;
		if (text.Length() != 32)
			return false;

		UID id;
		if (!UID::Parse(text, id))
		{
			return false;
		}
		item = editor->databaseModule->FindAsset(id);
		return item != nullptr;
	}

	void ContentWindow::OnViewTypeButtonClicked(ContextMenuButton* button)
	{
		m_View->ViewType = std::any_cast<ContentViewType>(button->Tag);
	}

	void ContentWindow::OnFilterClicked(ContextMenuButton* filterButton)
	{
		int i = std::any_cast<int>(filterButton->Tag);
		m_ViewDropdown->OnClicked(i);
	}

	void ContentWindow::OnSortByButtonClicked(ContextMenuButton* button)
	{
		switch (std::any_cast<SortType>(button->Tag))
		{
		case SortType::AlphabeticOrder:
			_sortType = SortType::AlphabeticOrder;
			break;
		case SortType::AlphabeticReverse:
			_sortType = SortType::AlphabeticReverse;
			break;
		}
		RefreshView(SelectedNode);
	}

	void ContentWindow::OnContentDatabaseItemRemoved(ContentItem* contentItem)
	{
		ContentFolder* folder;
		if (TypeTryCast<ContentFolder>(contentItem, folder))
		{
			ContentTreeNode* node = folder->Node;

			// Check if current location contains it as a parent
			if (contentItem->Find(CurrentViewFolder))
			{
				// Navigate to root to prevent leaks
				ShowRoot();
			}

			// Check if folder is in navigation
			if (m_NavigationRedo.Contains(node) || m_NavigationUndo.Contains(node))
			{
				// Clear all to prevent leaks
				NavigationClearHistory();
			}
		}
	}

	void ContentWindow::OnFoldersSearchBoxTextChanged()
	{
		// Skip events during setup or init stuff
		if (GetIsLayoutLocked())
			return;

		RootContentTreeNode* root = _root;
		root->LockChildrenRecursive();

		// Update tree
		String query = m_FoldersSearchBox->Text;
		root->UpdateFilter(query);

		root->UnlockChildrenRecursive();
		PerformLayout();
		PerformLayout();
	}

	void ContentWindow::OnTreeSelectionChanged(List<TreeNode*> from, List<TreeNode*> to)
	{
		// Navigate
		ContentTreeNode* source = from.Count() > 0 ? TypeTryCast<ContentTreeNode>(from[0]) : nullptr;
		ContentTreeNode* target = to.Count() > 0 ? TypeTryCast<ContentTreeNode>(to[0]): nullptr;
		Navigate(source, target);

		if (target != nullptr)
		{
			target->Focus();
		}
	}

	void ContentWindow::UpdateItemsSearch()
	{
		// Skip events during setup or init stuff
		if (GetIsLayoutLocked())
			return;

		// Check if clear filters
		if (m_ItemsSearchBox->TextLength == 0 && !m_ViewDropdown->HasSelection)
		{
			m_View->IsSearching = false;
			RefreshView();
			return;
		}

		// Apply filter
		auto items = List<ContentItem*>(8);
		String& query = m_ItemsSearchBox->Text;
		int filtersLength = m_ViewDropdown->items.Count();
		bool* filters = NewArray<bool>(m_ViewDropdown->items.Count());
		if (m_ViewDropdown->HasSelection)
		{
			// Update filters flags
			for (int i = 0; i < filtersLength; i++)
			{
				filters[i] = m_ViewDropdown->Selection.operator->().Contains(i);
			}
		}
		else
		{
			// No filters
			for (int i = 0; i < filtersLength; i++)
			{
				filters[i] = true;
			}
		}

		AssetItem* assetItem = nullptr;
		// Search by filter only
		bool showAllFiles = _showAllFiles;
		if (query.IsEmpty())
		{
			if (SelectedNode == _root)
			{
				// Special case for root folder
				for (int i = 0; i < _root->ChildrenCount(); i++)
				{
					ContentTreeNode* node;
					if (TypeTryCast<ContentTreeNode>(_root->GetChild(i), node))
					{
						UpdateItemsSearchFilter(node->GetFolder(), items, filters, showAllFiles);
					}
				}
			}
			else
			{
				UpdateItemsSearchFilter(CurrentViewFolder, items, filters, showAllFiles);
			}
		}
		// Search by asset ID
		else if (TryParseAssetId(query, assetItem))
		{
			items.Add((ContentItem*)assetItem);
		}
		// Search by custom query text
		else
		{
			if (SelectedNode == _root)
			{
				// Special case for root folder
				for (int i = 0; i < _root->ChildrenCount(); i++)
				{
					ContentTreeNode* node;
					if (TypeTryCast<ContentTreeNode>(_root->GetChild(i), node))
					{
						UpdateItemsSearchFilter(node->GetFolder(), items, filters, showAllFiles, query);
					}
				}
			}
			else
			{
				UpdateItemsSearchFilter(CurrentViewFolder, items, filters, showAllFiles, query);
			}
		}

		m_View->IsSearching = true;
		m_View->ShowItems(items, _sortType);
	}

	bool ContentWindow::OnRenameValidate(RenamePopup* popup, String& value)
	{
		return false; //Editor.ContentEditing.IsValidAssetName((ContentItem*)popup.Tag, value, out _);
	}

	void ContentWindow::OnRenameClosed(RenamePopup* popup)
	{
		// Restore scrolling in content view
		if (m_ContentViewPanel->vScrollBar != nullptr)
			m_ContentViewPanel->vScrollBar->ThumbEnabled = true;
		if (m_ContentViewPanel->vScrollBar != nullptr)
			m_ContentViewPanel->vScrollBar->ThumbEnabled = true;
		ScrollingOnContentView(true);

		// Check if was creating new element
		if (m_NewElement != nullptr)
		{
			// Destroy mock control
			m_NewElement->ParentFolder = nullptr;
			m_NewElement->Dispose();
			m_NewElement = nullptr;
		}
	}

	void ContentWindow::OnTreeSelectionChanged(List<TreeNode*>& from, List<TreeNode*>& to)
	{
		// Navigate
		ContentTreeNode* source = from.Count() > 0 ? TypeTryCast<ContentTreeNode>(from[0]) : nullptr;
		ContentTreeNode* target = to.Count() > 0 ? TypeTryCast<ContentTreeNode>(to[0]) : nullptr;
		Navigate(source, target);

		if (target != nullptr)
		{
			target->Focus();
		}
	}

	void ContentWindow::Navigate(ContentTreeNode* source, ContentTreeNode* target)
	{
		if (target == nullptr)
			target = _root;

		// Check if can do this action
		if (m_NavigationUnlocked && source != target)
		{
			// Lock navigation
			m_NavigationUnlocked = false;

			// Check if already added to the Undo on the top
			if (source != nullptr && (m_NavigationUndo.Count() == 0 || m_NavigationUndo.Peek() != source))
			{
				// Add to Undo list
				m_NavigationUndo.Push(source);
			}

			// Show folder contents and select tree node
			RefreshView(target);
			m_Tree->Select(target);
			target->ExpandAllParents();

			// Clear redo list
			m_NavigationRedo.Clear();

			// Set valid sizes for stacks
			// RedoList.SetSize(32);
			// UndoList.SetSize(32);

			// Update search
			UpdateItemsSearch();

			// Unlock navigation
			m_NavigationUnlocked = true;

			// Update UI
			UpdateUI();
		}
	}

	void ContentWindow::UpdateNavigationBar()
	{
		if (m_NavigationBar == nullptr)
			return;

		bool wasLayoutLocked = m_NavigationBar->GetIsLayoutLocked();
		m_NavigationBar->SetIsLayoutLocked(true);

		// Remove previous buttons
		m_NavigationBar->DisposeChildren();

		// Spawn buttons
		List<ContentTreeNode*>& nodes = NavUpdateCache;
		nodes.Clear();
		ContentTreeNode* node = SelectedNode;
		while (node != nullptr)
		{
			nodes.Add(node);
			node = node->ParentNode();
		}
		float x = NavigationBar::DefaultButtonsMargin;
		float h = m_ToolStrip->ItemsHeight - 2 * ToolStrip::DefaultMarginV;
		for (int i = nodes.Count() - 1; i >= 0; i--)
		{
			ContentNavigationButton* button = New<ContentNavigationButton>(nodes[i], x, ToolStrip::DefaultMarginV, h);
			button->PerformLayout();
			x += button->Width + NavigationBar::DefaultButtonsMargin;
			m_NavigationBar->AddChild(button);
			if (i > 0)
			{
				ContentNavigationSeparator* separator = New<ContentNavigationSeparator>(button, x, ToolStrip::DefaultMarginV, h);
				separator->PerformLayout();
				x += separator->Width + NavigationBar::DefaultButtonsMargin;
				m_NavigationBar->AddChild(separator);
			}
		}
		nodes.Clear();

		// Update
		m_NavigationBar->SetIsLayoutLocked(wasLayoutLocked);
		m_NavigationBar->PerformLayout();
	}

	void ContentWindow::RefreshViewItemsThumbnails()
	{
		auto& items = m_View->GetItems();
		for (int i = 0; i < items.Count(); i++)
		{
			items[i]->RefreshThumbnail();
		}
	}

	String ContentWindow::GetClonedAssetPath(ContentItem* item)
	{
		String& sourcePath = item->Path;
		StringView sourceFolder = FileSystem::GetDirectoryName(sourcePath);

		// Find new name for clone
		String destinationName;
		if (item->IsFolder)
		{
			// destinationName = Utilities.Utils.IncrementNameNumber(item->ShortName, x => !Directory.Exists(StringUtils.CombinePaths(sourceFolder, x)));
		}
		else
		{
			// String extension = FileSystem::GetExtension(sourcePath);
			// destinationName = Utilities.Utils.IncrementNameNumber(item.ShortName, x => !File.Exists(StringUtils.CombinePaths(sourceFolder, x + extension))) + extension;
		}

		String path = FileSystem::CombinePaths(sourceFolder, destinationName);
		FileSystem::NormalizePath(path);
		return path;
	}

	ContentFolder* ContentWindow::__GetCurrentViewFolder()
	{
		if (SelectedNode == nullptr)
		{
			return nullptr;
		}
		return SelectedNode->GetFolder();
	}

	ContentTreeNode* ContentWindow::__GetSelectedNode()
	{
		return TypeTryCast<ContentTreeNode>(m_Tree->SelectedNode);
	}

	void ContentWindow::ShowContextMenuForItem(ContentItem* item, Float2& location, bool isTreeNode)
	{
		// Assert.IsNull(_newElement);

		// Cache data
		bool isValidElement = item != nullptr;
		ContentOperate* proxy = editor->databaseModule->GetProxy(item);
		ContentFolder* folder = nullptr;
		bool isFolder = false;
		if (isValidElement)
		{
			isFolder = item->IsFolder;
			folder = isFolder ? (ContentFolder*)item : item->ParentFolder;
		}
		else
		{
			folder = CurrentViewFolder;
		}
		// Assert.IsNotNull(folder);
		bool isRootFolder = CurrentViewFolder == _root->GetFolder();

		// Create context menu
		ContextMenuButton* b;
		ContextMenu* cm = New<ContextMenu>();
		cm->Tag = item;

		if (isTreeNode)
		{
			b = cm->AddButton(SE_TEXT("Expand All"), CreateFunc<ContentWindow, &ContentWindow::OnExpandAllClicked>(this));
			b->Enabled = CurrentViewFolder->Node->ChildrenCount() != 0;

			b = cm->AddButton(SE_TEXT("Collapse All"), CreateFunc<ContentWindow, &ContentWindow::OnCollapseAllClicked>(this));
			b->Enabled = CurrentViewFolder->Node->ChildrenCount() != 0;

			cm->AddSeparator();
		}

		ContentFolder* contentFolder;
		if (TypeTryCast(item, contentFolder) && TypeIs<ProjectTreeNode>(contentFolder->Node))
		{
			cm->AddButton(SE_TEXT("Show in explorer"), CreateFunc ([this]()
			{
				FileSystem::ShowFileExplorer(CurrentViewFolder->Path);
			}));
		}
		else if (isValidElement)
		{
			b = cm->AddButton(SE_TEXT("Open"), CreateFunc([this, item]()
			{
				Open(item);
			}));
			b->Enabled = proxy != nullptr || isFolder;

			if (m_View->SelectedCount > 1)
			{
				b = cm->AddButton(SE_TEXT("Open (all selected)"), CreateFunc([this]()
				{
					for (ContentItem* e : m_View->GetSelection())
					{
						Open(e);
					}
				}));
			}

			cm->AddButton(SE_TEXT("Show in explorer"), CreateFunc([this, item]()
			{
				StringView path = FileSystem::GetDirectoryName(item->Path);
				FileSystem::ShowFileExplorer(path);
			}));

			if (item->HasDefaultThumbnail == false)
			{
				if (m_View->SelectedCount > 1)
				{
					cm->AddButton(SE_TEXT("Refresh thumbnails"), CreateFunc([this]()
					{
						for (ContentItem* e : m_View->GetSelection())
						{
							e->RefreshThumbnail();
						}
					}));
				}
				else
				{
					cm->AddButton(SE_TEXT("Refresh thumbnail"), CreateFunc([item]()
					{
						item->RefreshThumbnail();
					}));
				}
			}

			if (!isFolder)
			{
				// b = cm->AddButton(SE_TEXT("Reimport"), CreateFunc<ContentWindow, &ContentWindow::ReimportSelection>(this));
				// b->Enabled = proxy != nullptr && proxy->CanReimport(item);

				BinaryAssetItem* binaryAsset;
				if (TypeTryCast(item, binaryAsset))
				{
					String importPath;
					if (!binaryAsset->GetImportPath(importPath))
					{
						StringView importLocation = FileSystem::GetDirectoryName(importPath);
						if (!importLocation.IsEmpty() && FileSystem::DirectoryExists(importLocation))
						{
							String importLocationTemp = importLocation;
							cm->AddButton(SE_TEXT("Show import location"), CreateFunc([importLocationTemp]()
							{
								FileSystem::ShowFileExplorer(importLocationTemp);
							}));
						}
					}
				}

				AssetItem* assetItem;
				if (TypeTryCast(item, assetItem))
				{
					if (assetItem->GetIsLoaded())
					{
						cm->AddButton(SE_TEXT("Reload"), CreateFunc([assetItem]()
						{
							assetItem->Reload();
						}));
					}
					/*cm->AddButton(SE_TEXT("Copy asset ID"), () => Clipboard.Text = JsonSerializer.GetStringID(assetItem.ID));
					cm->AddButton(SE_TEXT("Select actors using this asset"), () => Editor.SceneEditing.SelectActorsUsingAsset(assetItem.ID));
					cm->AddButton(SE_TEXT("Show asset references graph"), () => Editor.Windows.Open(new AssetReferencesGraphWindow(Editor, assetItem)));*/
				}

				/*if (Editor.CanExport(item.Path))
				{
					b = cm->AddButton("Export", ExportSelection);
				}*/
			}

			if (isFolder && TypeIs<MainContentTreeNode>(folder->Node))
			{
				cm->AddSeparator();
			}
			else
			{
				cm->AddButton(SE_TEXT("Delete"), CreateFunc([this, item]()
				{
					Delete(item);
				}));

				/*cm->AddSeparator();

				cm->AddButton(SE_TEXT("Duplicate"), m_View->CDuplicate);

				cm->AddButton(SE_TEXT("Copy"), m_View->CCopy);*/
			}

			b = cm->AddButton(SE_TEXT("Paste"), CreateFunc([this]()
			{
				m_View->Paste();
			}));
			b->Enabled = m_View->CanPaste();

			if (isFolder && TypeIs<MainContentTreeNode>(folder->Node))
			{
				// Do nothing
			}
			else
			{
				cm->AddButton(SE_TEXT("Rename"), CreateFunc([this, item]()
				{
					Rename(item);
				}));
			}

			// Custom options
			/*ContextMenuShow?.Invoke(cm, item);
			proxy?.OnContentWindowContextMenu(cm, item);
			item.OnContextMenu(cm);

			cm->AddButton("Copy name to Clipboard", () => Clipboard.Text = item.NamePath);

			cm->AddButton("Copy path to Clipboard", () => Clipboard.Text = item.Path);*/
		}
		else
		{
			cm->AddButton(SE_TEXT("Show in explorer"), CreateFunc([this]()
			{
				FileSystem::ShowFileExplorer(CurrentViewFolder->Path);
			}));

			b = cm->AddButton(SE_TEXT("Paste"), CreateFunc([this]()
			{
				m_View->Paste();
			}));
			b->Enabled = m_View->CanPaste();

			cm->AddButton(SE_TEXT("Refresh"), CreateFunc([this]()
			{
				EditorApp::Ins().databaseModule->RefreshFolder(CurrentViewFolder, true);
			}));

			cm->AddButton(SE_TEXT("Refresh all thumbnails"), CreateFunc<ContentWindow, &ContentWindow::RefreshViewItemsThumbnails>(this));
		}

		cm->AddSeparator();

		ContentFolder* projectFolder;
		if (!isRootFolder && !(TypeTryCast(item, projectFolder) && TypeIs<ProjectTreeNode>(projectFolder->Node)))
		{
			cm->AddButton(SE_TEXT("New folder"), CreateFunc([this]()
			{
				// NewFolder();
			}));
		}


		if (!isRootFolder)
		{
			// Create
			ContextMenuChildMenu* createMenu = cm->GetOrAddChildMenu(SE_TEXT("Create"));;

			auto find = CreateFunc([](ContentOperate* const& a)
			{
				return a->IsProxyFor(Typeof<SceneAsset>());
			});
			int findIndex = ListExtensions::IndexOf(editor->databaseModule->Operate, find);
			if (findIndex != INVALID_INDEX)
			{
				ContentOperate* operate = editor->databaseModule->Operate[findIndex];
				createMenu->ContextMenu->AddButton(SE_TEXT("Scene"), CreateFunc([this, operate]()
				{
					this->CreateItem(operate);
				}));
			}
		}

		// Loop through each proxy and user defined json type and add them to the context menu
		/*var actorType = new ScriptType(typeof());
		var scriptType = new ScriptType(typeof(Script));
		foreach (var type in Editor.CodeEditing.All.Get())
		{
			if (type.IsAbstract || type.Type == null)
				continue;
			if (actorType.IsAssignableFrom(type) || scriptType.IsAssignableFrom(type))
				continue;

			// Get attribute
			ContentContextMenuAttribute attribute = null;
			foreach (var typeAttribute in type.GetAttributes(false))
			{
				if (typeAttribute is ContentContextMenuAttribute contentContextMenuAttribute)
				{
					attribute = contentContextMenuAttribute;
					break;
				}
			}
			if (attribute == null)
				continue;

			// Get context proxy
			ContentProxy p = null;
			if (type.Type.IsSubclassOf(typeof(ContentProxy)))
			{
				p = Editor.ContentDatabase.Proxy.Find(x => x.GetType() == type.Type);
			}
			else if (type.CanCreateInstance)
			{
				// User can use attribute to put their own assets into the content context menu
				var generic = typeof(SpawnableJsonAssetProxy<>).MakeGenericType(type.Type);
				var instance = Activator.CreateInstance(generic);
				p = instance as AssetProxy;
			}
			if (p == null)
				continue;

			if (p.CanCreate(folder))
			{
				var parts = attribute.Path.Split('/');
				ContextMenuChildMenu childCM = null;
				bool mainCM = true;
				for (int i = 0; i < parts?.Length; i++)
				{
					var part = parts[i].Trim();
					if (i == parts.Length - 1)
					{
						if (mainCM)
						{
							cm->AddButton(part, () => NewItem(p));
							mainCM = false;
						}
						else if (childCM != null)
						{
							childCM.ContextMenu.AddButton(part, () => NewItem(p));
							childCM.ContextMenu.AutoSort = true;
						}
					}
					else
					{
						if (mainCM)
						{
							childCM = cm->GetOrAddChildMenu(part);
							childCM.ContextMenu.AutoSort = true;
							mainCM = false;
						}
						else if (childCM != null)
						{
							childCM = childCM.ContextMenu.GetOrAddChildMenu(part);
							childCM.ContextMenu.AutoSort = true;
						}
					}
				}
			}
		}*/

		if (folder->CanHaveAssets)
		{
			cm->AddButton(SE_TEXT("Import file"), CreateFunc([this]()
			{
				/*m_View->ClearSelection();
				editor->importingModule->ShowImportFileDialog(CurrentViewFolder);*/
			}));
		}

		// Remove any leftover separator
		/*if (cm->ItemsContainer.Children.LastOrDefault() is ContextMenuSeparator)
			cm->ItemsContainer.Children.Last().Dispose();*/

		// Show it
		cm->Show(this, location);
	}

	void ContentWindow::OnExpandAllClicked(ContextMenuButton* button)
	{
		CurrentViewFolder->Node->ExpandAll();
	}
	void ContentWindow::OnCollapseAllClicked(ContextMenuButton* button)
	{
		CurrentViewFolder->Node->CollapseAll();
	}

	void ContentWindow::ViewDropdown::OnClicked(int index)
	{
		OnItemClicked(index);
	}

	void ContentWindow::ViewDropdown::Draw()
	{
		// Cache data
		Rectangle clientRect = Rectangle(Float2::Zero, Size);
		float margin = clientRect.GetHeight() * 0.2f;
		float boxSize = clientRect.GetHeight() - margin * 2;
		bool isOpened = IsPopupOpened;
		bool enabled = EnabledInHierarchy();
		Color backgroundColor = BackgroundColor;
		Color borderColor = BorderColor;
		Color arrowColor = ArrowColor;
		if (!enabled)
		{
			backgroundColor *= 0.5f;
			arrowColor *= 0.7f;
		}
		else if (isOpened || _mouseDown)
		{
			backgroundColor = BackgroundColorSelected;
			borderColor = BorderColorSelected;
			arrowColor = ArrowColorSelected;
		}
		else if (IsMouseOver)
		{
			backgroundColor = BackgroundColorHighlighted;
			borderColor = BorderColorHighlighted;
			arrowColor = ArrowColorHighlighted;
		}

		// Background
		Render2D::FillRectangle(clientRect, backgroundColor);
		Render2D::DrawRectangle(clientRect, borderColor);

		// Draw text
		float textScale = Height / DefaultHeight;
		Rectangle textRect = Rectangle(margin, 0, clientRect.GetWidth() - boxSize - 2.0f * margin, clientRect.GetHeight());
		Render2D::PushClip(textRect);
		Color textColor = TextColor;
		Render2D::RenderText(Font.GetFont(), SE_TEXT("View"), textRect, enabled ? textColor : textColor * 0.5f,
			TextAlignment::Near, TextAlignment::Center, TextWrapping::NoWrap, 1.0f, textScale);
		Render2D::PopClip();

		// Arrow
		if (ArrowImage != nullptr)
		{
			ArrowImage->Draw(Rectangle(clientRect.GetWidth() - margin - boxSize, margin, boxSize, boxSize), arrowColor);
		}
	}
	
	bool ContentWindow::ViewDropdown::OnMouseUp(Float2 location, MouseButton button)
	{
		// Check flags
		if (_mouseDown && !_blockPopup)
		{
			// Clear flag
			_mouseDown = false;

			// Ensure to have valid menu
			if (_popupMenu == nullptr)
			{
				_popupMenu = OnCreatePopup();
				_popupMenu->MaximumItemsInViewCount = MaximumItemsInViewCount;
				_popupMenu->VisibleChanged.Bind([this](Control* cm)
				{
					RootControl* win = Root;
					_blockPopup = win != nullptr && Rectangle(Float2::Zero, Size).Contains(PointFromWindow(win->GetMousePosition()));
					if (!_blockPopup)
						Focus();
				});
			}

			// Check if menu hs been already shown
			if (_popupMenu->Visible)
			{
				// Hide
				_popupMenu->Hide();
				return true;
			}

			// Show
			_popupMenu->Show(this, Float2(1, Height));
		}
		else
		{
			_blockPopup = false;
		}

		return true;
	}
} // SE