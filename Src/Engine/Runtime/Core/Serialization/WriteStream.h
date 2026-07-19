#pragma once

#include "Stream.h"
#include "Runtime/Core/Templates.h"

namespace SE
{
	/**
	 * 所有数据写入流的基类
	 */
	class SE_API_RUNTIME WriteStream : public Stream
	{
	public:
		/// <summary>
		/// Writes bytes to the stream
		/// </summary>
		/// <param name="data">Data to write</param>
		/// <param name="bytes">Amount of bytes to write</param>
		virtual void WriteBytes(const void* data, uint32 bytes) = 0;

	public:
		// Writes byte to the stream
		// @param data Data to write
		inline void WriteByte(byte data)
		{
			WriteBytes(&data, sizeof(byte));
		}

		// Writes bool to the stream
		// @param data Data to write
		inline void WriteBool(bool data)
		{
			WriteBytes(&data, sizeof(bool));
		}

		// Writes char to the stream
		// @param data Data to write
		inline void WriteChar(char data)
		{
			WriteBytes(&data, sizeof(char));
		}

		// Writes char to the stream
		// @param data Data to write
		inline void WriteChar(wchar_t data)
		{
			WriteBytes(&data, sizeof(wchar_t));
		}

		// Writes uint8 to the stream
		// @param data Data to write
		inline void WriteUint8(uint8 data)
		{
			WriteBytes(&data, sizeof(uint8));
		}

		// Writes int8 to the stream
		// @param data Data to write
		inline void WriteInt8(int8 data)
		{
			WriteBytes(&data, sizeof(int8));
		}

		// Writes uint16 to the stream
		// @param data Data to write
		inline void WriteUint16(uint16 data)
		{
			WriteBytes(&data, sizeof(uint16));
		}

		// Writes int16 to the stream
		// @param data Data to write
		inline void WriteInt16(int16 data)
		{
			WriteBytes(&data, sizeof(int16));
		}

		// Writes uint32 to the stream
		// @param data Data to write
		inline void WriteUint32(uint32 data)
		{
			WriteBytes(&data, sizeof(uint32));
		}

		// Writes int32 to the stream
		// @param data Data to write
		inline void WriteInt32(int32 data)
		{
			WriteBytes(&data, sizeof(int32));
		}

		// Writes int64 to the stream
		// @param data Data to write
		inline void WriteInt64(int64 data)
		{
			WriteBytes(&data, sizeof(int64));
		}

		// Writes uint64 to the stream
		// @param data Data to write
		inline void WriteUint64(uint64 data)
		{
			WriteBytes(&data, sizeof(uint64));
		}

		// Writes float to the stream
		// @param data Data to write
		inline void WriteFloat(float data)
		{
			WriteBytes(&data, sizeof(float));
		}

		// Writes double to the stream
		// @param data Data to write
		inline void WriteDouble(double data)
		{
			WriteBytes(&data, sizeof(double));
		}

		template<typename T>
		void WriteVector2(const Vector2Base<T>& data)
		{
			WriteBytes(&data.x, sizeof(T));
			WriteBytes(&data.y, sizeof(T));
		}

		template<typename T>
		void WriteVector3(const Vector3Base<T>& data)
		{
			WriteBytes(&data.x, sizeof(T));
			WriteBytes(&data.y, sizeof(T));
			WriteBytes(&data.z, sizeof(T));
		}

		template<typename T>
		void WriteVector4(const Vector4Base<T>& data)
		{
			WriteBytes(&data.x, sizeof(T));
			WriteBytes(&data.y, sizeof(T));
			WriteBytes(&data.z, sizeof(T));
			WriteBytes(&data.w, sizeof(T));
		}

	public:
		// Writes text to the stream
		// @param data Text to write
		// @param length Text length
		void WriteText(const char* text, int32 length)
		{
			WriteBytes((const void*)text, sizeof(char) * length);
		}

		// Writes text to the stream
		// @param data Text to write
		// @param length Text length
		void WriteText(const wchar_t* text, int32 length)
		{
			WriteBytes((const void*)text, sizeof(wchar_t) * length);
		}

		// Write UTF BOM character sequence
		void WriteBOM()
		{
			WriteByte(0xEF);
			WriteByte(0xBB);
			WriteByte(0xBF);
		}

