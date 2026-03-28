
#include "TreeNode.h"

#include "Tree.h"
#include "Runtime/Engine.h"
#include "Runtime/Render/2D/Font.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/Utilities/Time.h"

namespace SE::Editor
{
	uint64 TreeNode::_dragEndFrame = 0;

	TreeNode::TreeNode() : _animationProgress(1.0f),
		_cachedHeight(_headerHeight), _iconCollaped(SpriteHandle::Invalid), _iconOpened(SpriteHandle::Invalid), _mouseDownTime(-1)
	{
	}

	TreeNode::TreeNode(bool canChangeOrder) : TreeNode(canChangeOrder, SpriteHandle::Invalid, SpriteHandle::Invalid)
	{
	}

	TreeNode::TreeNode(bool canChangeOrder, SpriteHandle iconCollapsed, SpriteHandle iconOpened) : ContainerControl(0, 0, 64, 16)
	{
		_canChangeOrder = canChangeOrder;
		_animationProgress = 1.0f;
		_cachedHeight = _headerHeight;
		_iconCollaped = iconCollapsed;
		_iconOpened = iconOpened;
		_mouseDownTime = -1;

		Style* style = Style::Current;
		TextColor = style->TextColor;
		BackgroundColorSelected = style->BackgroundSelected;
		BackgroundColorHighlighted = style->BackgroundHighlighted;
		BackgroundColorSelectedUnfocused = style->LightBackground;
		TextFont = FontReference(style->FontSmall);
	}

	void TreeNode::Expand(bool noAnimation)
	{
		// Parents first
		ExpandAllParents(noAnimation);

		// Change state
		if (_opened && _animationProgress >= 1.0f)
			return;
		bool prevState = _opened;
		_opened = true;
		if (noAnimation)
			_animationProgress = 1.0f;
		else if (prevState != _opened)
			_animationProgress = 1.0f - _animationProgress;

		// Update
		OnExpandedChanged();
		OnExpandAnimationChanged();
	}

	void TreeNode::Collapse(bool noAnimation)
	{
		// Change state
		if (!_opened && _animationProgress >= 1.0f)
			return;
		bool prevState = _opened;
		_opened = false;
		if (noAnimation)
			_animationProgress = 1.0f;
		else if (prevState != _opened)
			_animationProgress = 1.0f - _animationProgress;

		// Update
		OnExpandedChanged();
		OnExpandAnimationChanged();
	}

