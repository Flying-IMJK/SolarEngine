#include "ResourceID.h"
#include "Core/Platform/FileSystem.h"
#include "Core/TypeSystem/Types.h"

#include "Runtime/System/ResourceSystem.h"
//-------------------------------------------------------------------------

namespace SGE
{
	struct SubResourcePathUtils
	{
		inline bool IsValid() const { return !m_parentExtension.IsEmpty() && m_parentExtension.Length() <= 4; }

		bool DecomposePathString( String const& pathStr )
		{
			// Check for path delimiter
			m_lastPathDelimiterIndex = pathStr.Find( ResPath::s_pathDelimiter );
			if ( m_lastPathDelimiterIndex == INVALID_INDEX )
			{
				return false;
			}

			// Check for directory path
			if ( m_lastPathDelimiterIndex == pathStr.Length() - 1 )
			{
				return false;
			}

			// If we found a parent, create the substring for it and get the extension
			m_parentPath = String(pathStr.Get(), m_lastPathDelimiterIndex);
			m_childResourceFilename = &pathStr[m_lastPathDelimiterIndex + 1];
			String extension = FileSystem::GetExtension(m_parentPath.Get());
			//FileSystem::FindExtensionStartIdx( m_parentPath.Get(), ResPath::s_pathDelimiter );

			// If no extension found, parent is likely a directory
			if ( extension == String::Empty )
			{
				return false;
			}

			// Ensure extension is less than 4 characters since resource types use a 4CC type ID
			m_parentExtension = extension;
			return IsValid();
		}

		void GenerateFilePath( String& outPath )
		{
			ENGINE_ASSERT( IsValid() );

			size_t const parentPathPortionLength = m_parentPath.Length() - m_parentExtension.Length() - 1; // parent length without extension and delimiter
			outPath.Resize( parentPathPortionLength + m_childResourceFilename.Length() + 1 ); // Add back delimiter and child resource name

			// Copy parent path without extension and null terminator
			memcpy( outPath.Get(), m_parentPath.Get(), ( parentPathPortionLength ) * sizeof( m_parentPath[0] ) );

			// Set delimiter
			outPath[parentPathPortionLength] = ResPath::s_subResourceFilePathDelimiter;

			// Copy child resource name with null terminator
			memcpy( outPath.Get() + parentPathPortionLength + 1, m_childResourceFilename.Get(), m_childResourceFilename.Length() * sizeof( m_parentPath[0] ) );
		}

	public:

		String    m_parentPath;
		String    m_parentExtension;
		String    m_childResourceFilename;
		uint64    m_lastPathDelimiterIndex = INVALID_INDEX;
	};

	// Extremely naive data path validation function, this is definitely not robust!
	bool ResPath::IsValidPath(const Char *pPath)
	{
		ENGINE_ASSERT(pPath != nullptr);

		if (StringUtils::Length(pPath) == 0)
		{
			return false;
		}

		if (StringUtils::Compare(ResPath::s_pathPrefix, pPath, s_pathPrefixLength) != 0)
		{
			return false;
		}

		Char t = SE_TEXT('\\');
		if (StringUtils::Find(pPath, &t) != nullptr)
		{
			return false;
		}

		return true;
	}

	ResPath ResPath::FromFileSystemPath(String const &rawResourceDirectoryPath, String const &filePath)
	{
		ENGINE_ASSERT(!rawResourceDirectoryPath.IsEmpty() && FileSystem::IsDirectory(rawResourceDirectoryPath) && !filePath.IsEmpty());

		ResPath path;

		if (FileSystem::IsUnderDirectory(filePath, rawResourceDirectoryPath))
		{
			String tempPath = ResPath::s_pathPrefix;
			tempPath.Append(filePath.Substring(rawResourceDirectoryPath.Length()));
			FileSystem::NormalizePath(tempPath);

			path = ResPath(tempPath);
		}

		return path;
	}

	String ResPath::ToFileSystemPath(String const &rawResourceDirectoryPath, ResPath const &resourcePath)
	{
		ENGINE_ASSERT(!rawResourceDirectoryPath.IsEmpty() && FileSystem::IsDirectory(rawResourceDirectoryPath) && resourcePath.IsValid());

		// Replace slashes and remove prefix
		String tempPath(rawResourceDirectoryPath);
		tempPath += resourcePath.m_path.Substring(7);

		// Check if it's a sub-resource path
		if (resourcePath.IsFile() )
		{
			SubResourcePathUtils spu;
			if (spu.DecomposePathString( tempPath ) )
			{
				spu.GenerateFilePath( tempPath );
			}
		}

		// Finalize path
		FileSystem::NormalizePath(tempPath);
//        FileSystem::GetCorrectCaseForPath(tempPath.Get(), tempPath);

		return tempPath;
	}

	//-------------------------------------------------------------------------

	ResPath::ResPath(String const &path)
		: m_path(path)
	{
		ENGINE_ASSERT(m_path.IsEmpty() || IsValidPath(m_path));
		OnPathMemberChanged();
	}

	ResPath::ResPath(char const *pPath)
		: m_path(pPath)
	{
		ENGINE_ASSERT(m_path.IsEmpty() || IsValidPath(m_path));
		OnPathMemberChanged();
	}