		// Writes text to the stream
		// @param data Text to write
		void WriteText(const StringView& text);
		void WriteText(const StringAnsiView& text);

	public:
		void Write(const StringView& data);
		void Write(const StringView& data, int16 lock);
		void Write(const StringAnsiView& data);
		void Write(const StringAnsiView& data, int8 lock);
/*		void Write(const CommonValue& data);*/

		template<typename T>
		FORCE_INLINE typename TEnableIf<TAnd<TIsPODType<T>, TNot<TIsPointer<T>>>::Value>::Type Write(const T& data)
		{
			WriteBytes((const void*)&data, sizeof(T));
		}

		template<typename T>
		typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Write(const T* data)
		{
			uint32 id[4] = { 0 };
			if (data)
				memcpy(id, &data->GetID(), sizeof(id));
			WriteBytes(id, sizeof(id));
		}

		template<typename T>
		inline void Write(const ScriptingObjectReference<T>& v)
		{
			Write(v.Get());
		}
		template<typename T>
		inline void Write(const SoftObjectReference<T>& v)
		{
			Write(v.Get());
		}
		template<typename T>
		inline void Write(const AssetRef<T>& v)
		{
			Write(v.Get());
		}
		template<typename T>
		inline void Write(const WeakAssetRef<T>& v)
		{
			Write(v.Get());
		}
		template<typename T>
		inline void Write(const SoftAssetRef<T>& v)
		{
			Write(v.Get());
		}

		template<typename T, typename AllocationType = HeapAllocation>
		void Write(const List<T, AllocationType>& data)
		{
			const int32 size = data.Count();
			WriteInt32(size);
			if (size > 0)
			{
				if (TIsPODType<T>::Value && !TIsPointer<T>::Value)
					WriteBytes(data.Get(), size * sizeof(T));
				else
				{
					for (int32 i = 0; i < size; i++)
						Write(data[i]);
				}
			}
		}

		template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
		void Write(const Dictionary<KeyType, ValueType, AllocationType>& data)
		{
			const int32 count = data.Count();
			WriteInt32(count);
			for (const auto& e : data)
			{
				Write(e.Key);
				Write(e.value);
			}
		}

		/// <summary>
		/// Serializes object to Json and writes it as a raw data (ver+length+bytes).
		/// </summary>
		/// <remarks>Writes version number, data length and actual data bytes to the stream.</remarks>
		/// <param name="obj">The object to serialize.</param>
		/// <param name="otherObj">The instance of the object to compare with and serialize only the modified properties. If null, then serialize all properties.</param>
		void WriteJson(ISerializable* obj, const void* otherObj = nullptr);
		void WriteJson(const StringAnsiView& json);

	public:
		// Writes String to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		void WriteString(const StringView& data);

		// Writes String to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		// @param lock Characters pass in the stream
		void WriteString(const StringView& data, int16 lock);

		// Writes Ansi String to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		void WriteStringAnsi(const StringAnsiView& data);

		// Writes Ansi String to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		// @param lock Characters pass in the stream
		void WriteStringAnsi(const StringAnsiView& data, int8 lock);

/*		// Writes CommonValue to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		void WriteCommonValue(const CommonValue& data);

		// Writes VariantType to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		void WriteVariantType(const VariantType& data);

		// Writes Variant to the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to write
		void WriteVariant(const Variant& data);*/

		/// <summary>
		/// Write data array
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		/// </summary>
		/// <param name="data">Array to write</param>
		template<typename T, typename AllocationType = HeapAllocation>
		inline void WriteArray(const List<T, AllocationType>& data)
		{
			Write(data);
		}

	public:
		// Serialization of math types with float or double depending on the context (must match deserialization)
		// Set useDouble=true to explicitly use 64-bit precision for serialized data
/*		void WriteBoundingBox(const BoundingBox& box, bool useDouble = false);
		void WriteBoundingSphere(const BoundingSphere& sphere, bool useDouble = false);
		void WriteTransform(const Transform& transform, bool useDouble = false);
		void WriteRay(const Ray& ray, bool useDouble = false);*/

	public:
		// [Stream]
		bool CanWrite() override
		{
			return true;
		}
	};
}
