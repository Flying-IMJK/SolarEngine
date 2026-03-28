#include "CoreTypeConversions.h"
#include "Types.h"
//#include "Core/Resource/ResourcePtr.h"
#include "Core/Serialization/MemoryWriteStream.h"
#include "Core/Serialization/MemoryReadStream.h"
#include "Core/Utilities/Time.h"
#include "Core/Math/Color.h"
#include "Core/Types/UID.h"
#include "Core/Types/Strings/StringID.h"
#include "Core/Types/BitFlags.h"
#include "Core/Math/Math.h"
// #include "Core/Types/Percentage.h"

#include "Core/Math/FloatCurve.h"
#include "Core/Math/NumericRange.h"

#include "Info/TypeEnumInfo.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

//-------------------------------------------------------------------------

namespace SE
{
    void StringToFloatArray(String const &str, int32 const numFloats, float *pFloats)
    {
        char substr[128] = {0};
        int32 resIdx = 0;

        bool complete = false;
        int startIdx = 0;
        while (resIdx < numFloats && !complete)
        {
            int endIdx = str.FindFirstOf(',', startIdx);
            if (endIdx == INVALID_INDEX)
            {
                endIdx = str.Length();
                complete = true;
            }

            size_t sizeToCopy = endIdx - startIdx;
            ENGINE_ASSERT(sizeToCopy < 128);
            memcpy(substr, &str[startIdx], sizeToCopy);
            substr[sizeToCopy] = '\0';

            pFloats[resIdx++] = std::strtof(substr, nullptr);
            startIdx = endIdx + 1;
        }
    }

    void FloatArrayToString(float const *pFloats, int32 const numFloats, String &strValue)
    {
        strValue.Clear();

        for (int32 i = 0; i < numFloats; i++)
        {
            strValue += StringUtils::ToString(pFloats[i]);

            if (i != (numFloats - 1))
            {
                strValue += ',';
            }
        }
    }

    void StringToIntArray(String const &str, int32 const numInts, int32 *pInts)
    {
        char substr[128] = {0};
        int32 resIdx = 0;

        bool complete = false;
		int startIdx = 0;
        while (resIdx < numInts && !complete)
        {
            int endIdx = str.FindFirstOf(',', startIdx);
            if (endIdx == INVALID_INDEX)
            {
                endIdx = str.Length();
                complete = true;
            }

            size_t sizeToCopy = endIdx - startIdx;
            ENGINE_ASSERT(sizeToCopy < 128);
            memcpy(substr, &str[startIdx], sizeToCopy);
            substr[sizeToCopy] = '\0';

            pInts[resIdx++] = std::strtoul(substr, nullptr, 0);
            startIdx = endIdx + 1;
        }
    }

    void IntArrayToString(int32 const *pInts, int32 const numInts, String &strValue)
    {
        strValue.Clear();

        for (int32 i = 0; i < numInts; i++)
        {
            strValue += StringUtils::ToString(pInts[i]);

            if (i != (numInts - 1))
            {
                strValue += ',';
            }
        }
    }

    /*inline static bool ConvertStringToBitFlags(EnumInfo const &enumInfo, String const &str, BitFlags &outFlags)
    {
        outFlags.ClearAllFlags();

        if (str.IsEmpty())
        {
            return true;
        }

        // Handle hex format explicitly
        //-------------------------------------------------------------------------

        if (str.StartsWith(SE_TEXT("0x")))
        {
            uint32 value;
			StringUtils::ParseHex(str.Get(), &value);
//				std::strtoul(str.Get(), nullptr, 16);
            outFlags.Set(value);
            return true;
        }

        //-------------------------------------------------------------------------

        Char buffer[256] = {0};
        size_t bufferIdx = 0;
        bool isReadingEnumValue = false;
        bool hasReadWhiteSpace = false;

        //-------------------------------------------------------------------------

        auto ProcessReadValue = [&]()
        {
            if (isReadingEnumValue)
            {
                buffer[bufferIdx] = 0;
                bufferIdx = 0;
                isReadingEnumValue = false;

                int64 flag;
                if (enumInfo.TryGetConstantValue(StringID(buffer), flag))
                {
                    outFlags.SetFlag((uint8)flag, true);
                    return true;
                }
            }

            return false;
        };

        //-------------------------------------------------------------------------

        size_t const length = str.Length();
        for (auto i = 0u; i < length; i++)
        {
            if (str[i] == '|')
            {
                if (!ProcessReadValue())
                {
                    outFlags.ClearAllFlags();
                    return false;
                }
                hasReadWhiteSpace = false;
            }
            else if (str[i] == ' ')
            {
                if (isReadingEnumValue)
                {
                    hasReadWhiteSpace = true;
                }
            }
            else
            {
                // We read a white space while reading an enum value
                if (hasReadWhiteSpace)
                {
                    outFlags.ClearAllFlags();
                    return false;
                }

                isReadingEnumValue = true;
                buffer[bufferIdx] = str[i];
                bufferIdx++;
                ENGINE_ASSERT(bufferIdx < 256);
            }
        }

        //-------------------------------------------------------------------------

        if (!ProcessReadValue())
        {
            outFlags.ClearAllFlags();
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    inline static bool ConvertBitFlagsToString(EnumInfo const &enumInfo, BitFlags const &flags, String &strValue)
    {
        strValue.Clear();

        for (auto i = 0u; i < 32; i++)
        {
            if (flags.IsFlagSet((uint8_t)i))
            {
                StringID label;
                if (!enumInfo.TryGetConstantLabel(i, label))
                {
                    strValue.Clear();
                    return false;
                }

                strValue += label.ToString();
                strValue += "|";
            }
        }

        //-------------------------------------------------------------------------

        if (!strValue.IsEmpty())
        {
            strValue.Remove(strValue.Length() - 1, 1);
        }

        return true;
    }*/

