

#include "QueryFilterHelper.h"

namespace SE::Editor
{
	bool QueryFilterHelper::Match(String& filter, String& text)
	{
		// Empty inputs
		if (filter.IsEmpty() || text.IsEmpty())
			return false;

		// Full match
		if (filter == text)
		{
			return true;
		}

		bool hasMatch = false;

		// Find matching sequences
		// We do simple iteration over the characters
		int textLength = text.Length();
		int filterLength = filter.Length();
		int searchEnd = textLength - filterLength;
		for (int textPos = 0; textPos <= searchEnd; textPos++)
		{
			// Skip if the current text position doesn't match the filter start
			if (StringUtils::ToLower(filter[0]) != StringUtils::ToLower(text[textPos]))
				continue;

			int matchStartPos = -1;
			int endPos = textPos + filterLength;
			int filterPos = 0;

			for (int i = textPos; i < endPos; i++, filterPos++)
			{
				Char filterChar = StringUtils::ToLower(filter[filterPos]);
				Char textChar = StringUtils::ToLower(text[i]);

				if (filterChar == textChar)
				{
					// Check if start the matching sequence
					if (matchStartPos == -1)
					{
						matchStartPos = textPos;
					}
				}
				else
				{
					// Check if stop matching sequence
					if (matchStartPos != -1)
					{
						int length = textPos - matchStartPos;
						if (length >= MinLength)
							hasMatch = true;
						textPos = matchStartPos + length;
						matchStartPos = -1;
					}
					break;
				}
			}

			// Check sequence on the end
			if (matchStartPos != -1 && filterPos == filterLength)
			{
				int length = endPos - matchStartPos;
				if (length >= MinLength)
					hasMatch = true;
				textPos = matchStartPos + length;
			}
		}

		return hasMatch;
	}
	
    bool QueryFilterHelper::Match(String& filter, String& text, List<Range>& matches)
	{
	    // Empty inputs
	    matches.Clear();
		if (filter.IsEmpty() || text.IsEmpty())
			return false;

	    // Full match
	    if (filter == text)
	    {
	        matches.Add(Range(0, filter.Length()));
	        return true;
	    }

	    List<Range> ranges;

	    // Find matching sequences by doing simple iteration over the characters
	    int textLength = text.Length();
	    int filterLength = filter.Length();
	    int searchEnd = textLength - filterLength;
	    for (int textPos = 0; textPos <= searchEnd; textPos++)
	    {
	        // Skip if the current text position doesn't match the filter start
	        if (StringUtils::ToLower(filter[0]) != StringUtils::ToLower(text[textPos]))
	            continue;

	        int matchStartPos = -1;
	        int endPos = textPos + filterLength;
	        int filterPos = 0;

	        for (int i = textPos; i < endPos; i++, filterPos++)
	        {
	            Char filterChar = StringUtils::ToLower(filter[filterPos]);
	            Char textChar = StringUtils::ToLower(text[i]);

	            if (filterChar == textChar)
	            {
	                // Check if start the matching sequence
	                if (matchStartPos == -1)
	                {
	                    matchStartPos = textPos;
	                }
	            }
	            else
	            {
	                // Check if stop matching sequence
	                if (matchStartPos != -1)
	                {
	                    int length = textPos - matchStartPos;
	                    if (length >= MinLength)
	                    {
	                    	ranges.Add(Range(matchStartPos, length));
	                    }
	                    textPos = matchStartPos + length;
	                    matchStartPos = -1;
	                }
	                break;
	            }
	        }

	        // Check sequence on the end
	        if (matchStartPos != -1 && filterPos == filterLength)
	        {
	            int length = endPos - matchStartPos;
	            if (length >= MinLength)
	            {
	            	ranges.Add(Range(matchStartPos, length));
	            }
	            textPos = matchStartPos + length;
	        }
	    }

	    // Check if has any range
	    if (ranges.Count() > 0 )
	    {
	        matches = MoveTemp(ranges);
	        return true;
	    }

	    return false;
	}

    String ToString(QueryFilterHelper::Range& range)
	{
	    return String::Format(SE_TEXT("StartIndex: {0}, Length: {0}"), range.StartIndex, range.Length);;
	}
} // SE