	void TreeNode::ExpandAll(bool noAnimation)
	{
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		Expand(noAnimation);

		for (int i = 0; i < m_Children.Count(); i++)
		{
			TreeNode* node = TypeTryCast<TreeNode>(m_Children[i]);
			if (node != nullptr)
			{
				node->ExpandAll(noAnimation);
			}
		}

		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void TreeNode::CollapseAll(bool noAnimation)
	{
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		Collapse(noAnimation);

		for (int i = 0; i < m_Children.Count(); i++)
		{
			TreeNode* node = TypeTryCast<TreeNode>(m_Children[i]);
			if (node != nullptr)
			{
				node->CollapseAll(noAnimation);
			}
		}

		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void TreeNode::ExpandAllParents(bool noAnimation)
	{
		TreeNode* treeNode = TypeTryCast<TreeNode>(Parent);
		if (treeNode != nullptr)
		{
			treeNode->Expand(noAnimation);
		}
	}

	void TreeNode::EndAnimation()
	{
		if (_animationProgress < 1.0f)
		{
			_animationProgress = 1.0f;
			OnExpandAnimationChanged();
		}
	}

	void TreeNode::Select()
	{
		ParentTree->Select(this);
	}

	void TreeNode::Update(float deltaTime)
	{
		// Drop/down animation
		if (_animationProgress < 1.0f)
		{
			bool isDeltaSlow = deltaTime > (1 / 20.0f);

			// Update progress
			if (isDeltaSlow)
			{
				_animationProgress = 1.0f;
			}
			else
			{
				const float openCloseAnimationTime = 0.1f;
				_animationProgress += deltaTime / openCloseAnimationTime;
				if (_animationProgress > 1.0f)
					_animationProgress = 1.0f;
			}

			// Arrange controls
			OnExpandAnimationChanged();
		}

		// Check for long press
		const float longPressTimeSeconds = 0.6f;
		if (_isMouseDown && Time::GetUnscaledGameTime() - _mouseDownTime > longPressTimeSeconds)
		{
			OnLongPress();
		}

		// Don't update collapsed children
		if (_opened)
		{
			ContainerControl::Update(deltaTime);
		}
	}

	void TreeNode::Draw()
	{
		// Cache data
		Style* style = Style::Current;
		Tree* tree = ParentTree;
		bool isSelected = tree->Selection.Contains(this);
		bool isFocused = tree->ContainsFocus();
		float left = _xOffset + 16; // offset + arrow
		Rectangle textRect = {left, 0, Width - left, _headerHeight};
		_margin.ShrinkRectangle(textRect);

		// Draw background
		if (isSelected || _mouseOverHeader)
		{
			Render2D::FillRectangle(_headerRect, (isSelected && isFocused) ? BackgroundColorSelected : (_mouseOverHeader ? BackgroundColorHighlighted : BackgroundColorSelectedUnfocused));
		}

		// Draw arrow
		if (HasAnyVisibleChild)
		{
			Render2D::DrawSprite(_opened ? style->ArrowDown : style->ArrowRight, ArrowRect, _mouseOverHeader ? style->Foreground : style->ForegroundGrey);
		}

		// Draw icon
		if (_iconCollaped.IsValid())
		{
			Render2D::DrawSprite(_opened ? _iconOpened : _iconCollaped, Rectangle(textRect.GetLeft(), 0, 16, 16), IconColor);
			textRect.SetX(textRect.GetX() + 18.0f);
			textRect.SetWidth(textRect.GetWidth() - 18.0f);
		}

		// Draw text
		Color textColor = CacheTextColor();
		Render2D::RenderText(TextFont.GetFont(), _text, textRect, textColor, TextAlignment::Near, TextAlignment::Center);

		// Draw drag and drop effect
		if (IsDragOver() && _tree->DraggedOverNode == this)
		{
			switch (_dragOverMode)
			{
			case DragItemPositioning::At:
				Render2D::FillRectangle(textRect, style->Selection);
				Render2D::DrawRectangle(textRect, style->SelectionBorder);
				break;
			case DragItemPositioning::Above:
				Render2D::DrawRectangle({textRect.GetX(), textRect.GetTop() - DefaultDragInsertPositionMargin * 0.5f - DefaultNodeOffsetY - _margin.Top, textRect.GetWidth(), DefaultDragInsertPositionMargin}, style->SelectionBorder);
				break;
			case DragItemPositioning::Below:
				Render2D::DrawRectangle({textRect.GetX(), textRect.GetBottom() + _margin.Bottom - DefaultDragInsertPositionMargin * 0.5f, textRect.GetWidth(), DefaultDragInsertPositionMargin}, style->SelectionBorder);
				break;
			}
		}

		// Show tree guide lines
		if (tree != nullptr)
		{
			TreeNode* parentNode =  TypeTryCast<TreeNode>(Parent);
			bool thisNodeIsLast = false;
			while (parentNode != nullptr && parentNode != ParentTree->GetChild(0))
			{
				float bottomOffset = 0;
				float topOffset = 0;

				if (Parent == parentNode && this == Parent->GetChild(0))
					topOffset = 2;

				if (thisNodeIsLast && parentNode->Children().Count() == 1)
					bottomOffset = topOffset != 0 ? 4 : 2;

				if (Parent == parentNode && this == Parent->GetChild(Parent->Children().Count() - 1) && !_opened)
				{
					thisNodeIsLast = true;
					bottomOffset = topOffset != 0 ? 4 : 2;
				}

				float leftOffset = 9;
				// Adjust offset for icon image
				if (_iconCollaped.IsValid())
					leftOffset += 18;
				Rectangle lineRect1 = Rectangle(parentNode->TextRect.operator->().GetLeft() - leftOffset, parentNode->HeaderRect.operator->().GetTop() + topOffset,
					1, parentNode->HeaderRect.operator->().GetHeight() - bottomOffset);
				Render2D::FillRectangle(lineRect1, isSelected ? style->ForegroundGrey : style->LightBackground);
				parentNode = TypeTryCast<TreeNode>(parentNode->Parent);
			}
		}

		// Base
		if (_opened)
		{
			if (ClipChildren)
			{
				Render2D::PushClip(Rectangle(0, _headerHeight, Width, Height - _headerHeight));
				ContainerControl::Draw();
				Render2D::PopClip();
			}
			else
			{
				ContainerControl::Draw();
			}
		}
	}

	bool TreeNode::OnMouseDown(Float2 location, MouseButton button)
	{
		UpdateMouseOverFlags(location);

		// Check if mouse hits bar and node isn't a root
		if (_mouseOverHeader)
		{
			// Check if left button goes down
			if (button == MouseButton::Left)
			{
				_isMouseDown = true;
				_mouseDownPos = location;
				_mouseDownTime = 0;//Time::UnscaledGameTime;
			}

			// Handled
			Focus();
			return true;
		}

		// Base
		if (_opened)
			return ContainerControl::OnMouseDown(location, button);

		// Handled
		Focus();
		return true;
	}

	bool TreeNode::OnMouseUp(Float2 location, MouseButton button)
	{
		UpdateMouseOverFlags(location);

		// Clear flag for left button
		if (button == MouseButton::Left && _isMouseDown)
		{
			_isMouseDown = false;
			_mouseDownTime = -1;
		}

		// Check if mouse hits bar and node isn't a root
		if (_mouseOverHeader)
		{
			// Skip mouse up event right after drag drop ends
			if (button == MouseButton::Left && Engine::FrameCount - _dragEndFrame < 10)
				return true;

			// Prevent from selecting node when user is just clicking at an arrow
			if (!_mouseOverArrow)
			{
				// Check if user is pressing control key
				Tree* tree = ParentTree;
				RootControl* window = tree->Root;
				if (window->GetKey(KeyboardKeys::Shift))
				{
					// Select range
					tree->SelectRange(this);
				}
				else if (window->GetKey(KeyboardKeys::Control))
				{
					// Add/Remove
					tree->AddOrRemoveSelection(this);
				}
				else if (button == MouseButton::Right && tree->Selection.Contains(this))
				{
					// Do nothing
				}
				else
				{
					// Select
					tree->Select(this);
				}
			}

			// Check if mouse hits arrow
			if (_mouseOverArrow && HasAnyVisibleChild)
			{
				if (ParentTree->Root->GetKey(KeyboardKeys::Alt))
				{
					if (_opened)
						CollapseAll();
					else
						ExpandAll();
				}
				else
				{
					if (_opened)
						Collapse();
					else
						Expand();
				}
			}

			// Check if mouse hits bar
			if (button == MouseButton::Right && TestHeaderHit(location))
			{
				ParentTree->OnRightClickInternal(this, location);
			}

			// Handled
			Focus();
			return true;
		}

		// Check if mouse hits bar
		if (button == MouseButton::Right && TestHeaderHit(location))
		{
			ParentTree->OnRightClickInternal(this, location);
		}

		// Base
		return ContainerControl::OnMouseUp(location, button);
	}

	bool TreeNode::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		// Check if mouse hits bar
		if (TestHeaderHit(location))
		{
			return OnMouseDoubleClickHeader(location, button);
		}

		// Check if animation has been finished
		if (_animationProgress >= 1.0f)
		{
			// Base
			return ContainerControl::OnMouseDoubleClick(location, button);
		}

		return false;
	}

	void TreeNode::OnMouseMove(Float2 location)
	{
		UpdateMouseOverFlags(location);

		// Check if start drag and drop
		if (_isMouseDown && Float2::Distance(_mouseDownPos, location) > 10.0f)
		{
			// Clear flag
			_isMouseDown = false;
			_mouseDownTime = -1;

			// Start
			BeginDragDrop();
			return;
		}

		// Check if animation has been finished
		if (_animationProgress >= 1.0f)
		{
			// Base
			if (_opened)
				ContainerControl::OnMouseMove(location);
		}
	}

	void TreeNode::OnMouseLeave()
	{
		// Clear flags
		_mouseOverArrow = false;
		_mouseOverHeader = false;

		// Check if start drag and drop
		if (_isMouseDown)
		{
			// Clear flag
			_isMouseDown = false;
			_mouseDownTime = -1;

			// Start
			BeginDragDrop();
		}

		// Base
		ContainerControl::OnMouseLeave();
	}

	bool TreeNode::OnKeyDown(KeyboardKeys key)
	{
		// Base
		if (_opened)
			return ContainerControl::OnKeyDown(key);
		return false;
	}

	void TreeNode::OnKeyUp(KeyboardKeys key)
	{
		// Base
		if (_opened)
			ContainerControl::OnKeyUp(key);
	}

	void TreeNode::OnChildResized(Control* control)
	{
		// Optimize if child is tree node that is not visible
		if (!_opened && TypeAs<TreeNode>(control))
			return;

		PerformLayout();

		ContainerControl::OnChildResized(control);
	}

	DragDropEffect TreeNode::OnDragEnter(const Float2& location, DragData* data)
	{
		DragDropEffect result = ContainerControl::OnDragEnter(location, data);

		// Check if no children handled that event
		_dragOverMode = DragItemPositioning::None;
		if (result == DragDropEffect::None)
		{
			UpdateDragPositioning(location);

			// Check if mouse is over header
			_isDragOverHeader = TestHeaderHit(location);
			if (_isDragOverHeader)
			{
				if (ParentTree != nullptr)
					ParentTree->DraggedOverNode = this;

				// Expand node if mouse goes over arrow
				if (ArrowRect.operator->().Contains(location) && HasAnyVisibleChild)
					Expand(true);

				result = OnDragEnterHeader(data);
			}

			if (result == DragDropEffect::None)
				_dragOverMode = DragItemPositioning::None;
		}

		return result;
	}

	DragDropEffect TreeNode::OnDragMove(const Float2& location, DragData* data)
	{
		DragDropEffect result = ContainerControl::OnDragMove(location, data);

		// Check if no children handled that event
		ClearDragPositioning();
		if (result == DragDropEffect::None)
		{
			UpdateDragPositioning(location);

			// Check if mouse is over header
			bool isDragOverHeader = TestHeaderHit(location);
			if (isDragOverHeader)
			{
				if (ParentTree != nullptr)
					ParentTree->DraggedOverNode = this;

				// Expand node if mouse goes over arrow
				if (ArrowRect.operator->() .Contains(location) && HasAnyVisibleChild)
					Expand(true);

				if (!_isDragOverHeader)
					result = OnDragEnterHeader(data);
				else
					result = OnDragMoveHeader(data);
			}
			else if (_isDragOverHeader)
			{
				OnDragLeaveHeader();
			}
			_isDragOverHeader = isDragOverHeader;

			if (result == DragDropEffect::None)
				_dragOverMode = DragItemPositioning::None;
		}

		return result;
	}

	DragDropEffect TreeNode::OnDragDrop(const Float2& location, DragData* data)
	{
		DragDropEffect result = ContainerControl::OnDragDrop(location, data);

		// Check if no children handled that event
		if (result == DragDropEffect::None)
		{
			UpdateDragPositioning(const_cast<Float2&>(location));
			_dragEndFrame = Engine::FrameCount;

			// Check if mouse is over header
			if (TestHeaderHit(const_cast<Float2&>(location)))
			{
				result = OnDragDropHeader(data);
			}
		}

		// Clear cache
		_isDragOverHeader = false;
		ClearDragPositioning();

		return result;
	}

	void TreeNode::OnDragLeave()
	{
		// Clear cache
		if (_isDragOverHeader)
		{
			_isDragOverHeader = false;
			OnDragLeaveHeader();
		}
		ClearDragPositioning();

		ContainerControl::OnDragLeave();
	}

	bool TreeNode::OnTestTooltipOverControl(Float2 location)
	{
		return TestHeaderHit(location) && ShowTooltip();
	}

	void TreeNode::PerformLayout(bool force)
	{
		if (GetIsLayoutLocked() && !force)
		{
			return;
		}

		bool wasLocked = GetIsLayoutLocked();
		if (!wasLocked)
		{
			LockChildrenRecursive();
		}

		// Auto-size tree nodes to match the parent size
		ContainerControl* parent = Parent;
		float width = TypeAs<TreeNode>(parent) ? parent->Width : Width;

		// Optimize layout logic if node is collapsed
		if (_opened || _animationProgress < 1.0f)
		{
			Width = width;
			PerformLayoutBeforeChildren();
			for (int i = 0; i < m_Children.Count(); i++)
			{
				m_Children[i]->PerformLayout(true);
			}
			PerformLayoutAfterChildren();
		}
		else
		{
			// TODO: perform layout for any non-TreeNode controls
			_cachedHeight = _headerHeight;
			Size = Float2(width, _headerHeight);
		}

		if (!wasLocked)
			UnlockChildrenRecursive();
	}

	int TreeNode::Compare(const Control* other) const
	{
		const TreeNode* node = TypeTryCast<TreeNode>(other);
		if (node != nullptr)
		{
			return StringUtils::Compare(Text.operator->().Get(), node->Text.operator->().Get());
		}
		return ContainerControl::Compare(other);
	}

	void TreeNode::OnDestroy()
	{
		if (ParentTree != nullptr)
		{
			ParentTree->Selection.Remove(this);
		}

		ContainerControl::OnDestroy();
	}

	bool TreeNode::OnMouseDoubleClickHeader(Float2& location, MouseButton button)
	{
		if (HasAnyVisibleChild)
		{
			// Toggle open state
			if (_opened)
				Collapse();
			else
				Expand();
		}

		// Handled
		return true;
	}

	void TreeNode::OnExpandAnimationChanged()
	{
		if (ParentTree != nullptr)
			ParentTree->PerformLayout();
		else if (Parent != nullptr)
			Parent->PerformLayout();
		else
			PerformLayout();
	}

	bool TreeNode::TestHeaderHit(const Float2& location)
	{
		return _headerRect.Contains(location);
	}

	Color TreeNode::CacheTextColor()
	{
		return Enabled ? TextColor : TextColor * 0.6f;
	}

	void TreeNode::UpdateTextWidth()
	{
		if (_textChanged)
		{
			Font* font = TextFont.GetFont();
			if (font)
			{
				_textWidth = font->MeasureText(_text).x;
				_textChanged = false;
			}
		}
	}

	void TreeNode::DrawChildren()
	{
		// Draw all visible child controls
		List<Control*>& children = m_Children;
		if (children.Count() == 0)
			return;

		auto GetChildGlobalRectangle = [](Control* control, Matrix3x3 globalTransform)
		{
			Matrix3x3 globalChildTransform;
			Matrix3x3::Multiply(control->GetCachedTransform(), globalTransform, globalChildTransform);
			return Rectangle(globalChildTransform.M31, globalChildTransform.M32, control->Width * globalChildTransform.M11, control->Height * globalChildTransform.M22);
		};

		if (CullChildren)
		{
			Rectangle globalClipping = Rectangle(0, 0, 0, 0);
			Matrix3x3 globalTransform = Matrix3x3::Identity;
			Render2D::PeekClip(globalClipping);
			Render2D::PeekTransform(globalTransform);

			// Try to estimate the rough location of the first node, assuming the node height is constant
			Rectangle firstChildGlobalRect = GetChildGlobalRectangle(children[0], globalTransform);
			int firstVisibleChild = Math::Clamp((int)Math::Floor((globalClipping.GetY() - firstChildGlobalRect.GetTop()) / _headerHeight) + 1, 0, children.Count() - 1);
			if (GetChildGlobalRectangle(children[firstVisibleChild], globalTransform).GetTop() > globalClipping.GetTop() || !children[firstVisibleChild]->Visible)
			{
				// Estimate overshoot, either it's partially visible or hidden in the tree
				for (; firstVisibleChild > 0; firstVisibleChild--)
				{
					Control* child = children[firstVisibleChild];
					if (!child->Visible)
						continue;

					if (GetChildGlobalRectangle(child, globalTransform).GetTop() < globalClipping.GetTop())
						break;
				}
			}

			for (int i = firstVisibleChild; i < children.Count(); i++)
			{
				Control* child = children[i];
				if (!child->Visible)
					continue;

				Rectangle childGlobalRect = GetChildGlobalRectangle(child, globalTransform);
				if (!globalClipping.Intersects(childGlobalRect))
					break;

				Render2D::PushTransform(child->GetCachedTransform());
				child->Draw();
				Render2D::PopTransform();
			}


		}
		else
		{
			for (int i = 0; i < children.Count(); i++)
			{
				Control* child = children[i];
				if (child->Visible)
				{
					Render2D::PushTransform(child->GetCachedTransform());
					child->Draw();
					Render2D::PopTransform();
				}
			}
		}
	}

	void TreeNode::OnSizeChanged()
	{
		ContainerControl::OnSizeChanged();

		_headerRect = Rectangle(0, 0, Width, _headerHeight);
	}

	bool TreeNode::CanNavigateChild(Control* child)
	{
		// Closed tree node skips navigation for hidden children
		if (IsCollapsed && TypeAs<TreeNode>(child))
			return false;
		return ContainerControl::CanNavigateChild(child);
	}

	void TreeNode::OnParentChangedInternal()
	{
		_tree = nullptr;

		ContainerControl::OnParentChangedInternal();
	}

	void TreeNode::PerformLayoutAfterChildren()
	{
		float y = _headerHeight;
		float height = _headerHeight;
		float xOffset = _xOffset + ChildrenIndent;

		// Skip full layout if it's fully collapsed
		if (_opened || _animationProgress < 1.0f)
		{
			y -= _cachedHeight * (_opened ? 1.0f - _animationProgress : _animationProgress);
			for (int i = 0; i < m_Children.Count(); i++)
			{
				TreeNode* node;
				if (TypeTryCast<TreeNode>(m_Children[i], node) && node->Visible)
				{
					node->_xOffset = xOffset;
					node->Location = Float2(0, y);
					float nodeHeight = node->Height + DefaultNodeOffsetY;
					y += nodeHeight;
					height += nodeHeight;
				}
			}
		}

		_cachedHeight = height;
		Height = Math::Max(_headerHeight, y);
	}

	void TreeNode::PerformLayoutBeforeChildren()
	{
		if (_opened)
		{
			// Update the nodes nesting level before the actual positioning
			float xOffset = _xOffset + ChildrenIndent;
			for (int i = 0; i < m_Children.Count(); i++)
			{
				TreeNode* node;
				if (TypeTryCast<TreeNode>(m_Children[i], node))
				{
					node->_xOffset = xOffset;
				}
			}
		}

		ContainerControl::PerformLayoutBeforeChildren();
	}

	float TreeNode::__GetMinimumWidth()
	{
		UpdateTextWidth();

		float minWidth = _xOffset + _textWidth + 6 + 16;
		if (_iconCollaped.IsValid())
			minWidth += 16;

		if (_opened || _animationProgress < 1.0f)
		{
			for (int i = 0; i < m_Children.Count(); i++)
			{
				TreeNode* node;
				if (TypeTryCast<TreeNode>(m_Children[i], node) && node->Visible)
				{
					minWidth = Math::Max(minWidth, node->MinimumWidth.Get());
				}
			}
		}

		return minWidth;
	}

	void TreeNode::UpdateDragPositioning(const Float2& location)
	{
		// Check collision with drag areas
		if (Rectangle(_headerRect.GetX(), _headerRect.GetY() - DefaultDragInsertPositionMargin - DefaultNodeOffsetY, _headerRect.GetWidth(), DefaultDragInsertPositionMargin * 2.0f).Contains(location))
			_dragOverMode = DragItemPositioning::Above;
		else if ((IsCollapsed || !HasAnyVisibleChild) && Rectangle(_headerRect.GetY(), _headerRect.GetBottom() - DefaultDragInsertPositionMargin, _headerRect.GetWidth(), DefaultDragInsertPositionMargin * 2.0f).Contains(location))
			_dragOverMode = DragItemPositioning::Below;
		else
			_dragOverMode = DragItemPositioning::At;

		// Update DraggedOverNode
		Tree* tree = ParentTree;
		if (_dragOverMode == DragItemPositioning::None)
		{
			if (tree != nullptr && tree->DraggedOverNode == this)
				tree->DraggedOverNode = nullptr;
		}
		else if (tree != nullptr)
			tree->DraggedOverNode = this;
	}

	void TreeNode::ClearDragPositioning()
	{
		_dragOverMode = DragItemPositioning::None;
		Tree* tree = ParentTree;
		if (tree != nullptr && tree->DraggedOverNode == this)
			tree->DraggedOverNode = nullptr;
	}

	void TreeNode::UpdateMouseOverFlags(Float2 location)
	{
		// Cache flags
		_mouseOverArrow = HasAnyVisibleChild && ArrowRect.operator->().Contains(location);
		_mouseOverHeader = Rectangle(0, 0, Width, _headerHeight - 1).Contains(location);
		if (_mouseOverHeader)
		{
			// Allow non-scrollable controls to stay on top of the header and override the mouse behaviour
			for (int i = 0; i < Children().Count(); i++)
			{
				Float2 childSpaceLocation;
				if (!Children()[i]->IsScrollable && IntersectsChildContent(GetChild(i), location, childSpaceLocation))
				{
					_mouseOverHeader = false;
					break;
				}
			}
		}
	}

	bool TreeNode::__GetIsCollapsedInHierarchy()
	{
		TreeNode* parentNode = TypeTryCast<TreeNode>(Parent);

		return IsCollapsed || (parentNode != nullptr && parentNode->IsCollapsedInHierarchy);
	}

	void TreeNode::__SetText(String& value)
	{
		_text = value;
		_textChanged = true;
		PerformLayout();
	}

	bool TreeNode::__GetHasAnyVisibleChild()
	{

		bool result = false;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			TreeNode* node = TypeTryCast<TreeNode>(m_Children[i]);
			if (node && node->Visible)
			{
				result = true;
				break;
			}
		}
		return result;
	}

