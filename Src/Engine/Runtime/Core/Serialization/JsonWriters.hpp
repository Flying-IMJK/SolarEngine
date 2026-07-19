#pragma once

#include "Json.h"
#include "JsonWriter.h"

namespace SE
{
	template<typename WriterType>
	class SE_API_RUNTIME JsonWriterBase : public JsonWriter
	{
	protected:

		WriterType writer;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="JsonWriterBase"/> class.
		/// </summary>
		/// <param name="buffer">The output buffer.</param>
		JsonWriterBase(Json::StringBuffer& buffer) : JsonWriter(), writer(buffer)
		{
		}

	public:

		inline WriterType& GetWriter()
		{
			return writer;
		}

	public:

		// [JsonWriter]
		void Key(const char* str, int32 length) override
		{
			writer.Key(str, static_cast<rapidjson::SizeType>(length));
		}

		void String(const char* str, int32 length) override
		{
			writer.String(str, length);
		}

		void RawValue(const char* json, int32 length) override
		{
			writer.RawValue(json, length);
		}

		void Bool(bool d) override
		{
			writer.Bool(d);
		}

		void Int(int32 d) override
		{
			writer.Int(d);
		}

		void Int64(int64 d) override
		{
			writer.Int64(d);
		}

		void Uint(uint32 d) override
		{
			writer.Uint(d);
		}

		void Uint64(uint64 d) override
		{
			writer.Uint64(d);
		}

		void Float(float d) override
		{
			writer.Float(d);
		}

		void Double(double d) override
		{
			writer.Double(d);
		}

		void StartObject() override
		{
			writer.StartObject();
		}

		void EndObject() override
		{
			writer.EndObject();
		}

		void StartArray() override
		{
			writer.StartArray();
		}

		void EndArray(int32 count = 0) override
		{
			writer.EndArray(count);
		}
	};

	class SE_API_RUNTIME CompactJsonWriterImpl : public Json::Writer<Json::StringBuffer>
	{
	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="CompactJsonWriterImpl"/> class.
		/// </summary>
		/// <param name="buffer">The buffer.</param>
		CompactJsonWriterImpl(Json::StringBuffer& buffer)
			: Writer(buffer)
		{
		}

	public:

		void RawValue(const char* json, int32 length)
		{
			Prefix(rapidjson::kObjectType);
			WriteRawValue(json, length);
		}

		void Float(float d)
		{
			Prefix(rapidjson::kNumberType);
			WriteDouble(d);
		}
	};

	/// <summary>
	/// Json writer creating compact and optimized text.
	/// </summary>
	using CompactJsonWriter = JsonWriterBase<CompactJsonWriterImpl>;

	class PrettyJsonWriterImpl : public Json::PrettyWriter<Json::StringBuffer>
	{
	public:

		typedef Json::PrettyWriter<Json::StringBuffer> Writer;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="PrettyJsonWriterImpl"/> class.
		/// </summary>
		/// <param name="buffer">The buffer.</param>
		PrettyJsonWriterImpl(Json::StringBuffer& buffer)
			: PrettyWriter(buffer)
		{
			SetIndent('\t', 1);
		}

		virtual ~PrettyJsonWriterImpl() = default;

	public:

		inline void RawValue(const char* json, int32 length)
		{
			PrettyPrefix(rapidjson::kObjectType);
			WriteRawValue(json, length);
		}

		inline void Float(float d)
		{
			PrettyPrefix(rapidjson::kNumberType);
			WriteDouble(d);
		}
	};

	/// <summary>
	/// Json writer creating prettify text.
	/// </summary>
	using PrettyJsonWriter = JsonWriterBase<PrettyJsonWriterImpl>;
}