
#include "SEUtils.h"

#include "SEClass.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Runtime/Scripting/Scripting.h"

namespace SE
{

namespace
{
    // typeName in format System.Collections.Generic.Dictionary`2[KeyType,ValueType]
    void GetDictionaryKeyValueTypes(const StringAnsiView& typeName, SEClass*& keyClass, SEClass*& valueClass)
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

StringView MUtils::ToString(SEString* str)
{
    if (str == nullptr)
        return StringView::Empty;
    return SECore::String::GetChars(str);
}

StringAnsi MUtils::ToStringAnsi(SEString* str)
{
    if (str == nullptr)
        return StringAnsi::Empty;
    return StringAnsi(SECore::String::GetChars(str));
}

void MUtils::ToString(SEString* str, String& result)
{
    if (str)
    {
        const StringView chars = SECore::String::GetChars(str);
        result.Set(chars.Get(), chars.Length());
    }
    else
        result.Clear();
}

void MUtils::ToString(SEString* str, StringView& result)
{
    if (str)
        result = SECore::String::GetChars(str);
    else
        result = StringView();
}

void MUtils::ToString(SEString* str, Variant& result)
{
    result.SetString(str ? SECore::String::GetChars(str) : StringView::Empty);
}

void MUtils::ToString(SEString* str, StringAnsi& result)
{
    if (str)
    {
        const StringView chars = SECore::String::GetChars(str);
        result.Set(chars.Get(), chars.Length());
    }
    else
        result.Clear();
}

SEString* MUtils::ToString(const char* str)
{
    if (str == nullptr || *str == 0)
        return SECore::String::GetEmpty();
    return SECore::String::New(str, StringUtils::Length(str));
}

SEString* MUtils::ToString(const StringAnsi& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return SECore::String::GetEmpty();
    return SECore::String::New(str.Get(), len);
}

SEString* MUtils::ToString(const String& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return SECore::String::GetEmpty();
    return SECore::String::New(str.Get(), len);
}

SEString* MUtils::ToString(const String& str, SEDomain* domain)
{
    const int32 len = str.Length();
    if (len <= 0)
        return SECore::String::GetEmpty(domain);
    return SECore::String::New(str.Get(), len, domain);
}

SEString* MUtils::ToString(const StringAnsiView& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return SECore::String::GetEmpty();
    return SECore::String::New(str.Get(), str.Length());
}

SEString* MUtils::ToString(const StringView& str)
{
    const int32 len = str.Length();
    if (len <= 0)
        return SECore::String::GetEmpty();
    return SECore::String::New(str.Get(), len);
}

SEString* MUtils::ToString(const StringView& str, SEDomain* domain)
{
    const int32 len = str.Length();
    if (len <= 0)
        return SECore::String::GetEmpty(domain);
    return SECore::String::New(str.Get(), len, domain);
}

ScriptingTypeHandle MUtils::UnboxScriptingTypeHandle(SETypeObject* value)
{
    SEClass* klass = GetClass(value);
    if (!klass)
        return ScriptingTypeHandle();
    const StringAnsi& typeName = klass->GetFullName();
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (!typeHandle)
        LOG(Warning, "Unknown scripting type {}", String(typeName));
    return typeHandle;
}

SETypeObject* MUtils::BoxScriptingTypeHandle(const ScriptingTypeHandle& value)
{
    SETypeObject* result = nullptr;
    if (value)
    {
        SETypes* mType = value.GetType().ManagedClass->GetType();
        result = INTERNAL_TYPE_GET_OBJECT(mType);
    }
    return result;
}

VariantTypeHandle MUtils::UnboxVariantType(SETypes* type)
{
    if (!type)
        return VariantTypeHandle(VariantTypes::Null);
    const auto& stdTypes = *StdTypesContainer::Instance();
    SEClass* klass = SECore::Type::GetClass(type);
    SETypes types = SECore::Type::GetType(type);

    // Fast type detection for in-built types
    switch (types)
    {
    case SETypes::Void:
        return VariantTypeHandle(VariantTypes::Void);
    case SETypes::Boolean:
        return VariantTypeHandle(VariantTypes::Bool);
    case SETypes::I1:
    case SETypes::I2:
        return VariantTypeHandle(VariantTypes::Int16);
    case SETypes::U1:
    case SETypes::U2:
        return VariantTypeHandle(VariantTypes::Uint16);
    case SETypes::I4:
    case SETypes::Char:
        return VariantTypeHandle(VariantTypes::Int);
    case SETypes::U4:
        return VariantTypeHandle(VariantTypes::Uint);
    case SETypes::I8:
        return VariantTypeHandle(VariantTypes::Int64);
    case SETypes::U8:
        return VariantTypeHandle(VariantTypes::Uint64);
    case SETypes::R4:
        return VariantTypeHandle(VariantTypes::Float);
    case SETypes::R8:
        return VariantTypeHandle(VariantTypes::Double);
    case SETypes::String:
        return VariantTypeHandle(VariantTypes::String);
    case SETypes::Ptr:
        return VariantTypeHandle(VariantTypes::Pointer);
    case SETypes::ValueType:
        if (klass == stdTypes.GuidClass)
            return VariantTypeHandle(VariantTypes::UID);
        if (klass == stdTypes.Vector2Class)
            return VariantTypeHandle(VariantTypes::Vector2);
        if (klass == stdTypes.Vector3Class)
            return VariantTypeHandle(VariantTypes::Vector3);
        if (klass == stdTypes.Vector4Class)
            return VariantTypeHandle(VariantTypes::Vector4);
        if (klass == Int2::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Int2);
        if (klass == Int3::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Int3);
        if (klass == Int4::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Int4);
        if (klass == Float2::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Float2);
        if (klass == Float3::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Float3);
        if (klass == Float4::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Float4);
        if (klass == Double2::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Double2);
        if (klass == Double3::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Double3);
        if (klass == Double4::TypeInitializer.GetClass())
            return VariantTypeHandle(VariantTypes::Double4);
        if (klass == stdTypes.ColorClass)
            return VariantTypeHandle(VariantTypes::Color);
        if (klass == stdTypes.BoundingBoxClass)
            return VariantTypeHandle(VariantTypes::BoundingBox);
        if (klass == stdTypes.QuaternionClass)
            return VariantTypeHandle(VariantTypes::Quaternion);
        if (klass == stdTypes.TransformClass)
            return VariantTypeHandle(VariantTypes::Transform);
        if (klass == stdTypes.BoundingSphereClass)
            return VariantTypeHandle(VariantTypes::BoundingSphere);
        if (klass == stdTypes.RectangleClass)
            return VariantTypeHandle(VariantTypes::Rectangle);
        if (klass == stdTypes.MatrixClass)
            return VariantTypeHandle(VariantTypes::Matrix);
        break;
    case SETypes::Object:
        return VariantTypeHandle(VariantTypes::ManagedObject);
    case SETypes::SzArray:
        if (klass == SECore::Array::GetClass(SECore::TypeCache::Byte))
            return VariantTypeHandle(VariantTypeHandle::Blob);
        break;
    }

    // Get actual typename for full type info
    if (!klass)
        return VariantTypeHandle(VariantTypeHandle::Null);
    const StringAnsiView fullname = klass->GetFullName();
    switch (types)
    {
    case SETypes::SzArray:
    case SETypes::Array:
        return VariantTypeHandle(VariantTypes::Array, fullname);
    case SETypes::Enum:
        return VariantTypeHandle(VariantTypes::Enum, fullname);
    case SETypes::ValueType:
        return VariantTypeHandle(VariantTypes::Structure, fullname);
    }
    if (klass == stdTypes.TypeClass)
        return VariantTypeHandle(VariantTypes::Typename);
    if (klass->IsSubClassOf(Asset::GetStaticClass()))
    {
        if (klass == Asset::GetStaticClass())
            return VariantTypeHandle(VariantTypes::Asset);
        return VariantTypeHandle(VariantTypes::Asset, fullname);
    }
    if (klass->IsSubClassOf(ScriptingObject::GetStaticClass()))
    {
        if (klass == ScriptingObject::GetStaticClass())
            return VariantTypeHandle(VariantTypes::Object);
        return VariantTypeHandle(VariantTypes::Object, fullname);
    }
    // TODO: support any dictionary unboxing

    LOG_ERROR("Scripting", "Invalid managed type to unbox {0}", String(fullname));
    return VariantTypeHandle();
}

SETypeObject* MUtils::BoxVariantType(const VariantTypeHandle& value)
{
    if (value.Type == VariantTypeHandle::Null)
        return nullptr;
    SEClass* klass = GetClass(value);
    if (!klass)
    {
        LOG_ERROR("Scripting", "Invalid native type to box {0}", value);
        return nullptr;
    }
    SETypes* mType = klass->GetType();
    return INTERNAL_TYPE_GET_OBJECT(mType);
}

Variant MUtils::UnboxVariant(SEObject* value)
{
    if (value == nullptr)
        return Variant::Null;
    const auto& stdTypes = *StdTypesContainer::Instance();
    SEClass* klass = SECore::Object::GetClass(value);

    SETypes* mType = klass->GetType();
    const SETypes mTypes = SECore::Type::GetType(mType);
    void* unboxed = SECore::Object::Unbox(value);

    // Fast type detection for in-built types
    switch (mTypes)
    {
    case SETypes::Void:
        return Variant(VariantTypeHandle(VariantTypeHandle::Void));
    case SETypes::Boolean:
        return *static_cast<bool*>(unboxed);
    case SETypes::I1:
        return *static_cast<int8*>(unboxed);
    case SETypes::U1:
        return *static_cast<uint8*>(unboxed);
    case SETypes::I2:
        return *static_cast<int16*>(unboxed);
    case SETypes::U2:
        return *static_cast<uint16*>(unboxed);
    case SETypes::Char:
        return *static_cast<Char*>(unboxed);
    case SETypes::I4:
        return *static_cast<int32*>(unboxed);
    case SETypes::U4:
        return *static_cast<uint32*>(unboxed);
    case SETypes::I8:
        return *static_cast<int64*>(unboxed);
    case SETypes::U8:
        return *static_cast<uint64*>(unboxed);
    case SETypes::R4:
        return *static_cast<float*>(unboxed);
    case SETypes::R8:
        return *static_cast<double*>(unboxed);
    case SETypes::String:
        return Variant(MUtils::ToString((SEString*)value));
    case SETypes::Ptr:
        return *static_cast<void**>(unboxed);
    case SETypes::ValueType:
        if (klass == stdTypes.GuidClass)
            return Variant(*static_cast<Guid*>(unboxed));
        if (klass == stdTypes.Vector2Class)
            return *static_cast<Vector2*>(unboxed);
        if (klass == stdTypes.Vector3Class)
            return *static_cast<Vector3*>(unboxed);
        if (klass == stdTypes.Vector4Class)
            return *static_cast<Vector4*>(unboxed);
        if (klass == Int2::TypeInitializer.GetClass())
            return *static_cast<Int2*>(unboxed);
        if (klass == Int3::TypeInitializer.GetClass())
            return *static_cast<Int3*>(unboxed);
        if (klass == Int4::TypeInitializer.GetClass())
            return *static_cast<Int4*>(unboxed);
        if (klass == Float2::TypeInitializer.GetClass())
            return *static_cast<Float2*>(unboxed);
        if (klass == Float3::TypeInitializer.GetClass())
            return *static_cast<Float3*>(unboxed);
        if (klass == Float4::TypeInitializer.GetClass())
            return *static_cast<Float4*>(unboxed);
        if (klass == Double2::TypeInitializer.GetClass())
            return *static_cast<Double2*>(unboxed);
        if (klass == Double3::TypeInitializer.GetClass())
            return *static_cast<Double3*>(unboxed);
        if (klass == Double4::TypeInitializer.GetClass())
            return *static_cast<Double4*>(unboxed);
        if (klass == stdTypes.ColorClass)
            return *static_cast<Color*>(unboxed);
        if (klass == stdTypes.BoundingBoxClass)
            return Variant(*static_cast<BoundingBox*>(unboxed));
        if (klass == stdTypes.QuaternionClass)
            return *static_cast<Quaternion*>(unboxed);
        if (klass == stdTypes.TransformClass)
            return Variant(*static_cast<Transform*>(unboxed));
        if (klass == stdTypes.BoundingSphereClass)
            return *static_cast<BoundingSphere*>(unboxed);
        if (klass == stdTypes.RectangleClass)
            return *static_cast<Rectangle*>(unboxed);
        if (klass == stdTypes.MatrixClass)
            return Variant(*reinterpret_cast<Matrix*>(unboxed));
        break;
    case SETypes::SzArray:
    case SETypes::List:
    {
        void* ptr = SECore::Array::GetAddress((SEArray*)value);
        const SEClass* arrayClass = klass == stdTypes.ManagedArrayClass ? SECore::Array::GetArrayClass((SEArray*)value) : klass;
        const SEClass* elementClass = arrayClass->GetElementClass();
        if (elementClass == SECore::TypeCache::Byte)
        {
            Variant v;
            v.SetBlob(ptr, SECore::Array::GetLength((SEArray*)value));
            return v;
        }
        const StringAnsiView fullname = arrayClass->GetFullName();
        Variant v;
        v.SetType(MoveTemp(VariantTypeHandle(VariantTypeHandle::Types::Array, fullname)));
        auto& array = v.AsArray();
        array.Resize(SECore::Array::GetLength((SEArray*)value));
        const StringAnsiView elementTypename(*fullname, fullname.Length() - 2);
        const int32 elementSize = elementClass->GetInstanceSize();
        if (elementClass->IsEnum())
        {
            // Array of Enums
            for (int32 i = 0; i < array.Count(); i++)
            {
                array[i].SetType(VariantTypeHandle(VariantTypeHandle::Enum, elementTypename));
                Platform::MemoryCopy(&array[i].AsUint64, (byte*)ptr + elementSize * i, elementSize);
            }
        }
        else if (elementClass->IsValueType())
        {
            // Array of Structures
            VariantTypeHandle elementType = UnboxVariantType(elementClass->GetType());
            switch (elementType.Type)
            {
            case VariantTypeHandle::Bool:
            case VariantTypeHandle::Int:
            case VariantTypeHandle::Uint:
            case VariantTypeHandle::Int64:
            case VariantTypeHandle::Uint64:
            case VariantTypeHandle::Float:
            case VariantTypeHandle::Double:
            case VariantTypeHandle::Float2:
            case VariantTypeHandle::Float3:
            case VariantTypeHandle::Float4:
            case VariantTypeHandle::Color:
            case VariantTypeHandle::Guid:
            case VariantTypeHandle::Quaternion:
            case VariantTypeHandle::Rectangle:
            case VariantTypeHandle::Int2:
            case VariantTypeHandle::Int3:
            case VariantTypeHandle::Int4:
            case VariantTypeHandle::Int16:
            case VariantTypeHandle::Uint16:
            case VariantTypeHandle::Double2:
            case VariantTypeHandle::Double3:
#if !USE_LARGE_WORLDS
            case VariantTypeHandle::BoundingSphere:
            case VariantTypeHandle::BoundingBox:
            case VariantTypeHandle::Ray:
#endif
                // Optimized unboxing of raw data type
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto& a = array[i];
                    a.SetType(elementType);
                    Platform::MemoryCopy(&a.AsData, (byte*)ptr + elementSize * i, elementSize);
                }
                break;
            case VariantTypeHandle::Transform:
            case VariantTypeHandle::Matrix:
            case VariantTypeHandle::Double4:
#if USE_LARGE_WORLDS
            case VariantType::BoundingSphere:
            case VariantType::BoundingBox:
            case VariantType::Ray:
#endif
                // Optimized unboxing of raw data type
                for (int32 i = 0; i < array.Count(); i++)
                {
                    auto& a = array[i];
                    a.SetType(elementType);
                    Platform::MemoryCopy(a.AsBlob.Data, (byte*)ptr + elementSize * i, elementSize);
                }
                break;
            case VariantTypeHandle::Structure:
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
                        SEObject* boxed = SECore::Object::New(elementClass);
                        Platform::MemoryCopy(SECore::Object::Unbox(boxed), managed, elementSize);
                        type.Struct.Unbox(a.AsBlob.Data, boxed);
                    }
                    break;
                }
                LOG_ERROR("Scripting", "Invalid type to unbox {0}", v.Type);
                break;
            }
            default:
                LOG_ERROR("Scripting", "Invalid type to unbox {0}", v.Type);
                break;
            }
        }
        else
        {
            // Array of Objects
            for (int32 i = 0; i < array.Count(); i++)
                array[i] = UnboxVariant(((SEObject**)ptr)[i]);
        }
        return v;
    }
    case SETypes::GenericInst:
    {
        if (klass->GetName() == "Dictionary`2" && klass->GetNamespace() == "System.Collections.Generic")
        {
            // Dictionary
            ManagedDictionary managed(value);
            SEArray* managedKeys = managed.GetKeys();
            int32 length = managedKeys ? SECore::Array::GetLength(managedKeys) : 0;
            Dictionary<Variant, Variant> native;
            native.EnsureCapacity(length);
            SEObject** managedKeysPtr = SECore::Array::GetAddress<SEObject*>(managedKeys);
            for (int32 i = 0; i < length; i++)
            {
                SEObject* keyManaged = managedKeysPtr[i];
                SEObject* valueManaged = managed.GetValue(keyManaged);
                native.Add(UnboxVariant(keyManaged), UnboxVariant(valueManaged));
            }
            Variant v(MoveTemp(native));
            v.Type.SetTypeName(klass->GetFullName());
            return v;
        }
        break;
    }
    }

    if (klass->IsSubClassOf(Asset::GetStaticClass()))
        return static_cast<Asset*>(ScriptingObject::ToNative(value));
    if (klass->IsSubClassOf(ScriptingObject::GetStaticClass()))
        return ScriptingObject::ToNative(value);
    if (klass->IsEnum())
    {
        const StringAnsiView fullname = klass->GetFullName();
        Variant v;
        v.Type = MoveTemp(VariantTypeHandle(VariantTypeHandle::Enum, fullname));
        // TODO: what about 64-bit enum? use enum size with memcpy
        v.AsUint64 = *static_cast<uint32*>(SECore::Object::Unbox(value));
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
            v.Type = MoveTemp(VariantTypeHandle(VariantTypeHandle::Structure, fullname));
            v.AsBlob.Data = Allocator::Allocate(type.Size);
            v.AsBlob.Length = type.Size;
            type.Struct.Ctor(v.AsBlob.Data);
            type.Struct.Unbox(v.AsBlob.Data, value);
            return v;
        }
        return Variant(value);
    }

    return Variant(value);
}

