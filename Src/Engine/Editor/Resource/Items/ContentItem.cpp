
#include "ContentItem.h"

#include "ContentFolder.h"
#include "Core/Platform/FileSystem.h"
#include "Editor/EditorIcons.h"
#include "Editor/GUI/Drag/DragItems.hpp"
#include "Editor/Modules/UIModule.h"
#include "Editor/Resource/ContentView.h"
#include "Editor/Resource/Thumbnails/Thumbnails.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	Rectangle ContentItem::__GetTextRectangle()
	{
		// Skip when hidden
		if (!Visible)
			return Rectangle::Empty;

		ContentView* view = TypeTryCast<ContentView>(Parent);
		Float2 size = Size;
		ContentViewType viewType = ContentViewType::Tiles;
		if (view != nullptr)
		{
			viewType = view->ViewType;
		}
		switch (viewType)
		{
			case ContentViewType::Tiles:
			{
				float textHeight = DefaultTextHeight * size.x / DefaultWidth;
				return Rectangle(0, size.y - textHeight, size.x, textHeight);
			}
			case ContentViewType::List:
			{
				float thumbnailSize = size.y - 2 * DefaultMarginSize;
				float textHeight = Math::Min(size.y, 24.0f);
				return Rectangle(thumbnailSize + DefaultMarginSize * 2, (size.y - textHeight) * 0.5f, size.x - textHeight - DefaultMarginSize * 3.0f, textHeight);
			}
		}

		return Rectangle::Empty;
	}

	ContentFolder* ContentItem::__GetParentFolder()
	{
		return _parentFolder;
	};

	void ContentItem::__SetParentFolder(ContentFolder* value)
	{
		if (_parentFolder == value)
			return;

		// Remove from old
		if (_parentFolder != nullptr)
		{
			_parentFolder->Children.Remove(this);
		}

		// Link
		_parentFolder = value;

		// Add to new
		if (_parentFolder != nullptr)
		{
			_parentFolder->Children.Add(this);
		}

		OnParentFolderChanged();
	};

	void ContentItem::UpdatePath(String value)
	{
		// Set path
		FileSystem::NormalizePath(value);
		Path = value;
		FileName = FileSystem::GetFileName(value);
		ShortName = FileSystem::GetFileNameWithoutExtension(value);

		// Fire event
		OnPathChanged();
		for (int i = 0; i < _references.Count(); i++)
		{
			_references[i]->OnItemRenamed(this);
		}
	}

	void ContentItem::RefreshThumbnail()
	{
		// Skip if item has default thumbnail
		if (HasDefaultThumbnail)
			return;

		Thumbnails* thumbnails = EditorApp::Ins().uiModule->thumbnails;

		// Delete old thumbnail and remove it from the cache
		thumbnails->DeletePreview(this);

		// Request new one (if need to)
		if (_references.Count() > 0)
		{
			thumbnails->RequestPreview(this);
		}
	}

	void ContentItem::UpdateTooltipText()
	{
		/*StringBuilder sb = StringBuilder();
		OnBuildTooltipText(sb);
		if (sb.Length() != 0 && sb[sb.Length() - 1] == '\n')
		{
			// Remove new-line from end
			int sub = 1;
			if (sb.Length() != 1 && sb[sb.Length() - 2] == '\r')
				sub = 2;
			sb.Length() -= sub;
		}
		TooltipText = sb.ToString();*/
	}

	ContentItem* ContentItem::Find(String path)
	{
		return Path == path ? this : nullptr;
	}

	bool ContentItem::Find(ContentItem* item)
	{
		return this == item;
	}

	ContentItem* ContentItem::Find(UID id)
	{
		return nullptr;
	}

	void ContentItem::DrawThumbnail(Rectangle rectangle)
	{
		// Draw shadow
		/*if (DrawShadow())
		{
			const float thumbnailInShadowSize = 50.0f;
			var shadowRect = rectangle.MakeExpanded((DefaultThumbnailSize - thumbnailInShadowSize) * rectangle.Width / DefaultThumbnailSize * 1.3f);
			if (!_shadowIcon.IsValid)
				_shadowIcon = Editor.Instance.Icons.AssetShadow128;
			Render2D::DrawSprite(_shadowIcon, shadowRect);
		}*/

		// Draw thumbnail
		if (Thumbnail.IsValid())
			Render2D::DrawSprite(Thumbnail, rectangle);
		else
			Render2D::FillRectangle(rectangle, Colors::Black);
	}

	void ContentItem::DrawThumbnail(Rectangle rectangle, bool shadow)
	{
		// Draw shadow
		if (shadow)
		{
			const float thumbnailInShadowSize = 50.0f;
			Rectangle shadowRect = rectangle.MakeExpanded((DefaultThumbnailSize - thumbnailInShadowSize) * rectangle.GetWidth() / DefaultThumbnailSize * 1.3f);
			if (!_shadowIcon.IsValid())
				_shadowIcon = EditorIcons::AssetShadow128;
			Render2D::DrawSprite(_shadowIcon, shadowRect);
		}

		// Draw thumbnail
		if (Thumbnail.IsValid())
			Render2D::DrawSprite(Thumbnail, rectangle);
		else
			Render2D::FillRectangle(rectangle, Colors::Black);
	}

	void ContentItem::AddReference(IContentItemOwner* obj)
	{
		ENGINE_ASSERT(obj != nullptr);
		ENGINE_ASSERT(!_references.Contains(obj))
		_references.Add(obj);

		// Check if need to generate preview
		if (_references.Count() == 1 && !Thumbnail.IsValid())
		{
			RequestThumbnail();
		}
	}

	void ContentItem::RemoveReference(IContentItemOwner* obj)
	{
		if (_references.Remove(obj))
		{
			// Check if need to release the preview
			if (_references.Count() == 0 && Thumbnail.IsValid())
			{
				ReleaseThumbnail();
			}
		}
	}

	void ContentItem::OnDelete()
	{
		// Fire event
		while (_references.Count() > 0)
		{
			IContentItemOwner* reference = _references[0];
			reference->OnItemDeleted(this);
			RemoveReference(reference);
		}

		// Release thumbnail
		if (Thumbnail.IsValid())
		{
			ReleaseThumbnail();
		}
	}

	void ContentItem::NavigationFocus()
	{
		Control::NavigationFocus();

		if (IsFocused)
		{
			ContentView* view = TypeTryCast<ContentView>(Parent);
			if (view != nullptr)
			{
				view->Select(this);
			}
		}
	}

	void ContentItem::Draw()
	{
		Float2 size = Size;
		Style* style = Style::Current;
		ContentView* view = TypeTryCast<ContentView>(Parent);
		bool isSelected = view->IsSelected(this);
		Rectangle clientRect = Rectangle(Float2::Zero, size);
		Rectangle textRect = TextRectangle;
		Rectangle thumbnailRect;
		TextAlignment nameAlignment;
		switch (view->ViewType)
		{
			case ContentViewType::Tiles:
			{
				float thumbnailSize = size.x;
				thumbnailRect = Rectangle(0, 0, thumbnailSize, thumbnailSize);
				nameAlignment = TextAlignment::Center;

				if (TypeIs<ContentFolder>(this))
				{
					// Small shadow
					Rectangle shadowRect = Rectangle(2, 2, clientRect.GetWidth() + 1, clientRect.GetHeight() + 1);
					Color color = Colors::Black.AlphaMultiplied(0.2f);
					Render2D::FillRectangle(shadowRect, color);
					Render2D::FillRectangle(clientRect, style->Background.RGBMultiplied(1.25f));

					float accentHeight = 2 * view->ViewScale;
					Rectangle barRect = Rectangle(0, thumbnailRect.GetHeight() - accentHeight, clientRect.GetWidth(), accentHeight);
					Render2D::FillRectangle(barRect, Colors::DimGray);

					if (isSelected)
						Render2D::FillRectangle(clientRect, Parent->ContainsFocus() ? style->BackgroundSelected : style->LightBackground);
					else if (IsMouseOver)
						Render2D::FillRectangle(clientRect, style->BackgroundHighlighted);

					DrawThumbnail(thumbnailRect, false);
				}
				else
				{
					// Small shadow
					Rectangle shadowRect = Rectangle(2, 2, clientRect.GetWidth() + 1, clientRect.GetHeight() + 1);
					Color color = Colors::Black.AlphaMultiplied(0.2f);
					Render2D::FillRectangle(shadowRect, color);

					Render2D::FillRectangle(clientRect, style->Background.RGBMultiplied(1.25f));
					Render2D::FillRectangle(TextRectangle, style->LightBackground);

					DrawThumbnail(thumbnailRect, false);

					float accentHeight = 2 * view->ViewScale;
					Rectangle barRect = Rectangle(0, thumbnailRect.GetHeight() - accentHeight, clientRect.GetWidth(), accentHeight);
					Render2D::FillRectangle(barRect, Colors::DimGray);

					if (isSelected)
					{
						Render2D::FillRectangle(textRect, Parent->ContainsFocus() ? style->BackgroundSelected : style->LightBackground);
						Render2D::DrawRectangle(clientRect, Parent->ContainsFocus() ? style->BackgroundSelected : style->LightBackground);
					}
					else if (IsMouseOver)
					{
						Render2D::FillRectangle(textRect, style->BackgroundHighlighted);
						Render2D::DrawRectangle(clientRect, style->BackgroundHighlighted);
					}
				}
				break;
			}
			case ContentViewType::List:
			{
				float thumbnailSize = size.y - 2 * DefaultMarginSize;
				thumbnailRect = Rectangle(DefaultMarginSize, DefaultMarginSize, thumbnailSize, thumbnailSize);
				nameAlignment = TextAlignment::Near;

				if (isSelected)
					Render2D::FillRectangle(clientRect, Parent->ContainsFocus() ? style->BackgroundSelected : style->LightBackground);
				else if (IsMouseOver)
					Render2D::FillRectangle(clientRect, style->BackgroundHighlighted);

				DrawThumbnail(thumbnailRect);
				break;
			}
		}

		// Draw short name
		Render2D::PushClip(textRect);
		Render2D::RenderText(style->FontMedium, ShowFileExtension || view->ShowFileExtensions ? FileName : ShortName, textRect, style->TextColor,
			nameAlignment, TextAlignment::Center, TextWrapping::WrapWords, 1.0f, 0.95f);
		Render2D::PopClip();
	}


	bool ContentItem::OnMouseDown(Float2 location, MouseButton button)
	{
		Focus();

		if (button == MouseButton::Left)
		{
			// Cache data
			_isMouseDown = true;
			_mouseDownStartPos = location;
		}

		return true;
	}

	bool ContentItem::OnMouseUp(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && _isMouseDown)
		{
			// Clear flag
			_isMouseDown = false;

			// Fire event
			ContentView* otherItem = TypeTryCast<ContentView>(this);
			if (otherItem != nullptr)
			{
				otherItem->OnItemClick(this);
			}
		}

		return Control::OnMouseUp(location, button);
	}

	bool ContentItem::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		Focus();

		ContentView* otherItem = TypeTryCast<ContentView>(Parent);
		if (otherItem != nullptr)
		{
			// Open
			otherItem->OnItemDoubleClick(this);
		}

		return true;
	}

	void ContentItem::OnMouseMove(Float2 location)
	{
		// Check if start drag and drop
		if (_isMouseDown && Float2::Distance(_mouseDownStartPos, location) > 10.0f)
		{
			// Clear flag
			_isMouseDown = false;

			// Start drag drop
			DoDrag();
		}
	}

	void ContentItem::OnMouseLeave()
	{
		// Check if start drag and drop
		if (_isMouseDown)
		{
			// Clear flag
			_isMouseDown = false;

			// Start drag drop
			DoDrag();
		}

		Control::OnMouseLeave();
	}

	void ContentItem::OnSubmit()
	{
		// Open
		/*AssetView* otherItem = TypeTryCast<AssetView>(other);
		(Parent as ContentView).OnItemDoubleClick(this);*/

		Control::OnSubmit();
	}

	int ContentItem::Compare(const Control* other) const
	{
		const ContentItem* otherItem = TypeTryCast<ContentItem>(other);
		if (otherItem != nullptr)
		{
			if (otherItem->IsFolder)
				return 1;
			return StringUtils::Compare(ShortName.Get(), otherItem->ShortName.Get());
		}

		return Control::Compare(other);
	}

	void ContentItem::OnDestroy()
	{
		// Fire event
		while (_references.Count() > 0)
		{
			IContentItemOwner* reference = _references[0];
			reference->OnItemDispose(this);
			RemoveReference(reference);
		}

		// Release thumbnail
		if (Thumbnail.IsValid())
		{
			ReleaseThumbnail();
		}

		Control::OnDestroy();
	}


	ContentItem::ContentItem(StringView path) : Control(0, 0, DefaultWidth, DefaultHeight)
	{
		// Set path
		Path = path;
		FileName = FileSystem::GetFileName(path);
		ShortName = FileSystem::GetFileNameWithoutExtension(path);
	}

	void ContentItem::RequestThumbnail()
	{
		EditorApp::Ins().uiModule->thumbnails->RequestPreview(this);
	}

	void ContentItem::ReleaseThumbnail()
	{
		// Simply unlink sprite
		Thumbnail = SpriteHandle::Invalid;
	}

	void ContentItem::OnReimport()
	{
		for (int i = 0; i < _references.Count(); i++)
			_references[i]->OnItemReimported(this);
		RefreshThumbnail();
	}

	void ContentItem::DoDrag()
	{
		if (!CanDrag)
			return;

		DragData* data = nullptr;

		// Check if is selected
		ContentView* view;
		if (TypeTryCast<ContentView>(Parent, view) && view->IsSelected(this))
		{
			// Drag selected item
			data = DragItems::GetDragData(view->GetSelection());
		}
		else
		{
			// Drag single item
			data = DragItems::GetDragData(this);
		}

		// Start drag operation
		DoDragDrop(data);
	}

	void ContentItem::OnBuildTooltipText(StringBuilder sb)
	{
		/*sb.Append("Type: ").Append(TypeDescription).AppendLine();
		if (FileSystem::FileExists(Path))
		{
			sb.Append("Size: ").Append(Utilities.Utils.FormatBytesCount((int)new FileInfo(Path).Length)).AppendLine();
		}
		sb.Append("Path: ").Append(Utilities.Utils.GetAssetNamePathWithExt(Path)).AppendLine();*/
	}

	bool ContentItem::__GetCanRename()
	{
		return true;
	}

	bool ContentItem::__GetCanDrag()
	{
		return Root != nullptr;
	}

	bool ContentItem::__GetExists()
	{
		return FileSystem::FileExists(Path);
	}

	SpriteHandle ContentItem::__GetDefaultThumbnail()
	{
		return SpriteHandle::Invalid;
	}

	bool ContentItem::__GetHasDefaultThumbnail()
	{
		return false;
	}
} // SE