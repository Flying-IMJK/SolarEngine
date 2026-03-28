#pragma once
#include "TypeID.h"
#include "TypeIDCore.h"

//-------------------------------------------------------------------------

namespace SE
{
    struct UID;
    class Tag;
    struct Color32;
    struct Quaternion;
    class Matrix4x4;
    struct Quaternion;
    class Microseconds;
    class Milliseconds;
    class Seconds;

    struct FloatRange;
    class FloatCurve;

    class ResID;
    class ResPath;

    template <typename T>
    class TBFlags;
	template <typename T>
	class EnumFlags;
}

//-------------------------------------------------------------------------

namespace SE
{
	//-------------------------------------------------------------------------
	// Core Type Registry
	//-------------------------------------------------------------------------

	class SE_API_CORE CoreTypeRegistry
	{
	public:
		struct CoreTypeRecord
		{
			TypeID id;
			uint64 typeSize;
			uint64 typeAlignment;

#ifdef SE_DEVELOPMENT
			Char friendlyName[30];
#endif
		};

	private:
		static bool s_areCoreTypeRecordsInitialized;

	public:
		static void Initialize();
		static void Shutdown();

		static bool IsCoreType(TypeID typeID);
		static TypeIDCore GetType(TypeID typeID);
		static uint64 GetTypeSize(TypeID typeID);
		static uint64 GetTypeAlignment(TypeID typeID);

		static TypeID GetTypeID(TypeIDCore coreType);

		static uint64 GetTypeSize(TypeIDCore coreType);

		static uint64 GetTypeAlignment(TypeIDCore coreType);

#ifdef SE_DEVELOPMENT
		static Char const* GetFriendlyName(TypeIDCore coreType);
#endif
	};

	//-------------------------------------------------------------------------
	// Query
	//-------------------------------------------------------------------------

	TypeID SE_API_CORE GetCoreTypeID(TypeIDCore coreType);
	TypeIDCore SE_API_CORE GetCoreType(TypeID typeID);
	bool SE_API_CORE IsCoreType(TypeID typeID);

	//-------------------------------------------------------------------------

	template<typename T>
	inline TypeID SE_API_CORE GetCoreTypeID()
	{
		return TypeID();
	}
	template<template<typename> typename C>
	inline TypeID SE_API_CORE GetCoreTypeID()
	{
		return TypeID();
	}