SEObject* MUtils::BoxVariant(const Variant& value)
{
    const auto& stdTypes = *StdTypesContainer::Instance();
    switch (value.Type.Type)
    {
    case VariantTypeHandle::Null:
    case VariantTypeHandle::Void:
        return nullptr;
    case VariantTypeHandle::Bool:
        return SECore::Object::Box((void*)&value.AsBool, SECore::TypeCache::Boolean);
    case VariantTypeHandle::Int16:
        return SECore::Object::Box((void*)&value.AsInt16, SECore::TypeCache::Int16);
    case VariantTypeHandle::Uint16:
        return SECore::Object::Box((void*)&value.AsUint16, SECore::TypeCache::UInt16);
    case VariantTypeHandle::Int:
        return SECore::Object::Box((void*)&value.AsInt, SECore::TypeCache::Int32);
    case VariantTypeHandle::Uint:
        return SECore::Object::Box((void*)&value.AsUint, SECore::TypeCache::UInt32);
    case VariantTypeHandle::Int64:
        return SECore::Object::Box((void*)&value.AsInt64, SECore::TypeCache::Int64);
    case VariantTypeHandle::Uint64:
        return SECore::Object::Box((void*)&value.AsUint64, SECore::TypeCache::UInt64);
    case VariantTypeHandle::Float:
        return SECore::Object::Box((void*)&value.AsFloat, SECore::TypeCache::Single);
    case VariantTypeHandle::Double:
        return SECore::Object::Box((void*)&value.AsDouble, SECore::TypeCache::Double);
    case VariantTypeHandle::Float2:
        return SECore::Object::Box((void*)&value.AsData, Float2::TypeInitializer.GetClass());
    case VariantTypeHandle::Float3:
        return SECore::Object::Box((void*)&value.AsData, Float3::TypeInitializer.GetClass());
    case VariantTypeHandle::Float4:
        return SECore::Object::Box((void*)&value.AsData, Float4::TypeInitializer.GetClass());
    case VariantTypeHandle::Double2:
        return SECore::Object::Box((void*)&value.AsData, Double2::TypeInitializer.GetClass());
    case VariantTypeHandle::Double3:
        return SECore::Object::Box((void*)&value.AsData, Double3::TypeInitializer.GetClass());
    case VariantTypeHandle::Double4:
        return SECore::Object::Box((void*)&value.AsData, Double4::TypeInitializer.GetClass());
    case VariantTypeHandle::Color:
        return SECore::Object::Box((void*)&value.AsData, stdTypes.ColorClass);
    case VariantTypeHandle::Guid:
        return SECore::Object::Box((void*)&value.AsData, stdTypes.GuidClass);
    case VariantTypeHandle::String:
#if USE_NETCORE
        return (SEObject*)MUtils::ToString((StringView)value);
#else
        return (SEObject*)MUtils::ToString((StringView)value);
#endif
    case VariantTypeHandle::Quaternion:
        return SECore::Object::Box((void*)&value.AsData, stdTypes.QuaternionClass);
    case VariantTypeHandle::BoundingSphere:
        return SECore::Object::Box((void*)&value.AsBoundingSphere(), stdTypes.BoundingSphereClass);
    case VariantTypeHandle::Rectangle:
        return SECore::Object::Box((void*)&value.AsData, stdTypes.RectangleClass);
    case VariantTypeHandle::Pointer:
        return SECore::Object::Box((void*)&value.AsPointer, SECore::TypeCache::IntPtr);
    case VariantTypeHandle::Ray:
        return SECore::Object::Box((void*)&value.AsRay(), stdTypes.RayClass);
    case VariantTypeHandle::BoundingBox:
        return SECore::Object::Box((void*)&value.AsBoundingBox(), stdTypes.BoundingBoxClass);
    case VariantTypeHandle::Transform:
        return SECore::Object::Box(value.AsBlob.Data, stdTypes.TransformClass);
    case VariantTypeHandle::Matrix:
        return SECore::Object::Box(value.AsBlob.Data, stdTypes.MatrixClass);
    case VariantTypeHandle::Blob:
        return (SEObject*)ToArray(Span<byte>((const byte*)value.AsBlob.Data, value.AsBlob.Length));
    case VariantTypeHandle::Object:
        return value.AsObject ? value.AsObject->GetOrCreateManagedInstance() : nullptr;
    case VariantTypeHandle::Asset:
        return value.AsAsset ? value.AsAsset->GetOrCreateManagedInstance() : nullptr;
    case VariantTypeHandle::Array:
    {
        SEArray* managed;
        const auto& array = value.AsArray();
        if (value.Type.TypeName)
        {
            const StringAnsiView elementTypename(value.Type.TypeName, StringUtils::Length(value.Type.TypeName) - 2);
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(elementTypename);
            SEClass* elementClass;
            if (typeHandle && typeHandle.GetType().ManagedClass)
                elementClass = typeHandle.GetType().ManagedClass;
            else
                elementClass = Scripting::FindClass(elementTypename);
            if (!elementClass)
            {
                LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
                return nullptr;
            }
            const int32 elementSize = elementClass->GetInstanceSize();
            managed = SECore::Array::New(elementClass, array.Count());
            if (elementClass->IsEnum())
            {
                // Array of Enums
                byte* managedPtr = (byte*)SECore::Array::GetAddress(managed);
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
                byte* managedPtr = (byte*)SECore::Array::GetAddress(managed);
                switch (elementType.Type)
                {
                case VariantTypeHandle::Bool:
                case VariantTypeHandle::Int:
                case VariantTypeHandle::Uint:
                case VariantTypeHandle::Int64:
                case VariantTypeHandle::Uint64:
                case VariantTypeHandle::Float:
                case VariantTypeHandle::Double:
                case VariantTypeHandle::Float2:
                case VariantTypeHandle::Float3:
                case VariantTypeHandle::Float4:
                case VariantTypeHandle::Color:
                case VariantTypeHandle::Guid:
                case VariantTypeHandle::Quaternion:
                case VariantTypeHandle::Rectangle:
                case VariantTypeHandle::Int2:
                case VariantTypeHandle::Int3:
                case VariantTypeHandle::Int4:
                case VariantTypeHandle::Int16:
                case VariantTypeHandle::Uint16:
                case VariantTypeHandle::Double2:
                case VariantTypeHandle::Double3:
#if !USE_LARGE_WORLDS
                case VariantTypeHandle::BoundingSphere:
                case VariantTypeHandle::BoundingBox:
                case VariantTypeHandle::Ray:
#endif
                    // Optimized boxing of raw data type
                    for (int32 i = 0; i < array.Count(); i++)
                        Platform::MemoryCopy(managedPtr + elementSize * i, &array[i].AsData, elementSize);
                    break;
                case VariantTypeHandle::Transform:
                case VariantTypeHandle::Matrix:
                case VariantTypeHandle::Double4:
#if USE_LARGE_WORLDS
                case VariantType::BoundingSphere:
                case VariantType::BoundingBox:
                case VariantType::Ray:
#endif
                    // Optimized boxing of raw data type
                    for (int32 i = 0; i < array.Count(); i++)
                        Platform::MemoryCopy(managedPtr + elementSize * i, array[i].AsBlob.Data, elementSize);
                    break;
                case VariantTypeHandle::Structure:
                    if (typeHandle)
                    {
                        const ScriptingType& type = typeHandle.GetType();
                        ASSERT(type.Type == ScriptingTypes::Structure);
                        for (int32 i = 0; i < array.Count(); i++)
                        {
                            // TODO: optimize structures boxing to not return SEObject* but use raw managed object to prevent additional boxing here
                            SEObject* boxed = type.Struct.Box(array[i].AsBlob.Data);
                            Platform::MemoryCopy(managedPtr + elementSize * i, SECore::Object::Unbox(boxed), elementSize);
                        }
                        break;
                    }
                    LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
                    break;
                default:
                    LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
                    break;
                }
            }
            else
            {
                // Array of Objects
                for (int32 i = 0; i < array.Count(); i++)
                    SECore::GC::WriteArrayRef(managed, BoxVariant(array[i]), i);
            }
        }
        else
        {
            // object[]
            managed = SECore::Array::New(SECore::TypeCache::Object, array.Count());
            for (int32 i = 0; i < array.Count(); i++)
                SECore::GC::WriteArrayRef(managed, BoxVariant(array[i]), i);
        }
        return (SEObject*)managed;
    }
    case VariantTypeHandle::Dictionary:
    {
        // Get dictionary key and value types
        SEClass *keyClass, *valueClass;
        GetDictionaryKeyValueTypes(value.Type.GetTypeName(), keyClass, valueClass);
        if (!keyClass || !valueClass)
        {
            LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
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
    case VariantTypeHandle::Structure:
    {
        if (value.AsBlob.Data == nullptr)
            return nullptr;
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(value.Type.TypeName));
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            return type.Struct.Box(value.AsBlob.Data);
        }
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
        return nullptr;
    }
    case VariantTypeHandle::Enum:
    {
        const auto klass = Scripting::FindClass(StringAnsiView(value.Type.TypeName));
        if (klass)
            return SECore::Object::Box((void*)&value.AsUint64, klass);
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
        return nullptr;
    }
    case VariantTypeHandle::ManagedObject:
#if USE_NETCORE
        return value.AsUint64 ? SECore::GCHandle::GetTarget(value.AsUint64) : nullptr;
#else
        return value.AsUint ? SECore::GCHandle::GetTarget(value.AsUint) : nullptr;
#endif
    case VariantTypeHandle::Typename:
    {
        const auto klass = Scripting::FindClass((StringAnsiView)value);
        if (klass)
            return (SEObject*)GetType(klass);
        LOG_ERROR("Scripting", "Invalid type to box {0}", value);
        return nullptr;
    }
    default:
        LOG_ERROR("Scripting", "Invalid type to box {0}", value.Type);
        return nullptr;
    }
}

