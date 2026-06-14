
#include "CLRUtils.h"

#include "CLRClass.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Runtime/Scripting/Scripting.h"
#include "Runtime/Scripting/ScriptingObject.h"
#include "Runtime/Scripting/Internal/ManagedDictionary.h"

namespace SE
{

namespace
{
    // typeName in format System.Collections.Generic.Dictionary`2[KeyType,ValueType]
    void GetDictionaryKeyValueTypes(const StringAnsiView& typeName, CLRClass*& keyClass, CLRClass*& valueClass)
    {
        const int32 keyStart = typeName.Find('[');
        const int32 keyEnd = typeName.Find(',');
        const int32 valueEnd = typeName.Find(']');
        const StringAnsiView keyTypename(*typeName + keyStart + 1, keyEnd - keyStart - 1);
        const StringAnsiView valueTypename(*typeName + keyEnd + 1, valueEnd - keyEnd - 1);
        keyClass = Scripting::FindClass(keyTypename);
        valueClass = Scripting::FindClass(valueTypename);
    }
}

StringView CLRUtils::ToString(CLRString* str)
{
    if (str == nullptr)
        return StringView::Empty;
    return CLRCore::String::GetChars(str);
}

StringAnsi CLRUtils::ToStringAnsi(CLRString* str)
{
    if (str == nullptr)
        return StringAnsi::Empty;
    return StringAnsi(CLRCore::String::GetChars(str));
}

void CLRUtils::ToString(CLRString* str, String& result)
{
    if (str)
    {
        const StringView chars = CLRCore::String::GetChars(str);
        result.Set(chars.Get(), chars.Length());
    }
    else
        result.Clear();
}

void CLRUtils::ToString(CLRString* str, StringView& result)
{
    if (str)
        result = CLRCore::String::GetChars(str);
    else
        result = StringView();
}

void CLRUtils::ToString(CLRString* str, Variant& result)
{
    result.SetString(str ? CLRCore::String::GetChars(str) : StringView::Empty);
}

void CLRUtils::ToString(CLRString* str, StringAnsi& result)
{
    if (str)
    {
        const StringView chars = CLRCore::String::GetChars(str);
        result.SetChars(chars.Get(), chars.Length());
    }
    else
        result.Clear();
}

CLRString* CLRUtils::ToString(const char* str)
{
    if (str == nullptr || *str == 0)
        return CLRCore::String::GetEmpty();
    return CLRCore::String::New(str, StringUtils::Length(str));
}

CLRString* CLRUtils::ToString(const StringAnsi& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return CLRCore::String::GetEmpty();
    return CLRCore::String::New(str.Get(), len);
}

CLRString* CLRUtils::ToString(const String& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return CLRCore::String::GetEmpty();
    return CLRCore::String::New(str.Get(), len);
}

CLRString* CLRUtils::ToString(const String& str, CLRDomain* domain)
{
    const int32 len = str.Length();
    if (len <= 0)
        return CLRCore::String::GetEmpty(domain);
    return CLRCore::String::New(str.Get(), len, domain);
}

CLRString* CLRUtils::ToString(const StringAnsiView& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return CLRCore::String::GetEmpty();
    return CLRCore::String::New(str.Get(), str.Length());
}

CLRString* CLRUtils::ToString(const StringView& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return CLRCore::String::GetEmpty();
    return CLRCore::String::New(str.Get(), len);
}

CLRString* CLRUtils::ToString(const StringView& str, CLRDomain* domain)
{
    const int32 len = str.Length();
    if (len <= 0)
        return CLRCore::String::GetEmpty(domain);
    return CLRCore::String::New(str.Get(), len, domain);
}

ScriptingTypeHandle CLRUtils::UnboxScriptingTypeHandle(CLRTypeObject* value)
{
    CLRClass* klass = GetClass(value);
    if (!klass)
        return ScriptingTypeHandle();
    const StringAnsi& typeName = klass->GetFullName();
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (!typeHandle)
        LOG_WARNING("Scripting", "Unknown scripting type {}", typeName);
    return typeHandle;
}

CLRTypeObject* CLRUtils::BoxScriptingTypeHandle(const ScriptingTypeHandle& value)
{
    CLRTypeObject* result = nullptr;
    if (value)
    {
        CLRType* mType = value.GetType().ManagedClass->GetType();
        result = mType;
    }
    return result;
}

VariantTypeHandle CLRUtils::UnboxVariantType(CLRType* type)
{
    if (!type)
    {
        return VariantTypeHandle(VariantTypes::Null);
    }

    CLRClass* klass = CLRCore::Type::GetClass(type);
    CLRTypes types = CLRCore::Type::GetType(type);

    // Fast type detection for in-built types
    switch (types)
    {
    case CLRTypes::Void:
        return VariantTypeHandle(VariantTypes::Void);
    case CLRTypes::Boolean:
        return VariantTypeHandle(VariantTypes::Bool);
    case CLRTypes::I1:
    case CLRTypes::I2:
        return VariantTypeHandle(VariantTypes::Int16);
    case CLRTypes::U1:
    case CLRTypes::U2:
        return VariantTypeHandle(VariantTypes::Uint16);
    case CLRTypes::I4:
    case CLRTypes::Char:
        return VariantTypeHandle(VariantTypes::Int);
    case CLRTypes::U4:
        return VariantTypeHandle(VariantTypes::Uint);
    case CLRTypes::I8:
        return VariantTypeHandle(VariantTypes::Int64);
    case CLRTypes::U8:
        return VariantTypeHandle(VariantTypes::Uint64);
    case CLRTypes::R4:
        return VariantTypeHandle(VariantTypes::Float);
    case CLRTypes::R8:
        return VariantTypeHandle(VariantTypes::Double);
    case CLRTypes::String:
        return VariantTypeHandle(VariantTypes::String);
    case CLRTypes::Ptr:
        return VariantTypeHandle(VariantTypes::Pointer);
    case CLRTypes::ValueType:
        if (klass == CLRCore::TypeCache::UID)
            return VariantTypeHandle(VariantTypes::UID);
        if (klass == CLRCore::TypeCache::Vector2)
            return VariantTypeHandle(VariantTypes::Vector2);
        if (klass == CLRCore::TypeCache::Vector3)
            return VariantTypeHandle(VariantTypes::Vector3);
        if (klass == CLRCore::TypeCache::Vector4)
            return VariantTypeHandle(VariantTypes::Vector4);
        if (klass == CLRCore::TypeCache::Int2)
            return VariantTypeHandle(VariantTypes::Int2);
        if (klass == CLRCore::TypeCache::Int3)
            return VariantTypeHandle(VariantTypes::Int3);
        if (klass == CLRCore::TypeCache::Int4)
            return VariantTypeHandle(VariantTypes::Int4);
        if (klass == CLRCore::TypeCache::Float2)
            return VariantTypeHandle(VariantTypes::Float2);
        if (klass == CLRCore::TypeCache::Float3)
            return VariantTypeHandle(VariantTypes::Float3);
        if (klass == CLRCore::TypeCache::Float4)
            return VariantTypeHandle(VariantTypes::Float4);
        if (klass == CLRCore::TypeCache::Double2)
            return VariantTypeHandle(VariantTypes::Double2);
        if (klass == CLRCore::TypeCache::Double3)
            return VariantTypeHandle(VariantTypes::Double3);
        if (klass == CLRCore::TypeCache::Double4)
            return VariantTypeHandle(VariantTypes::Double4);
        if (klass == CLRCore::TypeCache::Color)
            return VariantTypeHandle(VariantTypes::Color);
        if (klass == CLRCore::TypeCache::BoundingBox)
            return VariantTypeHandle(VariantTypes::BoundingBox);
        if (klass == CLRCore::TypeCache::Quaternion)
            return VariantTypeHandle(VariantTypes::Quaternion);
        if (klass == CLRCore::TypeCache::Transform)
            return VariantTypeHandle(VariantTypes::Transform);
        if (klass == CLRCore::TypeCache::BoundingSphere)
            return VariantTypeHandle(VariantTypes::BoundingSphere);
        if (klass == CLRCore::TypeCache::Rectangle)
            return VariantTypeHandle(VariantTypes::Rectangle);
        if (klass == CLRCore::TypeCache::Matrix)
            return VariantTypeHandle(VariantTypes::Matrix);
        break;
    case CLRTypes::Object:
        return VariantTypeHandle(VariantTypes::ManagedObject);
    case CLRTypes::SzArray:
        if (klass == CLRCore::Array::GetClass(CLRCore::TypeCache::Byte))
        {
            return VariantTypeHandle(VariantTypes::Blob);
        }
        break;
    }

    // Get actual typename for full type info
    if (!klass)
        return VariantTypeHandle(VariantTypes::Null);
    const StringAnsiView fullname = klass->GetFullName();
    switch (types)
    {
    case CLRTypes::SzArray:
    case CLRTypes::List:
        return VariantTypeHandle(VariantTypes::Array, fullname);
    case CLRTypes::Enum:
        return VariantTypeHandle(VariantTypes::Enum, fullname);
    case CLRTypes::ValueType:
        return VariantTypeHandle(VariantTypes::Structure, fullname);
    }
    if (klass == CLRCore::TypeCache::Type)
        return VariantTypeHandle(VariantTypes::Typename);
    if (klass->IsSubClassOf(Asset::GetScriptingClass()))
    {
        if (klass == Asset::GetScriptingClass())
            return VariantTypeHandle(VariantTypes::Asset);
        return VariantTypeHandle(VariantTypes::Asset, fullname);
    }
    if (klass->IsSubClassOf(ScriptingObject::GetScriptingClass()))
    {
        if (klass == ScriptingObject::GetScriptingClass())
            return VariantTypeHandle(VariantTypes::Object);
        return VariantTypeHandle(VariantTypes::Object, fullname);
    }
    // TODO: support any dictionary unboxing

    LOG_ERROR("Scripting", "Invalid managed type to unbox {0}", String(fullname));
    return VariantTypeHandle();
}

CLRTypeObject* CLRUtils::BoxVariantType(const VariantTypeHandle& value)
{
    if (value.Type == VariantTypes::Null)
        return nullptr;
    CLRClass* klass = GetClass(value);
    if (!klass)
    {
        LOG_ERROR("Scripting", "Invalid native type to box {0}", value.ToString());
        return nullptr;
    }
    CLRType* mType = klass->GetType();
    return mType;
}

Variant CLRUtils::UnboxVariant(CLRObject* value)
{
    if (value == nullptr)
    {
        return Variant::Null;
    }
    CLRClass* klass = CLRCore::Object::GetClass(value);

    CLRType* mType = klass->GetType();
    const CLRTypes mTypes = CLRCore::Type::GetType(mType);
    void* unboxed = CLRCore::Object::Unbox(value);

    // Fast type detection for in-built types
    switch (mTypes)
    {
    case CLRTypes::Void:
        return Variant(VariantTypeHandle(VariantTypes::Void));
    case CLRTypes::Boolean:
        return *static_cast<bool*>(unboxed);
    case CLRTypes::I1:
        return *static_cast<int8*>(unboxed);
    case CLRTypes::U1:
        return *static_cast<uint8*>(unboxed);
    case CLRTypes::I2:
        return *static_cast<int16*>(unboxed);
    case CLRTypes::U2:
        return *static_cast<uint16*>(unboxed);
    case CLRTypes::Char:
        return *static_cast<Char*>(unboxed);
    case CLRTypes::I4:
        return *static_cast<int32*>(unboxed);
    case CLRTypes::U4:
        return *static_cast<uint32*>(unboxed);
    case CLRTypes::I8:
        return *static_cast<int64*>(unboxed);
    case CLRTypes::U8:
        return *static_cast<uint64*>(unboxed);
    case CLRTypes::R4:
        return *static_cast<float*>(unboxed);
    case CLRTypes::R8:
        return *static_cast<double*>(unboxed);
    case CLRTypes::String:
        return Variant(CLRUtils::ToString((CLRString*)value));
    case CLRTypes::Ptr:
        return *static_cast<void**>(unboxed);
    case CLRTypes::ValueType:
        if (klass == CLRCore::TypeCache::UID)
            return Variant(*static_cast<UID*>(unboxed));
        if (klass == CLRCore::TypeCache::Vector2)
            return *static_cast<Float2*>(unboxed);
        if (klass == CLRCore::TypeCache::Vector3)
            return *static_cast<Float3*>(unboxed);
        if (klass == CLRCore::TypeCache::Vector4)
            return *static_cast<Float4*>(unboxed);
        if (klass == CLRCore::TypeCache::Int2)
            return *static_cast<Int2*>(unboxed);
        if (klass == CLRCore::TypeCache::Int3)
            return *static_cast<Int3*>(unboxed);
        if (klass == CLRCore::TypeCache::Int4)
            return *static_cast<Int4*>(unboxed);
        if (klass == CLRCore::TypeCache::Float2)
            return *static_cast<Float2*>(unboxed);
        if (klass == CLRCore::TypeCache::Float3)
            return *static_cast<Float3*>(unboxed);
        if (klass == CLRCore::TypeCache::Float4)
            return *static_cast<Float4*>(unboxed);
        if (klass == CLRCore::TypeCache::Double2)
            return *static_cast<Double2*>(unboxed);
        if (klass == CLRCore::TypeCache::Double3)
            return *static_cast<Double3*>(unboxed);
        if (klass == CLRCore::TypeCache::Double4)
            return *static_cast<Double4*>(unboxed);
        if (klass == CLRCore::TypeCache::Color)
            return *static_cast<Color*>(unboxed);
        if (klass == CLRCore::TypeCache::BoundingBox)
            return Variant(*static_cast<BoundingBox*>(unboxed));
        if (klass == CLRCore::TypeCache::Quaternion)
            return *static_cast<Quaternion*>(unboxed);
        if (klass == CLRCore::TypeCache::Transform)
            return Variant(*static_cast<Transform*>(unboxed));
        if (klass == CLRCore::TypeCache::BoundingSphere)
            return *static_cast<BoundingSphere*>(unboxed);
        if (klass == CLRCore::TypeCache::Rectangle)
            return *static_cast<Rectangle*>(unboxed);
        if (klass == CLRCore::TypeCache::Matrix)
            return Variant(*reinterpret_cast<Matrix*>(unboxed));
        break;
    case CLRTypes::SzArray:
    case CLRTypes::List:
    {
        void* ptr = CLRCore::Array::GetAddress((CLRArray*)value);
        const CLRClass* arrayClass = klass == CLRCore::TypeCache::ManagedArrayClass ? CLRCore::Array::GetArrayClass((CLRArray*)value) : klass;
        const CLRClass* elementClass = arrayClass->GetElementClass();
        if (elementClass == CLRCore::TypeCache::Byte)
        {
            Variant v;
            v.SetBlob(ptr, CLRCore::Array::GetLength((CLRArray*)value));
            return v;
        }
        const StringAnsiView fullname = arrayClass->GetFullName();
        Variant v;
        v.SetType(MoveTemp(VariantTypeHandle(VariantTypes::Array, fullname)));
        auto& array = v.AsArray();
        array.Resize(CLRCore::Array::GetLength((CLRArray*)value));
        const StringAnsiView elementTypename(*fullname, fullname.Length() - 2);
        const int32 elementSize = elementClass->GetInstanceSize();
        if (elementClass->IsEnum())
        {
            // Array of Enums
            for (int32 i = 0; i < array.Count(); i++)
            {
                array[i].SetType(VariantTypeHandle(VariantTypes::Enum, elementTypename));
                Platform::MemoryCopy(&array[i].AsUint64, (byte*)ptr + elementSize * i, elementSize);
            }
        }
        else if (elementClass->IsValueType())
        {
            // Array of Structures
            VariantTypeHandle elementType = UnboxVariantType(elementClass->GetType());
            switch (elementType.Type)
            {
            case VariantTypes::Bool:
            case VariantTypes::Int:
            case VariantTypes::Uint:
            case VariantTypes::Int64:
            case VariantTypes::Uint64:
            case VariantTypes::Float:
            case VariantTypes::Double:
            case VariantTypes::Float2:
            case VariantTypes::Float3:
            case VariantTypes::Float4:
            case VariantTypes::Color:
            case VariantTypes::UID:
            case VariantTypes::Quaternion:
            case VariantTypes::Rectangle:
            case VariantTypes::Int2:
            case VariantTypes::Int3:
            case VariantTypes::Int4:
            case VariantTypes::Int16:
            case VariantTypes::Uint16:
            case VariantTypes::Double2:
            case VariantTypes::Double3:
#if !USE_LARGE_WORLDS
            case VariantTypes::BoundingSphere:
            case VariantTypes::BoundingBox:
            case VariantTypes::Ray:
#endif
                // Optimized unboxing of raw data type
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto& a = array[i];
                    a.SetType(elementType);
                    Platform::MemoryCopy(&a.AsData, (byte*)ptr + elementSize * i, elementSize);
                }
                break;
            case VariantTypes::Transform:
            case VariantTypes::Matrix:
            case VariantTypes::Double4:
#if USE_LARGE_WORLDS
            case VariantTypes::BoundingSphere:
            case VariantTypes::BoundingBox:
            case VariantTypes::Ray:
#endif
                // Optimized unboxing of raw data type
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto& a = array[i];
                    a.SetType(elementType);
                    Platform::MemoryCopy(a.AsBlob.Data, (byte*)ptr + elementSize * i, elementSize);
                }
                break;
            case VariantTypes::Structure:
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(elementType.TypeName);
                if (typeHandle)
                {
                    // Unbox array of structures
                    const ScriptingType& type = typeHandle.GetType();
                    ASSERT(type.Type == ScriptingTypes::Structure);
                    // TODO: optimize this for large arrays to prevent multiple AllocStructure calls in Variant::SetType by using computed struct type
                    for (int32 i = 0; i < array.Count(); i++)
                    {
                        auto& a = array[i];
                        a.SetType(elementType);
                        void* managed = (byte*)ptr + elementSize * i;
                        // TODO: optimize structures unboxing to not require SEObject* but raw managed value data to prevent additional boxing here
                        CLRObject* boxed = CLRCore::Object::New(elementClass);
                        Platform::MemoryCopy(CLRCore::Object::Unbox(boxed), managed, elementSize);
                        type.Struct.Unbox(a.AsBlob.Data, boxed);
                    }
                    break;
                }
                LOG_ERROR("Scripting", "Invalid type to unbox {0}", v.Type.ToString());
                break;
            }
            default:
                LOG_ERROR("Scripting", "Invalid type to unbox {0}", v.Type.ToString());
                break;
            }
        }
        else
        {
            // Array of Objects
            for (int32 i = 0; i < array.Count(); i++)
                array[i] = UnboxVariant(((CLRObject**)ptr)[i]);
        }
        return v;
    }
    case CLRTypes::GenericInst:
    {
        if (klass->GetName() == "Dictionary`2" && klass->GetNamespace() == "System.Collections.Generic")
        {
            // Dictionary
            ManagedDictionary managed(value);
            CLRArray* managedKeys = managed.GetKeys();
            int32 length = managedKeys ? CLRCore::Array::GetLength(managedKeys) : 0;
            Dictionary<Variant, Variant> native;
            native.EnsureCapacity(length);
            CLRObject** managedKeysPtr = CLRCore::Array::GetAddress<CLRObject*>(managedKeys);
            for (int32 i = 0; i < length; i++)
            {
                CLRObject* keyManaged = managedKeysPtr[i];
                CLRObject* valueManaged = managed.GetValue(keyManaged);
                native.Add(UnboxVariant(keyManaged), UnboxVariant(valueManaged));
            }
            Variant v(MoveTemp(native));
            v.Type.SetTypeName(klass->GetFullName());
            return v;
        }
        break;
    }
    }

    if (klass->IsSubClassOf(Asset::GetScriptingClass()))
        return static_cast<Asset*>(ScriptingObject::ToNative(value));
    if (klass->IsSubClassOf(ScriptingObject::GetScriptingClass()))
        return ScriptingObject::ToNative(value);
    if (klass->IsEnum())
    {
        const StringAnsiView fullname = klass->GetFullName();
        Variant v;
        v.Type = MoveTemp(VariantTypeHandle(VariantTypes::Enum, fullname));
        // TODO: what about 64-bit enum? use enum size with memcpy
        v.AsUint64 = *static_cast<uint32*>(CLRCore::Object::Unbox(value));
        return v;
    }
    if (klass->IsValueType())
    {
        const StringAnsiView fullname = klass->GetFullName();
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(fullname);
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            Variant v;
            v.Type = MoveTemp(VariantTypeHandle(VariantTypes::Structure, fullname));
            v.AsBlob.Data = PlatformAllocator::Allocate(type.Size);
            v.AsBlob.Length = type.Size;
            type.Struct.Ctor(v.AsBlob.Data);
            type.Struct.Unbox(v.AsBlob.Data, value);
            return v;
        }
        return Variant(value);
    }

    return Variant(value);
}

