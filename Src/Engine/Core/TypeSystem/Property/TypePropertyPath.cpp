#include "TypePropertyPath.h"
#include "../../Types/Strings/String.h"

//-------------------------------------------------------------------------

namespace SE
{
    TypePropertyPath::TypePropertyPath(String &pathString)
    {
        List<String> pathStrings;
		pathString.Split(SE_TEXT('/'), pathStrings);

        ENGINE_ASSERT(!pathStrings.IsEmpty());

        auto const numPathEntries = pathStrings.Count();
        for (auto i = 0u; i < numPathEntries; i++)
        {
            if (isdigit(pathStrings[i][0]))
            {
                ENGINE_ASSERT(!m_pathElements.IsEmpty());
				StringUtils::Parse(pathStrings[i].Get(), &m_pathElements.Last().arrayElementIdx);// atoi(pathStrings[i].Get());
            }
            else
            {
                m_pathElements.Add(PathElement(StringID(pathStrings[i])));
            }
        }
    }

	String TypePropertyPath::ToString() const
    {
		String pathString;

        int32_t const numElements = (int32_t)m_pathElements.Count();
        if (numElements > 0)
        {
            for (auto i = 0; i < numElements - 1; i++)
            {
                auto const &pathElement = m_pathElements[i];

                pathString += pathElement.propertyID.ToString();

                if (pathElement.IsArrayElement())
                {
                    pathString += String::Format(SE_TEXT("/{0}/"), pathElement.arrayElementIdx);
                }
                else
                {
                    pathString += '/';
                }
            }

            // We dont want a trailing slash for the last element
            //-------------------------------------------------------------------------

            auto const &pathElement = m_pathElements.Last();
            pathString += pathElement.propertyID;
            if (pathElement.IsArrayElement())
            {
                pathString += String::Format(SE_TEXT("/{0}"), pathElement.arrayElementIdx);
            }
        }

        return pathString;
    }
}