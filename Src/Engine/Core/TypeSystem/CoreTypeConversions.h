#pragma once

#include "Property/TypeProperty.h"
#include "Info/TypeEnumInfo.h"
#include "Types.h"

//-------------------------------------------------------------------------

namespace SE
{
    class Types;
}

//-------------------------------------------------------------------------
// Core Type Serialization
//-------------------------------------------------------------------------
// Basic serialization of core types to/from string and binary representations
// Primarily used to serialize core type properties
// Each core type needs to implement a serializer function specializations
// This also handles enums and bitflags as core types

namespace SE
{
    SE_API_CORE bool ConvertStringToNativeType(TypeID typeID, TypeID templateArgumentTypeID, String const &strValue, void *pValue);
    SE_API_CORE bool ConvertNativeTypeToString(TypeID typeID, TypeID templateArgumentTypeID, void const *pValue, String &strValue);
    SE_API_CORE bool ConvertBinaryToNativeType(TypeID typeID, TypeID templateArgumentTypeID, List<uint8> const &byteArray, void *pValue);
    SE_API_CORE bool ConvertNativeTypeToBinary(TypeID typeID, TypeID templateArgumentTypeID, void const *pValue, List<uint8> &byteArray);
    SE_API_CORE bool ConvertStringToBinary(TypeID typeID, TypeID templateArgumentTypeID, String const &strValue, List<uint8> &byteArray);
    SE_API_CORE bool IsValidStringValueForType(TypeID typeID, TypeID templateArgumentTypeID, String const &strValue);

	template <typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
	StringView ConvertEnumToString(T value)
	{
		return Types::GetEnumInfo<T>()->GetConstantLabel((int)value).ToString();
	}

    //-------------------------------------------------------------------------

    inline bool ConvertStringToNativeType(TypeProperty const &propertyInfo, String const &strValue, void *pValue)
    {
        return ConvertStringToNativeType(propertyInfo.typeID, propertyInfo.templateArgumentTypeID, strValue, pValue);
    }

    inline bool ConvertNativeTypeToString(TypeProperty const &propertyInfo, void const *pValue, String &strValue)
    {
        return ConvertNativeTypeToString(propertyInfo.typeID, propertyInfo.templateArgumentTypeID, pValue, strValue);
    }

    inline bool ConvertBinaryToNativeType(TypeProperty const &propertyInfo, List<uint8> const &byteArray, void *pValue)
    {
        return ConvertBinaryToNativeType(propertyInfo.typeID, propertyInfo.templateArgumentTypeID, byteArray, pValue);
    }

    inline bool ConvertNativeTypeToBinary(TypeProperty const &propertyInfo, void const *pValue, List<uint8> &byteArray)
    {
        return ConvertNativeTypeToBinary(propertyInfo.typeID, propertyInfo.templateArgumentTypeID, pValue, byteArray);
    }

    inline bool ConvertStringToBinary(TypeProperty const &propertyInfo, String const &strValue, List<uint8> &byteArray)
    {
        return ConvertStringToBinary(propertyInfo.typeID, propertyInfo.templateArgumentTypeID, strValue, byteArray);
    }

    inline bool IsValidStringValueForType(TypeProperty const &propertyInfo, String const &strValue)
    {
        return IsValidStringValueForType(propertyInfo.typeID, propertyInfo.templateArgumentTypeID, strValue);
    }

    //-------------------------------------------------------------------------

    // Convert a comma separated string of floats into an array of floats
    SE_API_CORE void StringToFloatArray(String const &str, int32 const numFloats, float *pFloats);

    // Convert an array of floats into a comma separated string of floats
    SE_API_CORE void FloatArrayToString(float const *pFloats, int32 const numFloats, String &strValue);

    // Convert a comma separated string of ints into an array of ints
    SE_API_CORE void StringToIntArray(String const &str, int32 const numInts, int32 *pInts);

    // Convert an array of ints into a comma separated string of ints
    SE_API_CORE void IntArrayToString(int32 const *pInts, int32 const numInts, String &strValue);
}