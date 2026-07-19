

#include "SpriteAtlasPacker.h"

#include "Runtime/Core/Types/Collections/Sorting.h"

namespace SE
{
	void SpriteAtlasPacker::FilterSelfSubAreas(List<Rect*>& areas)
	{
		for (int i = areas.Count() - 1; i >= 0; i--)
		{
			Rect* filtered = areas[i];
			for (int j = areas.Count() - 1; j >= 0; j--)
			{
				if (i != j)
				{
					Rect* area = areas[j];
					if (filtered->x >= area->x && filtered->y >= area->y && filtered->right <= area->right && filtered->bottom <= area->bottom)
					{
						FreeRectangle(filtered);
						Rect* topOfStack = areas.Pop();
						if (i < areas.Count())
						{
							// Move the one on the top to the freed position
							areas[i] = topOfStack;
						}
						break;
					}
				}
			}
		}
	}
	
	void SpriteAtlasPacker::GenerateNewFreeAreas(Rect* target, List<Rect*>& areas, List<Rect*>& results)
	{
		// Increase dimensions by one to get the areas on right / bottom this rectangle touches
		// Also add the padding here
		int x = target->x;
		int y = target->y;
		int right = target->right + 1 + mPadding;
		int bottom = target->bottom + 1 + mPadding;

		Rect* targetWithPadding = nullptr;
		if (mPadding == 0)
		{
			targetWithPadding = target;
		}

		for (int i = areas.Count() - 1; i >= 0; i--)
		{
			Rect* area = areas[i];
			if (!(x >= area->right || right <= area->x || y >= area->bottom || bottom <= area->y))
			{
				if (targetWithPadding == nullptr)
				{
					targetWithPadding = AllocateRectangle(target->x, target->y, target->width + mPadding, target->height + mPadding);
				}

				GenerateDividedAreas(targetWithPadding, area, results);
				Rect* topOfStack = areas.Pop();
				if (i < areas.Count())
				{
					// Move the one on the top to the freed position
					areas[i] = topOfStack;
				}
			}
		}

		if (targetWithPadding != nullptr && targetWithPadding != target)
		{
			FreeRectangle(targetWithPadding);
		}

		FilterSelfSubAreas(results);
	}
	
	void SpriteAtlasPacker::GenerateDividedAreas(Rect* divider, Rect* area, List<Rect*>& results)
	{
		int count = 0;

		int rightDelta = area->right - divider->right;
		if (rightDelta > 0)
		{
			results.Add(AllocateRectangle(divider->right, area->y, rightDelta, area->height));
			count++;
		}

		int leftDelta = divider->x - area->x;
		if (leftDelta > 0)
		{
			results.Add(AllocateRectangle(area->x, area->y, leftDelta, area->height));
			count++;
		}

		int bottomDelta = area->bottom - divider->bottom;
		if (bottomDelta > 0)
		{
			results.Add(AllocateRectangle(area->x, divider->bottom, area->width, bottomDelta));
			count++;
		}

		int topDelta = divider->y - area->y;
		if (topDelta > 0)
		{
			results.Add(AllocateRectangle(area->x, area->y, area->width, topDelta));
			count++;
		}

		if (count == 0 && (divider->width < area->width || divider->height < area->height))
		{
			// Only touching the area, store the area itself
			results.Add(area);
		}
		else
		{
			FreeRectangle(area);
		}
	}
	
	int SpriteAtlasPacker::GetFreeAreaIndex(int width, int height)
	{
		Rect* best = m_OutsideRectangle;
		int index = -1;

		int paddedWidth = width + mPadding;
		int paddedHeight = height + mPadding;

		int count = m_FreeAreas.Count();
		for (int i = count - 1; i >= 0; i--)
		{
			Rect* free = m_FreeAreas[i];
			if (free->x < mPackedWidth || free->y < mPackedHeight)
			{

				// Within the packed area, padding required
				if (free->x < best->x && paddedWidth <= free->width && paddedHeight <= free->height)
				{
					index = i;
					if ((paddedWidth == free->width && free->width <= free->height && free->right < mWidth) || (paddedHeight == free->height && free->height <= free->width))
						break;

					best = free;
				}

			} else
			{
				// Outside the current packed area, no padding required
				if (free->x < best->x && width <= free->width && height <= free->height)
				{
					index = i;
					if ((width == free->width && free->width <= free->height && free->right < mWidth) || (height == free->height && free->height <= free->width))
						break;

					best = free;
				}
			}
		}

		return index;
	}
	
