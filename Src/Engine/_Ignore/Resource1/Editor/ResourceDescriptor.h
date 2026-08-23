#pragma once

#include "Editor/API.h"
#include "Core/TypeSystem/IReflectedType.h"
#include "Core/TypeSystem/Types.h"
#include "Runtime/Resource/ResourceID.h"
#include "Core/Serialization/JsonWriters.hpp"
#include "Core/Serialization/Serialization.h"
#include "Core/Platform/File.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
	/**
	 * 资源描述符
	 */
    struct SE_API_EDITOR ResourceDescriptor : public IReflectedType
    {
        SE_CLASS(ResourceDescriptor, IReflectedType)

    public:
        // Try to read a descriptor from a file without knowing the type
        static inline ResourceDescriptor* TryReadFromFile(String const& descriptorPath )
        {
			StringAnsi context;
			if (!File::ReadAllText(descriptorPath, context))
			{
				LOG_ERROR( "Resource", "Resource Descriptor Failed to read resource descriptor file: {0}", descriptorPath);
				return nullptr;
			}

			Json::Document stream;
			stream.Parse(context.Get(), context.Length());

			if (stream.HasParseError())
			{
				LOG_ERROR( "Resource", "Resource Descriptor file failed to parse : {0}", descriptorPath);
				return nullptr;
			}

			String typeName;
			DESERIALIZE_MEMBER(TypeID, typeName);
			TypeID typeId(typeName);
			auto typeinfo = Types::GetTypeInfo(typeId);
			ResourceDescriptor* instance = Cast<ResourceDescriptor>(typeinfo->CreateType());

			instance->Deserialize(stream);

			return instance;
        }

        // Try to read a specific descriptor from a file
        template<typename T>
        static bool TryReadFromFile(String const& descriptorPath, T& outData )
        {
            static_assert(std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );

			StringAnsi context;
			if (!File::ReadAllText(descriptorPath, context))
			{
				LOG_ERROR( "Resource", "Resource Descriptor Failed to read resource descriptor file: {0}", descriptorPath);
				return false;
			}

			Json::Document stream;
			stream.Parse(context.Get(), context.Length());

			if (stream.HasParseError())
			{
				LOG_ERROR( "Resource", "Resource Descriptor file failed to parse : {0}", descriptorPath);
				return false;
			}

			String typeName;
			DESERIALIZE_MEMBER(TypeID, typeName);
			TypeID typeId(typeName);
			if (outData.GetTypeID() != typeId)
			{
				LOG_ERROR( "Resource", "Resource Descriptor file type mismatch : {0}", descriptorPath);
				return false;
			}

			outData.Deserialize(stream);

            return true;
        }

        // Write a descriptor to a file
        template<typename T>
        static bool TryWriteToFile(String const& descriptorPath, T * pDescriptorData )
        {
			static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );
			ENGINE_ASSERT( !descriptorPath.IsEmpty());

			Json::StringBuffer context;
			CompactJsonWriter typeWriter(context);

			typeWriter.StartObject();
			typeWriter.JKEY("TypeID");
			StringView typeID = pDescriptorData->GetTypeID().ToString();
//			typeWriter.String(typeID);
			pDescriptorData->Serialize(typeWriter, nullptr);
			typeWriter.EndObject();
            return File::WriteAllText(descriptorPath, String(context.GetString(), context.GetLength()), Encoding::EncodingType::UTF8);
        }

    public:

        ResourceDescriptor() = default;
        ResourceDescriptor( ResourceDescriptor const& ) = default;
        virtual ~ResourceDescriptor() = default;

        ResourceDescriptor& operator=( ResourceDescriptor const& rhs ) = default;

        virtual void Clear() = 0;

        // Get all the resources that are required for the compilation of the resource
        virtual void GetCompileDependencies( List<ResPath>& outDependencies ) = 0;

        // Is this a valid descriptor - This only signifies whether all the required data is set and not whether the resource or any other authored data within the descriptor is valid
        virtual bool IsValid() const = 0;

        // Can this descriptor be created by a user in the editor?
        virtual bool IsUserCreateableDescriptor() const { return false; }

        // What is the compiled resource type for this descriptor - Only needed for user createable descriptors
        virtual TypeID GetCompiledResourceTypeID() const = 0;

		virtual void Serialize(SerializeStream& stream, const void* otherObj) override
		{

		}

		virtual void Deserialize(DeserializeStream& stream) override
		{

		}

		SE_PROPERTY()
		ResPath      resPath;
		SE_PROPERTY()
		ResID		 resID;
	};
}