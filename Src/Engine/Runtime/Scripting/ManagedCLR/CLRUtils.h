#pragma once
#include "CLRCore.h"
#include "CLRTypes.h"
#include "Core/Types/Strings/StringView.h"
#include "Runtime/API.h"
#include "Runtime/Scripting/ScriptingType.h"
#include "Runtime/Utilities/Variant.h"

namespace SE
{
    struct Version;
    class CultureInfo;
    template<typename AllocationType>
    class BitArray;

    namespace CLRUtils
    {
        extern SE_API_RUNTIME StringView ToString(CLRString* str);
        extern SE_API_RUNTIME StringAnsi ToStringAnsi(CLRString* str);
        extern SE_API_RUNTIME void ToString(CLRString* str, String& result);
        extern SE_API_RUNTIME void ToString(CLRString* str, StringView& result);
        extern SE_API_RUNTIME void ToString(CLRString* str, Variant& result);
        extern SE_API_RUNTIME void ToString(CLRString* str, StringAnsi& result);

        extern SE_API_RUNTIME CLRString* ToString(const char* str);
        extern SE_API_RUNTIME CLRString* ToString(const StringAnsi& str);
        extern SE_API_RUNTIME CLRString* ToString(const String& str);
        extern SE_API_RUNTIME CLRString* ToString(const String& str, CLRDomain* domain);
        extern SE_API_RUNTIME CLRString* ToString(const StringAnsiView& str);
        extern SE_API_RUNTIME CLRString* ToString(const StringView& str);
        extern SE_API_RUNTIME CLRString* ToString(const StringView& str, CLRDomain* domain);

        extern SE_API_RUNTIME ScriptingTypeHandle UnboxScriptingTypeHandle(CLRTypeObject* value);
        extern SE_API_RUNTIME CLRTypeObject* BoxScriptingTypeHandle(const ScriptingTypeHandle& value);
        extern SE_API_RUNTIME VariantTypeHandle UnboxVariantType(CLRType* type);
        extern SE_API_RUNTIME CLRTypeObject* BoxVariantType(const VariantTypeHandle& value);
        extern SE_API_RUNTIME Variant UnboxVariant(CLRObject* value);
        extern SE_API_RUNTIME CLRObject* BoxVariant(const Variant& value);
    }

    // Converter for data of type T between managed and unmanaged world
    template<typename T, typename Enable = void>
    struct CLRConverter
    {
        CLRObject* Box(const T& data, const CLRClass* klass);
        void Unbox(T& result, CLRObject* data);
        void ToManagedArray(CLRArray* result, const Span<T>& data);
        void ToNativeArray(Span<T>& result, const CLRArray* data);
    };

    // Converter for POD types (that can use raw memory copy).
    template<typename T>
    struct CLRConverter<T, typename TEnableIf<TAnd<TIsPODType<T>, TNot<TIsBaseOf<class ScriptingObject, typename TRemovePointer<T>::Type>>>::Value>::Type>
    {
        CLRObject* Box(const T& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(T& result, CLRObject* data)
        {
            if (data)
            {
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(T));
            }
        }

        void ToManagedArray(CLRArray* result, const Span<T>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(T));
        }

