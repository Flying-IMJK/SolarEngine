#pragma once
#include "SECore.h"
#include "SETypes.h"
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

    namespace MUtils
    {
        extern SE_API_RUNTIME StringView ToString(SEString* str);
        extern SE_API_RUNTIME StringAnsi ToStringAnsi(SEString* str);
        extern SE_API_RUNTIME void ToString(SEString* str, String& result);
        extern SE_API_RUNTIME void ToString(SEString* str, StringView& result);
        extern SE_API_RUNTIME void ToString(SEString* str, Variant& result);
        extern SE_API_RUNTIME void ToString(SEString* str, StringAnsi& result);

        extern SE_API_RUNTIME SEString* ToString(const char* str);
        extern SE_API_RUNTIME SEString* ToString(const StringAnsi& str);
        extern SE_API_RUNTIME SEString* ToString(const String& str);
        extern SE_API_RUNTIME SEString* ToString(const String& str, SEDomain* domain);
        extern SE_API_RUNTIME SEString* ToString(const StringAnsiView& str);
        extern SE_API_RUNTIME SEString* ToString(const StringView& str);
        extern SE_API_RUNTIME SEString* ToString(const StringView& str, SEDomain* domain);

        extern SE_API_RUNTIME ScriptingTypeHandle UnboxScriptingTypeHandle(SETypeObject* value);
        extern SE_API_RUNTIME SETypeObject* BoxScriptingTypeHandle(const ScriptingTypeHandle& value);
        extern SE_API_RUNTIME VariantTypeHandle UnboxVariantType(SEType* type);
        extern SE_API_RUNTIME SETypeObject* BoxVariantType(const VariantTypeHandle& value);
        extern SE_API_RUNTIME Variant UnboxVariant(SEObject* value);
        extern SE_API_RUNTIME SEObject* BoxVariant(const Variant& value);
    }

    // Converter for data of type T between managed and unmanaged world
    template<typename T, typename Enable = void>
    struct MConverter
    {
        SEObject* Box(const T& data, const SEClass* klass);
        void Unbox(T& result, SEObject* data);
        void ToManagedArray(SEArray* result, const Span<T>& data);
        void ToNativeArray(Span<T>& result, const SEArray* data);
    };

    // Converter for POD types (that can use raw memory copy).
    template<typename T>
    struct MConverter<T, typename TEnableIf<TAnd<TIsPODType<T>, TNot<TIsBaseOf<class ScriptingObject, typename TRemovePointer<T>::Type>>>::Value>::Type>
    {
        SEObject* Box(const T& data, const SEClass* klass)
        {
            return SECore::Object::Box((void*)&data, klass);
        }

        void Unbox(T& result, SEObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, SECore::Object::Unbox(data), sizeof(T));
        }

        void ToManagedArray(SEArray* result, const Span<T>& data)
        {
            Platform::MemoryCopy(SECore::List::GetAddress(result), data.Get(), data.Length() * sizeof(T));
        }

        void ToNativeArray(Span<T>& result, const SEArray* data)
        {
            Platform::MemoryCopy(result.Get(), SECore::List::GetAddress(data), result.Length() * sizeof(T));
        }
    };

    // Converter for String.
    template<>
    struct MConverter<String>
    {
        SEObject* Box(const String& data, const SEClass* klass)
        {
            SEString* str = MUtils::ToString(data);
            return SECore::Object::Box(str, klass);
        }

        void Unbox(String& result, SEObject* data)
        {
            SEString* str = (SEString*)SECore::Object::Unbox(data);
            result = MUtils::ToString(str);
        }

        void ToManagedArray(SEArray* result, const Span<String>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = (SEObject*)MUtils::ToString(data.Get()[i]);
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<String>& result, const SEArray* data)
        {
            SEString** dataPtr = SECore::List::GetAddress<SEString*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                MUtils::ToString(dataPtr[i], result.Get()[i]);
        }
    };

    // Converter for StringAnsi.
    template<>
    struct MConverter<StringAnsi>
    {
        SEObject* Box(const StringAnsi& data, const SEClass* klass)
        {
            return (SEObject*)MUtils::ToString(data);
        }

        void Unbox(StringAnsi& result, SEObject* data)
        {
            result = MUtils::ToStringAnsi((SEString*)data);
        }

        void ToManagedArray(SEArray* result, const Span<StringAnsi>& data)
        {
            if (data.Length() == 0)
                return;
            auto* objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = (SEObject*)MUtils::ToString(data.Get()[i]);
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<StringAnsi>& result, const SEArray* data)
        {
            SEString** dataPtr = SECore::List::GetAddress<SEString*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                MUtils::ToString(dataPtr[i], result.Get()[i]);
        }
    };

    // Converter for StringView.
    template<>
    struct MConverter<StringView>
    {
        SEObject* Box(const StringView& data, const SEClass* klass)
        {
            return (SEObject*)MUtils::ToString(data);
        }

        void Unbox(StringView& result, SEObject* data)
        {
            result = MUtils::ToString((SEString*)data);
        }

        void ToManagedArray(SEArray* result, const Span<StringView>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = (SEObject*)MUtils::ToString(data.Get()[i]);
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<StringView>& result, const SEArray* data)
        {
            SEString** dataPtr = SECore::List::GetAddress<SEString*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                MUtils::ToString(dataPtr[i], result.Get()[i]);
        }
    };

    // Converter for Variant.
    template<>
    struct MConverter<Variant>
    {
        SEObject* Box(const Variant& data, const SEClass* klass)
        {
            return MUtils::BoxVariant(data);
        }

        void Unbox(Variant& result, SEObject* data)
        {
            result = MUtils::UnboxVariant(data);
        }

        void ToManagedArray(SEArray* result, const Span<Variant>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = MUtils::BoxVariant(data[i]);
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<Variant>& result, const SEArray* data)
        {
            SEObject** dataPtr = SECore::List::GetAddress<SEObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = MUtils::UnboxVariant(dataPtr[i]);
        }
    };

    // Converter for Scripting Objects (collection of pointers).
    template<typename T>
    struct MConverter<T*, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
    {
        SEObject* Box(T* data, const SEClass* klass)
        {
            return data ? data->GetOrCreateManagedInstance() : nullptr;
        }

        void Unbox(T*& result, SEObject* data)
        {
            result = (T*)ScriptingObject::ToNative(data);
        }

        void ToManagedArray(SEArray* result, const Span<T*>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data.Get()[i] ? data.Get()[i]->GetOrCreateManagedInstance() : nullptr;
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<T*>& result, const SEArray* data)
        {
            SEObject** dataPtr = SECore::List::GetAddress<SEObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // Converter for Scripting Objects (collection of values).
    template<typename T>
    struct MConverter<T, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
    {
        SEObject* Box(const T& data, const SEClass* klass)
        {
            return data.GetOrCreateManagedInstance();
        }

        void ToManagedArray(SEArray* result, const Span<T>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data.Get()[i].GetOrCreateManagedInstance();
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }
    };

    // Converter for ScriptingObject References.
    template<typename T>
    class ScriptingObjectReference;

    template<typename T>
    struct MConverter<ScriptingObjectReference<T>>
    {
        SEObject* Box(const ScriptingObjectReference<T>& data, const SEClass* klass)
        {
            return data.GetManagedInstance();
        }

        void Unbox(ScriptingObjectReference<T>& result, SEObject* data)
        {
            result = (T*)ScriptingObject::ToNative(data);
        }

        void ToManagedArray(SEArray* result, const Span<ScriptingObjectReference<T>>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data[i].GetManagedInstance();
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<ScriptingObjectReference<T>>& result, const SEArray* data)
        {
            SEObject** dataPtr = SECore::List::GetAddress<SEObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // Converter for Asset References.
    template<typename T>
    class AssetReference;

    template<typename T>
    struct MConverter<AssetReference<T>>
    {
        SEObject* Box(const AssetReference<T>& data, const SEClass* klass)
        {
            return data.GetManagedInstance();
        }

        void Unbox(AssetReference<T>& result, SEObject* data)
        {
            result = (T*)ScriptingObject::ToNative(data);
        }

        void ToManagedArray(SEArray* result, const Span<AssetReference<T>>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data[i].GetManagedInstance();
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<AssetReference<T>>& result, const SEArray* data)
        {
            SEObject** dataPtr = SECore::List::GetAddress<SEObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // TODO: use MarshalAs=Guid on SoftAssetReference to pass guid over bindings and not load asset in glue code
    template<typename T>
    class SoftAssetReference;
    template<typename T>
    struct MConverter<SoftAssetReference<T>>
    {
        void ToManagedArray(SEArray* result, const Span<SoftAssetReference<T>>& data)
        {
            if (data.Length() == 0)
                return;
            SEObject** objects = (SEObject**)PlatformAllocator::Allocate(data.Length() * sizeof(SEObject*));
            for (int32 i = 0; i < data.Length(); i++)
                objects[i] = data[i].GetManagedInstance();
            SECore::GC::WriteArrayRef(result, Span<SEObject*>(objects, data.Length()));
            PlatformAllocator::Free(objects);
        }

        void ToNativeArray(Span<SoftAssetReference<T>>& result, const SEArray* data)
        {
            SEObject** dataPtr = SECore::List::GetAddress<SEObject*>(data);
            for (int32 i = 0; i < result.Length(); i++)
                result.Get()[i] = (T*)ScriptingObject::ToNative(dataPtr[i]);
        }
    };

    // Converter for List.
    template<typename T>
    struct MConverter<List<T>>
    {
        SEObject* Box(const List<T>& data, const SEClass* klass)
        {
            if (!klass)
                return nullptr;
            SEArray* result = SECore::List::New(klass->GetElementClass(), data.Count());
            MConverter<T> converter;
            converter.ToManagedArray(result, Span<T>(data.Get(), data.Count()));
            return (SEObject*)result;
        }

        void Unbox(List<T>& result, SEObject* data)
        {
            SEArray* array = SECore::List::Unbox(data);
            const int32 length = array ? SECore::List::GetLength(array) : 0;
            result.Resize(length);
            MConverter<T> converter;
            Span<T> resultSpan(result.Get(), length);
            converter.ToNativeArray(resultSpan, array);
        }
    };

    namespace MUtils
    {
        // Outputs the full typename for the type of the specified object.
        extern SE_API_RUNTIME const StringAnsi& GetClassFullname(SEObject* obj);

        // Returns the class of the provided object.
        extern SE_API_RUNTIME SEClass* GetClass(SEObject* object);

        // Returns the class of the provided type.
        extern SE_API_RUNTIME SEClass* GetClass(SETypeObject* type);

        // Returns the class of the provided VariantType value.
        extern SE_API_RUNTIME SEClass* GetClass(const VariantTypeHandle& value);

        // Returns the class of the provided Variant value.
        extern SE_API_RUNTIME SEClass* GetClass(const Variant& value);

        // Returns the type of the provided object.
        extern SE_API_RUNTIME SETypeObject* GetType(SEObject* object);

        // Returns the type of the provided class.
        extern SE_API_RUNTIME SETypeObject* GetType(SEClass* klass);

        /// <summary>
        /// Boxes the native value into the managed object.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="valueClass">The value type class.</param>
        template<class T>
        SEObject* Box(const T& value, const SEClass* valueClass)
        {
            MConverter<T> converter;
            return converter.Box(value, valueClass);
        }

        /// <summary>
        /// Unboxes SEObject to the native value of the given type.
        /// </summary>
        template<class T>
        T Unbox(SEObject* object)
        {
            MConverter<T> converter;
            T result;
            converter.Unbox(result, object);
            return result;
        }

        /// <summary>
        /// Links managed array data to the unmanaged BytesContainer.
        /// </summary>
        /// <param name="arrayObj">The array object.</param>
        /// <returns>The result data container with linked array data bytes (not copied).</returns>
        extern SE_API_RUNTIME BytesContainer LinkArray(SEArray* arrayObj);

        /// <summary>
        /// Allocates new managed array of data and copies contents from given native array.
        /// </summary>
        /// <param name="data">The array object.</param>
        /// <param name="valueClass">The array values type class.</param>
        /// <returns>The output array.</returns>
        template<typename T>
        SEArray* ToArray(const Span<T>& data, const SEClass* valueClass)
        {
            if (!valueClass)
                return nullptr;
            SEArray* result = SECore::List::New(valueClass, data.Length());
            MConverter<T> converter;
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
        FORCE_INLINE SEArray* ToArray(const List<T, AllocationType>& data, const SEClass* valueClass)
        {
            return MUtils::ToArray(Span<T>(data.Get(), data.Count()), valueClass);
        }

        /// <summary>
        /// Converts the managed array into native array container object.
        /// </summary>
        /// <param name="arrayObj">The managed array object.</param>
        /// <returns>The output array.</returns>
        template<typename T, typename AllocationType = HeapAllocation>
        List<T, AllocationType> ToArray(SEArray* arrayObj)
        {
            List<T, AllocationType> result;
            const int32 length = arrayObj ? SECore::List::GetLength(arrayObj) : 0;
            result.Resize(length);
            MConverter<T> converter;
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
        Span<T> ToSpan(SEArray* arrayObj)
        {
            T* ptr = (T*)SECore::List::GetAddress(arrayObj);
            const int32 length = arrayObj ? SECore::List::GetLength(arrayObj) : 0;
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
        void ToArray(SEArray* arrayObj, DataContainer<T>& result)
        {
            const int32 length = arrayObj ? SECore::List::GetLength(arrayObj) : 0;
            if (length == 0)
            {
                result.Release();
                return;
            }
            T* bytesRaw = (T*)SECore::List::GetAddress(arrayObj);
            result.Link(bytesRaw, length);
        }

        /// <summary>
        /// Allocates new managed bytes array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE SEArray* ToArray(const Span<byte>& data)
        {
            return ToArray(data, SECore::TypeCache::Byte);
        }

        /// <summary>
        /// Allocates new managed bytes array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE SEArray* ToArray(List<byte>& data)
        {
            return ToArray(Span<byte>(data.Get(), data.Count()), SECore::TypeCache::Byte);
        }

        /// <summary>
        /// Allocates new managed strings array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE SEArray* ToArray(const Span<String>& data)
        {
            return ToArray(data, SECore::TypeCache::String);
        }

        /// <summary>
        /// Allocates new managed strings array and copies data from the given unmanaged data container.
        /// </summary>
        /// <param name="data">The input data.</param>
        /// <returns>The output array.</returns>
        FORCE_INLINE SEArray* ToArray(const List<String>& data)
        {
            return ToArray(Span<String>(data.Get(), data.Count()), SECore::TypeCache::String);
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

        extern void* VariantToManagedArgPtr(Variant& value, SEType* type, bool& failed);

        extern SEObject* ToManaged(const Version& value);
        extern Version ToNative(SEObject* value);
    };
}
