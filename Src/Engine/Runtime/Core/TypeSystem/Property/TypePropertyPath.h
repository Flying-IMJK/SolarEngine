#pragma once

#include "Runtime/Core/Types/Strings/StringID.h"

//-------------------------------------------------------------------------
// The path to a property within a reflected type
//-------------------------------------------------------------------------
// Examples:
//  "Foo/Bar" -> set the bar value of nested type foo
//  "Foo/BarArray/0" -> set the first value of BarArray of nested type foo

namespace SE
{
    class SE_API_RUNTIME TypePropertyPath
    {
    public:
        struct PathElement
        {
            PathElement() : arrayElementIdx(-1) {}
            PathElement(StringID id) : propertyID(id), arrayElementIdx(-1) {}
            PathElement(StringID id, int32 arrayElementIdx) : propertyID(id), arrayElementIdx(arrayElementIdx) {}

            inline bool IsArrayElement() const { return arrayElementIdx != -1; }

            inline bool operator==(PathElement const &other) const
            {
                return propertyID == other.propertyID && arrayElementIdx == other.arrayElementIdx;
            }

            inline bool operator!=(PathElement const &other) const
            {
                return !(*this == other);
            }

        public:
            StringID propertyID;
            int32 arrayElementIdx;
        };

    public:
        TypePropertyPath() {}
        TypePropertyPath(String &pathString);

        inline bool IsValid() const
        {
            return !m_pathElements.IsEmpty();
        }

        inline int64 GetNumElements() const
        {
            return m_pathElements.Count();
        }

        inline PathElement const &FirstElement() const
        {
            ENGINE_ASSERT(IsValid());
            return m_pathElements.First();
        }

        inline PathElement const &GetLastElement() const
        {
            ENGINE_ASSERT(IsValid());
            return m_pathElements.Peek();
        }

        inline bool IsPathToArrayElement() const
        {
            ENGINE_ASSERT(IsValid());
            return GetLastElement().IsArrayElement();
        }

        inline void Append(StringID newElement, int32 arrayElementIdx = -1)
        {
            ENGINE_ASSERT(newElement != StringID::Invalid && arrayElementIdx >= -1);
            m_pathElements.Add(PathElement(newElement, arrayElementIdx));
        }

        inline void RemoveLastElement()
        {
            ENGINE_ASSERT(IsValid());
            m_pathElements.Pop();
        }

        inline void ReplaceLastElement(StringID newElement, int32 arrayElementIdx = -1)
        {
            ENGINE_ASSERT(IsValid());
            ENGINE_ASSERT(newElement != StringID::Invalid && arrayElementIdx >= -1);
            m_pathElements.Last() = PathElement(newElement, arrayElementIdx);
        }

        inline TypePropertyPath GetPathWithoutFirstElement() const
        {
            ENGINE_ASSERT(IsValid());
            ENGINE_ASSERT(m_pathElements.Count() > 1);
            TypePropertyPath subPath;
            subPath.m_pathElements = List<PathElement>(m_pathElements);
			subPath.m_pathElements.RemoveAt(0);
            return subPath;
        }

        inline PathElement const &operator[](int64 idx) const
        {
            ENGINE_ASSERT(idx < m_pathElements.Count());
            return m_pathElements[idx];
        }

        inline bool operator==(TypePropertyPath const &other) const
        {
            int64 const numElements = m_pathElements.Count();
            if (numElements != other.GetNumElements())
            {
                return false;
            }

            for (auto i = 0u; i < numElements; i++)
            {
                if (m_pathElements[i] != other.m_pathElements[i])
                {
                    return false;
                }
            }

            return true;
        }

        inline bool operator!=(TypePropertyPath const &other) const
        {
            return !(*this == other);
        }

        inline TypePropertyPath &operator+=(StringID newElement)
        {
            m_pathElements.Add(PathElement(newElement));
            return *this;
        }

		String ToString() const;

    private:
        List<PathElement> m_pathElements;
    };
}