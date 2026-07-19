#include "CoreTypes.h"
//#include "Core/Resource/ResourcePtr.h"
//#include "../Types/Percentage.h"
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Types/BitFlags.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Time.h"
#include "Runtime/Core/Math/Transform.h"
#include "Runtime/Core/Math/NumericRange.h"
#include "Runtime/Core/Math/FloatCurve.h"
#include "Runtime/Core/Math/Vector2.h"

//-------------------------------------------------------------------------
#ifdef SE_DEVELOPMENT
#define FRIEDNLY_NAME_COPY(name) SE::StringUtils::Copy(s_coreTypeRecords[(uint8_t)name].friendlyName, &(SE_TEXT(#name))[12])
#else
#define FRIEDNLY_NAME_COPY(name)
#endif

#define REGISTER_TYPE_RECORD( coreTypeEnum, fullyQualifiedTypeName)															\
	ID = SE::TypeID(SE_TEXT(MACRO_TO_STR(fullyQualifiedTypeName)));															\
	ENGINE_ASSERT(s_coreTypeRecords[(uint8_t)coreTypeEnum].id == SE::TypeID::Invalid);										\
	s_coreTypeRecords[(uint8_t)coreTypeEnum] = { ID, sizeof( fullyQualifiedTypeName ), alignof( fullyQualifiedTypeName)};	\
	FRIEDNLY_NAME_COPY(coreTypeEnum);


#define REGISTER_TEMPLATE_TYPE_RECORD_GENERIC( coreTypeEnum, fullyQualifiedTypeName, baseSpecialization )					\
	ID = SE::TypeID(SE_TEXT(MACRO_TO_STR(fullyQualifiedTypeName<baseSpecialization>>)));									\
	ENGINE_ASSERT(s_coreTypeRecords[(uint8_t)coreTypeEnum].id == SE::TypeID::Invalid);										\
	s_coreTypeRecords[(uint8_t)coreTypeEnum] = { ID, sizeof( fullyQualifiedTypeName<baseSpecialization> ), alignof( fullyQualifiedTypeName<baseSpecialization> ) };\
	FRIEDNLY_NAME_COPY(coreTypeEnum);

SE::CoreTypeRegistry::CoreTypeRecord s_coreTypeRecords[static_cast<uint8>(SE::TypeIDCore::NumTypes)];

void TypeRegistry()
{
	SE::TypeID ID;

	REGISTER_TYPE_RECORD(SE::TypeIDCore::Bool, bool);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Uint8, uint8);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int8, int8);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Uint16, uint16);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int16, int16);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Uint32, uint32);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int32, int32);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Uint64, uint64);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int64, int64);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Float, float);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Double, double);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::UUID, SE::UID);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::StringID, SE::StringID);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::TypeID, SE::TypeID);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::String, SE::String);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Color, SE::Color32);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Float2, SE::Vector2Base<float>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Float3, SE::Vector3Base<float>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Float4, SE::Vector4Base<float>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Double2, SE::Vector2Base<double>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Double3, SE::Vector3Base<double>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Double4, SE::Vector4Base<double>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int2, SE::Vector2Base<int>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int3, SE::Vector3Base<int>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Int4, SE::Vector4Base<int>);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Quaternion, SE::Quaternion);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Matrix, SE::Matrix);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Transform, SE::Transform);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Microseconds, SE::Microseconds);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Milliseconds, SE::Milliseconds);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::Seconds, SE::Seconds);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::FloatRange, SE::FloatRange);
	REGISTER_TYPE_RECORD(SE::TypeIDCore::FloatCurve, SE::FloatCurve);

	REGISTER_TEMPLATE_TYPE_RECORD_GENERIC(SE::TypeIDCore::TBitFlags, SE::EnumFlags, enum class TempEnum);
	REGISTER_TEMPLATE_TYPE_RECORD_GENERIC(SE::TypeIDCore::List, SE::List, uint8);
}


//-------------------------------------------------------------------------

namespace SE
{
	bool CoreTypeRegistry::s_areCoreTypeRecordsInitialized = false;

	//-------------------------------------------------------------------------

	void CoreTypeRegistry::Initialize()
	{
		ENGINE_ASSERT(!s_areCoreTypeRecordsInitialized);
		TypeRegistry();
		s_areCoreTypeRecordsInitialized = true;
	}

	void CoreTypeRegistry::Shutdown()
	{
		ENGINE_ASSERT( s_areCoreTypeRecordsInitialized);
		s_areCoreTypeRecordsInitialized = false;
	}

	//-------------------------------------------------------------------------

	bool CoreTypeRegistry::IsCoreType(TypeID typeID)
	{
		ENGINE_ASSERT(s_areCoreTypeRecordsInitialized);
		for (auto i = 0; i < (uint8)TypeIDCore::NumTypes; i++)
		{
			if (s_coreTypeRecords[i].id == typeID)
			{
				return true;
			}
		}

		return false;
	}

	TypeIDCore CoreTypeRegistry::GetType(TypeID typeID)
	{
		ENGINE_ASSERT(s_areCoreTypeRecordsInitialized);

		for (int32_t i = 0; i < (int32_t)TypeIDCore::NumTypes; i++)
		{
			if (s_coreTypeRecords[i].id == typeID)
			{
				return (TypeIDCore)i;
			}
		}

		return TypeIDCore::Invalid;
	}

	uint64 CoreTypeRegistry::GetTypeSize(TypeID typeID)
	{
		ENGINE_ASSERT(s_areCoreTypeRecordsInitialized);

		for (auto i = 0; i < (uint8_t)TypeIDCore::NumTypes; i++)
		{
			if (s_coreTypeRecords[i].id == typeID)
			{
				return s_coreTypeRecords[i].typeSize;
			}
		}

		ENGINE_UNREACHABLE_CODE();
		return 0;
	}

	uint64 CoreTypeRegistry::GetTypeAlignment(TypeID typeID)
	{
		ENGINE_ASSERT(s_areCoreTypeRecordsInitialized);

		for (auto i = 0; i < (uint8_t)TypeIDCore::NumTypes; i++)
		{
			if (s_coreTypeRecords[i].id == typeID)
			{
				return s_coreTypeRecords[i].typeAlignment;
			}
		}

		ENGINE_UNREACHABLE_CODE();
		return 0;
	}

	TypeID CoreTypeRegistry::GetTypeID(TypeIDCore coreType)
	{
		return s_coreTypeRecords[(uint8)coreType].id;
	}

	uint64 CoreTypeRegistry::GetTypeSize(TypeIDCore coreType)
	{
		return s_coreTypeRecords[(uint8)coreType].typeSize;
	}

	uint64 CoreTypeRegistry::GetTypeAlignment(TypeIDCore coreType)
	{
		return s_coreTypeRecords[(uint8)coreType].typeAlignment;
	}

#ifdef SE_DEVELOPMENT
	Char const* CoreTypeRegistry::GetFriendlyName(TypeIDCore coreType)
	{
		return s_coreTypeRecords[(uint8)coreType].friendlyName;
	}
#endif


	//-------------------------------------------------------------------------
	// Query
	//-------------------------------------------------------------------------

	TypeID GetCoreTypeID(TypeIDCore coreType)
	{
		return CoreTypeRegistry::GetTypeID(coreType);
	}
	TypeIDCore GetCoreType(TypeID typeID)
	{
		return CoreTypeRegistry::GetType(typeID);
	}
	bool IsCoreType(TypeID typeID)
	{
		return CoreTypeRegistry::IsCoreType(typeID);
	}

}
