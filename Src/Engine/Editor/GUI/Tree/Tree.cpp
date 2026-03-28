
#include "Tree.h"
#include "TreeNode.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	Tree::Tree() : Tree(false)
	{
	}

	Tree::Tree(bool supportMultiSelect) : ContainerControl(0, 0, 100, 100)
	{
		IsScrollable = true;
		AutoFocus = false;

		m_SupportMultiSelect = supportMultiSelect;
		m_KeyUpdateTime = KeyUpdateTimeout;
		m_Margin = {0, 0, 0, 0};
	}

	void Tree::OnRightClickInternal(TreeNode* node, Float2& location)
	{
		RightClick(node, location);
	}

	void Tree::Select(TreeNode* node)
	{
		ENGINE_ASSERT(node != nullptr);

		// Check if won't change
		if (Selection.Count() == 1 && SelectedNode == node)
			return;

		// Cache previous state
		List<TreeNode*> prev = List<TreeNode*>(Selection);

		// Update selection
		Selection.Clear();
		Selection.Add(node);

		// Ensure that node can be visible (all it's parents are expanded)
		node->ExpandAllParents();

		node->Focus();

		// Fire event
		SelectedChanged(prev, Selection);
	}

	void Tree::Select(List<TreeNode*> &nodes)
	{
		auto predicate = CreateFunc([](TreeNode* const &a, TreeNode* const &b)
		{
			return a == b;
		});

		// Check if won't change
		if (Selection.Count() == nodes.Count() && ListExtensions::SequenceEqual(Selection, nodes, predicate))
			return;

		// Cache previous state
		List<TreeNode*> prev = List<TreeNode*>(Selection);

		// Update selection
		Selection.Clear();
		if (m_SupportMultiSelect)
		{
			Selection.Add(nodes);
		}
		else if (nodes.Count() > 0)
		{
			Selection.Add(nodes[0]);
		}

		// Ensure that every selected node can be visible (all it's parents are expanded)
		// TODO: maybe use faster tree walk or faster algorythm?
		for (int i = 0; i < Selection.Count(); i++)
		{
			Selection[i]->ExpandAllParents();
		}

		// Fire event
		SelectedChanged(prev, Selection);
	}

	void Tree::Deselect()
	{
		// Check if won't change
		if (Selection.Count() == 0)
			return;

		// Cache previous state
		List<TreeNode*> prev = List<TreeNode*>(Selection);

		// Update selection
		Selection.Clear();

		// Fire event
		SelectedChanged(prev, Selection);
	}

	void Tree::AddOrRemoveSelection(TreeNode* node)
	{
		// Cache previous state
		List<TreeNode*> prev = List<TreeNode*>(Selection);

		// Check if is selected
		int index = Selection.Find(node);
		if (index != -1)
		{
			// Remove
			Selection.RemoveAt(index);
		}
		else
		{
			if (!m_SupportMultiSelect)
				Selection.Clear();

			// Add
			Selection.Add(node);
		}

		// Fire event
		SelectedChanged(prev, Selection);
	}

	void Tree::SelectRange(TreeNode* endNode)
	{
		if (m_SupportMultiSelect && Selection.Count() > 0)
		{
			// Cache previous state
			List<TreeNode*> prev = List<TreeNode*>(Selection);

			// Update selection
			Rectangle selectionRect = CalcNodeRangeRect(Selection[0]);
			for (int i = 1; i < Selection.Count(); i++)
			{
				selectionRect = Rectangle::Union(selectionRect, CalcNodeRangeRect(Selection[i]));
			}
			Rectangle endNodeRect = CalcNodeRangeRect(endNode);
			if (endNodeRect.GetTop() - Math::EPSILON <= selectionRect.GetTop())
			{
				float diff = selectionRect.GetTop() - endNodeRect.GetTop();
				selectionRect.Location.y -= diff;
				selectionRect.Size.y += diff;
			}
			else if (endNodeRect.GetBottom() + Math::EPSILON >= selectionRect.GetBottom())
			{
				float diff = endNodeRect.GetBottom() - selectionRect.GetBottom();
				selectionRect.Size.y += diff;
			}
			Selection.Clear();
			WalkSelectRangeExpandedTree(Selection, TypeTryCast<TreeNode>(m_Children[0]), selectionRect);

			auto predicate = CreateFunc([](TreeNode* const &a, TreeNode* const &b)
			{
				return a == b;
			});

			// Check if changed
			if (Selection.Count() != prev.Count() || !ListExtensions::SequenceEqual(Selection, prev, predicate))
			{
				// Fire event
				SelectedChanged(prev, Selection);
			}
		}
		else
		{
			Select(endNode);
		}
	}

	void Tree::SelectAllExpanded()
	{
		BulkSelectUpdateExpanded(true);
	}

	void Tree::DeselectAll()
	{
		BulkSelectUpdateExpanded(false);
	}

	void Tree::Update(float deltaTime)
	{
		TreeNode* node = SelectedNode;

		// Check if has focus and if any node is focused and it isn't a root
		if (ContainsFocus() && node != nullptr && node->AutoFocus)
		{
			RootControl* window = Root;
			if (window->GetKeyDown(KeyboardKeys::ArrowUp) || window->GetKeyDown(KeyboardKeys::ArrowDown))
				m_KeyUpdateTime = KeyUpdateTimeout;

			WindowRootControl* windowRoot = TypeTryCast<WindowRootControl>(window);
			if (m_KeyUpdateTime >= KeyUpdateTimeout && windowRoot != nullptr && windowRoot->Window()->IsFocused())
			{
				bool keyUpArrow = window->GetKey(KeyboardKeys::ArrowUp);
				bool keyDownArrow = window->GetKey(KeyboardKeys::ArrowDown);

				// Check if arrow flags are different
				if (keyDownArrow != keyUpArrow)
				{
					ContainerControl* nodeParent = node->Parent;
					TreeNode* parentNode = TypeTryCast<TreeNode>(nodeParent);
					int myIndex = nodeParent->GetChildIndex(node);
					ENGINE_ASSERT(myIndex != -1);

					// Up
					TreeNode* toSelect = nullptr;
					if (keyUpArrow)
					{
						if (myIndex == 0)
						{
							// Select parent
							toSelect = parentNode;
						}
						else
						{
							// Select previous parent child
							toSelect = TypeTryCast<TreeNode>(nodeParent->GetChild(myIndex - 1));

							// Select last child if is valid and expanded and has any children
							if (toSelect != nullptr && toSelect->IsExpanded && toSelect->HasAnyVisibleChild)
							{
								toSelect = TypeTryCast<TreeNode>(toSelect->GetChild(toSelect->ChildrenCount() - 1));
							}
						}
					}
					// Down
					else
					{
						if (node->IsExpanded && node->HasAnyVisibleChild)
						{
							// Select the first child
							toSelect = TypeTryCast<TreeNode>(node->GetChild(0));
						}
						else if (myIndex == nodeParent->ChildrenCount() - 1)
						{
							// Select next node after parent
							while (parentNode != nullptr && toSelect == nullptr)
							{
								int parentIndex = parentNode->IndexInParent;
								if (parentIndex != -1 && parentIndex < parentNode->Parent->ChildrenCount() - 1)
								{
									toSelect = TypeTryCast<TreeNode>(parentNode->Parent->GetChild(parentIndex + 1));
								}
								parentNode = TypeTryCast<TreeNode>(parentNode->Parent);
							}
						}
						else
						{
							// Select next parent child
							toSelect = TypeTryCast<TreeNode>(nodeParent->GetChild(myIndex + 1));
						}
					}
					if (toSelect != nullptr && toSelect->AutoFocus)
					{
						// Select
						Select(toSelect);
						toSelect->Focus();
					}

					// Reset time
					m_KeyUpdateTime = 0.0f;
				}
			}
			else
			{
				// Update time
				m_KeyUpdateTime += deltaTime;
			}

			if (window->GetKeyDown(KeyboardKeys::ArrowRight))
			{
				if (node->IsExpanded)
				{
					// Select first child if has
					if (node->HasAnyVisibleChild)
					{
						Select(TypeTryCast<TreeNode>(node->GetChild(0)));
						node->Focus();
					}
				}
				else
				{
					// Expand selected node
					node->Expand();
				}
			}
			else if (window->GetKeyDown(KeyboardKeys::ArrowLeft))
			{
				if (node->IsCollapsed)
				{
					// Select parent if has and is not a root
					TreeNode* nodeParentNode = TypeTryCast<TreeNode>(node->Parent);
					if (node->HasParent() && nodeParentNode && nodeParentNode->AutoFocus)
					{
						Select(nodeParentNode);
						nodeParentNode->Focus();
					}
				}
				else
				{
					// Collapse selected node
					node->Collapse();
				}
			}
		}

		ContainerControl::Update(deltaTime);
	}

	bool Tree::OnKeyDown(KeyboardKeys key)
	{
		// Check if can use multi selection
		if (m_SupportMultiSelect)
		{
			/*InputOptions options = Editor.Instance.Options.Options.Input;

			// Select all expanded nodes
			if (options.SelectAll.Process(this))
			{
				SelectAllExpanded();
				return true;
			}
			else if (options.DeselectAll.Process(this))
			{
				DeselectAll();
				return true;
			}*/
		}

		return ContainerControl::OnKeyDown(key);
	}

	void Tree::OnGetFocus()
	{
		// Reset timer
		m_KeyUpdateTime = 0;

		ContainerControl::OnGetFocus();
	}

	void Tree::OnParentResized()
	{
		PerformLayout();

		ContainerControl::OnParentResized();
	}

	void Tree::PerformLayoutBeforeChildren()
	{
		if (m_AutoSize)
		{
			// Use max of parent clint area width and root node width
			ContainerControl* parent = Parent;
			float tempWidth = parent != nullptr ? Math::Max(parent->GetClientArea().GetWidth(), 0.0f) : 0.0f;
			for (int i = 0; i < m_Children.Count(); i++)
			{
				TreeNode* node;
				if (TypeTryCast<TreeNode>(m_Children[i], node) && node->Visible)
				{
					tempWidth = Math::Max(node->MinimumWidth.Get(), tempWidth);
				}
			}
			for (int i = 0; i < m_Children.Count(); i++)
			{
				TreeNode* node;
				if (TypeTryCast<TreeNode>(m_Children[i], node) && node->Visible)
				{
					node->Width = tempWidth;
				}
			}
			float marginWidth = m_Margin.GetWidth();
			Width = tempWidth + marginWidth;
		}

		ContainerControl::PerformLayoutBeforeChildren();
	}

	void Tree::PerformLayoutAfterChildren()
	{
		ContainerControl::PerformLayoutAfterChildren();

		// Arrange children
		float y = m_Margin.Top;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			TreeNode* node;
			if (TypeTryCast<TreeNode>(m_Children[i], node) && node->Visible)
			{
				node->Location = Float2(m_Margin.Left, y);
				y += node->Height + TreeNode::DefaultNodeOffsetY;
			}
		}

		if (m_AutoSize)
		{
			// Update height based on the nodes
			float bottom = 0.0f;
			for (int i = 0; i < m_Children.Count(); i++)
			{
				TreeNode* node;
				if (TypeTryCast(m_Children[i], node) && node->Visible)
				{
					bottom = Math::Max(bottom, node->Bottom.operator->());
				}
			}
			float marginBottom = m_Margin.Bottom;
			Height = bottom + marginBottom;
		}
	}

	void Tree::WalkSelectExpandedTree(List<TreeNode*> selection, TreeNode* node)
	{
		for (int i = 0; i < node->ChildrenCount(); i++)
		{
			TreeNode* child = TypeTryCast<TreeNode>(node->GetChild(i));
			if (child != nullptr)
			{
				selection.Add(child);
				if (child->IsExpanded)
				{
					WalkSelectExpandedTree(selection, child);
				}
			}
		}
	}

	void Tree::BulkSelectUpdateExpanded(bool select)
	{
		if (m_SupportMultiSelect)
		{
			// Cache previous state
			List<TreeNode*> prev = List<TreeNode*>(Selection);

			// Update selection
			Selection.Clear();
			if (select)
			{
				WalkSelectExpandedTree(Selection, TypeTryCast<TreeNode>(m_Children[0]));
			}

			auto predicate = CreateFunc([](TreeNode* const &a, TreeNode* const &b)
			{
				return a == b;
			});

			// Check if changed
			if (Selection.Count() != prev.Count() || !ListExtensions::SequenceEqual(Selection, prev, predicate))
			{
				// Fire event
				SelectedChanged(prev, Selection);
			}
		}
	}

	void Tree::WalkSelectRangeExpandedTree(List<TreeNode*> selection, TreeNode* node, Rectangle& range)
	{
		for (int i = 0; i < node->ChildrenCount(); i++)
		{
			TreeNode* child = TypeTryCast<TreeNode>(node->GetChild(i));
			if ( child != nullptr && child->Visible)
			{
				Float2 pos = child->PointToParent(this, Float2::One);
				if (range.Contains(pos))
				{
					selection.Add(child);
				}

				Rectangle nodeArea = Rectangle(pos, child->Size);
				if (child->IsExpanded && range.Intersects(nodeArea))
					WalkSelectRangeExpandedTree(selection, child, range);
			}
		}
	}

	Rectangle Tree::CalcNodeRangeRect(TreeNode* node)
	{
		Float2 pos = node->PointToParent(this, Float2::One);
		return Rectangle(pos, Float2(10000, 4));
	}
} // SE