	SpriteAtlasPacker::Rect* SpriteAtlasPacker::AllocateRectangle(int x, int y, int width, int height)
	{
		if (m_RectangleStack.Count() > 0)
		{
			Rect* rectangle = m_RectangleStack.Pop();
			rectangle->x = x;
			rectangle->y = y;
			rectangle->width = width;
			rectangle->height = height;
			rectangle->right = x + width;
			rectangle->bottom = y + height;

			return rectangle;
		}

		return New<Rect>(x, y, width, height);
	}
	
	void SpriteAtlasPacker::FreeRectangle(Rect* rectangle)
	{
		m_RectangleStack.Add(rectangle);
	}
	
	SpriteAtlasPacker::SortableSize SpriteAtlasPacker::AllocateSize(int width, int height, int id)
	{
		if (m_SortableSizeStack.Count() > 0) {

			SortableSize size = m_SortableSizeStack.Pop();
			size.width = width;
			size.height = height;
			size.id = id;

			return size;
		}

		return SortableSize(width, height, id);
	}

	void SpriteAtlasPacker::FreeSize(SortableSize size)
	{
		m_SortableSizeStack.Add(size);
	}

	SpriteAtlasPacker::SpriteAtlasPacker(int width, int height, int padding)
	{
		m_OutsideRectangle = New<Rect>(width + 1, height + 1, 0, 0);
		Reset(width, height, padding);
	}

	void SpriteAtlasPacker::Reset(int width, int height, int padding)
	{
		while (m_InsertedRectangles.Count() > 0)
			FreeRectangle(m_InsertedRectangles.Pop());

		while (m_FreeAreas.Count() > 0)
			FreeRectangle(m_FreeAreas.Pop());

		mWidth = width;
		mHeight = height;

		mPackedWidth = 0;
		mPackedHeight = 0;

		m_FreeAreas.Add(AllocateRectangle(0, 0, mWidth, mHeight));

		while (m_InsertList.Count() > 0)
			FreeSize(m_InsertList.Pop());

		mPadding = padding;
	}

	SpriteAtlasPacker::Rect SpriteAtlasPacker::GetRectangle(int id)
	{
		int* index = m_InsertedRectanglesMap.TryGet(id);
		if (index != nullptr)
		{
			Rect* inserted = m_InsertedRectangles[*index];
			return *inserted;
		}

		return Rect();
	}

	int SpriteAtlasPacker::GetRectangleId(int index)
	{
		Rect* inserted = m_InsertedRectangles[index];
		return inserted->id;
	}
	
	void SpriteAtlasPacker::InsertRectangle(int width, int height, int id)
	{
		SortableSize sortableSize = AllocateSize(width, height, id);
		m_InsertList.Add(sortableSize);
	}
	
	int SpriteAtlasPacker::PackRectangles(bool sort)
	{
		if (sort)
		{
			auto f = CreateFunc([]( SortableSize const &a, SortableSize const&b)
			{
				return a.width * a.height < b.height * b.height;
			});
			Sorting::QuickSort(m_InsertList, f);
		}

		while (m_InsertList.Count() > 0)
		{
			SortableSize sortableSize = m_InsertList.Pop();
			int width = sortableSize.width;
			int height = sortableSize.height;

			int index = GetFreeAreaIndex(width, height);
			if (index >= 0)
			{
				Rect* freeArea = m_FreeAreas[index];
				Rect* target = AllocateRectangle(freeArea->x, freeArea->y, width, height);
				target->id = sortableSize.id;

				// Generate the new free areas, these are parts of the old ones intersected or touched by the target
				GenerateNewFreeAreas(target, m_FreeAreas, m_NewFreeAreas);

				while (m_NewFreeAreas.Count() > 0)
					m_FreeAreas.Add(m_NewFreeAreas.Pop());

				m_InsertedRectanglesMap.Add(target->id, m_InsertedRectangles.Count());
				m_InsertedRectangles.Add(target);

				if (target->right > mPackedWidth)
					mPackedWidth = target->right;

				if (target->bottom > mPackedHeight)
					mPackedHeight = target->bottom;
			}

			FreeSize(sortableSize);
		}

		return GetRectangleCount();
	}
} // SE