#pragma once

#include "JsonFwd.h"
#include "Core/API.h"

namespace SE
{
	class ISerializeModifier;
	class JsonWriter;

	typedef Json::Document SerializeDocument;
	/// <summary>
	/// Serialization output stream
	/// </summary>
	typedef Json::Value DeserializeStream;
	/// <summary>
	/// Serialization input stream
	/// </summary>
	typedef JsonWriter SerializeStream;


	struct SE_API_CORE SerializeContext
	{
		class SE_API_CORE ObjectScope
		{
		private:
			SerializeContext &context;
			void* lastOtherObject;

		public:
			ObjectScope(SerializeContext &context, void* otherObj) : context(context), lastOtherObject(context.otherObj)
			{
				context.otherObj = otherObj;
			}

			ObjectScope(SerializeContext &context, const void* otherObj) : context(context), lastOtherObject(context.otherObj)
			{
				context.otherObj = const_cast<void*>(otherObj);
			}

			~ObjectScope()
			{
				context.otherObj = lastOtherObject;
			}
		};

		SerializeStream& stream;
		// The instance of the object to compare with and serialize only the modified properties. If null, then serialize all properties.
		void* otherObj;

		SerializeContext(SerializeStream& stream): stream(stream), otherObj(nullptr) {}

		SerializeContext(SerializeStream& stream, void* object): stream(stream), otherObj(object) {}

		SerializeContext(SerializeStream& stream, const void* object): stream(stream), otherObj(const_cast<void*>(object)) {}
	};

	struct SE_API_CORE DeserializeContext
	{
		class SE_API_CORE StreamScope
		{
		private:
			DeserializeContext* context;
			DeserializeStream* lastStream;

		public:
			StreamScope(DeserializeContext &context, DeserializeStream& stream);

			StreamScope(DeserializeContext &context, const DeserializeStream& stream);

			~StreamScope();
		};

		DeserializeStream* stream;
		ISerializeModifier* modifier;

		DeserializeContext(DeserializeStream& stream, ISerializeModifier* modifier): stream(&stream), modifier(modifier) {}
	};

	/// <summary>
	/// Interface for objects that can be serialized/deserialized to/from JSON format.
	/// </summary>
	class SE_API_CORE ISerializable
	{
	public:

		/// <summary>
		/// Finalizes an instance of the <see cref="ISerializable"/> class.
		/// </summary>
		virtual ~ISerializable() = default;

		/// <summary>
		/// 将 object 序列化到输出流中。
		/// </summary>
		/// <param name="context">context</param>
		virtual void Serialize(SerializeContext &context) = 0;

		/// <summary>
		/// 从 input 流中反序列化 object。
		/// </summary>
		/// <param name="context">context</param>
		virtual void Deserialize(DeserializeContext &context) = 0;

		/// <summary>
		/// Deserializes object from the input stream child member. Won't deserialize it if member is missing.
		/// </summary>
		/// <param name="context">context</param>
		/// <param name="memberName">The input stream member to lookup.</param>
		void DeserializeIfExists(DeserializeContext &context, const char* memberName);
	};

}