	template<>
	inline TypeID GetCoreTypeID<bool>()
	{
		return GetCoreTypeID(TypeIDCore::Bool);
	}
	template<>
	inline TypeID GetCoreTypeID<int8>()
	{
		return GetCoreTypeID(TypeIDCore::Int8);
	}
	template<>
	inline TypeID GetCoreTypeID<int16>()
	{
		return GetCoreTypeID(TypeIDCore::Int16);
	}
	template<>
	inline TypeID GetCoreTypeID<int32>()
	{
		return GetCoreTypeID(TypeIDCore::Int32);
	}
	template<>
	inline TypeID GetCoreTypeID<int64>()
	{
		return GetCoreTypeID(TypeIDCore::Int64);
	}
	template<>
	inline TypeID GetCoreTypeID<uint8>()
	{
		return GetCoreTypeID(TypeIDCore::Uint8);
	}
	template<>
	inline TypeID GetCoreTypeID<uint16>()
	{
		return GetCoreTypeID(TypeIDCore::Uint16);
	}
	template<>
	inline TypeID GetCoreTypeID<uint32>()
	{
		return GetCoreTypeID(TypeIDCore::Uint32);
	}
	template<>
	inline TypeID GetCoreTypeID<uint64>()
	{
		return GetCoreTypeID(TypeIDCore::Uint64);
	}
	template<>
	inline TypeID GetCoreTypeID<float>()
	{
		return GetCoreTypeID(TypeIDCore::Float);
	}
	template<>
	inline TypeID GetCoreTypeID<double>()
	{
		return GetCoreTypeID(TypeIDCore::Double);
	}
	template<>
	inline TypeID GetCoreTypeID<UID>()
	{
		return GetCoreTypeID(TypeIDCore::UUID);
	}
	template<>
	inline TypeID GetCoreTypeID<StringID>()
	{
		return GetCoreTypeID(TypeIDCore::StringID);
	}
	template<>
	inline TypeID GetCoreTypeID<TypeID>()
	{
		return GetCoreTypeID(TypeIDCore::TypeID);
	}
	template<>
	inline TypeID GetCoreTypeID<String>()
	{
		return GetCoreTypeID(TypeIDCore::String);
	}
	template<>
	inline TypeID GetCoreTypeID<Color32>()
	{
		return GetCoreTypeID(TypeIDCore::Color);
	}
	template<>
	inline TypeID GetCoreTypeID<Float2>()
	{
		return GetCoreTypeID(TypeIDCore::Float2);
	}
	template<>
	inline TypeID GetCoreTypeID<Float3>()
	{
		return GetCoreTypeID(TypeIDCore::Float3);
	}
	template<>
	inline TypeID GetCoreTypeID<Float4>()
	{
		return GetCoreTypeID(TypeIDCore::Float4);
	}
	template<>
	inline TypeID GetCoreTypeID<Quaternion>()
	{
		return GetCoreTypeID(TypeIDCore::Quaternion);
	}
	template<>
	inline TypeID GetCoreTypeID<Matrix4x4>()
	{
		return GetCoreTypeID(TypeIDCore::Matrix);
	}
	template<>
	inline TypeID GetCoreTypeID<Microseconds>()
	{
		return GetCoreTypeID(TypeIDCore::Microseconds);
	}
	template<>
	inline TypeID GetCoreTypeID<Milliseconds>()
	{
		return GetCoreTypeID(TypeIDCore::Milliseconds);
	}
	template<>
	inline TypeID GetCoreTypeID<Seconds>()
	{
		return GetCoreTypeID(TypeIDCore::Seconds);
	}
	// template <>
	// inline TypeID GetCoreTypeID<Percentage>() { return GetCoreTypeID(TypeIDCore::Percentage); }
	// template <>
	// inline TypeID GetCoreTypeID<IntRange>() { return GetCoreTypeID(TypeIDCore::IntRange); }
	template<>
	inline TypeID GetCoreTypeID<FloatRange>()
	{
		return GetCoreTypeID(TypeIDCore::FloatRange);
	}
	template<>
	inline TypeID GetCoreTypeID<FloatCurve>()
	{
		return GetCoreTypeID(TypeIDCore::FloatCurve);
	}
	template<>
	inline TypeID GetCoreTypeID<TBFlags>()
	{
		return GetCoreTypeID(TypeIDCore::TBitFlags);
	}
	template<>
	inline TypeID GetCoreTypeID<EnumFlags>()
	{
		return GetCoreTypeID(TypeIDCore::TBitFlags);
	}
	/*template<>
	inline TypeID GetCoreTypeID<ResPath>()
	{
		return GetCoreTypeID(TypeIDCore::ResPath);
	}
	template<>
	inline TypeID GetCoreTypeID<ResTypeID>()
	{
		return GetCoreTypeID(TypeIDCore::ResourceTypeID);
	}
	template<>
	inline TypeID GetCoreTypeID<ResID>()
	{
		return GetCoreTypeID(TypeIDCore::ResourceID);
	}
	template<>
	inline TypeID GetCoreTypeID<ResPtr>()
	{
		return GetCoreTypeID(TypeIDCore::ResourcePtr);
	}
	template<>
	inline TypeID GetCoreTypeID<TResPtr>()
	{
		return GetCoreTypeID(TypeIDCore::TResourcePtr);
	}
*/
	//-------------------------------------------------------------------------
	// Validation for getters/setters
	//-------------------------------------------------------------------------

	template<typename T>
	inline bool ValidateTypeAgainstTypeID(TypeID typeID)
	{
		TypeID const nativeTypeID = GetCoreTypeID<T>();

		// Handle derivation of ResPtr to TResPtr
//		if (nativeTypeID == GetCoreTypeID(TypeIDCore::ResourcePtr))
//		{
//			return typeID == GetCoreTypeID(TypeIDCore::ResourcePtr) || typeID == GetCoreTypeID(TypeIDCore::TResourcePtr);
//		}

		// Handle derivation of BitFlags to TBitFlags
		/*if (nativeTypeID == GetCoreTypeID(TypeIDCore::BitFlags))
		{
			return typeID == GetCoreTypeID(TypeIDCore::BitFlags) || typeID == GetCoreTypeID(TypeIDCore::TBitFlags);
		}*/

		//-------------------------------------------------------------------------

		return GetCoreTypeID<T>() == typeID;
	}

	//-------------------------------------------------------------------------
	// Global conversion operators
	//-------------------------------------------------------------------------
	inline bool operator==(TypeID const& typeID, TypeIDCore coreType)
	{
		return typeID == GetCoreTypeID(coreType);
	}

	inline bool operator!=(TypeID const& typeID, TypeIDCore coreType)
	{
		return typeID != GetCoreTypeID(coreType);
	}

	inline bool operator==(TypeIDCore coreType, TypeID const& typeID)
	{
		return typeID == GetCoreTypeID(coreType);
	}

	inline bool operator!=(TypeIDCore coreType, TypeID const& typeID)
	{
		return typeID != GetCoreTypeID(coreType);
	}
}