const StringAnsi& MUtils::GetClassFullname(SEObject* obj)
{
    if (obj)
    {
        SEClass* mClass = SECore::Object::GetClass(obj);
        return mClass->GetFullName();
    }
    return StringAnsi::Empty;
}

SEClass* MUtils::GetClass(SETypeObject* type)
{
    if (type == nullptr)
        return nullptr;
    SETypes* mType = INTERNAL_TYPE_OBJECT_GET(type);
    return SECore::Type::GetClass(mType);
}

SEClass* MUtils::GetClass(const VariantTypeHandle& value)
{
    auto mclass = Scripting::FindClass(StringAnsiView(value.TypeName));
    if (mclass)
        return mclass;
    const auto& stdTypes = *StdTypesContainer::Instance();
    switch (value.Type)
    {
    case VariantTypeHandle::Void:
        return SECore::TypeCache::Void;
    case VariantTypeHandle::Bool:
        return SECore::TypeCache::Boolean;
    case VariantTypeHandle::Int16:
        return SECore::TypeCache::Int16;
    case VariantTypeHandle::Uint16:
        return SECore::TypeCache::UInt16;
    case VariantTypeHandle::Int:
        return SECore::TypeCache::Int32;
    case VariantTypeHandle::Uint:
        return SECore::TypeCache::UInt32;
    case VariantTypeHandle::Int64:
        return SECore::TypeCache::Int64;
    case VariantTypeHandle::Uint64:
        return SECore::TypeCache::UInt64;
    case VariantTypeHandle::Float:
        return SECore::TypeCache::Single;
    case VariantTypeHandle::Double:
        return SECore::TypeCache::Double;
    case VariantTypeHandle::Pointer:
        return SECore::TypeCache::IntPtr;
    case VariantTypeHandle::String:
        return SECore::TypeCache::String;
    case VariantTypeHandle::Object:
        return ScriptingObject::GetStaticClass();
    case VariantTypeHandle::Asset:
        return Asset::GetStaticClass();
    case VariantTypeHandle::Blob:
        return SECore::Array::GetClass(SECore::TypeCache::Byte);
    case VariantTypeHandle::Float2:
        return Double2::TypeInitializer.GetClass();
    case VariantTypeHandle::Float3:
        return Float3::TypeInitializer.GetClass();
    case VariantTypeHandle::Float4:
        return Float4::TypeInitializer.GetClass();
    case VariantTypeHandle::Double2:
        return Double2::TypeInitializer.GetClass();
    case VariantTypeHandle::Double3:
        return Double3::TypeInitializer.GetClass();
    case VariantTypeHandle::Double4:
        return Double4::TypeInitializer.GetClass();
    case VariantTypeHandle::Color:
        return stdTypes.ColorClass;
    case VariantTypeHandle::Guid:
        return stdTypes.GuidClass;
    case VariantTypeHandle::Typename:
        return stdTypes.TypeClass;
    case VariantTypeHandle::BoundingBox:
        return stdTypes.BoundingBoxClass;
    case VariantTypeHandle::BoundingSphere:
        return stdTypes.BoundingSphereClass;
    case VariantTypeHandle::Quaternion:
        return stdTypes.QuaternionClass;
    case VariantTypeHandle::Transform:
        return stdTypes.TransformClass;
    case VariantTypeHandle::Rectangle:
        return stdTypes.RectangleClass;
    case VariantTypeHandle::Ray:
        return stdTypes.RayClass;
    case VariantTypeHandle::Matrix:
        return stdTypes.MatrixClass;
    case VariantTypeHandle::Array:
        if (value.TypeName)
        {
            const StringAnsiView elementTypename(value.TypeName, StringUtils::Length(value.TypeName) - 2);
            mclass = Scripting::FindClass(elementTypename);
            if (mclass)
                return SECore::Array::GetClass(mclass);
        }
        return SECore::Array::GetClass(SECore::TypeCache::Object);
    case VariantTypeHandle::Dictionary:
    {
        SEClass *keyClass, *valueClass;
        GetDictionaryKeyValueTypes(value.GetTypeName(), keyClass, valueClass);
        if (!keyClass || !valueClass)
        {
            LOG_ERROR("Scripting", "Invalid type to box {0}", value.ToString());
            return nullptr;
        }
        return GetClass(ManagedDictionary::GetClass(keyClass->GetType(), valueClass->GetType()));
    }
    case VariantTypeHandle::ManagedObject:
        return SECore::TypeCache::Object;
    default: ;
    }
    return nullptr;
}

