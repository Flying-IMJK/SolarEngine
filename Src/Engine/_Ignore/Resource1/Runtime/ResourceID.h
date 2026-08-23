#pragma once

#include "Runtime/API.h"
#include "Core/Types/SGUID.h"
#include "Core/TypeSystem/IReflectedType.h"

namespace SGE
{
	class SE_API_RUNTIME ResPath : IReflectedType
	{
		SE_CLASS(ResPath, IReflectedType)
	public:
		constexpr static Char const* s_pathPrefix = SE_TEXT("data://");
		constexpr static int32 const s_pathPrefixLength = 7;
		constexpr static Char const s_pathDelimiter = '/';
		constexpr static Char const s_subResourceFilePathDelimiter = '_';

		static bool IsValidPath(Char const* pPath);
		static bool IsValidPath(String const& path)
		{
			return IsValidPath(path.Get());
		}

		static ResPath FromFileSystemPath(String const& rawResourceDirectoryPath, String const& filePath);
		static String ToFileSystemPath(String const& rawResourceDirectoryPath, ResPath const& resourcePath);

	public:
		ResPath() = default;
		ResPath(ResPath&& path);
		ResPath(ResPath const& path);
		explicit ResPath(String const& path);
		explicit ResPath(char const* pPath);
		explicit ResPath(String&& path);

		//-------------------------------------------------------------------------

		inline bool IsValid() const
		{
			return !m_path.IsEmpty() && IsValidPath(m_path);
		}
		inline void Clear()
		{
			m_path.Clear();
			m_ID = 0;
		}
		inline uint32 GetID() const
		{
			return m_ID;
		}

		// Path info
		//-------------------------------------------------------------------------

		// Returns the filename
		String GetFileName() const;

		// Returns the filename with all the 'extensions' removed (i.e. file.final.png -> file )
		String GetFileNameWithoutExtension() const;

		// Get the containing directory path for this path
		String GetParentDirectory() const;

		// Get the directory depth for this path e.g. D:\Foo\Bar\Moo.txt = 2
		int32 GetDirectoryDepth() const;

		// Get the full path depth for this path e.g. D:\Foo\Bar\Moo.txt = 3
		int32 GetPathDepth() const;

		// Is this a file path
		inline bool IsFile() const
		{
			ENGINE_ASSERT(IsValid());
			return !m_path.EndsWith(s_pathDelimiter);
		}

		// Is this a directory path
		inline bool IsDirectory() const
		{
			ENGINE_ASSERT(IsValid());
			return m_path.EndsWith(s_pathDelimiter);
		}

		// Sub-Resources
		//-------------------------------------------------------------------------

		// Is this potentially a sub-resource path (i.e. a path within a resource)
		bool IsSubResourcePath() const;

		// Get the path to the parent resource (this will return an invalid path for non sub-resource paths)
		ResPath GetParentResourcePath() const;

		// Extension
		//-------------------------------------------------------------------------

		// Returns the extension for this path (excluding the '.'). Returns an empty string if there is no extension!
		String GetExtension() const;

		// Returns a lowercase version of the extension (excluding the '.') if one exists else returns an empty string
		inline String GetLowercaseExtensionAsString() const
		{
			String ext = GetExtension();
			if (ext != String::Empty)
			{
				ext = ext.ToLower();
			}
			return ext;
		}

		// Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
		void ReplaceExtension(const StringView pExtension);

		// Replaces the extension (excluding the '.') for this path (will create an extensions if no extension exists)
		inline void ReplaceExtension(const Char* extension)
		{
			ReplaceExtension(StringView(extension));
		}

		// Conversion
		//-------------------------------------------------------------------------

		inline String const& GetString() const
		{
			return m_path;
		}
		inline Char const* c_str() const
		{
			return m_path.Get();
		}
		inline String ToFileSystemPath(String const& rawResourceDirectoryPath) const
		{
			return ToFileSystemPath(rawResourceDirectoryPath, *this);
		}

		// Operators
		//-------------------------------------------------------------------------

		inline bool operator==(ResPath const& rhs) const
		{
			return m_ID == rhs.m_ID;
		}
		inline bool operator!=(ResPath const& rhs) const
		{
			return m_ID != rhs.m_ID;
		}

		ResPath& operator=(ResPath&& path);
		ResPath& operator=(ResPath const& path);

	private:
		void OnPathMemberChanged();

	private:
		String m_path;
		uint32 m_ID = 0;
	};

	//-------------------------------------------------------------------------
	// 资源的唯一 ID - 用于资源查找和依赖关系
	//-------------------------------------------------------------------------
	// 包含资源UUID和资源类型
	//
	//-------------------------------------------------------------------------
	class SE_API_RUNTIME ResID
    {
    public:
        ResID() = default;
        inline ResID(SGUID id, TypeID type) : m_GUID(id), m_type(type) {  }

        inline bool IsValid() const { return m_GUID.IsValid() && m_type.IsValid(); }
        inline SGUID GetID() const { return m_GUID; }
		inline TypeID GetTypeID() const { return m_type; }

		void Clear();

        // Sub-resources
        //-------------------------------------------------------------------------

//        inline bool IsSubResourceID() const { return m_path.IsSubResourcePath(); }
//		ResID GetParentResourceID() const;
//		ResPath GetParentResourcePath() const;
//        inline ResTypeID GetParentResourceTypeID() const { return GetParentResourceID().GetResourceTypeID(); }

        //-------------------------------------------------------------------------

        inline bool operator==(ResID const &rhs) const { return m_GUID == rhs.m_GUID; }
        inline bool operator!=(ResID const &rhs) const { return m_GUID != rhs.m_GUID; }
	private:
        SGUID m_GUID;
		TypeID m_type;
    };


	inline uint32 GetHash(const ResID &resourceId)
	{
		return GetHash(resourceId.GetID());
	}

	inline int32 GetHash(ResPath const &id)
	{
		return id.GetID();
	}
}