
#include "Version.h"
#include "Core/Math/Math.h"
#include "Core/Types/Strings/String.h"

namespace SE
{
	VersionInfo::VersionInfo(int32 major, int32 minor, int32 build, int32 revision)
	{
		m_Major = Math::Max(major, 0);
		m_Minor = Math::Max(minor, 0);
		m_Build = Math::Max(build, -1);
		m_Revision = Math::Max(revision, -1);
	}

	VersionInfo::VersionInfo(int32 major, int32 minor, int32 build)
	{
		m_Major = Math::Max(major, 0);
		m_Minor = Math::Max(minor, 0);
		m_Build = Math::Max(build, -1);
		m_Revision = -1;
	}

	VersionInfo::VersionInfo(int32 major, int32 minor)
	{
		m_Major = Math::Max(major, 0);
		m_Minor = Math::Max(minor, 0);
		m_Build = -1;
		m_Revision = -1;
	}

	int32 VersionInfo::CompareTo(const VersionInfo& value) const
	{
		if (m_Major != value.m_Major)
		{
			if (m_Major > value.m_Major)
			{
				return 1;
			}
			return -1;
		}
		if (m_Minor != value.m_Minor)
		{
			if (m_Minor > value.m_Minor)
			{
				return 1;
			}
			return -1;
		}
		if (m_Build != value.m_Build)
		{
			if (m_Build > value.m_Build)
			{
				return 1;
			}
			return -1;
		}
		if (m_Revision == value.m_Revision)
		{
			return 0;
		}
		if (m_Revision > value.m_Revision)
		{
			return 1;
		}
		return -1;
	}

	String VersionInfo::ToString(int32 fieldCount) const
	{
		StringBuilder stringBuilder(32);
		switch (fieldCount)
		{
		case 0:
			break;
		case 1:
			stringBuilder.Append(Math::Max(m_Major, 0));
			break;
		case 2:
			stringBuilder.Append(Math::Max(m_Major, 0));
			stringBuilder.Append(SE_TEXT('.'));
			stringBuilder.Append(Math::Max(m_Minor, 0));
			break;
		case 3:
			stringBuilder.Append(Math::Max(m_Major, 0));
			stringBuilder.Append(SE_TEXT('.'));
			stringBuilder.Append(Math::Max(m_Minor, 0));
			stringBuilder.Append(SE_TEXT('.'));
			stringBuilder.Append(Math::Max(m_Build, 0));
			break;
		default:
			stringBuilder.Append(Math::Max(m_Major, 0));
			stringBuilder.Append(SE_TEXT('.'));
			stringBuilder.Append(Math::Max(m_Minor, 0));
			stringBuilder.Append(SE_TEXT('.'));
			stringBuilder.Append(Math::Max(m_Build, 0));
			stringBuilder.Append(SE_TEXT('.'));
			stringBuilder.Append(Math::Max(m_Revision, 0));
			break;
		}
		return stringBuilder.ToString();
	}

	String VersionInfo::ToString() const
	{
		if (m_Build == -1)
		{
			return ToString(2);
		}
		if (m_Revision == -1)
		{
			return ToString(3);
		}
		return ToString(4);
	}

	bool VersionInfo::Parse(const String& text, VersionInfo* value)
	{
		int32 major, minor, build, revision;

		List<String> parsedComponents(4);
		Char s = SE_TEXT('.');
		text.Split(s, parsedComponents);

		int32 parsedComponentsLength = parsedComponents.Count();
		if (parsedComponentsLength < 2 || parsedComponentsLength > 4)
		{
			return true;
		}

		if (StringUtils::Parse(*parsedComponents[0], &major))
		{
			return true;
		}

		if (StringUtils::Parse(*parsedComponents[1], &minor))
		{
			return true;
		}

		parsedComponentsLength -= 2;

		if (parsedComponentsLength > 0)
		{
			if (StringUtils::Parse(*parsedComponents[2], &build))
			{
				return true;
			}

			parsedComponentsLength--;

			if (parsedComponentsLength > 0)
			{
				if (StringUtils::Parse(*parsedComponents[3], &revision))
				{
					return true;
				}

				*value = VersionInfo(major, minor, build, revision);
			}
			else
			{
				*value = VersionInfo(major, minor, build);
			}
		}
		else
		{
			*value = VersionInfo(major, minor);
		}

		return false;
	}

} // SE