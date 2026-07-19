#pragma once
#include "GraphMeta.h"
#include "Runtime/Core/Types/Object.h"
#include "Runtime/Utilities/Variant.h"

namespace SE
{
	template<typename T>
	class DataContainer;
	typedef DataContainer<byte> BytesContainer;

	/// <summary>
	/// The channel mask modes.
	/// </summary>
	enum class ChannelMask
	{
		/// <summary>
		/// The red channel.
		/// </summary>
		Red = 0,

		/// <summary>
		/// The green channel.
		/// </summary>
		Green = 1,

		/// <summary>
		/// The blue channel.
		/// </summary>
		Blue = 2,

		/// <summary>
		/// The alpha channel.
		/// </summary>
		Alpha = 3,
	};

	/// <summary>
	/// Represents a parameter in the Graph.
	/// </summary>
	class SE_API_RUNTIME GraphParameter : public Object
	{
	public:
		/// <summary>
		/// Parameter type
		/// </summary>
		VariantTypeHandle Type;

		/// <summary>
		/// Parameter unique ID
		/// </summary>
		UID Identifier;

		/// <summary>
		/// Parameter name
		/// </summary>
		String Name;

		/// <summary>
		/// Parameter value
		/// </summary>
		Variant Value;

		/// <summary>
		/// True if is exposed outside
		/// </summary>
		bool IsPublic = true;

		/// <summary>
		/// Additional metadata
		/// </summary>
		GraphMeta Meta;

	public:
		/// <summary>
		/// Gets the typename of the parameter type (excluding in-build types).
		/// </summary>
		/// <returns>The typename of the parameter type.</returns>
		StringAnsiView GetTypeTypeName() const
		{
			return StringAnsiView(Type.TypeName);
		}

		/// <summary>
		/// Gets the data of the Visject Meta entry assigned to this parameter.
		/// </summary>
		/// <param name="typeID">Entry type ID</param>
		/// <returns>The entry data or empty if missing or not loaded.</returns>
		BytesContainer GetMetaData(int32 typeID) const;
	};
} // SE
