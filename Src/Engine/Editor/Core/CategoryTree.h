#pragma once

namespace SE::Editor
{
    template <typename T>
    struct CategoryItem
    {
		CategoryItem()
			: m_name(), m_data()
		{
		}

        CategoryItem(String const &name, T const &data)
            : m_name(name), m_data(data)
        {
        }

        String m_name;
        T m_data;
    };

    //-------------------------------------------------------------------------
    template <typename T>
    struct Category
    {
		Category()
			: m_name(), m_depth(-1)
		{
		}

        Category(String const &name, int32 depth = -1)
            : m_name(name), m_depth(depth)
        {
        }

        Category(String &&name, int32 depth = -1)
            : m_name(name), m_depth(depth)
        {
        }

        void Clear()
        {
            m_childCategories.Clear();
            m_items.Clear();
        }

        bool IsEmpty() const
        {
            return m_childCategories.IsEmpty() && m_items.IsEmpty();
        }

        bool HasItemsThatMatchFilter(Function<bool(CategoryItem<T> const &)> const &filter) const
        {
            for (auto const &childCategory : m_childCategories)
            {
                if (childCategory.HasItemsThatMatchFilter(filter))
                {
                    return true;
                }
            }

            //-------------------------------------------------------------------------

            for (auto const &item : m_items)
            {
                if (filter(item))
                {
                    return true;
                }
            }

            //-------------------------------------------------------------------------

            return false;
        }

        void AddItem(CategoryItem<T> const &itemToAdd, bool sortedInsert = true)
        {
            if (sortedInsert)
            {
				Function<bool(CategoryItem<T> const &)> predicate = [itemToAdd](CategoryItem<T> const &existingItem)
				{
				  return existingItem.m_name < itemToAdd.m_name;
				};

                int insertionPosition = ListExtensions::IndexOf(m_items, predicate);
				if (insertionPosition == INVALID_INDEX)
				{
					m_items.Add(itemToAdd);
				}
				else
				{
					m_items.Insert(insertionPosition, itemToAdd);
				}
            }
            else
            {
                m_items.Add(itemToAdd);
            }
        }

        void RemoveAllItems()
        {
            for (auto &childCategory : m_childCategories)
            {
                childCategory.RemoveAllItems();
            }

            m_items.Clear();
        }

        void RemoveAllEmptyChildCategories()
        {
            for (auto i = m_childCategories.Count() - 1; i >= 0; i--)
            {
                m_childCategories[i].RemoveAllEmptyChildCategories();

                if (m_childCategories[i].IsEmpty())
                {
                    m_childCategories.RemoveAt(i);
                }
            }
        }

    public:
        String m_name;
        List<Category<T>> m_childCategories;
        List<CategoryItem<T>> m_items;
        int32 m_depth = -1;
        bool m_isCollapsed = false;
    };

    // Category Tree
    //-------------------------------------------------------------------------
    // Allow you to build a simple category tree based on a string path (X/Y/Z) etc...

    template <typename T>
    class CategoryTree
    {
    public:
        CategoryTree()
            : m_rootCategory("")
        {
        }

        // Get the root category
        Category<T> const &GetRootCategory() const { return m_rootCategory; }

        // Get the root category
        Category<T> &GetRootCategory() { return m_rootCategory; }

        // Clear the tree
        void Clear() { m_rootCategory.Clear(); }

        // Add a new item
        void AddItem(String const &path, String const &itemName, T const &item)
        {
            if (path.IsEmpty())
            {
                m_rootCategory.m_items.Add(CategoryItem<T>(itemName, item));
            }
            else
            {
                List<String> splitPath = SplitPathString(path);
                Category<T> *pFoundCategory = FindOrCreateCategory(m_rootCategory, splitPath, 0);
                ENGINE_ASSERT(pFoundCategory != nullptr);
                pFoundCategory->AddItem(CategoryItem<T>(itemName, item));
            }
        }

        // Find a category given a specific path
        Category<T> *FindCategory(String const &path)
        {
            Category<T> *pFoundCategory = nullptr;

            if (path.IsEmpty())
            {
                pFoundCategory = &m_rootCategory;
                return pFoundCategory;
            }

            //-------------------------------------------------------------------------

            List<String> splitPath = SplitPathString(path);
            Category<T> *pCurrentCategory = &m_rootCategory;
            auto pathIter = splitPath.begin();

            while (pFoundCategory == nullptr && pathIter != splitPath.end())
            {
                bool foundValidChild = false;
                for (auto pChildCategory : pCurrentCategory->m_childCategories)
                {
                    if (pChildCategory->name == *pathIter)
                    {
                        pCurrentCategory = pChildCategory;
                        foundValidChild = true;
                        break;
                    }
                }

                //-------------------------------------------------------------------------

                if (foundValidChild)
                {
                    pathIter++;
                    if (pathIter == splitPath.end())
                    {
                        pFoundCategory = pCurrentCategory;
                    }
                }
                else
                {
                    break;
                }
            }

            return pFoundCategory;
        }

        // Remove all items but keep categories
        void RemoveAllItems()
        {
            m_rootCategory.RemoveAllItems();
        }

        // Remove all empty categories
        void RemoveEmptyCategories()
        {
            m_rootCategory.RemoveAllEmptyChildCategories();
        }

    private:
        // Split a category path into individual elements
        List<String> SplitPathString(String const &pathString)
        {
            ENGINE_ASSERT(!pathString.IsEmpty());
            List<String> splitPath;
			pathString.Split('/', splitPath);
            return splitPath;
        }

        Category<T> *FindOrCreateCategory(Category<T> &currentCategory, List<String> const &path, int32 currentPathIdx)
        {
            ENGINE_ASSERT(!path.IsEmpty());

            // If we are at the end of the path, this is the category we are looking for
            //-------------------------------------------------------------------------

            if (path.Count() == currentPathIdx)
            {
                return &currentCategory;
            }

            // Search
            //-------------------------------------------------------------------------

            for (Category<T> &childCategory : currentCategory.m_childCategories)
            {
                if (childCategory.m_name == path[currentPathIdx])
                {
                    return FindOrCreateCategory(childCategory, path, currentPathIdx + 1);
                }
            }

            // Create a new category and continue the search
			const String& currentPath = path.At(currentPathIdx);
			Function<bool(Category<T> const &, const String&)> predicate = [](Category<T> const &category, const String& path)
			{
			  return category.m_name < path;
			};

			int insertionPosition = ListExtensions::IndexOf(currentCategory.m_childCategories, predicate, currentPath);
			if (insertionPosition < 0)
			{
				insertionPosition = currentCategory.m_childCategories.Count();
				currentCategory.m_childCategories.Add(Category<T>(path[currentPathIdx], currentPathIdx));
			}

			return FindOrCreateCategory(currentCategory.m_childCategories[insertionPosition], path, currentPathIdx + 1);

//            auto insertionPosition = std::lower_bound(currentCategory.m_childCategories.begin(), currentCategory.m_childCategories.end(), path[currentPathIdx], Comparision)
//			currentCategory.m_childCategories.Insert(insertionPosition, Category<T>(path[currentPathIdx], currentPathIdx));
//            return FindOrCreateCategory(currentCategory.m_childCategories[insertionPosition], path, currentPathIdx + 1);
        }

    public:
        Category<T> m_rootCategory;
    };
}