CLRObject* CLRUtils::BoxVariant(const Variant& value)
{
    switch (value.Type.Type)
    {
    case VariantTypes::Null:
    case VariantTypes::Void:
        return nullptr;
    case VariantTypes::Bool:
        return CLRCore::Object::Box((void*)&value.AsBool, CLRCore::TypeCache::Boolean);
    case VariantTypes::Int16:
        return CLRCore::Object::Box((void*)&value.AsInt16, CLRCore::TypeCache::Int16);
    case VariantTypes::Uint16:
        return CLRCore::Object::Box((void*)&value.AsUint16, CLRCore::TypeCache::UInt16);
    case VariantTypes::Int:
        return CLRCore::Object::Box((void*)&value.AsInt, CLRCore::TypeCache::Int32);
    case VariantTypes::Uint:
        return CLRCore::Object::Box((void*)&value.AsUint, CLRCore::TypeCache::UInt32);
    case VariantTypes::Int64:
        return CLRCore::Object::Box((void*)&value.AsInt64, CLRCore::TypeCache::Int64);
    case VariantTypes::Uint64:
        return CLRCore::Object::Box((void*)&value.AsUint64, CLRCore::TypeCache::UInt64);
    case VariantTypes::Float:
        return CLRCore::Object::Box((void*)&value.AsFloat, CLRCore::TypeCache::Single);
    case VariantTypes::Double:
        return CLRCore::Object::Box((void*)&value.AsDouble, CLRCore::TypeCache::Double);
    case VariantTypes::Float2:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Float2);
    case VariantTypes::Float3:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Float3);
    case VariantTypes::Float4:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Float4);
    case VariantTypes::Double2:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Double2);
    case VariantTypes::Double3:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Double3);
    case VariantTypes::Double4:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Double4);
    case VariantTypes::Color:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Color);
    case VariantTypes::UID:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::UID);
    case VariantTypes::String:
        return (CLRObject*)CLRUtils::ToString((StringView)value);
    case VariantTypes::Quaternion:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Quaternion);
    case VariantTypes::BoundingSphere:
        return CLRCore::Object::Box((void*)&value.AsBoundingSphere(), CLRCore::TypeCache::BoundingSphere);
    case VariantTypes::Rectangle:
        return CLRCore::Object::Box((void*)&value.AsData, CLRCore::TypeCache::Rectangle);
    case VariantTypes::Pointer:
        return CLRCore::Object::Box((void*)&value.AsPointer, CLRCore::TypeCache::IntPtr);
    case VariantTypes::Ray:
        return CLRCore::Object::Box((void*)&value.AsRay(), CLRCore::TypeCache::Ray);
    case VariantTypes::BoundingBox:
        return CLRCore::Object::Box((void*)&value.AsBoundingBox(), CLRCore::TypeCache::BoundingBox);
    case VariantTypes::Transform:
        return CLRCore::Object::Box(value.AsBlob.Data, CLRCore::TypeCache::Transform);
    case VariantTypes::Matrix:
        return CLRCore::Object::Box(value.AsBlob.Data, CLRCore::TypeCache::Matrix);
    case VariantTypes::Blob:
        return (CLRObject*)ToArray(Span<byte>((const byte*)value.AsBlob.Data, value.AsBlob.Length));
    case VariantTypes::Object:
        return value.AsObject ? value.AsObject->GetOrCreateManagedInstance() : nullptr;
    case VariantTypes::Asset:
        return value.AsAsset ? value.AsAsset->GetOrCreateManagedInstance() : nullptr;
    case VariantTypes::Array:
    {
        CLRArray* managed;
        const auto& array = value.AsArray();
        if (value.Type.TypeName)
        {
            const StringAnsiView elementTypename(value.Type.TypeName, StringUtils::Length(value.Type.TypeName) - 2);
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(elementTypename);
            CLRClass* elementClass;
            if (typeHandle && typeHandle.GetType().ManagedClass)
                elementClass = typeHandle.GetType().ManagedClass;
            else
                elementClass = Scripting::FindClass(elementTypename);
            if (!elementClass)
            {
                LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
                return nullptr;
            }
            const int32 elementSize = elementClass->GetInstanceSize();
            managed = CLRCore::Array::New(elementClass, array.Count());
            if (elementClass->IsEnum())
            {
                // Array of Enums
                byte* managedPtr = (byte*)CLRCore::Array::GetAddress(managed);
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto data = (uint64)array[i];
                    Platform::MemoryCopy(managedPtr + elementSize * i, &data, elementSize);
                }
            }
            else if (elementClass->IsValueType())
            {
                // Array of Structures
                const VariantTypeHandle elementType = UnboxVariantType(elementClass->GetType());
                byte* managedPtr = (byte*)CLRCore::Array::GetAddress(managed);
                switch (elementType.Type)
                {
                case VariantTypes::Bool:
                case VariantTypes::Int:
                case VariantTypes::Uint:
                case VariantTypes::Int64:
                case VariantTypes::Uint64:
                case VariantTypes::Float:
                case VariantTypes::Double:
                case VariantTypes::Float2:
                case VariantTypes::Float3:
                case VariantTypes::Float4:
                case VariantTypes::Color:
                case VariantTypes::UID:
                case VariantTypes::Quaternion:
                case VariantTypes::Rectangle:
                case VariantTypes::Int2:
                case VariantTypes::Int3:
                case VariantTypes::Int4:
                case VariantTypes::Int16:
                case VariantTypes::Uint16:
                case VariantTypes::Double2:
                case VariantTypes::Double3:
#if !USE_LARGE_WORLDS
                case VariantTypes::BoundingSphere:
                case VariantTypes::BoundingBox:
                case VariantTypes::Ray:
#endif
                    // Optimized boxing of raw data type
                    for (int32 i = 0; i < array.Count(); i++)
                        Platform::MemoryCopy(managedPtr + elementSize * i, &array[i].AsData, elementSize);
                    break;
                case VariantTypes::Transform:
                case VariantTypes::Matrix:
                case VariantTypes::Double4:
#if USE_LARGE_WORLDS
                case VariantType::BoundingSphere:
                case VariantType::BoundingBox:
                case VariantType::Ray:
#endif
                    // Optimized boxing of raw data type
                    for (int32 i = 0; i < array.Count(); i++)
                        Platform::MemoryCopy(managedPtr + elementSize * i, array[i].AsBlob.Data, elementSize);
                    break;
                case VariantTypes::Structure:
                    if (typeHandle)
                    {
                        const ScriptingType& type = typeHandle.GetType();
                        ASSERT(type.Type == ScriptingTypes::Structure);
                        for (int32 i = 0; i < array.Count(); i++)
                        {
                            // TODO: optimize structures boxing to not return SEObject* but use raw managed object to prevent additional boxing here
                            CLRObject* boxed = type.Struct.Box(array[i].AsBlob.Data);
                            Platform::MemoryCopy(managedPtr + elementSize * i, CLRCore::Object::Unbox(boxed), elementSize);
                        }
                        break;
                    }
                    LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
                    break;
                default:
                    LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
                    break;
                }
            }
            else
            {
                // Array of Objects
                for (int32 i = 0; i < array.Count(); i++)
                    CLRCore::GC::WriteArrayRef(managed, BoxVariant(array[i]), i);
            }
        }
        else
        {
            // object[]
            managed = CLRCore::Array::New(CLRCore::TypeCache::Object, array.Count());
            for (int32 i = 0; i < array.Count(); i++)
                CLRCore::GC::WriteArrayRef(managed, BoxVariant(array[i]), i);
        }
        return (CLRObject*)managed;
    }
    case VariantTypes::Dictionary:
    {
        // Get dictionary key and value types
        CLRClass *keyClass, *valueClass;
        GetDictionaryKeyValueTypes(value.Type.GetTypeName(), keyClass, valueClass);
        if (!keyClass || !valueClass)
        {
            LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
            return nullptr;
        }

        // Allocate managed dictionary
        ManagedDictionary managed = ManagedDictionary::New(keyClass->GetType(), valueClass->GetType());
        if (!managed.Instance)
            return nullptr;

        // Add native keys and values
        const auto& dictionary = *value.AsDictionary;
        for (const auto& e : dictionary)
        {
            managed.Add(BoxVariant(e.Key), BoxVariant(e.Value));
        }

        return managed.Instance;
    }
    case VariantTypes::Structure:
    {
        if (value.AsBlob.Data == nullptr)
            return nullptr;
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(value.Type.TypeName));
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            return type.Struct.Box(value.AsBlob.Data);
        }
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
        return nullptr;
    }
    case VariantTypes::Enum:
    {
        const auto klass = Scripting::FindClass(StringAnsiView(value.Type.TypeName));
        if (klass)
            return CLRCore::Object::Box((void*)&value.AsUint64, klass);
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
        return nullptr;
    }
    case VariantTypes::ManagedObject:
        return value.AsUint64 ? CLRCore::GCHandle::GetTarget(value.AsUint64) : nullptr;
    case VariantTypes::Typename:
    {
        const auto klass = Scripting::FindClass((StringAnsiView)value);
        if (klass)
            return (CLRObject*)GetType(klass);
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.ToString());
        return nullptr;
    }
    default:
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type.ToString());
        return nullptr;
    }
}

