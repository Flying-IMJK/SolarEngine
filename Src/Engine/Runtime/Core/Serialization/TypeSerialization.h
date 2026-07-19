#pragma once

#include "Runtime/Core/TypeSystem/TypeDescriptors.h"
#include "Core/TypeSystem/IReflectedType.h"
#include "Core/FileSystem/FileSystemPath.h"
#include "JsonWriters.hpp"

//-------------------------------------------------------------------------

namespace SE
{
    class Types;
    class TypeInstanceModel;
}

//-------------------------------------------------------------------------
// Type Serialization
//-------------------------------------------------------------------------
// This file contain basic facilities to convert between JSON and the various type representations we have

#ifdef SE_DEVELOPMENT
namespace SE::Serialization
{
    constexpr static char const* const s_typeIDKey = "TypeID";
	constexpr static int const s_typeIDKeyLength = 5;

    // Type Descriptors
    //-------------------------------------------------------------------------

    SE_API_RUNTIME bool ReadTypeDescriptorFromJSON(Serialization::JsonValue const& typeObjectValue, TypeDescriptor& outDesc );
    SE_API_RUNTIME void WriteTypeDescriptorToJSON(Serialization::JsonWriter& writer, TypeDescriptor const& type );

    // Type Instances
    //-------------------------------------------------------------------------

    // Read the data for a native type from JSON - expect a fully created type to be supplied and will override the values
    SE_API_RUNTIME bool ReadNativeType(Serialization::JsonValue const& typeObjectValue, IReflectedType* pTypeInstance );

    // Read the data for a native type from JSON - expect a fully created type to be supplied and will override the values
    SE_API_RUNTIME bool ReadNativeTypeFromString(String const& jsonString, IReflectedType* pTypeInstance );

    // Serialize a supplied native type to JSON - creates a new JSON object for this type
    SE_API_RUNTIME void WriteNativeType(IReflectedType const* pTypeInstance, Serialization::JsonWriter& writer );

    // Writes out the type ID and property data for a supplied native type to an existing JSON object - Note: This function does not create a new json object!
    SE_API_RUNTIME void WriteNativeTypeContents(IReflectedType const* pTypeInstance, Serialization::JsonWriter& writer );

    // Write the property data for a supplied native type to JSON
    SE_API_RUNTIME void WriteNativeTypeToString(IReflectedType const* pTypeInstance, String& outString );

    // Create a new instance of a type from a supplied JSON version
    SE_API_RUNTIME IReflectedType* TryCreateAndReadNativeType(Serialization::JsonValue const& typeObjectValue );

    // Create a new instance of a type from a supplied JSON version
    template<typename T>
    T* TryCreateAndReadNativeType(Serialization::JsonValue const& typeObjectValue )
    {
        IReflectedType* pCreatedType = TryCreateAndReadNativeType(typeObjectValue );
        if ( pCreatedType != nullptr )
        {
            if ( IsOfType<T>( pCreatedType ) )
            {
                return reinterpret_cast<T*>( pCreatedType );
            }
            else
            {
                Delete( pCreatedType );
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------
    // Native Type Serialization : Reading
    //-------------------------------------------------------------------------
    // Supports multiple compound types in a single archive
    // An archive is either a single serialized type or an array of serialized types
    // Each type is serialized as a JSON object with a 'TypeID' property containing the type ID of the serialized type

    class SE_API_RUNTIME TypeArchiveReader : public JsonArchiveReader
    {
    public:
        // Get number of types serialized in the read json file
        inline int32_t GetNumSerializedTypes() const { return m_numSerializedTypes; }

        // Descriptor
        //-------------------------------------------------------------------------

        inline bool ReadType( TypeDescriptor& typeDesc )
        {
            return ReadTypeDescriptorFromJSON(GetObjectValueToBeDeserialized(), typeDesc );
        }

        inline TypeArchiveReader const& operator>>( TypeDescriptor& typeDesc )
        {
            bool const result = ReadType( typeDesc );
            ENGINE_ASSERT( result );
            return *this;
        }

        // Native
        //-------------------------------------------------------------------------
        // Do not try to serialize core-types using this reader

        inline bool ReadType( IReflectedType* pType )
        {
            return ReadNativeType(GetObjectValueToBeDeserialized(), pType );
        }

        inline IReflectedType* TryReadType()
        {
            return TryCreateAndReadNativeType(GetObjectValueToBeDeserialized() );
        }

        inline TypeArchiveReader const& operator>>( IReflectedType* pType )
        {
            bool const result = ReadType( pType );
            ENGINE_ASSERT( result );
            return *this;
        }

        inline TypeArchiveReader const& operator>>( IReflectedType& type )
        {
            bool const result = ReadType( &type );
            ENGINE_ASSERT( result );
            return *this;
        }

    private:

        virtual void Reset() override final;
        virtual void OnFileReadSuccess() override final;

        Serialization::JsonValue const& GetObjectValueToBeDeserialized();

    private:
        int32_t                                                     m_numSerializedTypes = 0;
        int32_t                                                     m_deserializedTypeIdx = 0;
    };

    //-------------------------------------------------------------------------
    // // Native Type Serialization : Writing
    //-------------------------------------------------------------------------
    // Supports multiple compound types in a single archive
    // An archive is either a single serialized type or an array of serialized types
    // Each type is serialized as a JSON object with a 'TypeID' property containing the type ID of the serialized type

    class SE_API_RUNTIME TypeArchiveWriter final : public JsonArchiveWriter
    {
    public:

        // Reset all serialized data without writing to disk
        void Reset() override;

        // Native
        //-------------------------------------------------------------------------
        // Do not try to serialize core-types using this writer

        template<typename T>
        inline TypeArchiveWriter& operator<<( T const* pType )
        {
            PreSerializeType();
            WriteNativeType(pType, m_writer );
            m_numTypesSerialized++;
            return *this;
        }

        // Descriptor
        //-------------------------------------------------------------------------

        inline TypeArchiveWriter& operator<< ( TypeDescriptor const& typeDesc )
        {
            PreSerializeType();
            WriteTypeDescriptorToJSON(m_writer, typeDesc );
            m_numTypesSerialized++;
            return *this;
        }

    private:

        using JsonArchiveWriter::GetWriter;

        void PreSerializeType();
        virtual void FinalizeSerializedData() override final;

    private:
        int32_t                                                     m_numTypesSerialized = 0;
    };
}
#endif