#include "TypeSerialization.h"
#include "Core/TypeSystem/Types.h"
#include "Core/TypeSystem/TypeCompositeInfo.h"

//-------------------------------------------------------------------------

using namespace SE;

//-------------------------------------------------------------------------
// Descriptors
//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SE::Serialization
{
    // Type descriptor reader needs to support both nested and unnested formats as it needs to read the outputs from both descriptor serialization and type model serialization
    struct TypeDescriptorReader
    {
        static bool ReadArrayDescriptor(TypeCompositeInfo const *pRootTypeInfo, TypeProperty const *pArrayPropertyInfo,
			Serialization::JsonValue const &arrayValue, List<PropertyDescriptor> &outPropertyValues, String const &propertyPathPrefix)
        {
            ENGINE_ASSERT(pArrayPropertyInfo != nullptr && arrayValue.IsArray());

            int32 const numElements = (int32)arrayValue.Size();
            for (int32 i = 0; i < numElements; i++)
            {
                if (arrayValue[i].IsArray())
                {
                    // We dont support arrays of arrays
                    LOG_ERROR("TypeSystem", "Serialization", "We dont support arrays of arrays");
                    return false;
                }
                else if (arrayValue[i].IsObject())
                {
                    if (CoreTypeRegistry::IsCoreType(pArrayPropertyInfo->m_typeID))
                    {
                        LOG_ERROR("TypeSystem", "Serialization", "Malformed json detected, object declared for core type property: {0}", pArrayPropertyInfo->m_ID.c_str());
                        return false;
                    }

                    auto pArrayPropertyTypeInfo = Types::GetTypeInfo(pArrayPropertyInfo->m_typeID);
                    String const newPrefix = String::Format("{0}{1}/", propertyPathPrefix.Get(), i);
                    if (!ReadTypeDescriptor(pRootTypeInfo, pArrayPropertyTypeInfo, arrayValue[i], outPropertyValues, newPrefix))
                    {
                        return false;
                    }
                }
                else // Add regular property value
                {
                    if (!CoreTypeRegistry::IsCoreType(pArrayPropertyInfo->m_typeID))
                    {
                        LOG_ERROR("TypeSystem", "Serialization", "Malformed json detected, only core type properties are allowed to be directly declared: {0}", pArrayPropertyInfo->m_ID.c_str());
                        return false;
                    }

                    if (!arrayValue[i].IsString())
                    {
                        LOG_ERROR("TypeSystem", "Serialization", "Malformed json detected, Core type values must be strings, property ({0}) has invalid value: {1}", pArrayPropertyInfo->m_ID.c_str(), arrayValue[i].GetString());
                        return false;
                    }

                    auto const propertyPath = PropertyPath(String::Format("{0}{1}", propertyPathPrefix.Get(), i));
                    auto pPropertyInfo = Types::ResolvePropertyPath(pRootTypeInfo, propertyPath);
                    if (pPropertyInfo != nullptr)
                    {
                        outPropertyValues.Add(PropertyDescriptor(propertyPath, *pPropertyInfo, arrayValue[i].GetString()));
                    }
                }
            }

            return true;
        }

        static bool ReadTypeDescriptor(TypeCompositeInfo const *pRootTypeInfo, TypeCompositeInfo const *pTypeInfo, Serialization::JsonValue const &typeObjectValue,
			List<PropertyDescriptor> &outPropertyValues, String const &propertyPathPrefix = String())
        {
            // Read properties
            //-------------------------------------------------------------------------

            for (auto itr = typeObjectValue.MemberBegin(); itr != typeObjectValue.MemberEnd(); ++itr)
            {
                StringID const propertyID(itr->name.GetString());
                TypeProperty const *pPropertyInfo = pTypeInfo->GetPropertyInfo(propertyID);
                if (pPropertyInfo == nullptr)
                {
                    continue;
                }

                if (itr->value.IsArray())
                {
                    String const newPrefix = String::Format("{0}{1}/", propertyPathPrefix.Get(), itr->name.GetString());
                    ReadArrayDescriptor(pRootTypeInfo, pPropertyInfo, itr->value, outPropertyValues, newPrefix);
                }
                else if (itr->value.IsObject())
                {
                    if (CoreTypeRegistry::IsCoreType(pPropertyInfo->m_typeID))
                    {
                        LOG_ERROR("TypeSystem", "Serialization", "Malformed json detected, object declared for core type property: {0}", itr->name.GetString());
                        return false;
                    }

                    auto pPropertyTypeInfo = Types::GetTypeInfo(pPropertyInfo->m_typeID);
                    String const newPrefix = String::Format("{0}{1}/", propertyPathPrefix.Get(), itr->name.GetString());
                    if (!ReadTypeDescriptor(pRootTypeInfo, pPropertyTypeInfo, itr->value, outPropertyValues, newPrefix))
                    {
                        return false;
                    }
                }
                else // Regular core type property
                {
                    if (!CoreTypeRegistry::IsCoreType(pPropertyInfo->m_typeID) && !pPropertyInfo->IsEnumProperty())
                    {
                        LOG_ERROR("TypeSystem", "Serialization", "Malformed json detected, only core type properties are allowed to be directly declared: {0}", itr->name.GetString());
                        return false;
                    }

                    if (!itr->value.IsString())
                    {
                        LOG_ERROR("TypeSystem", "Serialization", "Malformed json detected, Core type values must be strings, invalid value detected: {0}", itr->name.GetString());
                        return false;
                    }

                    PropertyPath const propertyPath(String::Format("{0}{1}", propertyPathPrefix.Get(), itr->name.GetString()));

                    auto pResolvedPropertyInfo = Types::ResolvePropertyPath(pRootTypeInfo, propertyPath);
                    if (pResolvedPropertyInfo != nullptr)
                    {
                        outPropertyValues.Add(PropertyDescriptor(propertyPath, *pResolvedPropertyInfo, itr->value.GetString()));
                    }
                }
            }

            return true;
        }
    };

    bool ReadTypeDescriptorFromJSON(Serialization::JsonValue const &typeObjectValue, TypeDescriptor &outDesc)
    {
        if (!typeObjectValue.IsObject())
        {
            LOG_ERROR("TypeSystem", "Serialization", "Supplied json value is not an object");
            return false;
        }

        // Get type info
        //-------------------------------------------------------------------------

        auto const typeIDIter = typeObjectValue.FindMember(s_typeIDKey);
        if (typeIDIter == typeObjectValue.MemberEnd())
        {
            LOG_ERROR("TypeSystem", "Serialization", "Missing typeID for object");
            return false;
        }

        TypeID const typeID(typeIDIter->value.GetString());
        auto const pTypeInfo = Types::GetTypeInfo(typeID);
        if (pTypeInfo == nullptr)
        {
            LOG_ERROR("TypeSystem", "Serialization", "Unknown type encountered: %s", typeID.c_str());
            return false;
        }

        // Read descriptor
        //-------------------------------------------------------------------------

        outDesc.typeID = TypeID(typeIDIter->value.GetString());
        return TypeDescriptorReader::ReadTypeDescriptor(pTypeInfo, pTypeInfo, typeObjectValue, outDesc.properties);
    }

    //-------------------------------------------------------------------------

    // Type descriptor serialization will collapse all properties into a single list per type, using the property paths as property names
    struct TypeDescriptorWriter
    {
        static void WriteProperty(Serialization::JsonWriter &writer, PropertyDescriptor const &propertyDesc)
        {
            writer.Key(propertyDesc.path.ToString().Get());
            writer.Key(propertyDesc.stringValue.Get());
        }

        static void WriteStructure(Serialization::JsonWriter &writer, TypeDescriptor const &typeDesc)
        {
            writer.StartObject();

            // Every type has to have a type ID
            writer.Key(s_typeIDKey);
            writer.String(typeDesc.typeID.c_str());

            // Write all property values
            for (auto const &propertyValue : typeDesc.properties)
            {
                WriteProperty(writer, propertyValue);
            }

            writer.EndObject();
        }
    };

    void WriteTypeDescriptorToJSON(Serialization::JsonWriter &writer, TypeDescriptor const &typeDesc)
    {
        ENGINE_ASSERT(typeDesc.IsValid());
        TypeDescriptorWriter::WriteStructure(writer, typeDesc);
    }
}

