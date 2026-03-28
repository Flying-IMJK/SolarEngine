
#include "ContentFolder.h"
#include "Core/Platform/FileSystem.h"
#include "Editor/EditorIcons.h"
#include "Editor/GUI/Drag/DragItems.hpp"
#include "Editor/Resource/Tree/ContentTreeNode.h"
#include "Editor/Resource/Tree/MainContentTreeNode.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/DragData.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	ContentFolder::ContentFolder(ContentFolderType type, StringView path, ContentTreeNode* node) : ContentItem(path)
	{
		FolderType = type;
		Node = node;
		ShortName = FileSystem::GetFileNameWithoutExtension(path);
	}

	ContentItem* ContentFolder::FindChild(StringView path)
	{
		for (int i = 0; i < Children.Count(); i++)
		{
			if (Children[i]->Path == path)
				return Children[i];
		}

		return nullptr;
	}

	void ContentFolder::UpdatePath(String value)
	{
		ContentItem::UpdatePath(value);

		ShortName = FileSystem::GetFileName(value);

		// Update node text
		Node->Text = ShortName;
	}

	ContentItem* ContentFolder::Find(String path)
	{
		// TODO: split name into parts and check each going tree structure level down - make it faster

		if (Path == path)
			return this;

		for (int i = 0; i < Children.Count(); i++)
		{
			ContentItem* result = Children[i]->Find(path);
			if (result != nullptr)
				return result;
		}

		return nullptr;
	}

	bool ContentFolder::Find(ContentItem* item)
	{
		if (item == this)
			return true;

		for (int i = 0; i < Children.Count(); i++)
		{
			if (Children[i]->Find(item))
				return true;
		}

		return false;
	}

	ContentItem* ContentFolder::Find(UID id)
	{
		for (int i = 0; i < Children.Count(); i++)
		{
			ContentItem* result = Children[i]->Find(id);
			if (result != nullptr)
				return result;
		}

		return nullptr;
	}

	int ContentFolder::Compare(const Control* other) const
	{
		const ContentItem* otherItem = TypeTryCast<ContentItem>(other);
		if (otherItem != nullptr)
		{
			if (!otherItem->IsFolder)
				return -1;
			return StringUtils::Compare(ShortName.Get(), otherItem->ShortName.Get());
		}

		return ContentItem::Compare(other);
	}

	void ContentFolder::Draw()
	{
		ContentItem::Draw();

		// Check if drag is over
		if (IsDragOver() && _validDragOver)
		{
			Style* style = Style::Current;
			Rectangle bounds = Rectangle(Float2::Zero, Size);
			Render2D::FillRectangle(bounds, style->Selection);
			Render2D::DrawRectangle(bounds, style->SelectionBorder);
		}
	}

	DragDropEffect ContentFolder::OnDragEnter(const Float2& location, DragData* data)
	{
		ContentItem::OnDragEnter(location, data);

		// Check if drop file(s)
		if (TypeIs<DragDataFiles>(data))
		{
			_validDragOver = true;
			return DragDropEffect::Copy;
		}

		// Check if drop asset(s)
		if (_dragOverItems == nullptr)
		{
			_dragOverItems = MakeRef<DragItems>(CreateFunc<ContentFolder, &ContentFolder::ValidateDragItem>(this));
		}
		_dragOverItems->OnDragEnter(data);
		_validDragOver = _dragOverItems->GetHasValidDrag();
		return _dragOverItems->GetEffect();
	}

	DragDropEffect ContentFolder::OnDragMove(const Float2& location, DragData* data)
	{
		ContentItem::OnDragMove(location, data);

		if (TypeAs<DragDataFiles>(data))
			return DragDropEffect::Copy;
		return _dragOverItems->GetEffect();
	}

	DragDropEffect ContentFolder::OnDragDrop(const Float2& location, DragData* data)
	{
		DragDropEffect result = ContentItem::OnDragDrop(location, data);

		// Check if drop file(s)
		const DragDataFiles* files = TypeTryCast<DragDataFiles>(data);
		if (files != nullptr)
		{
			// Import files
			// Editor.Instance.ContentImporting.Import(files->Files, this);
			ENGINE_UNREACHABLE_CODE();
			result = DragDropEffect::Copy;
		}
		else if (_dragOverItems->GetHasValidDrag())
		{
			// Move items
			// Editor.Instance.ContentDatabase.Move(_dragOverItems->Objects, this);
			ENGINE_UNREACHABLE_CODE();
			result = DragDropEffect::Move;
		}

		// Clear cache
		if (_dragOverItems != nullptr)
		{
			_dragOverItems->OnDragDrop();
		}
		_validDragOver = false;

		return result;
	}

	void ContentFolder::OnDragLeave()
	{
		if (_dragOverItems != nullptr)
		{
			_dragOverItems->OnDragLeave();
		}
		_validDragOver = false;

		ContentItem::OnDragLeave();
	}

	bool ContentFolder::ValidateDragItem(ContentItem* item)
	{
		// Reject itself and any parent
		return item != this && !item->Find(this);
	}

	void ContentFolder::OnBuildTooltipText(StringBuilder sb)
	{
		/*sb.Append("Type: ").Append(TypeDescription).AppendLine();
		sb.Append("Path: ").Append(Utilities.Utils.GetAssetNamePathWithExt(Path)).AppendLine();*/
	}

	ContentItemSearchFilter ContentFolder::__GetSearchFilter()
	{
		return ContentItemSearchFilter::Other;
	}

	void ContentFolder::OnParentFolderChanged()
	{
		// Update tree nodes structure
		if (ParentFolder != nullptr)
		{
			Node->Parent = ParentFolder->Node;
		}else
		{
			Node->Parent = nullptr;
		}


		ContentItem::OnParentFolderChanged();
	}

	bool ContentFolder::__GetCanRename()
	{
		bool hasParentFolder = ParentFolder != nullptr;
		bool isContentFolder = TypeAs<MainContentTreeNode>(Node);
		return hasParentFolder && !isContentFolder;
	}

	bool ContentFolder::__GetCanDrag()
	{
		return ParentFolder != nullptr;
	}

	bool ContentFolder::__GetExists()
	{
		return FileSystem::FileExists(Path);
	}

	SpriteHandle ContentFolder::__GetDefaultThumbnail()
	{
		return EditorIcons::Folder128;
	}

	bool ContentFolder::__GetHasDefaultThumbnail()
	{
		return true;
	}

	bool ContentFolder::__GetCanHaveAssets()
	{
		return FolderType == ContentFolderType::Content;
	}
} // SE