	ResPath::ResPath(String &&path)
	{
		m_path = path;
		ENGINE_ASSERT(m_path.IsEmpty() || IsValidPath(m_path));
		OnPathMemberChanged();
	}

	ResPath::ResPath(ResPath const &path)
		: m_path(path.m_path), m_ID(path.m_ID)
	{
	}

	ResPath::ResPath(ResPath &&path)
		: m_ID(path.m_ID)
	{
		m_path = path.m_path;
	}

	ResPath &ResPath::operator=(ResPath const &path)
	{
		m_path = path.m_path;
		m_ID = path.m_ID;
		return *this;
	}

	ResPath &ResPath::operator=(ResPath &&path)
	{
		m_path = path.m_path;
		m_ID = path.m_ID;
		return *this;
	}

	//-------------------------------------------------------------------------

	void ResPath::OnPathMemberChanged()
	{
		if (IsValidPath(m_path))
		{
			m_path = m_path.ToLower();
			m_ID = GetHash(m_path.Get());
		}
		else
		{
			m_path.Clear();
			m_ID = 0;
		}
	}

	String ResPath::GetFileName() const
	{
		ENGINE_ASSERT(IsValid() && IsFile());

		auto filenameStartIdx = m_path.FindLast(s_pathDelimiter);
		ENGINE_ASSERT(filenameStartIdx != INVALID_INDEX);
		filenameStartIdx++;

		return m_path.Substring(filenameStartIdx, m_path.Length() - filenameStartIdx);
	}

	String ResPath::GetFileNameWithoutExtension() const
	{
		ENGINE_ASSERT(IsValid() && IsFile());

		auto filenameStartIdx = m_path.FindLast(s_pathDelimiter);
		ENGINE_ASSERT(filenameStartIdx != INVALID_INDEX);
		filenameStartIdx++;

		//-------------------------------------------------------------------------

		int32 extStartIdx = FileSystem::FindExtensionStartIdx(m_path);
		if (extStartIdx != INVALID_INDEX)
		{
			return m_path.Substring(filenameStartIdx, extStartIdx - filenameStartIdx - 1);
		}
		else
		{
			return String(&m_path[filenameStartIdx]);
		}
	}

	String ResPath::GetParentDirectory() const
	{
		ENGINE_ASSERT(IsValid());

		size_t lastDelimiterIdx = m_path.Find(s_pathDelimiter);

		// Handle directory paths
		if (lastDelimiterIdx == m_path.Length() - 1)
		{
			lastDelimiterIdx = m_path.Find(&s_pathDelimiter, lastDelimiterIdx - 1);
		}

		//-------------------------------------------------------------------------

		String parentPath;

		// If we found a parent, create the substring for it
		if (lastDelimiterIdx != INVALID_INDEX)
		{
			parentPath = m_path.Substring(0, lastDelimiterIdx + 1);
		}

		return parentPath;
	}

	int32 ResPath::GetDirectoryDepth() const
	{
		int32_t dirDepth = -1;

		if (IsValid())
		{
			size_t delimiterIdx = m_path.Find(s_pathDelimiter);
			while (delimiterIdx != INVALID_INDEX)
			{
				dirDepth++;
				delimiterIdx = m_path.Find(&s_pathDelimiter, delimiterIdx + 1);
			}
		}

		return dirDepth;
	}

	int32 ResPath::GetPathDepth() const
	{
		int32 pathDepth = GetDirectoryDepth();

		if (IsFile())
		{
			pathDepth++;
		}

		return pathDepth;
	}

	bool ResPath::IsSubResourcePath() const
	{
		ENGINE_ASSERT( IsValid() );
		SubResourcePathUtils spu;
		return spu.DecomposePathString( m_path );
	}

	ResPath ResPath::GetParentResourcePath() const
	{
		ENGINE_ASSERT( IsValid() );

		ResPath parentResourcePath;
		SubResourcePathUtils spu;
		if ( spu.DecomposePathString( m_path ) )
		{
			parentResourcePath = ResPath( spu.m_parentPath.Get() );
		}

		return parentResourcePath;
	}

	String ResPath::GetExtension() const
	{
		ENGINE_ASSERT(IsValid() && IsFile());
		return FileSystem::GetExtension(m_path);
	}

	void ResPath::ReplaceExtension(StringView pExtension)
	{
		ENGINE_ASSERT(IsValid() && IsFile() && pExtension != nullptr);
		ENGINE_ASSERT(pExtension[0] != 0 && pExtension[0] != '.');

		int const extIdx = FileSystem::FindExtensionStartIdx(m_path);
		if (extIdx != INVALID_INDEX)
		{
			m_path = m_path.Substring(0, extIdx) + pExtension;
		}
		else // No extension, so just append
		{
			m_path.Append(SE_TEXT("."));
			m_path.Append(pExtension);
		}

		OnPathMemberChanged();
	}

	//-------------------------------------------------------------------------

	void ResID::Clear()
	{
		m_GUID = SGUID::Empty;
		m_type = TypeID();
	}
}