const StringAnsi& CLRUtils::GetClassFullname(CLRObject* obj)
{
    if (obj)
    {
        CLRClass* mClass = CLRCore::Object::GetClass(obj);
        return mClass->GetFullName();
    }
    return StringAnsi::Empty;
}

CLRClass* CLRUtils::GetClass(CLRTypeObject* type)
{
    if (type == nullptr)
    {
        return nullptr;
    }
    CLRType* mType = {type};// INTERNAL_TYPE_OBJECT_GET(type);
    return CLRCore::Type::GetClass(mType);
}

CLRClass* CLRUtils::GetClass(const VariantTypeHandle& value)
{
    auto mclass = Scripting::FindClass(StringAnsiView(value.TypeName));
    if (mclass)
        return mclass;

    switch (value.Type)
    {
    case VariantTypes::Void:
        return CLRCore::TypeCache::Void;
    case VariantTypes::Bool:
        return CLRCore::TypeCache::Boolean;
    case VariantTypes::Int16:
        return CLRCore::TypeCache::Int16;
    case VariantTypes::Uint16:
        return CLRCore::TypeCache::UInt16;
    case VariantTypes::Int:
        return CLRCore::TypeCache::Int32;
    case VariantTypes::Uint:
        return CLRCore::TypeCache::UInt32;
    case VariantTypes::Int64:
        return CLRCore::TypeCache::Int64;
    case VariantTypes::Uint64:
        return CLRCore::TypeCache::UInt64;
    case VariantTypes::Float:
        return CLRCore::TypeCache::Single;
    case VariantTypes::Double:
        return CLRCore::TypeCache::Double;
    case VariantTypes::Pointer:
        return CLRCore::TypeCache::IntPtr;
    case VariantTypes::String:
        return CLRCore::TypeCache::String;
    case VariantTypes::Object:
        return ScriptingObject::GetScriptingClass();
    case VariantTypes::Asset:
        return Asset::GetScriptingClass();
    case VariantTypes::Blob:
        return CLRCore::Array::GetClass(CLRCore::TypeCache::Byte);
    case VariantTypes::Float2:
        return CLRCore::TypeCache::Double2;
    case VariantTypes::Float3:
        return CLRCore::TypeCache::Float3;
    case VariantTypes::Float4:
        return CLRCore::TypeCache::Float4;
    case VariantTypes::Double2:
        return CLRCore::TypeCache::Double2;
    case VariantTypes::Double3:
        return CLRCore::TypeCache::Double3;
    case VariantTypes::Double4:
        return CLRCore::TypeCache::Double4;
    case VariantTypes::Color:
        return CLRCore::TypeCache::Color;
    case VariantTypes::UID:
        return CLRCore::TypeCache::UID;
    case VariantTypes::Typename:
        return CLRCore::TypeCache::Type;
    case VariantTypes::BoundingBox:
        return CLRCore::TypeCache::BoundingBox;
    case VariantTypes::BoundingSphere:
        return CLRCore::TypeCache::BoundingSphere;
    case VariantTypes::Quaternion:
        return CLRCore::TypeCache::Quaternion;
    case VariantTypes::Transform:
        return CLRCore::TypeCache::Transform;
    case VariantTypes::Rectangle:
        return CLRCore::TypeCache::Rectangle;
    case VariantTypes::Ray:
        return CLRCore::TypeCache::Ray;
    case VariantTypes::Matrix:
        return CLRCore::TypeCache::Matrix;
    case VariantTypes::Array:
        if (value.TypeName)
        {
            const StringAnsiView elementTypename(value.TypeName, StringUtils::Length(value.TypeName) - 2);
            mclass = Scripting::FindClass(elementTypename);
            if (mclass)
                return CLRCore::Array::GetClass(mclass);
        }
        return CLRCore::Array::GetClass(CLRCore::TypeCache::Object);
    case VariantTypes::Dictionary:
    {
        CLRClass *keyClass, *valueClass;
        GetDictionaryKeyValueTypes(value.GetTypeName(), keyClass, valueClass);
        if (!keyClass || !valueClass)
        {
            LOG_ERROR("Scripting", "Invalid type to box {0}", value.ToString());
            return nullptr;
        }
        return GetClass(ManagedDictionary::GetClass(keyClass->GetType(), valueClass->GetType()));
    }
    case VariantTypes::ManagedObject:
        return CLRCore::TypeCache::Object;
    default: ;
    }
    return nullptr;
}