SEClass* MUtils::GetClass(const Variant& value)
{
    const auto& stdTypes = *StdTypesContainer::Instance();
    switch (value.Type.Type)
    {
    case VariantTypes::Void:
        return SECore::TypeCache::Void;
    case VariantTypes::Bool:
        return SECore::TypeCache::Boolean;
    case VariantTypes::Int16:
        return SECore::TypeCache::Int16;
    case VariantTypes::Uint16:
        return SECore::TypeCache::UInt16;
    case VariantTypes::Int:
        return SECore::TypeCache::Int32;
    case VariantTypes::Uint:
        return SECore::TypeCache::UInt32;
    case VariantTypes::Int64:
        return SECore::TypeCache::Int64;
    case VariantTypes::Uint64:
        return SECore::TypeCache::UInt64;
    case VariantTypes::Float:
        return SECore::TypeCache::Single;
    case VariantTypes::Double:
        return SECore::TypeCache::Double;
    case VariantTypes::Pointer:
        return SECore::TypeCache::IntPtr;
    case VariantTypes::String:
        return SECore::TypeCache::String;
    case VariantTypes::Blob:
        return SECore::Array::GetClass(SECore::TypeCache::Byte);
    case VariantTypes::Float2:
        return Float2::TypeInitializer.GetClass();
    case VariantTypes::Float3:
        return Float3::TypeInitializer.GetClass();
    case VariantTypes::Float4:
        return Float4::TypeInitializer.GetClass();
    case VariantTypes::Double2:
        return Double2::TypeInitializer.GetClass();
    case VariantTypes::Double3:
        return Double3::TypeInitializer.GetClass();
    case VariantTypes::Double4:
        return Double4::TypeInitializer.GetClass();
    case VariantTypes::Color:
        return stdTypes.ColorClass;
    case VariantTypes::Guid:
        return stdTypes.GuidClass;
    case VariantTypes::Typename:
        return stdTypes.TypeClass;
    case VariantTypes::BoundingBox:
        return stdTypes.BoundingBoxClass;
    case VariantTypes::BoundingSphere:
        return stdTypes.BoundingSphereClass;
    case VariantTypes::Quaternion:
        return stdTypes.QuaternionClass;
    case VariantTypes::Transform:
        return stdTypes.TransformClass;
    case VariantTypes::Rectangle:
        return stdTypes.RectangleClass;
    case VariantTypes::Ray:
        return stdTypes.RayClass;
    case VariantTypes::Matrix:
        return stdTypes.MatrixClass;
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
        SEObject* obj = (SEObject*)value;
        if (obj)
            return SECore::Object::GetClass(obj);
    }
    default: ;
    }
    return GetClass(value.Type);
}

