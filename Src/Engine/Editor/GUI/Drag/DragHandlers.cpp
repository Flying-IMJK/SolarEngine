#include "DragHandlers.h"

namespace SE::Editor
{
	DragDropEffect DragHandlers::OnDragEnter(DragData* data)
	{
		DragDropEffect effect = DragDropEffect::None;

		for (int i = 0; i < Count(); i++)
		{
			DragHelperBase* dragHelper = this->operator[](i);
			if (dragHelper->OnDragEnter(data))
			{
				effect = dragHelper->GetEffect();
			}
		}

		return effect;
	}

	void DragHandlers::OnDragLeave()
	{
		for (int i = 0; i < Count(); i++)
		{
			this[i].OnDragLeave();
		}
	}

	void DragHandlers::OnDragDrop(DragEventArgs* dragEventArgs)
	{
		for (int i = 0; i < Count(); i++)
		{
			this[i].OnDragDrop(dragEventArgs);
		}
	}

	bool DragHandlers::GetHasValidDrag()
	{
		for (int i = 0; i < Count(); i++)
		{
			DragHelperBase* dragHelper = this->operator[](i);
			if (dragHelper->GetHasValidDrag())
			{
				return true;
			}
		}
		return false;
	}

	DragHelperBase* DragHandlers::WithValidDrag()
	{
		if (IsEmpty())
		{
			return nullptr;
		}

		for (int i = 0; i < Count(); i++)
		{
			DragHelperBase* dragHelper = this->operator[](i);
			if (dragHelper->GetHasValidDrag())
			{
				return dragHelper;
			}
		}

		return nullptr;
	}

	DragDropEffect DragHandlers::GetEffect()
	{
		for (int i = 0; i < Count(); i++)
		{
			DragHelperBase* dragHelper = this->operator[](i);
			if (dragHelper->GetHasValidDrag())
			{
				return dragHelper->GetEffect();
			}
		}

		return DragDropEffect::None;
	}
}