CLRClass* CLRUtils::GetClass(const Variant& value)
{
    switch (value.Type.Type)
    {
    case VariantTypes::Void:
        return CLRCore::TypeCache::Void;
    case VariantTypes::Bool:
        return CLRCore::TypeCache::Boolean;
    case VariantTypes::Int16:
        return CLRCore::TypeCache::Int16;
    case VariantTypes::Uint16:
        return CLRCore::TypeCache::UInt16;
    case VariantTypes::Int:
        return CLRCore::TypeCache::Int32;
    case VariantTypes::Uint:
        return CLRCore::TypeCache::UInt32;
    case VariantTypes::Int64:
        return CLRCore::TypeCache::Int64;
    case VariantTypes::Uint64:
        return CLRCore::TypeCache::UInt64;
    case VariantTypes::Float:
        return CLRCore::TypeCache::Single;
    case VariantTypes::Double:
        return CLRCore::TypeCache::Double;
    case VariantTypes::Pointer:
        return CLRCore::TypeCache::IntPtr;
    case VariantTypes::String:
        return CLRCore::TypeCache::String;
    case VariantTypes::Blob:
        return CLRCore::Array::GetClass(CLRCore::TypeCache::Byte);
    case VariantTypes::Float2:
        return CLRCore::TypeCache::Float2;
    case VariantTypes::Float3:
        return CLRCore::TypeCache::Float3;
    case VariantTypes::Float4:
        return CLRCore::TypeCache::Float4;
    case VariantTypes::Double2:
        return CLRCore::TypeCache::Double2;
    case VariantTypes::Double3:
        return CLRCore::TypeCache::Double3;
    case VariantTypes::Double4:
        return CLRCore::TypeCache::Double4;
    case VariantTypes::Color:
        return CLRCore::TypeCache::Color;
    case VariantTypes::UID:
        return CLRCore::TypeCache::UID;
    case VariantTypes::Typename:
        return CLRCore::TypeCache::Type;
    case VariantTypes::BoundingBox:
        return CLRCore::TypeCache::BoundingBox;
    case VariantTypes::BoundingSphere:
        return CLRCore::TypeCache::BoundingSphere;
    case VariantTypes::Quaternion:
        return CLRCore::TypeCache::Quaternion;
    case VariantTypes::Transform:
        return CLRCore::TypeCache::Transform;
    case VariantTypes::Rectangle:
        return CLRCore::TypeCache::Rectangle;
    case VariantTypes::Ray:
        return CLRCore::TypeCache::Ray;
    case VariantTypes::Matrix:
        return CLRCore::TypeCache::Matrix;
    case VariantTypes::Array:
    case VariantTypes::Dictionary:
        break;
    case VariantTypes::Object:
        return value.AsObject ? value.AsObject->GetClass() : nullptr;
    case VariantTypes::Asset:
        return value.AsAsset ? value.AsAsset->GetClass() : nullptr;
    case VariantTypes::Structure:
    case VariantTypes::Enum:
        return Scripting::FindClass(StringAnsiView(value.Type.TypeName));
    case VariantTypes::ManagedObject:
    {
        CLRObject* obj = (CLRObject*)&value;
        if (obj)
        {
            return CLRCore::Object::GetClass(obj);
        }
    }
    default: ;
    }
    return GetClass(value.Type);
}