    //-------------------------------------------------------------------------

    template <typename T>
    bool ConvertToBinary(TypeID typeID, TypeID templateArgumentTypeID, String const &strValue, List<uint8> &byteArray)
    {
        T value;
        if (!ConvertStringToNativeType(typeID, templateArgumentTypeID, strValue, &value))
        {
            return false;
        }

        return ConvertNativeTypeToBinary(typeID, templateArgumentTypeID, &value, byteArray);
    }

    //-------------------------------------------------------------------------

    bool ConvertStringToNativeType(TypeID typeID, TypeID templateArgumentTypeID, String const &str, void *pValue)
    {
        // Enums
        if (!IsCoreType(typeID))
        {
            TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(typeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);

            StringID const enumID(str);
            int64 const enumValue = pEnumInfo->GetConstantValue(enumID);

            // We only support up to 32 bit enum types...
            switch (pEnumInfo->underlyingType)
            {
            case TypeIDCore::Uint8:
            {
                *((uint8 *)pValue) = (uint8)enumValue;
            }
            break;

            case TypeIDCore::Int8:
            {
                *((int8 *)pValue) = (int8)enumValue;
            }
            break;

            case TypeIDCore::Uint16:
            {
                *((uint16 *)pValue) = (uint16)enumValue;
            }
            break;

            case TypeIDCore::Int16:
            {
                *((int16 *)pValue) = (int16)enumValue;
            }
            break;

            case TypeIDCore::Uint32:
            {
                *((uint32 *)pValue) = (uint32)enumValue;
            }
            break;

            case TypeIDCore::Int32:
            {
                *((int32 *)pValue) = (int32)enumValue;
            }
            break;

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }
        else // Real core types
        {
            TypeIDCore const typeToConvert = GetCoreType(typeID);
            switch (typeToConvert)
            {
            case TypeIDCore::Bool:
            {
                String &temp = const_cast<String&>(str);
                temp.ToLower();
                *reinterpret_cast<bool *>(pValue) = (temp == SE_TEXT("true"));
            }
            break;

            case TypeIDCore::Uint8:
            {
				uint8 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<uint8 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Int8:
            {
				int8 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<int8 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Uint16:
            {
				uint16 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<uint16 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Int16:
            {
				int16 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
				*reinterpret_cast<int16 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Uint32:
            {
				uint32 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<uint32 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Int32:
            {
				int32 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<int32 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Uint64:
            {
				uint64 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<uint64 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Int64:
            {
				int64 parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<int64 *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Float:
            {
				float parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<float *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::Double:
            {
				double parseValue;
				if (StringUtils::Parse(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<double *>(pValue) = parseValue;
            }
            break;

            case TypeIDCore::String:
            {
                *reinterpret_cast<String *>(pValue) = str;
            }
            break;

            case TypeIDCore::StringID:
            {
                *reinterpret_cast<StringID *>(pValue) = StringID(str.Get());
            }
            break;

            case TypeIDCore::TypeID:
            {
                ENGINE_UNREACHABLE_CODE()
                // *reinterpret_cast<TypeID *>(pValue) = TypeID(str.Get());
            }
            break;

            case TypeIDCore::UUID:
            {
				if (!UID::Parse(str, *reinterpret_cast<UID *>(pValue)))
				{
					return false;
				}
            }
            break;

            case TypeIDCore::Color:
            {
				uint32 parseValue;
				if (StringUtils::ParseHex(str.Get(), str.Length(), &parseValue))
				{
					return false;
				}
//                uint32 const colorType = std::strtoul(str.Get(), nullptr, 16);
                *reinterpret_cast<Color32 *>(pValue) = Color32(parseValue);
            }
            break;

            case TypeIDCore::Float2:
            {
                StringToFloatArray(str, 2, reinterpret_cast<Float2 *>(pValue)->Raw);
            }
            break;

            case TypeIDCore::Float3:
            {
                StringToFloatArray(str, 3, reinterpret_cast<Float3 *>(pValue)->Raw);
            }
            break;

            case TypeIDCore::Float4:
            {
                StringToFloatArray(str, 4, reinterpret_cast<Float4 *>(pValue)->Raw);
            }
            break;

            case TypeIDCore::Quaternion:
            {
                Float4 f4;
                StringToFloatArray(str, 4, f4.Raw);
                *reinterpret_cast<Quaternion *>(pValue) = Quaternion(f4.x, f4.y, f4.z, f4.w);
            }
            break;

            case TypeIDCore::Matrix:
            {
                float floatData[7];
                StringToFloatArray( str, 7, floatData );

                Quaternion const rotation( floatData[0], floatData[1], floatData[2], 0);
                Float3 const translation = Float3( floatData[3], floatData[4], floatData[5] );
                Matrix::Transformation( floatData[6], rotation, translation, *reinterpret_cast<Matrix*>( pValue ));
            }
            break;

            case TypeIDCore::Microseconds:
            {
				float parseValue;
				if (!StringUtils::Parse(str.Get(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<Microseconds *>(pValue) = Microseconds(parseValue);
            }
            break;

            case TypeIDCore::Milliseconds:
            {
				float parseValue;
				if (!StringUtils::Parse(str.Get(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<Milliseconds *>(pValue) = Milliseconds(parseValue);
            }
            break;

            case TypeIDCore::Seconds:
            {
				float parseValue;
				if (!StringUtils::Parse(str.Get(), &parseValue))
				{
					return false;
				}
                *reinterpret_cast<Seconds *>(pValue) = Seconds(parseValue);
            }
            break;

            // case TypeIDCore::Percentage:
            // {
            //     *reinterpret_cast<Percentage *>(pValue) = Percentage(std::strtof(str.Get(), nullptr));
            // }
            // break;

/*            case TypeIDCore::ResPath:
            {
                *reinterpret_cast<ResPath *>(pValue) = ResPath(str);
            }
            break;*/
/*
            case TypeIDCore::IntRange:
            {
                int32 intData[2];
                StringToIntArray(str, 2, intData);

                // Invalid range
                if (intData[0] > intData[1])
                {
                    *reinterpret_cast<IntRange *>(pValue) = IntRange();
                }
                else
                {
                    *reinterpret_cast<IntRange *>(pValue) = IntRange(intData[0], intData[1]);
                }
            }
            break;
*/
            case TypeIDCore::FloatRange:
            {
                float floatData[2];
                StringToFloatArray(str, 2, floatData);

                // Invalid range
                if (floatData[0] > floatData[1])
                {
                    *reinterpret_cast<FloatRange *>(pValue) = FloatRange();
                }
                else
                {
                    *reinterpret_cast<FloatRange *>(pValue) = FloatRange(floatData[0], floatData[1]);
                }
            }
            break;

            case TypeIDCore::FloatCurve:
            {
                FloatCurve &outCurve = *reinterpret_cast<FloatCurve *>(pValue);
                if (!FloatCurve::FromString(str, outCurve))
                {
                    return false;
                }
            }
            break;

/*            case TypeIDCore::ResourceTypeID:
            {
                *reinterpret_cast<ResTypeID *>(pValue) = ResTypeID(str);
            }
            break;

            case TypeIDCore::ResourcePtr:
            case TypeIDCore::TResourcePtr:
            {
                ResPtr &resourcePtr = *reinterpret_cast<ResPtr *>(pValue);
                if (str.IsEmpty())
                {
                    resourcePtr = ResPtr();
                }
                else
                {
                    if (!ResID::IsValidResourceIDString(str))
                    {
                        LOG_WARNING("TypeSystem", "Core Type Conversions Invalid resource ID string encountered: {o}", str.Get());
                        return false;
                    }
                    ResID  const ID(str);
                    resourcePtr = ResPtr(ID);
                }
            }
            break;

            case TypeIDCore::ResourceID:
            {
                if (str.IsEmpty())
                {
                    *reinterpret_cast<ResID  *>(pValue) = ResID ();
                }
                else
                {
                    if (!ResID ::IsValidResourceIDString(str))
                    {
                        LOG_WARNING("TypeSystem", "Core Type Conversions Invalid resource ID string encountered: {0}", str.Get());
                        return false;
                    }
                    *reinterpret_cast<ResID  *>(pValue) = ResID (str);
                }
            }
            break;*/

            /*case TypeIDCore::BitFlags:
            {
//				std::strtol(str.Get(), nullptr, 0)
				const Char* strChar = str.Get();
				uint32 bitValue;
				if (StringUtils::ParseHex(strChar, &bitValue))
				{
					LOG_WARNING("TypeSystem", "Core Type Conversions Invalid BitFlags string encountered: {0}", str.Get());
					return false;
				}
				reinterpret_cast<BitFlags *>(pValue)->Set(bitValue);
            }
            break;

            case TypeIDCore::TBitFlags:
            {
                EnumInfo const *pEnumInfo = Types::GetEnumInfo(templateArgumentTypeID);
                if (pEnumInfo == nullptr)
                {
                    LOG_WARNING("TypeSystem", "Core Type Conversions Unknown enum class ({0}) for TBitFlags", templateArgumentTypeID.ToStringID());
                    return false;
                }

                if (!ConvertStringToBitFlags(*pEnumInfo, str, *reinterpret_cast<BitFlags *>(pValue)))
                {
                    LOG_WARNING("TypeSystem", "Core Type Conversions Failed to convert string ({0}) into valid TBitFlags<{0}>", str.Get(), templateArgumentTypeID.ToStringID());
                    return false;
                }
            }
            break;*/

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }

        return true;
    }

    bool ConvertNativeTypeToString(TypeID typeID, TypeID templateArgumentTypeID, void const *pValue, String &strValue)
    {
        // Enums
        if (!IsCoreType(typeID))
        {
            TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(typeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);

            // We only support up to 32 bit enum types...
            switch (pEnumInfo->underlyingType)
            {
            case TypeIDCore::Uint8:
            {
                strValue = pEnumInfo->GetConstantLabel(*((uint8_t *)pValue)).ToString();
            }
            break;

            case TypeIDCore::Int8:
            {
                strValue = pEnumInfo->GetConstantLabel(*((int8_t *)pValue)).ToString();
            }
            break;

            case TypeIDCore::Uint16:
            {
                strValue = pEnumInfo->GetConstantLabel(*((uint16_t *)pValue)).ToString();
            }
            break;

            case TypeIDCore::Int16:
            {
                strValue = pEnumInfo->GetConstantLabel(*((int16_t *)pValue)).ToString();
            }
            break;

            case TypeIDCore::Uint32:
            {
                strValue = pEnumInfo->GetConstantLabel(*((uint32 *)pValue)).ToString();
            }
            break;

            case TypeIDCore::Int32:
            {
                strValue = pEnumInfo->GetConstantLabel(*((int32 *)pValue)).ToString();
            }
            break;

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }
        else // Real core types
        {
            TypeIDCore const typeToConvert = GetCoreType(typeID);
            switch (typeToConvert)
            {
            case TypeIDCore::Bool:
            {
                strValue = *reinterpret_cast<bool const *>(pValue) ? "True" : "False";
            }
            break;

            case TypeIDCore::Uint8:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<uint8_t const *>(pValue));
            }
            break;

            case TypeIDCore::Int8:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<int8_t const *>(pValue));
            }
            break;

            case TypeIDCore::Uint16:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<uint16_t const *>(pValue));
            }
            break;

            case TypeIDCore::Int16:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<int16_t const *>(pValue));
            }
            break;

            case TypeIDCore::Uint32:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<uint32 const *>(pValue));
            }
            break;

            case TypeIDCore::Int32:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<int32 const *>(pValue));
            }
            break;

            case TypeIDCore::Uint64:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<uint64_t const *>(pValue));
            }
            break;

            case TypeIDCore::Int64:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<int64_t const *>(pValue));
            }
            break;

            case TypeIDCore::Float:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<float const *>(pValue));
            }
            break;

            case TypeIDCore::Double:
            {
                strValue = StringUtils::ToString(*reinterpret_cast<double const *>(pValue));
            }
            break;

            case TypeIDCore::String:
            {
                strValue = *reinterpret_cast<String const *>(pValue);
            }
            break;

            case TypeIDCore::StringID:
            {
                StringView str = reinterpret_cast<StringID const *>(pValue)->ToString();
                if (str != StringView::Empty)
                {
                    strValue = str;
                }
                else
                {
                    strValue.Clear();
                }
            }
            break;

            case TypeIDCore::TypeID:
            {
                ENGINE_UNREACHABLE_CODE()
                /*StringView str = reinterpret_cast<TypeID const *>(pValue)->ToString();
                if (str != StringView::Empty)
                {
                    strValue = str;
                }
                else
                {
                    strValue.Clear();
                }*/
            }
            break;

            case TypeIDCore::UUID:
            {
                strValue = reinterpret_cast<UID const *>(pValue)->ToString();
            }
            break;

            case TypeIDCore::Color:
            {
                Color32 const &value = *reinterpret_cast<Color32 const *>(pValue);
                strValue = String::Format(SE_TEXT("{0}}:{1}:{2}:{3}X"), value.a, value.b, value.g, value.r);
            }
            break;

            case TypeIDCore::Float2:
            {
                FloatArrayToString(reinterpret_cast<Float2 const *>(pValue)->Raw, 2, strValue);
            }
            break;

            case TypeIDCore::Float3:
            {
                FloatArrayToString(reinterpret_cast<Float3 const *>(pValue)->Raw, 3, strValue);
            }
            break;

            case TypeIDCore::Float4:
            {
                FloatArrayToString(reinterpret_cast<Float4 const *>(pValue)->Raw, 4, strValue);
            }
            break;

            case TypeIDCore::Quaternion:
            {
                Quaternion const *v = reinterpret_cast<Quaternion const *>(pValue);
                FloatArrayToString(v->Raw, 4, strValue);
            }
            break;

            case TypeIDCore::Matrix:
            {
                Matrix const& value = *reinterpret_cast<Matrix const*>( pValue );

                Float3 scale;
                Quaternion rotation;
                Float3 translation;

                value.Decompose(scale, rotation, translation);
                float floatData[7];
                (Float3&) floatData = rotation.GetEuler();
                (Float3&) floatData[3] = value.GetTranslation();
                floatData[6] = value.GetScaleVector().x;

                FloatArrayToString( floatData, 7, strValue );
            }
            break;

            case TypeIDCore::Microseconds:
            {
                strValue = StringUtils::ToString(reinterpret_cast<Microseconds const *>(pValue)->ToFloat());
            }
            break;

            case TypeIDCore::Milliseconds:
            {
                strValue = StringUtils::ToString(reinterpret_cast<Milliseconds const *>(pValue)->ToFloat());
            }
            break;

            case TypeIDCore::Seconds:
            {
                strValue = StringUtils::ToString(reinterpret_cast<Seconds const *>(pValue)->ToFloat());
            }
            break;

            // case TypeIDCore::Percentage:
            // {
            //     strValue = StringTool::ToString(reinterpret_cast<Percentage const *>(pValue)->ToFloat());
            // }
            // break;

/*            case TypeIDCore::ResPath:
            {
                strValue = reinterpret_cast<ResPath const *>(pValue)->c_str();
            }
            break;

             case TypeIDCore::IntRange:
             {
                 IntRange const *pRange = reinterpret_cast<IntRange const *>(pValue);
                 IntArrayToString(&pRange->m_begin, 2, strValue);
             }
             break;*/

            case TypeIDCore::FloatRange:
            {
                FloatRange const *pRange = reinterpret_cast<FloatRange const *>(pValue);
                FloatArrayToString(&pRange->begin, 2, strValue);
            }
            break;

            case TypeIDCore::FloatCurve:
            {
                FloatCurve const *pCurve = reinterpret_cast<FloatCurve const *>(pValue);
                strValue = pCurve->ToString();
            }
            break;

            /*case TypeIDCore::ResourceTypeID:
            {
                strValue = reinterpret_cast<ResTypeID const *>(pValue)->ToString();
            }
            break;

            case TypeIDCore::ResourcePtr:
            case TypeIDCore::TResourcePtr:
            {
                strValue = reinterpret_cast<ResPtr const *>(pValue)->GetResourceID().ToString();
            }
            break;

            case TypeIDCore::ResourceID:
            {
                strValue = reinterpret_cast<ResID const *>(pValue)->ToString();
            }
            break;*/

            /*
            case TypeIDCore::BitFlags:
            {
                strValue = StringUtils::ToString(reinterpret_cast<BitFlags const *>(pValue)->Get());
            }
            break;

            case TypeIDCore::TBitFlags:
            {
                TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(templateArgumentTypeID);
                if (pEnumInfo == nullptr)
                {
                    LOG_WARNING("TypeSystem", "Core Type Conversions Unknown enum class ({0}) for TBitFlags", templateArgumentTypeID.ToStringID());
                    return false;
                }

                if (!ConvertBitFlagsToString(*pEnumInfo, *reinterpret_cast<BitFlags const *>(pValue), strValue))
                {
                    LOG_WARNING("TypeSystem", "Core Type Conversions Failed to convert string ({0}) into valid TBitFlags<%s>", strValue.Get(), templateArgumentTypeID.ToStringID());
                    return false;
                }
            }
            break;
            */

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }

        return true;
    }

    bool ConvertNativeTypeToBinary(TypeID typeID, TypeID templateArgumentTypeID, void const *pValue, List<uint8> &byteArray)
    {
//        Serialization::BinaryOutputArchive archive;
		MemoryWriteStream stream;

        // Enums
        if (!IsCoreType(typeID))
        {
            TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(typeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);

            // We only support up to 32 bit enum types...
            switch (pEnumInfo->underlyingType)
            {
            case TypeIDCore::Uint8:
            {
				stream.Write(*reinterpret_cast<uint8 const *>(pValue));
            }
            break;

            case TypeIDCore::Int8:
            {
				stream.Write(*reinterpret_cast<int8 const *>(pValue));
            }
            break;

            case TypeIDCore::Uint16:
            {
				stream.Write(*reinterpret_cast<uint16 const *>(pValue));
            }
            break;

            case TypeIDCore::Int16:
            {
				stream.Write(*reinterpret_cast<int16 const *>(pValue));
            }
            break;

            case TypeIDCore::Uint32:
            {
				stream.Write(*reinterpret_cast<uint32 const *>(pValue));
            }
            break;

            case TypeIDCore::Int32:
            {
				stream.Write(*reinterpret_cast<int32 const *>(pValue));
            }
            break;

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }
        else // Real core types
        {
            TypeIDCore const typeToConvert = GetCoreType(typeID);
            switch (typeToConvert)
            {
            case TypeIDCore::Bool:
            {
				stream.Write(*reinterpret_cast<bool const *>(pValue));
            }
            break;

            case TypeIDCore::Uint8:
            {
				stream.Write(*reinterpret_cast<uint8_t const *>(pValue));
            }
            break;

            case TypeIDCore::Int8:
            {
				stream.Write(*reinterpret_cast<int8_t const *>(pValue));
            }
            break;

            case TypeIDCore::Uint16:
            {
				stream.Write(*reinterpret_cast<uint16_t const *>(pValue));
            }
            break;

            case TypeIDCore::Int16:
            {
				stream.Write(*reinterpret_cast<int16_t const *>(pValue));
            }
            break;

            case TypeIDCore::Uint32:
            {
				stream.Write(*reinterpret_cast<uint32 const *>(pValue));
            }
            break;

            case TypeIDCore::Int32:
            {
				stream.Write(*reinterpret_cast<int32 const *>(pValue));
            }
            break;

            case TypeIDCore::Uint64:
            {
                stream.Write(*reinterpret_cast<uint64_t const *>(pValue));
            }
            break;

            case TypeIDCore::Int64:
            {
                stream.Write(*reinterpret_cast<int64_t const *>(pValue));
            }
            break;

            case TypeIDCore::Float:
            {
                stream.Write(*reinterpret_cast<float const *>(pValue));
            }
            break;

            case TypeIDCore::Double:
            {
                stream.Write(*reinterpret_cast<double const *>(pValue));
            }
            break;

            case TypeIDCore::String:
            {
                stream.Write(*reinterpret_cast<String const *>(pValue));
            }
            break;

            case TypeIDCore::StringID:
            {
				StringID const* value = reinterpret_cast<StringID const *>(pValue);
				stream.WriteUint32(*value);
            }
            break;

            case TypeIDCore::TypeID:
            {
                ENGINE_UNREACHABLE_CODE()
				/*TypeID const* value = reinterpret_cast<TypeID const *>(pValue);
				stream.WriteUint32(*value);*/
            }
            break;

            case TypeIDCore::UUID:
            {
                stream.Write(*reinterpret_cast<UID const *>(pValue));
            }
            break;

            case TypeIDCore::Color:
            {
				Color32 const* value = reinterpret_cast<Color32 const *>(pValue);
				stream.Write(value->value);
            }
            break;

            case TypeIDCore::Float2:
            {
				Float2 const* value = reinterpret_cast<Float2 const *>(pValue);
				stream.Write(value->x);
				stream.Write(value->y);
            }
            break;

            case TypeIDCore::Float3:
            {
				Float3 const* value = reinterpret_cast<Float3 const *>(pValue);
				stream.Write(value->x);
				stream.Write(value->y);
				stream.Write(value->z);
            }
            break;

            case TypeIDCore::Float4:
            {
				Float4 const* value = reinterpret_cast<Float4 const *>(pValue);
                stream.Write(value->x);
				stream.Write(value->y);
				stream.Write(value->z);
				stream.Write(value->w);
            }
            break;

            case TypeIDCore::Quaternion:
            {
                stream.Write(*reinterpret_cast<Quaternion const *>(pValue));
            }
            break;

            case TypeIDCore::Matrix:
            {
				Matrix const* value = reinterpret_cast<Matrix const *>(pValue);
				for (int i = 0; i < 4; i++)
				{
					for (int j = 0; j < 4; ++j)
					{
						stream.Write(value->Values[i][j]);
					}
				}
            }
            break;

            case TypeIDCore::Microseconds:
            {
				Microseconds const* value = reinterpret_cast<Microseconds const *>(pValue);
				stream.Write(value->ToFloat());
            }
            break;

            case TypeIDCore::Milliseconds:
            {
				Milliseconds const* value = reinterpret_cast<Milliseconds const *>(pValue);
				stream.Write(value->ToFloat());
            }
            break;

            case TypeIDCore::Seconds:
            {
				Seconds const* value = reinterpret_cast<Seconds const *>(pValue);
				stream.Write(value->ToFloat());
            }
            break;

            // case TypeIDCore::Percentage:
            // {
            //     stream.Write(*reinterpret_cast<Percentage const *>(pValue));
            // }
            // break;

/*            case TypeIDCore::ResPath:
            {
				ResPath const* value = reinterpret_cast<ResPath const *>(pValue);
				stream.Write(StringView(value->c_str()));
            }
            break;

             case TypeIDCore::IntRange:
             {
                 stream.Write(*reinterpret_cast<IntRange const *>(pValue));
             }
             break;*/

            case TypeIDCore::FloatRange:
            {
				FloatRange const* value = reinterpret_cast<FloatRange const *>(pValue);
				stream.Write(value->begin);
				stream.Write(value->end);
            }
            break;

            case TypeIDCore::FloatCurve:
            {
				FloatCurve const* value = reinterpret_cast<FloatCurve const *>(pValue);
				stream.Write(value->ToString());
            }
            break;

/*            case TypeIDCore::ResourceTypeID:
            {
				ResTypeID const* value = reinterpret_cast<ResTypeID const *>(pValue);
				stream.Write(value->ToString());
            }
            break;

            case TypeIDCore::ResourcePtr:
            case TypeIDCore::TResourcePtr:
            {
				ResPtr const* value = reinterpret_cast<ResPtr const *>(pValue);
				stream.Write(value->GetResourcePath().GetString());
            }
            break;

            case TypeIDCore::ResourceID:
            {
				ResID const* value = reinterpret_cast<ResID const *>(pValue);
				stream.Write(value->GetResourcePath().GetString());
            }
            break;*/

            /*case TypeIDCore::BitFlags:
            case TypeIDCore::TBitFlags:
            {
				BitFlags const* value = reinterpret_cast<BitFlags const *>(pValue);
				stream.Write(value->Get());
            }
            break;*/

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }

		byteArray.Add(stream.GetHandle(), stream.GetLength());
		stream.Close();
        return true;
    }

    bool ConvertBinaryToNativeType(TypeID typeID, TypeID templateArgumentTypeID, List<uint8> const &byteArray, void *pValue)
    {
//        Serialization::BinaryInputArchive archive;
//        archive.ReadFromBlob(byteArray);
		MemoryReadStream stream = MemoryReadStream(byteArray.Get(), byteArray.Count());

        // Enums
        if (!IsCoreType(typeID))
        {
            TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(typeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);

            // We only support up to 32 bit enum types...
            switch (pEnumInfo->underlyingType)
            {
            case TypeIDCore::Uint8:
            {
				stream.Read(*reinterpret_cast<uint8 *>(pValue));
            }
            break;

            case TypeIDCore::Int8:
            {
                stream.Read(*reinterpret_cast<int8_t *>(pValue));
            }
            break;

            case TypeIDCore::Uint16:
            {
                stream.Read(*reinterpret_cast<uint16_t *>(pValue));
            }
            break;

            case TypeIDCore::Int16:
            {
                stream.Read(*reinterpret_cast<int16_t *>(pValue));
            }
            break;

            case TypeIDCore::Uint32:
            {
                stream.Read(*reinterpret_cast<uint32 *>(pValue));
            }
            break;

            case TypeIDCore::Int32:
            {
                stream.Read(*reinterpret_cast<int32 *>(pValue));
            }
            break;

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }
        else // Real core types
        {
            TypeIDCore const typeToConvert = GetCoreType(typeID);
            switch (typeToConvert)
            {
            case TypeIDCore::Bool:
            {
                stream.Read(*reinterpret_cast<bool *>(pValue));
            }
            break;

            case TypeIDCore::Uint8:
            {
                stream.Read(*reinterpret_cast<uint8_t *>(pValue));
            }
            break;

            case TypeIDCore::Int8:
            {
                stream.Read(*reinterpret_cast<int8_t *>(pValue));
            }
            break;

            case TypeIDCore::Uint16:
            {
                stream.Read(*reinterpret_cast<uint16_t *>(pValue));
            }
            break;

            case TypeIDCore::Int16:
            {
                stream.Read(*reinterpret_cast<int16_t *>(pValue));
            }
            break;

            case TypeIDCore::Uint32:
            {
                stream.Read(*reinterpret_cast<uint32 *>(pValue));
            }
            break;

            case TypeIDCore::Int32:
            {
                stream.Read(*reinterpret_cast<int32 *>(pValue));
            }
            break;

            case TypeIDCore::Uint64:
            {
                stream.Read(*reinterpret_cast<uint64_t *>(pValue));
            }
            break;

            case TypeIDCore::Int64:
            {
                stream.Read(*reinterpret_cast<int64_t *>(pValue));
            }
            break;

            case TypeIDCore::Float:
            {
                stream.Read(*reinterpret_cast<float *>(pValue));
            }
            break;

            case TypeIDCore::Double:
            {
                stream.Read(*reinterpret_cast<double *>(pValue));
            }
            break;

            case TypeIDCore::String:
            {
                stream.Read(*reinterpret_cast<String *>(pValue));
            }
            break;

            case TypeIDCore::StringID:
            {
				uint32 id;
                stream.Read(id);
				*reinterpret_cast<StringID *>(pValue) = StringID(id);
            }
            break;

            case TypeIDCore::TypeID:
            {
                ENGINE_UNREACHABLE_CODE()
				/*uint32 ID;
                stream.Read(ID);
                *reinterpret_cast<TypeID *>(pValue) = TypeID(ID);*/
            }
            break;

            case TypeIDCore::UUID:
            {
                stream.Read(*reinterpret_cast<UID *>(pValue));
            }
            break;

            case TypeIDCore::Color:
            {
				uint32 color32;
				stream.Read(color32);
				*reinterpret_cast<Color32 *>(pValue) = Color32(color32);
            }
            break;

            case TypeIDCore::Float2:
            {
                stream.Read((*reinterpret_cast<Float2 *>(pValue)).x);
				stream.Read((*reinterpret_cast<Float2 *>(pValue)).y);
            }
            break;

            case TypeIDCore::Float3:
            {
				stream.Read((*reinterpret_cast<Float3 *>(pValue)).x);
				stream.Read((*reinterpret_cast<Float3 *>(pValue)).y);
				stream.Read((*reinterpret_cast<Float3 *>(pValue)).z);
            }
            break;

            case TypeIDCore::Float4:
            {
				stream.Read((*reinterpret_cast<Float4 *>(pValue)).x);
				stream.Read((*reinterpret_cast<Float4 *>(pValue)).y);
				stream.Read((*reinterpret_cast<Float4 *>(pValue)).z);
                stream.Read((*reinterpret_cast<Float4 *>(pValue)).w);
            }
            break;

            case TypeIDCore::Quaternion:
            {
                stream.Read(*reinterpret_cast<Quaternion *>(pValue));
            }
            break;

            case TypeIDCore::Matrix:
            {
                stream.Read((*reinterpret_cast<Matrix *>(pValue)).Values);
            }
            break;

            case TypeIDCore::Microseconds:
            {
				float value;
				stream.Read(value);
				*reinterpret_cast<Microseconds *>(pValue) = Microseconds(value);
            }
            break;

            case TypeIDCore::Milliseconds:
            {
				float value;
				stream.Read(value);
				*reinterpret_cast<Milliseconds *>(pValue) = Milliseconds(value);
            }
            break;

            case TypeIDCore::Seconds:
            {
				float value;
				stream.Read(value);
				*reinterpret_cast<Seconds *>(pValue) = Seconds(value);
            }
            break;

            // case TypeIDCore::Percentage:
            // {
            //     stream.Read(*reinterpret_cast<Percentage *>(pValue));
            // }
            // break;

/*            case TypeIDCore::ResPath:
            {
				String value;
				stream.Read(value);
				*reinterpret_cast<ResPath *>(pValue) = ResPath(value);
            }
            break;

             case TypeIDCore::IntRange:
             {
                 stream.Read(*reinterpret_cast<IntRange *>(pValue));
             }
             break;*/

            case TypeIDCore::FloatRange:
            {
                stream.Read((reinterpret_cast<FloatRange *>(pValue))->begin);
				stream.Read((reinterpret_cast<FloatRange *>(pValue))->end);
            }
            break;

            case TypeIDCore::FloatCurve:
            {
				String value;
                stream.Read(value);
				reinterpret_cast<FloatCurve *>(pValue)->FromString(value);
            }
            break;

            /*case TypeIDCore::ResourceTypeID:
            {
				String value;
				stream.Read(value);
				*reinterpret_cast<ResTypeID *>(pValue) = ResTypeID(value);
            }
            break;

            case TypeIDCore::ResourcePtr:
            case TypeIDCore::TResourcePtr:
            {
				String value;
				stream.Read(value);
				*reinterpret_cast<ResPtr *>(pValue) = ResPtr(value);
            }
            break;

            case TypeIDCore::ResourceID:
            {
				String value;
				stream.Read(value);
				*reinterpret_cast<ResID *>(pValue) = ResID(value);
            }
            break;*/

            /*case TypeIDCore::BitFlags:
            case TypeIDCore::TBitFlags:
            {
				uint32 value;
				stream.Read(value);
				reinterpret_cast<BitFlags *>(pValue)->Set(value);
            }
            break;*/

            default:
            {
                ENGINE_UNREACHABLE_CODE();
                return false;
            }
            break;
            }
        }

        return true;
    }

    bool ConvertStringToBinary(TypeID typeID, TypeID templateArgumentTypeID, String const &strValue, List<uint8> &byteArray)
    {
        byteArray.Clear();

        //-------------------------------------------------------------------------

        // Enums
        if (!IsCoreType(typeID))
        {
            TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(typeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);

            // We only support up to 32 bit enum types...
            switch (pEnumInfo->underlyingType)
            {
            case TypeIDCore::Uint8:
            {
                return ConvertToBinary<bool>(typeID, templateArgumentTypeID, strValue, byteArray);
            }

            case TypeIDCore::Int8:
            {
                return ConvertToBinary<int8_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }

            case TypeIDCore::Uint16:
            {
                return ConvertToBinary<uint16_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }
            case TypeIDCore::Int16:
            {
                return ConvertToBinary<int16_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }
            case TypeIDCore::Uint32:
            {
                return ConvertToBinary<uint32>(typeID, templateArgumentTypeID, strValue, byteArray);
            }
            case TypeIDCore::Int32:
            {
                return ConvertToBinary<int32>(typeID, templateArgumentTypeID, strValue, byteArray);
            }
            default:
            {
                ENGINE_UNREACHABLE_CODE();
            }
            break;
            }
        }
        else // Real core types
        {
            TypeIDCore const typeToConvert = GetCoreType(typeID);
            switch (typeToConvert)
            {
            case TypeIDCore::Bool:
            {
                return ConvertToBinary<bool>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Uint8:
            {
                return ConvertToBinary<uint8_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Int8:
            {
                return ConvertToBinary<int8_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Uint16:
            {
                return ConvertToBinary<uint16_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Int16:
            {
                return ConvertToBinary<int16_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Uint32:
            {
                return ConvertToBinary<uint32>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Int32:
            {
                return ConvertToBinary<int32>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Uint64:
            {
                return ConvertToBinary<uint64_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Int64:
            {
                return ConvertToBinary<int64_t>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Float:
            {
                return ConvertToBinary<float>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Double:
            {
                return ConvertToBinary<double>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::String:
            {
                return ConvertToBinary<String>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::StringID:
            {
                return ConvertToBinary<StringID>(typeID, templateArgumentTypeID, strValue, byteArray);
            }

            case TypeIDCore::TypeID:
            {
                return ConvertToBinary<TypeID>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::UUID:
            {
                return ConvertToBinary<UID>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Color:
            {
                return ConvertToBinary<Color32>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Float2:
            {
                return ConvertToBinary<Float2>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Float3:
            {
                return ConvertToBinary<Float3>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Float4:
            {
                return ConvertToBinary<Float4>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Quaternion:
            {
                return ConvertToBinary<Quaternion>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Matrix:
            {
                return ConvertToBinary<Matrix>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Microseconds:
            {
                return ConvertToBinary<Microseconds>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Milliseconds:
            {
                return ConvertToBinary<Milliseconds>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::Seconds:
            {
                return ConvertToBinary<Seconds>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            // case TypeIDCore::Percentage:
            // {
            //     return ConvertToBinary<Percentage>(typeID, templateArgumentTypeID, strValue, byteArray);
            // }
            // break;


//            case TypeIDCore::ResPath:
//            {
//                return ConvertToBinary<ResPath>(typeID, templateArgumentTypeID, strValue, byteArray);
//            }


            // case TypeIDCore::IntRange:
            // {
            //     return ConvertToBinary<IntRange>(typeID, templateArgumentTypeID, strValue, byteArray);
            // }
            // break;

            case TypeIDCore::FloatRange:
            {
                return ConvertToBinary<FloatRange>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::FloatCurve:
            {
                return ConvertToBinary<FloatCurve>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


/*            case TypeIDCore::ResourceTypeID:
            {
                return ConvertToBinary<ResTypeID>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::ResourcePtr:
            case TypeIDCore::TResourcePtr:
            {
                return ConvertToBinary<ResPtr>(typeID, templateArgumentTypeID, strValue, byteArray);
            }


            case TypeIDCore::ResourceID:
            {
                return ConvertToBinary<ResID>(typeID, templateArgumentTypeID, strValue, byteArray);
            }*/


            /*
            case TypeIDCore::BitFlags:
            case TypeIDCore::TBitFlags:
            {
                return ConvertToBinary<BitFlags>(typeID, templateArgumentTypeID, strValue, byteArray);
            }
            */


            default:
            {
                ENGINE_UNREACHABLE_CODE();
            }

            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    bool IsValidStringValueForType(TypeID typeID, TypeID templateArgumentTypeID, String const &strValue)
    {
        // Special cases: Enums and TBitFlags
        /*if (typeID == TypeIDCore::TBitFlags)
        {
            TypeEnumInfo const *pEnumInfo = Types::GetEnumInfo(templateArgumentTypeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);
            BitFlags flags;
            return ConvertStringToBitFlags(*pEnumInfo, strValue, flags);
        }
        else */if (!IsCoreType(typeID)) // Enums has unique typeIDs
        {
            auto const pEnumInfo = Types::GetEnumInfo(typeID);
            ENGINE_ASSERT(pEnumInfo != nullptr);
            return pEnumInfo->IsValidValue(StringID(strValue));
        }
        else // Real core types
        {
            // TODO
            return true;
        }
    }
}