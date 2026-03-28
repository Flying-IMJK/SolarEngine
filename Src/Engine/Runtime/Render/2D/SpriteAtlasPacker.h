#pragma once
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Collections/List.h"
#include "Runtime/API.h"

namespace SE
{
	/**
     * Class used to pack rectangles within container rectangle with close to optimal solution.
     */
	class SE_API_RUNTIME SpriteAtlasPacker
	{
	public:

		struct Rect
		{
			int x;
			int y;
			int width;
			int height;
			int right;
			int bottom;
			int id;

			Rect(): x(0), y(0), width(0), height(0), right(0), bottom(0), id(0)
			{
			}

			Rect(int x, int y, int width, int height): id(0)
			{
				this->x = x;
				this->y = y;
				this->width = width;
				this->height = height;
				this->right = x + width;
				this->bottom = y + height;
			}
		};

	private:
		struct SortableSize
		{
			int width;
			int height;
			int id;

			SortableSize() = default;

			SortableSize(int width, int height, int id)
			{
				this->width = width;
				this->height = height;
				this->id = id;
			}
		};

		int mWidth = 0;
		int mHeight = 0;
		int mPadding = 8;

		int mPackedWidth = 0;
		int mPackedHeight = 0;

		List<SortableSize> m_InsertList;

		Dictionary<int, int> m_InsertedRectanglesMap;
		List<Rect*> m_InsertedRectangles;
		List<Rect*> m_FreeAreas;
		List<Rect*> m_NewFreeAreas;

		Rect* m_OutsideRectangle;

		List<SortableSize> m_SortableSizeStack;
		List<Rect*> m_RectangleStack;

		void FilterSelfSubAreas(List<Rect*>& areas);

		void GenerateNewFreeAreas(Rect *target, List<Rect*>& areas, List<Rect*>& results);

		void GenerateDividedAreas(Rect *divider, Rect* area, List<Rect*>& results);

		int GetFreeAreaIndex(int width, int height);

		Rect* AllocateRectangle(int x, int y, int width, int height);

		void FreeRectangle(Rect* rectangle);

		SortableSize AllocateSize(int width, int height, int id);

		void FreeSize(SortableSize size);

	public:
		int GetRectangleCount() const { return m_InsertedRectangles.Count();  }

		int GetPackedWidth() const { return mPackedWidth; }
		int GetPackedHeight() const { return mPackedHeight; }

		int GetPadding() const { return mPadding; }

		SpriteAtlasPacker(int width, int height, int padding = 0);

		void Reset(int width, int height, int padding = 0);

		Rect GetRectangle(int index);

		int GetRectangleId(int index);

		void InsertRectangle(int width, int height, int id);

		int PackRectangles(bool sort = true);
	};
} // SE