CLRTypeObject* CLRUtils::GetType(CLRObject* object)
{
    if (!object)
        return nullptr;
    CLRClass* klass = CLRCore::Object::GetClass(object);
    return GetType(klass);
}

CLRTypeObject* CLRUtils::GetType(CLRClass* klass)
{
    if (!klass)
    {
        return nullptr;
    }
    CLRType* type = klass->GetType();
    return type;
}

BytesContainer CLRUtils::LinkArray(CLRArray* arrayObj)
{
    BytesContainer result;
    const int32 length = arrayObj ? CLRCore::Array::GetLength(arrayObj) : 0;
    if (length != 0)
    {
        result.Link((byte*)CLRCore::Array::GetAddress(arrayObj), length);
    }
    return result;
}

void* CLRUtils::VariantToManagedArgPtr(Variant& value, CLRType* type, bool& failed)
{
    // Convert Variant into matching managed type and return pointer to data for the method invocation
    CLRTypes mType = CLRCore::Type::GetType(type);
    switch (mType)
    {
    case CLRTypes::Boolean:
        if (value.Type.Type != VariantTypes::Bool)
            value = (bool)value;
        return &value.AsBool;
    case CLRTypes::Char:
    case CLRTypes::I1:
    case CLRTypes::I2:
        if (value.Type.Type != VariantTypes::Int16)
            value = (int16)value;
        return &value.AsInt16;
    case CLRTypes::I4:
        if (value.Type.Type != VariantTypes::Int)
            value = (int32)value;
        return &value.AsInt;
    case CLRTypes::U1:
    case CLRTypes::U2:
        if (value.Type.Type != VariantTypes::Uint16)
            value = (uint16)value;
        return &value.AsUint16;
    case CLRTypes::U4:
        if (value.Type.Type != VariantTypes::Uint)
            value = (uint32)value;
        return &value.AsUint;
    case CLRTypes::I8:
        if (value.Type.Type != VariantTypes::Int64)
            value = (int64)value;
        return &value.AsInt64;
    case CLRTypes::U8:
        if (value.Type.Type != VariantTypes::Uint64)
            value = (uint64)value;
        return &value.AsUint64;
    case CLRTypes::R4:
        if (value.Type.Type != VariantTypes::Float)
            value = (float)value;
        return &value.AsFloat;
    case CLRTypes::R8:
        if (value.Type.Type != VariantTypes::Double)
            value = (double)value;
        return &value.AsDouble;
    case CLRTypes::String:
        return CLRUtils::ToString((StringView)value);
    case CLRTypes::ValueType:
    {
        CLRClass* klass = CLRCore::Type::GetClass(type);
        if (klass->IsEnum())
        {
            if (value.Type.Type != VariantTypes::Enum)
            {
                value.SetType(VariantTypeHandle(VariantTypes::Enum, klass));
                value.AsUint64 = 0;
            }
            return &value.AsUint64;
        }

#define CASE_IN_BUILD_TYPE(type, access) \
if (klass == CLRCore::TypeCache::type) \
{ \
if (value.Type.Type != VariantTypes::type) \
value = Variant((type)value); \
return value.access; \
}
        /*CASE_IN_BUILD_TYPE(Color, AsData);
        CASE_IN_BUILD_TYPE(Quaternion, AsData);
        CASE_IN_BUILD_TYPE(UID, AsData);
        CASE_IN_BUILD_TYPE(Rectangle, AsData);
        CASE_IN_BUILD_TYPE(Matrix, AsBlob.Data);
        CASE_IN_BUILD_TYPE(Transform, AsBlob.Data);*/
#undef CASE_IN_BUILD_TYPE

#define CASE_IN_BUILD_TYPE(type, access) \
if (klass == CLRCore::TypeCache::type) \
{ \
if (value.Type.Type != VariantTypes::type) \
value = Variant((type)value); \
return (void*)&value.access(); \
}

        /*CASE_IN_BUILD_TYPE(Float2, AsVector2);
        CASE_IN_BUILD_TYPE(Float3, AsVector3);
        CASE_IN_BUILD_TYPE(Float4, AsVector4);
        CASE_IN_BUILD_TYPE(BoundingSphere, AsBoundingSphere);
        CASE_IN_BUILD_TYPE(BoundingBox, AsBoundingBox);
        CASE_IN_BUILD_TYPE(Ray, AsRay);

        CASE_IN_BUILD_TYPE(Float2, AsData);
        CASE_IN_BUILD_TYPE(Float3, AsData);
        CASE_IN_BUILD_TYPE(Float4, AsData);
        CASE_IN_BUILD_TYPE(Double2, AsData);
        CASE_IN_BUILD_TYPE(Double3, AsData);
        CASE_IN_BUILD_TYPE(Double4, AsBlob.Data);*/
#undef CASE_IN_BUILD_TYPE
        if (klass->IsValueType())
        {
            if (value.Type.Type == VariantTypes::Structure)
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(value.Type.TypeName));
                if (typeHandle && value.AsBlob.Data)
                {
                    auto& valueType = typeHandle.GetType();
                    if (valueType.ManagedClass == CLRCore::Type::GetClass(type))
                    {
                        return CLRCore::Object::Unbox(valueType.Struct.Box(value.AsBlob.Data));
                    }
                    LOG_ERROR("Scripting", "Cannot marshal argument of type {0} as {1}", String(valueType.Fullname), CLRCore::Type::ToString(type));
                }
            }
            else
            {
                const StringAnsiView fullname = klass->GetFullName();
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(fullname);
                if (typeHandle)
                {
                    auto& valueType = typeHandle.GetType();
                    value.SetType(VariantTypeHandle(VariantTypes::Structure, fullname));
                    return CLRCore::Object::Unbox(valueType.Struct.Box(value.AsBlob.Data));
                }
            }
        }
    }
        break;
    case CLRTypes::Enum:
    {
        if (value.Type.Type != VariantTypes::Enum)
            return nullptr;
        return &value.AsUint64;
    }
    case CLRTypes::Class:
    {
        if (value.Type.Type == VariantTypes::Null)
            return nullptr;
        CLRObject* object = BoxVariant(value);
        if (object && !CLRCore::Object::GetClass(object)->IsSubClassOf(CLRCore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case CLRTypes::Object:
        return BoxVariant(value);
    case CLRTypes::SzArray:
    case CLRTypes::List:
    {
        if (value.Type.Type != VariantTypes::Array)
            return nullptr;
        CLRObject* object = BoxVariant(value);
        auto typeStr = CLRCore::Type::ToString(type);
        if (object && !CLRCore::Object::GetClass(object)->IsSubClassOf(CLRCore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case CLRTypes::GenericInst:
    {
        if (value.Type.Type == VariantTypes::Null)
            return nullptr;
        CLRObject* object = BoxVariant(value);
        if (object && !CLRCore::Object::GetClass(object)->IsSubClassOf(CLRCore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case CLRTypes::Ptr:
        switch (value.Type.Type)
        {
    case VariantTypes::Pointer:
        return &value.AsPointer;
    case VariantTypes::Object:
        return &value.AsObject;
    case VariantTypes::Asset:
        return &value.AsAsset;
    case VariantTypes::Structure:
    case VariantTypes::Blob:
        return &value.AsBlob.Data;
    default:
        return nullptr;
        }
    default:
        break;
    }
    failed = true;
    return nullptr;
}

/*CLRObject* CLRUtils::ToManaged(const Version& value)
{
    auto scriptingClass = Scripting::GetStaticClass();
    if (scriptingClass == nullptr)
    {
        return nullptr;
    }

    auto versionToManaged = scriptingClass->GetMethod("VersionToManaged", 4);
    if (versionToManaged == nullptr)
    {
        return nullptr;
    }

    int32 major = value.Major();
    int32 minor = value.Minor();
    int32 build = value.Build();
    int32 revision = value.Revision();

    void* params[4];
    params[0] = &major;
    params[1] = &minor;
    params[2] = &build;
    params[3] = &revision;
    auto obj = versionToManaged->Invoke(nullptr, params, nullptr);

    return obj;
}

Version CLRUtils::ToNative(CLRObject* value)
{
    Version result;
    if (value)
#if USE_NETCORE
    {
        auto scriptingClass = Scripting::GetStaticClass();
        CHECK_RETURN(scriptingClass, result);
        auto versionToNative = scriptingClass->GetMethod("VersionToNative", 5);
        CHECK_RETURN(versionToNative, result);

        void* params[5];
        params[0] = value;
        params[1] = (byte*)&result;
        params[2] = (byte*)&result + sizeof(int32);
        params[3] = (byte*)&result + sizeof(int32) * 2;
        params[4] = (byte*)&result + sizeof(int32) * 3;
        versionToNative->Invoke(nullptr, params, nullptr);

        return result;
    }
#else
        return *(Version*)CLRCore::Object::Unbox(value);
#endif
    return result;
}*/

}