SETypeObject* MUtils::GetType(SEObject* object)
{
    if (!object)
        return nullptr;
    SEClass* klass = SECore::Object::GetClass(object);
    return GetType(klass);
}

SETypeObject* MUtils::GetType(SEClass* klass)
{
    if (!klass)
        return nullptr;
    SETypes* type = klass->GetType();
    return INTERNAL_TYPE_GET_OBJECT(type);
}

BytesContainer MUtils::LinkArray(SEArray* arrayObj)
{
    BytesContainer result;
    const int32 length = arrayObj ? SECore::Array::GetLength(arrayObj) : 0;
    if (length != 0)
    {
        result.Link((byte*)SECore::Array::GetAddress(arrayObj), length);
    }
    return result;
}

void* MUtils::VariantToManagedArgPtr(Variant& value, SETypes* type, bool& failed)
{
    // Convert Variant into matching managed type and return pointer to data for the method invocation
    SETypes mType = SECore::Type::GetType(type);
    switch (mType)
    {
    case SETypes::Boolean:
        if (value.Type.Type != VariantTypes::Bool)
            value = (bool)value;
        return &value.AsBool;
    case SETypes::Char:
    case SETypes::I1:
    case SETypes::I2:
        if (value.Type.Type != VariantTypes::Int16)
            value = (int16)value;
        return &value.AsInt16;
    case SETypes::I4:
        if (value.Type.Type != VariantTypes::Int)
            value = (int32)value;
        return &value.AsInt;
    case SETypes::U1:
    case SETypes::U2:
        if (value.Type.Type != VariantTypes::Uint16)
            value = (uint16)value;
        return &value.AsUint16;
    case SETypes::U4:
        if (value.Type.Type != VariantTypes::Uint)
            value = (uint32)value;
        return &value.AsUint;
    case SETypes::I8:
        if (value.Type.Type != VariantTypes::Int64)
            value = (int64)value;
        return &value.AsInt64;
    case SETypes::U8:
        if (value.Type.Type != VariantTypes::Uint64)
            value = (uint64)value;
        return &value.AsUint64;
    case SETypes::R4:
        if (value.Type.Type != VariantTypes::Float)
            value = (float)value;
        return &value.AsFloat;
    case SETypes::R8:
        if (value.Type.Type != VariantTypes::Double)
            value = (double)value;
        return &value.AsDouble;
    case SETypes::String:
        return MUtils::ToString((StringView)value);
    case SETypes::ValueType:
    {
        SEClass* klass = SECore::Type::GetClass(type);
        if (klass->IsEnum())
        {
            if (value.Type.Type != VariantTypes::Enum)
            {
                value.SetType(VariantTypeHandle(VariantTypes::Enum, klass));
                value.AsUint64 = 0;
            }
            return &value.AsUint64;
        }
        const auto stdTypes = StdTypesContainer::Instance();
#define CASE_IN_BUILD_TYPE(type, access) \
    if (klass == stdTypes->type##Class) \
    { \
        if (value.Type.Type != VariantType::type) \
            value = Variant((type)value); \
        return value.access; \
    }
        CASE_IN_BUILD_TYPE(Color, AsData);
        CASE_IN_BUILD_TYPE(Quaternion, AsData);
        CASE_IN_BUILD_TYPE(Guid, AsData);
        CASE_IN_BUILD_TYPE(Rectangle, AsData);
        CASE_IN_BUILD_TYPE(Matrix, AsBlob.Data);
        CASE_IN_BUILD_TYPE(Transform, AsBlob.Data);
#undef CASE_IN_BUILD_TYPE
#define CASE_IN_BUILD_TYPE(type, access) \
    if (klass == stdTypes->type##Class) \
    { \
        if (value.Type.Type != VariantType::type) \
            value = Variant((type)value); \
        return (void*)&value.access(); \
    }
        CASE_IN_BUILD_TYPE(Vector2, AsVector2);
        CASE_IN_BUILD_TYPE(Vector3, AsVector3);
        CASE_IN_BUILD_TYPE(Vector4, AsVector4);
        CASE_IN_BUILD_TYPE(BoundingSphere, AsBoundingSphere);
        CASE_IN_BUILD_TYPE(BoundingBox, AsBoundingBox);
        CASE_IN_BUILD_TYPE(Ray, AsRay);
#undef CASE_IN_BUILD_TYPE
#define CASE_IN_BUILD_TYPE(type, access) \
    if (klass == type::TypeInitializer.GetClass()) \
    { \
        if (value.Type.Type != VariantType::type) \
            value = Variant((type)value); \
        return value.access; \
    }
        CASE_IN_BUILD_TYPE(Float2, AsData);
        CASE_IN_BUILD_TYPE(Float3, AsData);
        CASE_IN_BUILD_TYPE(Float4, AsData);
        CASE_IN_BUILD_TYPE(Double2, AsData);
        CASE_IN_BUILD_TYPE(Double3, AsData);
        CASE_IN_BUILD_TYPE(Double4, AsBlob.Data);
#undef CASE_IN_BUILD_TYPE
        if (klass->IsValueType())
        {
            if (value.Type.Type == VariantTypes::Structure)
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(value.Type.TypeName));
                if (typeHandle && value.AsBlob.Data)
                {
                    auto& valueType = typeHandle.GetType();
                    if (valueType.ManagedClass == SECore::Type::GetClass(type))
                    {
                        return SECore::Object::Unbox(valueType.Struct.Box(value.AsBlob.Data));
                    }
                    LOG_ERROR("Scripting", "Cannot marshal argument of type {0} as {1}", String(valueType.Fullname), SECore::Type::ToString(type));
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
                    return SECore::Object::Unbox(valueType.Struct.Box(value.AsBlob.Data));
                }
            }
        }
    }
    break;
    case SETypes::Enum:
    {
        if (value.Type.Type != VariantTypes::Enum)
            return nullptr;
        return &value.AsUint64;
    }
    case SETypes::Class:
    {
        if (value.Type.Type == VariantTypes::Null)
            return nullptr;
        SEObject* object = BoxVariant(value);
        if (object && !SECore::Object::GetClass(object)->IsSubClassOf(SECore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case SETypes::Object:
        return BoxVariant(value);
    case SETypes::SzArray:
    case SETypes::Array:
    {
        if (value.Type.Type != VariantTypes::Array)
            return nullptr;
        SEObject* object = BoxVariant(value);
        auto typeStr = SECore::Type::ToString(type);
        if (object && !SECore::Object::GetClass(object)->IsSubClassOf(SECore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case SETypes::GenericInst:
    {
        if (value.Type.Type == VariantTypes::Null)
            return nullptr;
        SEObject* object = BoxVariant(value);
        if (object && !SECore::Object::GetClass(object)->IsSubClassOf(SECore::Type::GetClass(type)))
            object = nullptr;
        return object;
    }
    case SETypes::Ptr:
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

SEObject* MUtils::ToManaged(const Version& value)
{
#if USE_NETCORE
    auto scriptingClass = Scripting::GetStaticClass();
    CHECK_RETURN(scriptingClass, nullptr);
    auto versionToManaged = scriptingClass->GetMethod("VersionToManaged", 4);
    CHECK_RETURN(versionToManaged, nullptr);

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
#else
    auto obj = SECore::Object::New(Scripting::FindClass("System.Version"));
    Platform::MemoryCopy(SECore::Object::Unbox(obj), &value, sizeof(Version));
#endif
    return obj;
}

Version MUtils::ToNative(SEObject* value)
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
        return *(Version*)SECore::Object::Unbox(value);
#endif
    return result;
}

}
