
#pragma once

#include "Stream.h"
#include "Core/Templates.h"

/*extern FLAXENGINE_API_CORE class ScriptingObject* FindObject(const Guid& id, class MClass* type);
extern FLAXENGINE_API_CORE class Asset* LoadAsset(const Guid& id, const struct ScriptingTypeHandle& type);*/

namespace SE
{
	/**
	 * 所有数据读取流的基类
	 */
	class SE_API_CORE ReadStream : public Stream
	{
	public:
		/// <summary>
		/// Reads bytes from the stream
		/// </summary>
		/// <param name="data">Data to read</param>
		/// <param name="bytes">Amount of bytes to read</param>
		virtual void ReadBytes(void* data, uint32 bytes) = 0;

	public:
		// Reads byte from the stream
		// @returns Single byte
		byte ReadByte()
		{
			byte data;
			ReadBytes(&data, 1);
			return data;
		}

		// Reads bool from the stream
		bool ReadBool()
		{
			byte data;
			ReadBytes(&data, 1);
			return data != 0;
		}

		// Reads char from the stream
		// @param data Data to read
		char ReadChar()
		{
			char data;
			ReadBytes(&data, 1);
			return data;
		}

		inline void ReadByte(byte* data)
		{
			ReadBytes(data, sizeof(byte));
		}

		// Reads char from the stream
		// @param data Data to read
		inline void ReadChar(char* data)
		{
			ReadBytes(data, sizeof(char));
		}

		// Reads uint8 from the stream
		// @param data Data to read
		inline void ReadUint8(uint8* data)
		{
			ReadBytes(data, sizeof(uint8));
		}

		// Reads int8 from the stream
		// @param data Data to read
		inline void ReadInt8(int8* data)
		{
			ReadBytes(data, sizeof(int8));
		}

		// Reads uint16 from the stream
		// @param data Data to read
		inline void ReadUint16(uint16* data)
		{
			ReadBytes(data, sizeof(uint16));
		}

		// Reads int16 from the stream
		// @param data Data to read
		inline void ReadInt16(int16* data)
		{
			ReadBytes(data, sizeof(int16));
		}

		// Reads uint32 from the stream
		// @param data Data to read
		inline void ReadUint32(uint32* data)
		{
			ReadBytes(data, sizeof(uint32));
		}

		// Reads int32 from the stream
		// @param data Data to read
		inline void ReadInt32(int32* data)
		{
			ReadBytes(data, sizeof(int32));
		}

		// Reads uint64 from the stream
		// @param data Data to read
		inline void ReadUint64(uint64* data)
		{
			ReadBytes(data, sizeof(uint64));
		}

		// Reads int64 from the stream
		// @param data Data to read
		inline void ReadInt64(int64* data)
		{
			ReadBytes(data, sizeof(int64));
		}

		// Reads float from the stream
		// @param data Data to read
		inline void ReadFloat(float* data)
		{
			ReadBytes(data, sizeof(float));
		}

		// Reads double from the stream
		// @param data Data to read
		inline void ReadDouble(double* data)
		{
			ReadBytes(data, sizeof(double));
		}

		template<typename T>
		void ReadVector2(Vector2Base<T>* data)
		{
			ReadBytes(&data->x, sizeof(T));
			ReadBytes(&data->y, sizeof(T));
		}

		template<typename T>
		void ReadVector3(Vector3Base<T>* data)
		{
			ReadBytes(&data->x, sizeof(T));
			ReadBytes(&data->y, sizeof(T));
			ReadBytes(&data->z, sizeof(T));
		}

		template<typename T>
		void ReadVector4(Vector4Base<T>* data)
		{
			ReadBytes(&data->x, sizeof(T));
			ReadBytes(&data->y, sizeof(T));
			ReadBytes(&data->z, sizeof(T));
			ReadBytes(&data->w, sizeof(T));
		}

	public:
		void Read(String& data);
		void Read(String& data, int16 lock);
		void Read(StringAnsi& data);
		void Read(StringAnsi& data, int8 lock);
/*		void Read(CommonValue& data);*/