	DragItemPositioning TreeNode::__GetDragOverMode()
	{
		return _dragOverMode;
	}

	Rectangle TreeNode::__GetTextRect()
	{

		float left = _xOffset + 16; // offset + arrow
		Rectangle textRect = Rectangle(left, 0, Width - left, _headerHeight);

		// Margin
		_margin.ShrinkRectangle(textRect);

		// Icon
		if (_iconCollaped.IsValid())
		{
			textRect.SetX(textRect.GetX() + 18.0f);
			textRect.SetWidth(textRect.GetWidth() - 18.0f);
		}

		return textRect;
	}

	void TreeNode::__SetHeaderHeight(float value)
	{
		if (!Math::IsNearEqual(_headerHeight, value))
		{
			_headerHeight = value;
			PerformLayout();
		}
	}

	Rectangle TreeNode::__GetArrowRect()
	{
		return CustomArrowRect.has_value() ? CustomArrowRect.value() : Rectangle(_xOffset + 2 + _margin.Left, 2, 12, 12);
	}

	Tree* TreeNode::__GetParentTree()
	{
		if (_tree == nullptr)
		{
			TreeNode* upNode = TypeTryCast<TreeNode>(Parent);
			if (TypeAs<TreeNode>(Parent))
				_tree = TypeCast<TreeNode>(upNode)->ParentTree;
			else if (TypeAs<Tree>(Parent))
				_tree = TypeCast<Tree>(Parent);
		}
		return _tree;
	}

	bool TreeNode::__GetIsRoot()
	{
		return !(TypeAs<TreeNode>(Parent));
	}

	void TreeNode::__SetIsCollapsed(bool value)
	{
		if (value)
			Collapse(true);
		else
			Expand(true);
	}

	void TreeNode::__SetIsExpanded(bool value)
	{
		if (value)
			Expand(true);
		else
			Collapse(true);
	}
} // SE