//-------------------------------------------------------------------------
// Native
//-------------------------------------------------------------------------

namespace SE::Serialization
{
    struct NativeTypeReader
    {
        template <typename T>
        static void SetPropertyValue(void *pAddress, T value)
        {
            *((T *)pAddress) = value;
        }

        static bool ReadCoreType(TypeProperty const &propInfo, Serialization::JsonValue const &typeValue, void *pPropertyDataAddress)
        {
            ENGINE_ASSERT(pPropertyDataAddress != nullptr);

            if (typeValue.IsString())
            {
                String const valueString = String(typeValue.GetString());
                ConvertStringToNativeType(propInfo, valueString, pPropertyDataAddress);
            }
            else if (typeValue.IsBool())
            {
                ENGINE_ASSERT(propInfo.m_typeID == TypeIDCore::Bool);
                SetPropertyValue(pPropertyDataAddress, typeValue.GetBool());
            }
            else if (typeValue.IsInt64() || typeValue.IsUint64())
            {
                if (propInfo.m_typeID == TypeIDCore::Uint8)
                {
                    SetPropertyValue(pPropertyDataAddress, (uint8)typeValue.GetUint64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Int8)
                {
                    SetPropertyValue(pPropertyDataAddress, (int8)typeValue.GetInt64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Uint16)
                {
                    SetPropertyValue(pPropertyDataAddress, (uint16)typeValue.GetUint64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Int16)
                {
                    SetPropertyValue(pPropertyDataAddress, (int16)typeValue.GetInt64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Uint32)
                {
                    SetPropertyValue(pPropertyDataAddress, (uint32)typeValue.GetUint64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Int32)
                {
                    SetPropertyValue(pPropertyDataAddress, (int32)typeValue.GetInt64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Uint64)
                {
                    SetPropertyValue(pPropertyDataAddress, typeValue.GetUint64());
                }
                else if (propInfo.m_typeID == TypeIDCore::Int64)
                {
                    SetPropertyValue(pPropertyDataAddress, typeValue.GetInt64());
                }
                else // Invalid JSON data encountered
                {
                    LOG_ERROR("TypeSystem", "Serialization", "Invalid JSON file encountered");
                    return false;
                }
            }
            else if (typeValue.IsDouble())
            {
                if (propInfo.m_typeID == TypeIDCore::Float)
                {
                    SetPropertyValue(pPropertyDataAddress, typeValue.GetFloat());
                }
                else if (propInfo.m_typeID == TypeIDCore::Double)
                {
                    SetPropertyValue(pPropertyDataAddress, typeValue.GetDouble());
                }
                else // Invalid JSON data encountered
                {
                    LOG_ERROR("TypeSystem", "Serialization", "Invalid JSON file encountered");
                    return false;
                }
            }
            else // Invalid JSON data encountered
            {
                LOG_ERROR("TypeSystem", "Serialization", "Invalid JSON file encountered");
                return false;
            }

            return true;
        }

        //-------------------------------------------------------------------------

        static bool ReadType(Serialization::JsonValue const &currentJsonValue, TypeID typeID, IType *pTypeData)
        {
            ENGINE_ASSERT(!IsCoreType(typeID));

            auto const pTypeInfo = Types::GetTypeInfo(typeID);
            if (pTypeInfo == nullptr)
            {
                LOG_ERROR("TypeSystem", "Serialization", "Unknown type encountered: {0}", typeID.c_str());
                return false;
            }

            ENGINE_ASSERT(currentJsonValue.IsObject());
            auto const typeIDValueIter = currentJsonValue.FindMember(s_typeIDKey);
            ENGINE_ASSERT(typeIDValueIter != currentJsonValue.MemberEnd());
            TypeID const actualTypeID(typeIDValueIter->value.GetString());

            // If you hit this the type in the JSON file and the type you are trying to deserialize do not match
            if (typeID != actualTypeID && !Types::IsTypeDerivedFrom(actualTypeID, typeID))
            {
                LOG_ERROR("TypeSystem", "Serialization", "Type mismatch, expected {0}, encountered %s", typeID.c_str(), actualTypeID.c_str());
                return false;
            }

            //-------------------------------------------------------------------------

            for (auto const &propInfo : pTypeInfo->m_properties)
            {
                // Read Arrays
                auto pPropertyDataAddress = propInfo.GetPropertyAddress(pTypeData);
                if (propInfo.IsArrayProperty())
                {
                    // Try get serialized value
                    const char *pPropertyName = propInfo.m_ID.c_str();
                    auto const propertyNameIter = currentJsonValue.FindMember(pPropertyName);
                    if (propertyNameIter == currentJsonValue.MemberEnd())
                    {
                        continue;
                    }

                    ENGINE_ASSERT(propertyNameIter->value.IsArray());
					auto jsonArrayValue = propertyNameIter->value.GetArray();
                    size_t const numJSONArrayElements = propertyNameIter->value.Size();// jsonArrayValue.Size();

                    // Static array
                    if (propInfo.IsStaticArrayProperty())
                    {
                        if (propInfo.m_arraySize < (int32)numJSONArrayElements)
                        {
                            LOG_ERROR("TypeSystem", "Serialization", "Static array size mismatch for {0}, expected maximum {1} elements, encountered {2} elements", propInfo.m_size, propInfo.m_size, (int32)numJSONArrayElements);
                            return false;
                        }

                        uint8_t *pArrayElementAddress = reinterpret_cast<uint8_t *>(pPropertyDataAddress);
                        for (auto i = 0u; i < numJSONArrayElements; i++)
                        {
                            if (!ReadProperty(jsonArrayValue[i], propInfo, pArrayElementAddress))
                            {
                                return false;
                            }
                            pArrayElementAddress += propInfo.m_arrayElementSize;
                        }
                    }
                    else // Dynamic array
                    {
                        // If we have less elements in the json array than in the current type, clear the array as we will resize the array appropriately as part of reading the values
                        size_t const currentArraySize = pTypeInfo->GetArraySize(pTypeData, propInfo.m_ID.ToUint());
                        if (numJSONArrayElements < currentArraySize)
                        {
                            pTypeInfo->ClearArray(pTypeData, propInfo.m_ID.ToUint());
                        }

                        // Do the traversal backwards to only allocate once
                        for (int32 i = (int32)(numJSONArrayElements - 1); i >= 0; i--)
                        {
                            auto pArrayElementAddress = pTypeInfo->GetArrayElementDataPtr(pTypeData, propInfo.m_ID.ToUint(), i);
                            if (!ReadProperty(jsonArrayValue[i], propInfo, pArrayElementAddress))
                            {
                                return false;
                            }
                        }
                    }
                }
                else // Non-array type
                {
                    // Try get serialized value
                    const char *pPropertyName = propInfo.m_ID.c_str();
                    auto const propertyValueIter = currentJsonValue.FindMember(pPropertyName);
                    if (propertyValueIter == currentJsonValue.MemberEnd())
                    {
                        continue;
                    }

                    if (!ReadProperty(propertyValueIter->value, propInfo, pPropertyDataAddress))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        static bool ReadProperty(Serialization::JsonValue const &currentJsonValue, TypeProperty const &propertyInfo, void *pPropertyInstance)
        {
            if (IsCoreType(propertyInfo.m_typeID) || propertyInfo.IsEnumProperty())
            {
                String const typeName = propertyInfo.m_typeID.c_str();
                return ReadCoreType(propertyInfo, currentJsonValue, pPropertyInstance);
            }
            else // Complex Type
            {
                return ReadType(currentJsonValue, propertyInfo.m_typeID, (IType *)pPropertyInstance);
            }
        }
    };

    bool ReadNativeType(Serialization::JsonValue const &typeObjectValue, IType *pTypeInstance)
    {
        if (!typeObjectValue.IsObject())
        {
            LOG_ERROR("TypeSystem", "Serialization", "Supplied json value is not an object");
            return false;
        }

        if (!typeObjectValue.HasMember(s_typeIDKey))
        {
            LOG_ERROR("TypeSystem", "Serialization", "Missing typeID for object");
            return false;
        }

        return NativeTypeReader::ReadType(typeObjectValue, pTypeInstance->GetTypeID(), pTypeInstance);
    }

    bool ReadNativeTypeFromString(String const &jsonString, IType *pTypeInstance)
    {
        ENGINE_ASSERT(!jsonString.IsEmpty() && pTypeInstance != nullptr);

        JsonArchiveReader reader;
        reader.ReadFromString(jsonString.Get());
        return ReadNativeType(reader.GetDocument(), pTypeInstance);
    }

    //-------------------------------------------------------------------------

    struct NativeTypeWriter
    {
        static void WriteType(PrettyJsonWriter &writer, String &scratchBuffer, TypeID typeID, IType const *pTypeInstance, bool createJsonObject = true)
        {
            ENGINE_ASSERT(!IsCoreType(typeID));
            auto const pTypeInfo = Types::GetTypeInfo(typeID);
            ENGINE_ASSERT(pTypeInfo != nullptr);

            if (createJsonObject)
            {
                writer.StartObject();
            }

            // Every type has to have a type ID
            writer.Key(s_typeIDKey, s_typeIDKeyLength);
			StringAnsiView typeIDString = typeID.ToString();
            writer.String(typeIDString.Get(), typeIDString.Length());

            //-------------------------------------------------------------------------

            for (auto const &propInfo : pTypeInfo->m_properties)
            {
                // Write Key
                const char *pPropertyName = propInfo.m_ID.c_str();
                ENGINE_ASSERT(pPropertyName != nullptr);
                writer.Key(pPropertyName);

                // Write Value
                auto pPropertyDataAddress = propInfo.GetPropertyAddress(pTypeInstance);
                if (propInfo.IsArrayProperty())
                {
                    size_t const elementByteSize = Types::GetTypeByteSize(propInfo.m_typeID);
                    ENGINE_ASSERT(elementByteSize > 0);

                    writer.StartArray();

                    size_t numArrayElements = 0;
                    uint8_t const *pElementAddress = nullptr;

                    // Static array
                    if (propInfo.IsStaticArrayProperty())
                    {
                        numArrayElements = propInfo.m_size / elementByteSize;
                        pElementAddress = propInfo.GetPropertyAddress<uint8_t>(pTypeInstance);
                    }
                    else // Dynamic array
                    {
                        List<uint8> const &dynamicArray = *propInfo.GetPropertyAddress<List<uint8>>(pTypeInstance);
                        size_t const arrayByteSize = dynamicArray.Count();
                        numArrayElements = arrayByteSize / elementByteSize;
                        pElementAddress = dynamicArray.Get();
                    }

                    // Write array elements
                    for (auto i = 0u; i < numArrayElements; i++)
                    {
                        WriteProperty(writer, scratchBuffer, propInfo, pElementAddress);
                        pElementAddress += elementByteSize;
                    }

                    writer.EndArray();
                }
                else
                {
                    WriteProperty(writer, scratchBuffer, propInfo, pPropertyDataAddress);
                }
            }

            if (createJsonObject)
            {
                writer.EndObject();
            }
        }

        static void WriteProperty(Serialization::JsonWriter &writer, String &scratchBuffer, TypeProperty const &propertyInfo, void const *pPropertyInstance)
        {
            if (IsCoreType(propertyInfo.m_typeID) || propertyInfo.IsEnumProperty())
            {
                ConvertNativeTypeToString(propertyInfo, pPropertyInstance, scratchBuffer);
                writer.String(scratchBuffer.Get());
            }
            else
            {
                WriteType(writer, scratchBuffer, propertyInfo.m_typeID, (IType *)pPropertyInstance);
            }
        }
    };

    void WriteNativeType(IType const *pTypeInstance, Serialization::JsonWriter &writer)
    {
        String scratchBuffer;
        scratchBuffer.Resize(255);
        NativeTypeWriter::WriteType(writer, scratchBuffer, pTypeInstance->GetTypeID(), pTypeInstance);
    }

    void WriteNativeTypeContents(IType const *pTypeInstance, Serialization::JsonWriter &writer)
    {
        String scratchBuffer;
        scratchBuffer.Resize(255);
        NativeTypeWriter::WriteType(writer, scratchBuffer, pTypeInstance->GetTypeID(), pTypeInstance, false);
    }

    void WriteNativeTypeToString(IType const *pTypeInstance, String &outString)
    {
        JsonArchiveWriter writer;
        WriteNativeType(pTypeInstance, *writer.GetWriter());
        outString = writer.GetStringBuffer().GetString();
    }

    IType *TryCreateAndReadNativeType(Serialization::JsonValue const &typeObjectValue)
    {
        auto const typeIDIter = typeObjectValue.FindMember(s_typeIDKey);
        if (typeIDIter == typeObjectValue.MemberEnd())
        {
            LOG_ERROR("TypeSystem", "Serialization", "Missing typeID for object");
            return nullptr;
        }

        TypeID const typeID(typeIDIter->value.GetString());
        auto const pTypeInfo = Types::GetTypeInfo(typeID);
        if (pTypeInfo == nullptr)
        {
            LOG_ERROR("TypeSystem", "Serialization", "Unknown type encountered: {0}", typeID.c_str());
            return nullptr;
        }

        IType *pTypeInstance = pTypeInfo->CreateType();

        if (!ReadNativeType(typeObjectValue, pTypeInstance))
        {
            Delete(pTypeInstance);
            return nullptr;
        }

        return pTypeInstance;
    }
}

//-------------------------------------------------------------------------
// Reader / Writer
//-------------------------------------------------------------------------

namespace SE::Serialization
{
    void TypeArchiveReader::OnFileReadSuccess()
    {
        if (m_document.IsArray())
        {
            m_numSerializedTypes = m_document.Size();
        }
        else
        {
            m_numSerializedTypes = m_document.IsObject() ? 1 : 0;
        }
    }

    void TypeArchiveReader::Reset()
    {
        JsonArchiveReader::Reset();
        m_numSerializedTypes = 0;
        m_deserializedTypeIdx = 0;
    }

    Serialization::JsonValue const &TypeArchiveReader::GetObjectValueToBeDeserialized()
    {
        ENGINE_ASSERT(m_deserializedTypeIdx < m_numSerializedTypes);

        if (m_document.IsArray())
        {
            ENGINE_ASSERT(m_document.IsArray());
            return m_document[m_deserializedTypeIdx++];
        }
        else
        {
            return m_document;
        }
    }

    //-------------------------------------------------------------------------

    void TypeArchiveWriter::Reset()
    {
        JsonArchiveWriter::Reset();
        m_numTypesSerialized = 0;
    }

    void TypeArchiveWriter::PreSerializeType()
    {
        if (m_numTypesSerialized == 1)
        {
            String const firstValueSerialized = m_stringBuffer.GetString();

            //-------------------------------------------------------------------------

            m_stringBuffer.Clear();
            m_writer.StartArray();
            m_writer.RawValue(firstValueSerialized.Get(), firstValueSerialized.Length(), rapidjson::Type::kObjectType);
        }
    }

    void TypeArchiveWriter::FinalizeSerializedData()
    {
        if (m_numTypesSerialized > 1)
        {
            m_writer.EndArray();
        }
    }
}
#endif