		template<typename T>
		inline typename TEnableIf<TIsPODType<T>::Value>::Type Read(T& data)
		{
			ReadBytes((void*)&data, sizeof(T));
		}

/*		template<typename T>
		typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Read(T*& data)
		{
			uint32 id[4];
			ReadBytes(id, sizeof(id));
			data = (T*)::FindObject(*(Guid*)id, T::GetStaticClass());
		}

		template<typename T>
		inline void Read(ScriptingObjectReference<T>& v)
		{
			T* ptr;
			Read(ptr);
			v = ptr;
		}
		template<typename T>
		inline void Read(SoftObjectReference<T>& v)
		{
			uint32 id[4];
			ReadBytes(id, sizeof(id));
			v.Set(*(Guid*)id);
		}
		template<typename T>
		inline void Read(AssetReference<T>& v)
		{
			uint32 id[4];
			ReadBytes(id, sizeof(id));
			v = (T*)::LoadAsset(*(Guid*)id, T::TypeInitializer);
		}
		template<typename T>
		inline void Read(WeakAssetReference<T>& v)
		{
			uint32 id[4];
			ReadBytes(id, sizeof(id));
			v = (T*)::LoadAsset(*(Guid*)id, T::TypeInitializer);
		}
		template<typename T>
		inline void Read(SoftAssetReference<T>& v)
		{
			uint32 id[4];
			ReadBytes(id, sizeof(id));
			v.Set(*(Guid*)id);
		}*/

		template<typename T, typename AllocationType = HeapAllocation>
		void Read(List<T, AllocationType>& data)
		{
			int32 size;
			ReadInt32(&size);
			data.Resize(size, false);
			if (size > 0)
			{
				if (TIsPODType<T>::Value && !TIsPointer<T>::Value)
					ReadBytes(data.Get(), size * sizeof(T));
				else
				{
					for (int32 i = 0; i < size; i++)
						Read(data[i]);
				}
			}
		}

		template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
		void Read(Dictionary<KeyType, ValueType, AllocationType>& data)
		{
			int32 count;
			ReadInt32(&count);
			data.Clear();
			data.EnsureCapacity(count);
			for (int32 i = 0; i < count; i++)
			{
				KeyType key;
				Read(key);
				Read(data[key]);
			}
		}

		/// <summary>
		/// Deserializes object from Json by reading it as a raw data (ver+length+bytes).
		/// </summary>
		/// <remarks>Reads version number, data length and actual data bytes from the stream.</remarks>
		/// <param name="obj">The object to deserialize.</param>
		void ReadJson(ISerializable* obj);

	public:
		// Reads WString from the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to read
		void ReadStringAnsi(StringAnsi* data);

		// Reads WString from the stream with a key
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to read
		void ReadStringAnsi(StringAnsi* data, int8 lock);

		// Reads String from the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to read
		void ReadString(String* data);

		// Reads String from the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to read
		// @param lock Characters pass in the stream
		void ReadString(String* data, int16 lock);

/*		// Reads CommonValue from the stream
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		// @param data Data to read
		void ReadCommonValue(CommonValue* data);*/

		/// <summary>
		/// Read data array
		/// [Deprecated on 11.10.2022, expires on 11.10.2024]
		/// </summary>
		/// <param name="data">Array to read</param>
		template<typename T, typename AllocationType = HeapAllocation>
		void ReadArray(List<T, AllocationType>* data)
		{
			Read(*data);
		}

	public:
		// Deserialization of math types with float or double depending on the context (must match serialization)
		// Set useDouble=true to explicitly use 64-bit precision for serialized data
/*		void ReadBoundingBox(BoundingBox* box, bool useDouble = false);
		void ReadBoundingSphere(BoundingSphere* sphere, bool useDouble = false);
		void ReadTransform(Transform* transform, bool useDouble = false);
		void ReadRay(Ray* ray, bool useDouble = false);*/

	public:
		// [Stream]
		bool CanRead() override
		{
			return GetPosition() != GetLength();
		}
	};
}
