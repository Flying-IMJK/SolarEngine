
#include "ManagedDictionary.h"

#include "Core/Logging/Logging.h"
#include "Runtime/Scripting/Scripting.h"
#include "Runtime/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Scripting/ManagedCLR/CLRCore.h"
#include "Runtime/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Scripting/ManagedCLR/CLRMethod.h"

namespace SE
{

	Dictionary<ManagedDictionary::KeyValueType, CLRTypeObject*> ManagedDictionary::CachedDictionaryTypes;
	ManagedDictionary::MakeGenericTypeThunk ManagedDictionary::MakeGenericType;
	ManagedDictionary::CreateInstanceThunk ManagedDictionary::CreateInstance;
	ManagedDictionary::AddDictionaryItemThunk ManagedDictionary::AddDictionaryItem;
	ManagedDictionary::GetDictionaryKeysThunk ManagedDictionary::GetDictionaryKeys;

	ManagedDictionary::ManagedDictionary(CLRObject* instance)
	{
		Instance = instance;
    	
		// Cache the thunks of the dictionary helper methods
		if (MakeGenericType == nullptr)
		{
			CLRClass* scriptingClass = Scripting::GetScriptingClass();
			ENGINE_ASSERT(scriptingClass);

			CLRMethod* makeGenericTypeMethod = scriptingClass->GetMethod("MakeGenericType", 2);
			ENGINE_ASSERT(makeGenericTypeMethod);
			MakeGenericType = (MakeGenericTypeThunk)makeGenericTypeMethod->GetThunk();

			CLRMethod* createInstanceMethod = CLRCore::TypeCache::Activator->GetMethod("CreateInstance", 2);
			ENGINE_ASSERT(createInstanceMethod);
			CreateInstance = (CreateInstanceThunk)createInstanceMethod->GetThunk();

			CLRMethod* addDictionaryItemMethod = scriptingClass->GetMethod("AddDictionaryItem", 3);
			ENGINE_ASSERT(addDictionaryItemMethod);
			AddDictionaryItem = (AddDictionaryItemThunk)addDictionaryItemMethod->GetThunk();
            
			CLRMethod* getDictionaryKeysItemMethod = scriptingClass->GetMethod("GetDictionaryKeys", 1);
			ENGINE_ASSERT(getDictionaryKeysItemMethod);
			GetDictionaryKeys = (GetDictionaryKeysThunk)getDictionaryKeysItemMethod->GetThunk();
		}
	}

	CLRTypeObject* ManagedDictionary::GetClass(CLRType* keyType, CLRType* valueType)
	{
		// Check if the generic type was generated earlier
		KeyValueType cacheKey = { keyType, valueType };
		CLRTypeObject* dictionaryType;
		if (CachedDictionaryTypes.TryGet(cacheKey, dictionaryType))
		{
			return dictionaryType;
		}

		CLRTypeObject* genericType = CLRUtils::GetType(CLRCore::TypeCache::Dictionary);
		CLRArray* genericArgs = CLRCore::Array::New(CLRCore::TypeCache::IntPtr, 2);

		CLRTypeObject** genericArgsPtr = CLRCore::Array::GetAddress<CLRTypeObject*>(genericArgs);
		genericArgsPtr[0] = keyType;
		genericArgsPtr[1] = valueType;

		CLRObject* exception = nullptr;
		dictionaryType = MakeGenericType(nullptr, genericType, genericArgs, &exception);

		if (exception)
		{
			CLRException ex(exception);
			ex.Log(Log::Severity::Error, SE_TEXT(""));
			return nullptr;
		}
		CachedDictionaryTypes.Add(cacheKey, dictionaryType);
		return dictionaryType;
	}

	ManagedDictionary ManagedDictionary::New(CLRType* keyType, CLRType* valueType)
	{
		ManagedDictionary result;
		CLRTypeObject* dictionaryType = GetClass(keyType, valueType);
		if (!dictionaryType)
		{
			return result;
		}

		CLRObject* exception = nullptr;
		CLRObject* instance = CreateInstance(nullptr, dictionaryType, nullptr, &exception);

		if (exception)
		{
			CLRException ex(exception);
			ex.Log(Log::Severity::Error, SE_TEXT(""));
			return result;
		}

		result.Instance = instance;
		return result;
	}

	void ManagedDictionary::Add(CLRObject* key, CLRObject* value)
	{
		ENGINE_ASSERT(Instance);

		CLRObject* exception = nullptr;
		AddDictionaryItem(nullptr, Instance, key, value, &exception);
		if (exception)
		{
			CLRException ex(exception);
			ex.Log(Log::Severity::Error, SE_TEXT(""));
		}
	}

	CLRArray* ManagedDictionary::GetKeys() const
	{
		if (Instance == nullptr)
		{
			return nullptr;
		}

		return GetDictionaryKeys(nullptr, Instance, nullptr);
	}

	CLRObject* ManagedDictionary::GetValue(CLRObject* key) const
	{
		if (Instance == nullptr)
		{
			return nullptr;
		}

		CLRClass* klass = CLRCore::Object::GetClass(Instance);
		CLRMethod* getItemMethod = klass->GetMethod("System.Collections.IDictionary.get_Item", 1);
		if (getItemMethod == nullptr)
		{
			return nullptr;
		}

		void* params[1];
		params[0] = key;
		return getItemMethod->Invoke(Instance, params, nullptr);
	}

	int32 GetHash(const ManagedDictionary::KeyValueType& other)
	{
		uint32 hash = GetHash((void*)other.keyType);
		HashCombine(hash, GetHash((void*)other.valueType));
		return hash;
	}
} // SE