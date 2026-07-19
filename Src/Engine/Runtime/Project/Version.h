#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Templates.h"

#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// Represents the version number made of major, minor, build and revision numbers.
	/// </summary>
	struct SE_API_RUNTIME VersionInfo
	{
	private:
		int32 m_Major;
		int32 m_Minor;
		int32 m_Build;
		int32 m_Revision;

	public:
		/// <summary>
		/// Initializes a new instance of the Version class with the specified major, minor, build, and revision numbers.
		/// </summary>
		/// <param name="major">The major version number.</param>
		/// <param name="minor">The minor version number.</param>
		/// <param name="build">The build number.</param>
		/// <param name="revision">The revision number.</param>
		VersionInfo(int32 major, int32 minor, int32 build, int32 revision);

		/// <summary>
		/// Initializes a new instance of the Version class using the specified major, minor, and build values.
		/// </summary>
		/// <param name="major">The major version number.</param>
		/// <param name="minor">The minor version number.</param>
		/// <param name="build">The build number.</param>
		VersionInfo(int32 major, int32 minor, int32 build);

		/// <summary>
		/// Initializes a new instance of the Version class using the specified major and minor values.
		/// </summary>
		/// <param name="major">The major version number.</param>
		/// <param name="minor">The minor version number.</param>
		VersionInfo(int32 major, int32 minor);

		/// <summary>
		/// Initializes a new instance of the Version class.
		/// </summary>
		VersionInfo()
		{
			m_Major = 0;
			m_Minor = 0;
			m_Revision = -1;
			m_Build = -1;
		}

	public:
		/// <summary>
		/// Gets the value of the build component of the version number for the current Version object.
		/// </summary>
		/// <returns>The build number, or -1 if the build number is undefined.</returns>
		FORCE_INLINE int32 Build() const
		{
			return m_Build;
		}

		/// <summary>
		/// Gets the value of the major component of the version number for the current Version object.
		/// </summary>
		/// <returns>The major version number.</returns>
		FORCE_INLINE int32 Major() const
		{
			return m_Major;
		}

		/// <summary>
		/// Gets the value of the minor component of the version number for the current Version object.
		/// </summary>
		/// <returns>The minor version number.</returns>
		FORCE_INLINE int32 Minor() const
		{
			return m_Minor;
		}

		/// <summary>
		/// Gets the value of the revision component of the version number for the current Version object.
		/// </summary>
		/// <returns>The revision number, or -1 if the revision number is undefined.</returns>
		FORCE_INLINE int32 Revision() const
		{
			return m_Revision;
		}

	public:
		/// <summary>
		/// Compares the current Version object to a specified Version object and returns an indication of their relative values.
		/// </summary>
		/// <param name="value">A Version object to compare to the current Version object, or null.</param>
		/// <returns>A signed integer that indicates the relative values of the two objects, as shown in the following table.Return value Meaning Less than zero The current Version object is a version before <paramref name="value" />. Zero The current Version object is the same version as <paramref name="value" />. Greater than zero The current Version object is a version subsequent to <paramref name="value" />. -or-<paramref name="value" /> is null.</returns>
		int32 CompareTo(const VersionInfo& value) const;

		/// <summary>
		/// Returns a value indicating whether the current Version object and a specified Version object represent the same value.
		/// </summary>
		/// <param name="obj">A Version object to compare to the current Version object, or null.</param>
		/// <returns>True if every component of the current Version object matches the corresponding component of the <paramref name="obj" /> parameter; otherwise, false.</returns>
		bool Equals(const VersionInfo& obj) const
		{
			return m_Major == obj.m_Major && m_Minor == obj.m_Minor && m_Build == obj.m_Build && m_Revision == obj.m_Revision;
		}

		FORCE_INLINE bool operator==(const VersionInfo& other) const
		{
			return Equals(other);
		}
		FORCE_INLINE bool operator>(const VersionInfo& other) const
		{
			return other < *this;
		}
		FORCE_INLINE bool operator>=(const VersionInfo& other) const
		{
			return other <= *this;
		}
		FORCE_INLINE bool operator!=(const VersionInfo& other) const
		{
			return !(*this == other);
		}
		FORCE_INLINE bool operator<(const VersionInfo& other) const
		{
			return CompareTo(other) < 0;
		}
		FORCE_INLINE bool operator<=(const VersionInfo& other) const
		{
			return CompareTo(other) <= 0;
		}

	public:
		/// <summary>
		/// Converts the value of the current Version object to its equivalent <see cref="T:String" /> representation.
		/// A specified count indicates the number of components to return.
		/// </summary>
		/// <returns>The <see cref="T:String" /> representation of the values of the major, minor, build, and revision components of the current Version object, each separated by a period character ('.'). The <paramref name="fieldCount" /> parameter determines how many components are returned.fieldCount Return Value 0 An empty string (""). 1 major 2 major.minor 3 major.minor.build 4 major.minor.build.revision For example, if you create Version object using the constructor Version(1,3,5), ToString(2) returns "1.3" and ToString(4) throws an exception.</returns>
		/// <param name="fieldCount">The number of components to return. The <paramref name="fieldCount" /> ranges from 0 to 4.</param>
		String ToString(int32 fieldCount) const;

		String ToString() const;

	public:
		/// <summary>
		/// Try to parse Version from string.
		/// </summary>
		/// <param name="text">Input text.</param>
		/// <param name="value">Result value.</param>
		/// <returns>True if cannot parse text, otherwise false.</returns>
		static bool Parse(const String& text, VersionInfo* value);
	};

	inline uint32 GetHash(const VersionInfo& key)
	{
		return ((key.Major() & 15) << 28) | ((key.Minor() & 255) << 20) | ((key.Build() & 255) << 12) | (key.Revision() & 4095);
	}
} // SE

template<>
struct TIsPODType<SE::VersionInfo>
{
	enum { Value = true };
};
