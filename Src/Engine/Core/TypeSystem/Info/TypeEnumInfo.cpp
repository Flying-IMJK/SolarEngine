#include "TypeEnumInfo.h"
#include "Core/Types/Collections/Sorting.h"
//-------------------------------------------------------------------------

namespace SE
{
	int32 TypeEnumInfo::GetNumConstants() const
	{
		return constants.Count();
	}

	bool TypeEnumInfo::IsValidValue(StringID label) const
	{
		for (auto const &constant : constants)
		{
			if (constant.id == label)
			{
				return true;
			}
		}

		return false;
	}

	int64 TypeEnumInfo::GetConstantValue(StringID label) const
	{
		for (auto const &constant : constants)
		{
			if (constant.id == label)
			{
				return constant.value;
			}
		}

		LOG_ERROR("Type System", "Invalid enum constant value ({0}) for enum ({1})", label, fullName);
		return constants.begin()->value;
	}

	bool TypeEnumInfo::TryGetConstantValue(StringID label, int64& outValue) const
	{
		for (auto const &constant : constants)
		{
			if (constant.id == label)
			{
				outValue = constant.value;
				return true;
			}
		}

		return false;
	}

	StringID TypeEnumInfo::GetConstantLabel(int64 value) const
	{
		for (auto const &constant : constants)
		{
			if (constant.value == value)
			{
				return constant.id;
			}
		}

		ENGINE_UNREACHABLE_CODE();
		return StringID();
	}

	bool TypeEnumInfo::TryGetConstantLabel(int64 value, StringID& outValue) const
	{
		for (auto const &constant : constants)
		{
			if (constant.value == value)
			{
				outValue = constant.id;
				return true;
			}
		}

		return false;
	}

	String const* TypeEnumInfo::TryGetConstantDescription(StringID label) const
	{
		for (auto const &constant : constants)
		{
			if (constant.id == label)
			{
				return &constant.description;
			}
		}

		return nullptr;
	}

#ifdef SE_DEVELOPMENT
	List<TypeEnumInfo::ConstantInfo> TypeEnumInfo::GetConstantsInAlphabeticalOrder() const
    {
		List<ConstantInfo> sortedConstants = constants;

		Function<bool(const ConstantInfo&, const ConstantInfo&)> compare = [](const ConstantInfo &a, const ConstantInfo &b)
		{
		  return a.alphabeticalOrder < b.alphabeticalOrder;
		};

		Sorting::QuickSort(sortedConstants, compare);
        return sortedConstants;
    }
#endif
}