        void ToNativeArray(Span<T>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(T));
        }
    };

    // Converter for String.
    template<>
    struct CLRConverter<String>
    {
        CLRObject* Box(const String& data, const CLRClass* klass)
        {
            CLRString* str = CLRUtils::ToString(data);
            return CLRCore::Object::Box(str, klass);
        }

        void Unbox(String& result, CLRObject* data)
        {
            CLRString* str = (CLRString*)CLRCore::Object::Unbox(data);
            result = CLRUtils::ToString(str);
        }

        void ToManagedArray(CLRArray* result, const Span<String>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = (CLRObject*)CLRUtils::ToString(data.Get()[i]);
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<String>& result, const CLRArray* data)
        {
            CLRString** dataPtr = CLRCore::Array::GetAddress<CLRString*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                CLRUtils::ToString(dataPtr[i], result.Get()[i]);
        }
    };

    // Converter for StringAnsi.
    template<>
    struct CLRConverter<StringAnsi>
    {
        CLRObject* Box(const StringAnsi& data, const CLRClass* klass)
        {
            return (CLRObject*)CLRUtils::ToString(data);
        }

        void Unbox(StringAnsi& result, CLRObject* data)
        {
            result = CLRUtils::ToStringAnsi((CLRString*)data);
        }

        void ToManagedArray(CLRArray* result, const Span<StringAnsi>& data)
        {
            if (data.Length() == 0)
                return;
            auto* objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = (CLRObject*)CLRUtils::ToString(data.Get()[i]);
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<StringAnsi>& result, const CLRArray* data)
        {
            CLRString** dataPtr = CLRCore::Array::GetAddress<CLRString*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                CLRUtils::ToString(dataPtr[i], result.Get()[i]);
        }
    };

    // Converter for StringView.
    template<>
    struct CLRConverter<StringView>
    {
        CLRObject* Box(const StringView& data, const CLRClass* klass)
        {
            return (CLRObject*)CLRUtils::ToString(data);
        }

        void Unbox(StringView& result, CLRObject* data)
        {
            result = CLRUtils::ToString((CLRString*)data);
        }

        void ToManagedArray(CLRArray* result, const Span<StringView>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = (CLRObject*)CLRUtils::ToString(data.Get()[i]);
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<StringView>& result, const CLRArray* data)
        {
            CLRString** dataPtr = CLRCore::Array::GetAddress<CLRString*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                CLRUtils::ToString(dataPtr[i], result.Get()[i]);
        }
    };

    // Converter for Variant.
    template<>
    struct CLRConverter<Variant>
    {
        CLRObject* Box(const Variant& data, const CLRClass* klass)
        {
            return CLRUtils::BoxVariant(data);
        }

        void Unbox(Variant& result, CLRObject* data)
        {
            result = CLRUtils::UnboxVariant(data);
        }

        void ToManagedArray(CLRArray* result, const Span<Variant>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = CLRUtils::BoxVariant(data[i]);
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<Variant>& result, const CLRArray* data)
        {
            CLRObject** dataPtr = CLRCore::Array::GetAddress<CLRObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = CLRUtils::UnboxVariant(dataPtr[i]);
        }
    };

    // Converter for Scripting Objects (collection of pointers).
    template<typename T>
    struct CLRConverter<T*, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
    {
        CLRObject* Box(T* data, const CLRClass* klass)
        {
            return data ? data->GetOrCreateManagedInstance() : nullptr;
        }

        void Unbox(T*& result, CLRObject* data)
        {
            result = (T*)ScriptingObject::ToNative(data);
        }

        void ToManagedArray(CLRArray* result, const Span<T*>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data.Get()[i] ? data.Get()[i]->GetOrCreateManagedInstance() : nullptr;
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<T*>& result, const CLRArray* data)
        {
            CLRObject** dataPtr = CLRCore::Array::GetAddress<CLRObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // Converter for Scripting Objects (collection of values).
    template<typename T>
    struct CLRConverter<T, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
    {
        CLRObject* Box(const T& data, const CLRClass* klass)
        {
            return data.GetOrCreateManagedInstance();
        }

        void ToManagedArray(CLRArray* result, const Span<T>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data.Get()[i].GetOrCreateManagedInstance();
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }
    };

    // Converter for ScriptingObject References.
    template<typename T>
    class ScriptingObjectReference;

    template<typename T>
    struct CLRConverter<ScriptingObjectReference<T>>
    {
        CLRObject* Box(const ScriptingObjectReference<T>& data, const CLRClass* klass)
        {
            return data.GetManagedInstance();
        }

        void Unbox(ScriptingObjectReference<T>& result, CLRObject* data)
        {
            result = (T*)ScriptingObject::ToNative(data);
        }

        void ToManagedArray(CLRArray* result, const Span<ScriptingObjectReference<T>>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data[i].GetManagedInstance();
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<ScriptingObjectReference<T>>& result, const CLRArray* data)
        {
            CLRObject** dataPtr = CLRCore::Array::GetAddress<CLRObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // Converter for Asset References.
    template<typename T>
    class AssetReference;

    template<typename T>
    struct CLRConverter<AssetReference<T>>
    {
        CLRObject* Box(const AssetReference<T>& data, const CLRClass* klass)
        {
            return data.GetManagedInstance();
        }

        void Unbox(AssetReference<T>& result, CLRObject* data)
        {
            result = (T*)ScriptingObject::ToNative(data);
        }

        void ToManagedArray(CLRArray* result, const Span<AssetReference<T>>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data[i].GetManagedInstance();
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<AssetReference<T>>& result, const CLRArray* data)
        {
            CLRObject** dataPtr = CLRCore::Array::GetAddress<CLRObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // TODO: use MarshalAs=Guid on SoftAssetReference to pass guid over bindings and not load asset in glue code
    template<typename T>
    class SoftAssetReference;
    template<typename T>
    struct CLRConverter<SoftAssetReference<T>>
    {
        void ToManagedArray(CLRArray* result, const Span<SoftAssetReference<T>>& data)
        {
            if (data.Length() == 0)
                return;
            CLRObject** objects = (CLRObject**)PlatformAllocator::Allocate(data.Length() * sizeof(CLRObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data[i].GetManagedInstance();
            CLRCore::GC::WriteArrayRef(result, Span<CLRObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<SoftAssetReference<T>>& result, const CLRArray* data)
        {
            CLRObject** dataPtr = CLRCore::Array::GetAddress<CLRObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // Converter for List.
    template<typename T>
    struct CLRConverter<List<T>>
    {
        CLRObject* Box(const List<T>& data, const CLRClass* klass)
        {
            if (!klass)
                return nullptr;
            CLRArray* result = CLRCore::Array::New(klass->GetElementClass(), data.Count());
            CLRConverter<T> converter;
            converter.ToManagedArray(result, Span<T>(data.Get(), data.Count()));
            return (CLRObject*)result;
        }

        void Unbox(List<T>& result, CLRObject* data)
        {
            CLRArray* array = CLRCore::Array::Unbox(data);
            const int32 length = array ? CLRCore::Array::GetLength(array) : 0;
            result.Resize(length);
            CLRConverter<T> converter;
            Span<T> resultSpan(result.Get(), length);
            converter.ToNativeArray(resultSpan, array);
        }
    };

    namespace CLRUtils
    {
        // Outputs the full typename for the type of the specified object.
        extern SE_API_RUNTIME const StringAnsi& GetClassFullname(CLRObject* obj);

        // Returns the class of the provided object.
        extern SE_API_RUNTIME CLRClass* GetClass(CLRObject* object);

        // Returns the class of the provided type.
        extern SE_API_RUNTIME CLRClass* GetClass(CLRTypeObject* type);

        // Returns the class of the provided VariantType value.
        extern SE_API_RUNTIME CLRClass* GetClass(const VariantTypeHandle& value);

        // Returns the class of the provided Variant value.
        extern SE_API_RUNTIME CLRClass* GetClass(const Variant& value);

        // Returns the type of the provided object.
        extern SE_API_RUNTIME CLRTypeObject* GetType(CLRObject* object);

        // Returns the type of the provided class.
        extern SE_API_RUNTIME CLRTypeObject* GetType(CLRClass* klass);

        /// <summary>
        /// Boxes the native value into the managed object.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="valueClass">The value type class.</param>
        template<class T>
        CLRObject* Box(const T& value, const CLRClass* valueClass)
        {
            CLRConverter<T> converter;
            return converter.Box(value, valueClass);
        }

        /// <summary>
        /// Unboxes SEObject to the native value of the given type.
        /// </summary>
        template<class T>
        T Unbox(CLRObject* object)
        {
            CLRConverter<T> converter;
            T result;
            converter.Unbox(result, object);
            return result;
        }

        /// <summary>
        /// Links managed array data to the unmanaged BytesContainer.
        /// </summary>
        /// <param name="arrayObj">The array object.</param>
        /// <returns>The result data container with linked array data bytes (not copied).</returns>
        extern SE_API_RUNTIME BytesContainer LinkArray(CLRArray* arrayObj);

        /// <summary>
        /// Allocates new managed array of data and copies contents from given native array.
        /// </summary>
        /// <param name="data">The array object.</param>
        /// <param name="valueClass">The array values type class.</param>
        /// <returns>The output array.</returns>
        template<typename T>
        CLRArray* ToArray(const Span<T>& data, const CLRClass* valueClass)
        {
            if (!valueClass)
                return nullptr;
            CLRArray* result = CLRCore::Array::New(valueClass, data.Length());
            CLRConverter<T> converter;
            converter.ToManagedArray(result, data);
            return result;
        }

        /// <summary>
        /// Allocates new managed array of data and copies contents from given native array.
        /// </summary>
        /// <param name="data">The array object.</param>
        /// <param name="valueClass">The array values type class.</param>
        /// <returns>The output array.</returns>
        template<typename T, typename AllocationType>
        FORCE_INLINE CLRArray* ToArray(const List<T, AllocationType>& data, const CLRClass* valueClass)
        {
            return CLRUtils::ToArray(Span<T>(data.Get(), data.Count()), valueClass);
        }

        /// <summary>
        /// Converts the managed array into native array container object.
        /// </summary>
        /// <param name="arrayObj">The managed array object.</param>
        /// <returns>The output array.</returns>
        template<typename T, typename AllocationType = HeapAllocation>
        List<T, AllocationType> ToArray(CLRArray* arrayObj)
        {
            List<T, AllocationType> result;
            const int32 length = arrayObj ? CLRCore::Array::GetLength(arrayObj) : 0;
            result.Resize(length);
            CLRConverter<T> converter;
            Span<T> resultSpan(result.Get(), length);
            converter.ToNativeArray(resultSpan, arrayObj);
            return result;
        }

        /// <summary>
        /// Converts the managed array into native Span.
        /// </summary>
        /// <param name="arrayObj">The managed array object.</param>
        /// <returns>The output array pointer and size.</returns>
        template<typename T>
        Span<T> ToSpan(CLRArray* arrayObj)
        {
            T* ptr = (T*)CLRCore::Array::GetAddress(arrayObj);
            const int32 length = arrayObj ? CLRCore::Array::GetLength(arrayObj) : 0;
            return Span<T>(ptr, length);
        }

        /// <summary>
        /// Converts the native array into native Span.
        /// </summary>
        /// <param name="data">The native array object.</param>
        /// <returns>The output array pointer and size.</returns>
        template<typename T, typename AllocationType = HeapAllocation>
        FORCE_INLINE Span<T> ToSpan(const List<T, AllocationType>& data)
        {
            return Span<T>(data.Get(), data.Count());
        }

        /// <summary>
        /// Links managed array data to the unmanaged DataContainer (must use simple type like struct or float/int/bool).
        /// </summary>
        /// <param name="arrayObj">The array object.</param>
        /// <param name="result">The result data (linked not copied).</param>
        template<typename T>
        void ToArray(CLRArray* arrayObj, DataContainer<T>& result)
        {
            const int32 length = arrayObj ? CLRCore::Array::GetLength(arrayObj) : 0;
            if (length == 0)
            {
                result.Release();
                return;
            }
            T* bytesRaw = (T*)CLRCore::Array::GetAddress(arrayObj);
            result.Link(bytesRaw, length);
        }

        /// <summary>
        /// Allocates new managed bytes array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE CLRArray* ToArray(const Span<byte>& data)
        {
            return ToArray(data, CLRCore::TypeCache::Byte);
        }

        /// <summary>
        /// Allocates new managed bytes array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE CLRArray* ToArray(List<byte>& data)
        {
            return ToArray(Span<byte>(data.Get(), data.Count()), CLRCore::TypeCache::Byte);
        }

        /// <summary>
        /// Allocates new managed strings array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE CLRArray* ToArray(const Span<String>& data)
        {
            return ToArray(data, CLRCore::TypeCache::String);
        }

        /// <summary>
        /// Allocates new managed strings array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE CLRArray* ToArray(const List<String>& data)
        {
            return ToArray(Span<String>(data.Get(), data.Count()), CLRCore::TypeCache::String);
        }

#if USE_NETCORE
        /// <summary>
        /// Allocates new boolean array and copies data from the given unmanaged data container. The managed runtime is responsible for releasing the returned array data.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE bool* ToBoolArray(const List<bool>& data)
        {
            // System.Runtime.InteropServices.Marshalling.ArrayMarshaller uses CoTask memory alloc to native data pointer
            bool* arr = (bool*)SECore::GC::AllocateMemory(data.Count() * sizeof(bool), true);
            memcpy(arr, data.Get(), data.Count() * sizeof(bool));
            return arr;
        }

        /// <summary>
        /// Allocates new boolean array and copies data from the given unmanaged data container. The managed runtime is responsible for releasing the returned array data.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        template<typename AllocationType = HeapAllocation>
        FORCE_INLINE bool* ToBoolArray(const BitArray<AllocationType>& data)
        {
            // System.Runtime.InteropServices.Marshalling.ArrayMarshaller uses CoTask memory alloc to native data pointer
            bool* arr = (bool*)SECore::GC::AllocateMemory(data.Count() * sizeof(bool), true);
            for (int i = 0; i < data.Count(); i++)
                arr[i] = data[i];
            return arr;
        }
#else
        FORCE_INLINE bool* ToBoolArray(const List<bool>& data)
        {
            return nullptr;
        }

        template<typename AllocationType = HeapAllocation>
        FORCE_INLINE bool* ToBoolArray(const BitArray<AllocationType>& data)
        {
            return nullptr;
        }
#endif

        extern void* VariantToManagedArgPtr(Variant& value, CLRType* type, bool& failed);

        // extern CLRObject* ToManaged(const Version& value);
        // extern Version ToNative(CLRObject* value);
    };
}
