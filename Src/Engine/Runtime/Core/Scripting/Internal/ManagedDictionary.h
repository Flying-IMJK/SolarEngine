#pragma once

#include "Runtime/Core/Scripting/ManagedCLR/CLRTypes.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"

namespace SE
{
	class SE_API_RUNTIME ManagedDictionary
	{
	public:
		struct KeyValueType
		{
			CLRType* keyType;
			CLRType* valueType;

			bool operator==(const KeyValueType& other) const
			{
				return keyType == other.keyType && valueType == other.valueType;
			}
		};

	private:
		static Dictionary<KeyValueType, CLRTypeObject*> CachedDictionaryTypes;


		typedef CLRTypeObject* (*MakeGenericTypeThunk)(CLRObject* instance, CLRTypeObject* genericType, CLRArray* genericArgs, CLRObject** exception);
		static MakeGenericTypeThunk MakeGenericType;

		typedef CLRObject* (*CreateInstanceThunk)(CLRObject* instance, CLRTypeObject* type, void* arr, CLRObject** exception);
		static CreateInstanceThunk CreateInstance;

		typedef void (*AddDictionaryItemThunk)(CLRObject* instance, CLRObject* dictionary, CLRObject* key, CLRObject* value, CLRObject** exception);
		static AddDictionaryItemThunk AddDictionaryItem;

		typedef CLRArray* (*GetDictionaryKeysThunk)(CLRObject* instance, CLRObject* dictionary, CLRObject** exception);
		static GetDictionaryKeysThunk GetDictionaryKeys;

	public:
		CLRObject* Instance;

		ManagedDictionary(CLRObject* instance = nullptr);

		template<typename KeyType, typename ValueType>
		static CLRObject* ToManaged(const Dictionary<KeyType, ValueType>& data, CLRType* keyType, CLRType* valueType)
		{
			CLRConverter<KeyType> keysConverter;
			CLRConverter<ValueType> valueConverter;
			ManagedDictionary result = New(keyType, valueType);
			CLRClass* keyClass = CLRCore::Type::GetClass(keyType);
			CLRClass* valueClass = CLRCore::Type::GetClass(valueType);
			for (auto i = data.Begin(); i.IsNotEnd(); ++i)
			{
				CLRObject* keyManaged = keysConverter.Box(i->Key, keyClass);
				CLRObject* valueManaged = valueConverter.Box(i->Value, valueClass);
				result.Add(keyManaged, valueManaged);
			}
			return result.Instance;
		}

		/// <summary>
		/// Converts the managed dictionary objects into the native dictionary collection.
		/// </summary>
		/// <param name="managed">The managed dictionary object.</param>
		/// <returns>The output array.</returns>
		template<typename KeyType, typename ValueType>
		static Dictionary<KeyType, ValueType> ToNative(CLRObject* managed)
		{
			Dictionary<KeyType, ValueType> result;
			const ManagedDictionary wrapper(managed);
			CLRArray* managedKeys = wrapper.GetKeys();
			if (managedKeys == nullptr)
				return result;
			int32 length = CLRCore::Array::GetLength(managedKeys);
			List<KeyType> keys;
			keys.Resize(length);
			result.EnsureCapacity(length);
			CLRConverter<KeyType> keysConverter;
			CLRConverter<ValueType> valueConverter;
			CLRObject** managedKeysPtr = CLRCore::Array::GetAddress<CLRObject*>(managedKeys);
			for (int32 i = 0; i < keys.Count(); i++)
			{
				KeyType& key = keys[i];
				CLRObject* keyManaged = managedKeysPtr[i];
				keysConverter.Unbox(key, keyManaged);
				CLRObject* valueManaged = wrapper.GetValue(keyManaged);
				ValueType& value = result[key];
				valueConverter.Unbox(value, valueManaged);
			}
			return result;
		}

		static CLRTypeObject* GetClass(CLRType* keyType, CLRType* valueType);

		static ManagedDictionary New(CLRType* keyType, CLRType* valueType);

		void Add(CLRObject* key, CLRObject* value);

		CLRArray* GetKeys() const;

		CLRObject* GetValue(CLRObject* key) const;
	};


	int32 GetHash(const ManagedDictionary::KeyValueType& other);
} // SE

