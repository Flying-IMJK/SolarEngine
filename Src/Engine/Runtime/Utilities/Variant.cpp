
#include "Variant.h"

#include "Core/Math/Transform.h"

#include "Core/Math/BoundingVolumes.h"
#include "Core/Math/Color.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Line.h"
#include "Core/Math/Rectangle.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/StringUtils.h"
#include "Core/Serialization/WriteStream.h"
#include "Core/Utilities/Formatting.h"
#include "Core/Types/Strings/StringView.h"
#include "Runtime/Resource/Storage/AssetStorages.h"

namespace SE
{
    namespace
    {
        const char* InBuiltTypesTypeNames[40] =
        {
            // @formatter:off
            "",// Null
            "System.Void",// Void
            "System.Boolean",// Bool
            "System.Int32",// Int
            "System.UInt32",// Uint
            "System.Int64",// Int64
            "System.UInt64",// Uint64
            "System.Single",// Float
            "System.Double",// Double
            "System.IntPtr",// Pointer
            "System.String",// String
            "System.Object",// Object
            "",// Structure
            "SE.Asset",// Asset
            "System.Byte[]",// Blob
            "",// Enum
            "SE.Float2",// Float2
            "SE.Float3",// Float3
            "SE.Float4",// Float4
            "SE.Color",// Color
            "System.Guid",// Guid
            "SE.BoundingBox",// BoundingBox
            "SE.BoundingSphere",// BoundingSphere
            "SE.Quaternion",// Quaternion
            "SE.Transform",// Transform
            "SE.Rectangle",// Rectangle
            "SE.Ray",// Ray
            "SE.Matrix",// Matrix
            "System.Object[]",// Array
            "System.Collections.Generic.Dictionary`2[System.Object,System.Object]",// Dictionary
            "System.Object",// ManagedObject
            "System.Type",// Typename
            "SE.Int2",// Int2
            "SE.Int3",// Int3
            "SE.Int4",// Int4
            "System.Int16",// Int16
            "System.UInt16",// Uint16
            "SE.Double2",// Double2
            "SE.Double3",// Double3
            "SE.Double4",// Double4
            // @formatter:on
        };
    }

    VariantTypeHandle::VariantTypeHandle(VariantTypes type, const StringView& typeName)
    {
        Type = type;
        TypeName = nullptr;
        const int32 length = typeName.Length();
        if (length)
        {
            TypeName = static_cast<char*>(PlatformAllocator::Allocate(length + 1));
            StringUtils::ConvertUTF162ANSI(typeName.Get(), TypeName, length);
            TypeName[length] = 0;
        }
    }

    VariantTypeHandle::VariantTypeHandle(VariantTypes type, const StringAnsiView& typeName)
    {
        Type = type;
        TypeName = nullptr;
        int32 length = typeName.Length();
        if (length)
        {
            TypeName = static_cast<char*>(PlatformAllocator::Allocate(length + 1));
            Platform::MemoryCopy(TypeName, typeName.Get(), length);
            TypeName[length] = 0;
        }
    }

    VariantTypeHandle::VariantTypeHandle(const StringAnsiView& typeName)
    {
        // Try using in-built type
        for (uint32 i = 0; i < ARRAY_SIZE(InBuiltTypesTypeNames); i++)
        {
            if (typeName == InBuiltTypesTypeNames[i])
            {
                new(this) VariantTypeHandle((VariantTypes)i);
                return;
            }
        }
        {
            // Aliases
            if (typeName == "SE.Vector2")
            {
                new(this) VariantTypeHandle(VariantTypes::Vector2);
                return;
            }
            if (typeName == "SE.Float3")
            {
                new(this) VariantTypeHandle(VariantTypes::Float3);
                return;
            }
            if (typeName == "SE.Vector4")
            {
                new(this) VariantTypeHandle(VariantTypes::Vector4);
                return;
            }
        }

        // Check case for array
        if (typeName.EndsWith(StringAnsiView("[]"), StringSearchCase::CaseSensitive))
        {
            new(this) VariantTypeHandle(VariantTypes::Array, StringAnsiView(typeName.Get(), typeName.Length() - 2));
            return;
        }

        // Try using scripting type
        /*const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            switch (type.Type)
            {
            case ScriptingTypes::Script:
            case ScriptingTypes::Class:
            case ScriptingTypes::Interface:
                new(this) MaterialVariantType(Object, typeName);
                return;
            case ScriptingTypes::Structure:
                new(this) MaterialVariantType(Structure, typeName);
                return;
            case ScriptingTypes::Enum:
                new(this) MaterialVariantType(Enum, typeName);
                return;
            }
        }*/

        // Try using managed class
#if USE_CSHARP
        if (const auto mclass = Scripting::FindClass(typeName))
        {
            if (mclass->IsEnum())
                new(this) MaterialVariantType(Enum, typeName);
            else
                new(this) MaterialVariantType(ManagedObject, typeName);
            return;
        }
#endif

        new(this) VariantTypeHandle();
        LOG_WARNING("", "Missing scripting type \'{0}\'", String(typeName));
    }

    VariantTypeHandle::VariantTypeHandle(const VariantTypeHandle& other)
    {
        Type = other.Type;
        TypeName = nullptr;
        const int32 length = StringUtils::Length(other.TypeName);
        if (length)
        {
            TypeName = static_cast<char*>(PlatformAllocator::Allocate(length + 1));
            Platform::MemoryCopy(TypeName, other.TypeName, length);
            TypeName[length] = 0;
        }
    }

    VariantTypeHandle::VariantTypeHandle(VariantTypeHandle&& other) noexcept
    {
        Type = other.Type;
        TypeName = other.TypeName;
        other.Type = VariantTypes::Null;
        other.TypeName = nullptr;
    }

    VariantTypeHandle& VariantTypeHandle::operator=(const VariantTypes& type)
    {
        Type = type;
        PlatformAllocator::Free(TypeName);
        TypeName = nullptr;
        return *this;
    }

    VariantTypeHandle& VariantTypeHandle::operator=(VariantTypeHandle&& other)
    {
        ENGINE_ASSERT(this != &other);
        Swap(Type, other.Type);
        Swap(TypeName, other.TypeName);
        return *this;
    }

    VariantTypeHandle& VariantTypeHandle::operator=(const VariantTypeHandle& other)
    {
        ENGINE_ASSERT(this != &other);
        Type = other.Type;
        PlatformAllocator::Free(TypeName);
        TypeName = nullptr;
        const int32 length = StringUtils::Length(other.TypeName);
        if (length)
        {
            TypeName = static_cast<char*>(PlatformAllocator::Allocate(length + 1));
            Platform::MemoryCopy(TypeName, other.TypeName, length);
            TypeName[length] = 0;
        }
        return *this;
    }

    bool VariantTypeHandle::operator==(const VariantTypes& type) const
    {
        return Type == type && TypeName == nullptr;
    }

    bool VariantTypeHandle::operator==(const VariantTypeHandle& other) const
    {
        if (Type == other.Type)
        {
            if (TypeName && other.TypeName)
            {
                return StringUtils::Compare(TypeName, other.TypeName) == 0;
            }
            return true;
        }
        return false;
    }

    void VariantTypeHandle::SetTypeName(const StringView& typeName)
    {
        if (StringUtils::Length(TypeName) != typeName.Length())
        {
            PlatformAllocator::Free(TypeName);
            TypeName = static_cast<char*>(PlatformAllocator::Allocate(typeName.Length() + 1));
            TypeName[typeName.Length()] = 0;
        }
        StringUtils::ConvertUTF162ANSI(typeName.Get(), TypeName, typeName.Length());
    }

    void VariantTypeHandle::SetTypeName(const StringAnsiView& typeName)
    {
        if (StringUtils::Length(TypeName) != typeName.Length())
        {
            PlatformAllocator::Free(TypeName);
            TypeName = static_cast<char*>(PlatformAllocator::Allocate(typeName.Length() + 1));
            TypeName[typeName.Length()] = 0;
        }
        Platform::MemoryCopy(TypeName, typeName.Get(), typeName.Length());
    }

    const char* VariantTypeHandle::GetTypeName() const
    {
        if (TypeName)
            return TypeName;
        return InBuiltTypesTypeNames[static_cast<int>(Type)];
    }

    VariantTypeHandle VariantTypeHandle::GetElementType() const
    {
        if (Type == VariantTypes::Array)
        {
            if (TypeName)
            {
                const StringAnsiView elementTypename(TypeName, StringUtils::Length(TypeName) - 2);
                return VariantTypeHandle(elementTypename);
            }
            return VariantTypeHandle(VariantTypes::Object);
        }
        return VariantTypeHandle();
    }

    String VariantTypeHandle::ToString() const
    {
        String result;
        switch (Type)
        {
        case VariantTypes::Null:
            result = SE_TEXT("Null");
            break;
        case VariantTypes::Void:
            result = SE_TEXT("Void");
            break;
        case VariantTypes::Bool:
            result = SE_TEXT("Bool");
            break;
        case VariantTypes::Int16:
            result = SE_TEXT("Int16");
            break;
        case VariantTypes::Uint16:
            result = SE_TEXT("Uint16");
            break;
        case VariantTypes::Int:
            result = SE_TEXT("Int");
            break;
        case VariantTypes::Uint:
            result = SE_TEXT("Uint");
            break;
        case VariantTypes::Int64:
            result = SE_TEXT("Int64");
            break;
        case VariantTypes::Uint64:
            result = SE_TEXT("Uint64");
            break;
        case VariantTypes::Float:
            result = SE_TEXT("Float");
            break;
        case VariantTypes::Double:
            result = SE_TEXT("Double");
            break;
        case VariantTypes::Pointer:
            result = SE_TEXT("Pointer");
            break;
        case VariantTypes::String:
            result = SE_TEXT("String");
            break;
        case VariantTypes::Object:
            result = SE_TEXT("Object");
            break;
        case VariantTypes::Structure:
            result = SE_TEXT("Structure");
            break;
        case VariantTypes::Asset:
            result = SE_TEXT("Asset");
            break;
        case VariantTypes::Blob:
            result = SE_TEXT("Blob");
            break;
        case VariantTypes::Enum:
            result = SE_TEXT("Enum");
            break;
        case VariantTypes::Float2:
            result = SE_TEXT("Float2");
            break;
        case VariantTypes::Float3:
            result = SE_TEXT("Float3");
            break;
        case VariantTypes::Float4:
            result = SE_TEXT("Float4");
            break;
        case VariantTypes::Color:
            result = SE_TEXT("Color");
            break;
        case VariantTypes::BoundingBox:
            result = SE_TEXT("BoundingBox");
            break;
        case VariantTypes::BoundingSphere:
            result = SE_TEXT("BoundingSphere");
            break;
        case VariantTypes::Quaternion:
            result = SE_TEXT("Quaternion");
            break;
        case VariantTypes::Transform:
            result = SE_TEXT("Transform");
            break;
        case VariantTypes::Rectangle:
            result = SE_TEXT("Rectangle");
            break;
        case VariantTypes::Ray:
            result = SE_TEXT("Ray");
            break;
        case VariantTypes::Matrix:
            result = SE_TEXT("Matrix");
            break;
        case VariantTypes::Array:
            result = SE_TEXT("Array");
            break;
        case VariantTypes::Dictionary:
            result = SE_TEXT("Dictionary");
            break;
        case VariantTypes::Typename:
            result = SE_TEXT("Type");
            break;
        case VariantTypes::Int2:
            result = SE_TEXT("Int2");
            break;
        case VariantTypes::Int3:
            result = SE_TEXT("Int3");
            break;
        case VariantTypes::Int4:
            result = SE_TEXT("Int4");
            break;
        case VariantTypes::Double2:
            result = SE_TEXT("Double2");
            break;
        case VariantTypes::Double3:
            result = SE_TEXT("Double3");
            break;
        case VariantTypes::Double4:
            result = SE_TEXT("Double4");
            break;
        default: ;
        }
        if (TypeName)
        {
            result += SE_TEXT(" ");
            result += TypeName;
        }
        return result;
    }
	
    VariantTypeHandle::~VariantTypeHandle()
    {
        PlatformAllocator::Free(TypeName);
    }


    const Variant Variant::Zero(0.0f);
    const Variant Variant::One(1.0f);
    const Variant Variant::Null(nullptr);
    const Variant Variant::False(false);
    const Variant Variant::True(true);

    Variant::Variant(const Variant& other)
    {
        Type = VariantTypeHandle();
        *this = other;
    }

    Variant::Variant(Variant&& other) noexcept
        : Type(MoveTemp(other.Type))
    {
        switch (Type.Type)
        {
        case VariantTypes::Object:
            AsObject = other.AsObject;
            if (AsObject)
            {
                // AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(&other);
                // AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
                other.AsObject = nullptr;
            }
            break;
        case VariantTypes::Asset:
            AsAsset = other.AsAsset;
            if (AsAsset)
            {
                AsAsset->OnUnloadedEvent.Unbind<Variant, &Variant::OnAssetUnloaded>(&other);
                AsAsset->OnUnloadedEvent.Bind<Variant, &Variant::OnAssetUnloaded>(this);
                other.AsAsset = nullptr;
            }
            break;
        case VariantTypes::Array:
            new(reinterpret_cast<List<Variant, HeapAllocation>*>(AsData))List<Variant, HeapAllocation>(MoveTemp(*reinterpret_cast<List<Variant, HeapAllocation>*>(other.AsData)));
            reinterpret_cast<List<Variant, HeapAllocation>*>(other.AsData)->~List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            AsDictionary = other.AsDictionary;
            other.AsDictionary = nullptr;
            break;
        case VariantTypes::Null:
        case VariantTypes::Void:
        case VariantTypes::MAX:
            break;
        case VariantTypes::Structure:
        case VariantTypes::Blob:
        case VariantTypes::String:
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
            AsBlob.Data = other.AsBlob.Data;
            AsBlob.Length = other.AsBlob.Length;
            other.AsBlob.Data = nullptr;
            other.AsBlob.Length = 0;
            break;
        default:
            Platform::MemoryCopy(AsData, other.AsData, sizeof(AsData));
            break;
        }
    }

    Variant::Variant(decltype(nullptr))
        : Type(VariantTypes::Null)
    {
    }

    Variant::Variant(const VariantTypeHandle& type)
        : Type(type)
    {
    }

    Variant::Variant(VariantTypeHandle&& type)
        : Type(MoveTemp(type))
    {
    }

    Variant::Variant(bool v)
        : Type(VariantTypes::Bool)
    {
        AsBool = v;
    }

    Variant::Variant(int16 v)
        : Type(VariantTypes::Int16)
    {
        AsInt16 = v;
    }

    Variant::Variant(uint16 v)
        : Type(VariantTypes::Uint16)
    {
        AsUint16 = v;
    }

    Variant::Variant(int32 v)
        : Type(VariantTypes::Int)
    {
        AsInt = v;
    }

    Variant::Variant(uint32 v)
        : Type(VariantTypes::Uint)
    {
        AsUint = v;
    }

    Variant::Variant(int64 v)
        : Type(VariantTypes::Int64)
    {
        AsInt64 = v;
    }

    Variant::Variant(uint64 v)
        : Type(VariantTypes::Uint64)
    {
        AsUint64 = v;
    }

    Variant::Variant(float v)
        : Type(VariantTypes::Float)
    {
        AsFloat = v;
    }

    Variant::Variant(double v)
        : Type(VariantTypes::Double)
    {
        AsDouble = v;
    }

    Variant::Variant(void* v)
        : Type(VariantTypes::Pointer)
    {
        AsPointer = v;
    }

    Variant::Variant(Object* v)
        : Type(VariantTypes::Object)
    {
        AsObject = v;
        if (v)
        {
            // Type.SetTypeName(v->GetType().Fullname);
            // v->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
        }
    }

    Variant::Variant(Asset* v)
        : Type(VariantTypes::Asset)
    {
        AsAsset = v;
        if (v)
        {
            // Type.SetTypeName(v->GetType().Fullname);
            v->AddReference();
            v->OnUnloadedEvent.Bind<Variant, &Variant::OnAssetUnloaded>(this);
        }
    }
    

    Variant::Variant(const StringView& v)
        : Type(VariantTypes::String)
    {
        if (v.Length() > 0)
        {
            const int32 length = v.Length() * sizeof(Char) + 2;
            AsBlob.Data = PlatformAllocator::Allocate(length);
            AsBlob.Length = length;
            Platform::MemoryCopy(AsBlob.Data, v.Get(), length);
            ((Char*)AsBlob.Data)[v.Length()] = 0;
        }
        else
        {
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
        }
    }

    Variant::Variant(const StringAnsiView& v)
        : Type(VariantTypes::String)
    {
        if (v.Length() > 0)
        {
            const int32 length = v.Length() * sizeof(Char) + 2;
            AsBlob.Data = PlatformAllocator::Allocate(length);
            AsBlob.Length = length;
            int32 tmp;
            StringUtils::ConvertANSI2UTF16(v.Get(), (Char*)AsBlob.Data, v.Length(), tmp);
            ((Char*)AsBlob.Data)[v.Length()] = 0;
        }
        else
        {
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
        }
    }

    Variant::Variant(const Char* v)
        : Variant(StringView(v))
    {
    }

    Variant::Variant(const char* v)
        : Variant(StringAnsiView(v))
    {
    }

    Variant::Variant(const UID& v)
    {
        *(UID*)AsData = v;
    }

    Variant::Variant(const Float2& v)
        : Type(VariantTypes::Float2)
    {
        *(Float2*)AsData = v;
    }

    Variant::Variant(const Float3& v)
        : Type(VariantTypes::Float3)
    {
        *(Float3*)AsData = v;
    }

    Variant::Variant(const Float4& v)
        : Type(VariantTypes::Float4)
    {
        *(Float4*)AsData = v;
    }

    Variant::Variant(const Double2& v)
        : Type(VariantTypes::Double2)
    {
        *(Double2*)AsData = v;
    }

    Variant::Variant(const Double3& v)
        : Type(VariantTypes::Double3)
    {
        *(Double3*)AsData = v;
    }

    Variant::Variant(const Double4& v)
        : Type(VariantTypes::Double4)
    {
        AsBlob.Length = sizeof(Double4);
        AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
        *(Double4*)AsBlob.Data = v;
    }

    Variant::Variant(const Int2& v)
        : Type(VariantTypes::Int2)
    {
        *(Int2*)AsData = v;
    }

    Variant::Variant(const Int3& v)
        : Type(VariantTypes::Int3)
    {
        *(Int3*)AsData = v;
    }

    Variant::Variant(const Int4& v)
        : Type(VariantTypes::Int4)
    {
        *(Int4*)AsData = v;
    }

    Variant::Variant(const Color& v)
        : Type(VariantTypes::Color)
    {
        *(Color*)AsData = v;
    }

    Variant::Variant(const Quaternion& v)
        : Type(VariantTypes::Quaternion)
    {
        *(Quaternion*)AsData = v;
    }

    Variant::Variant(const BoundingSphere& v)
        : Type(VariantTypes::BoundingSphere)
    {
#if USE_LARGE_WORLDS
        AsBlob.Length = sizeof(BoundingSphere);
        AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
        *(BoundingSphere*)AsBlob.Data = v;
#else
        *(BoundingSphere*)AsData = v;
#endif
    }

    Variant::Variant(const Rectangle& v)
        : Type(VariantTypes::Rectangle)
    {
        *(Rectangle*)AsData = v;
    }

    Variant::Variant(const BoundingBox& v)
        : Type(VariantTypes::BoundingBox)
    {
#if USE_LARGE_WORLDS
        AsBlob.Length = sizeof(BoundingBox);
        AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
        *(BoundingBox*)AsBlob.Data = v;
#else
        *(BoundingBox*)AsData = v;
#endif
    }

    Variant::Variant(const Transform& v)
        : Type(VariantTypes::Transform)
    {
        AsBlob.Length = sizeof(Transform);
        AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
        *(Transform*)AsBlob.Data = v;
    }

    Variant::Variant(const Ray& v)
        : Type(VariantTypes::Ray)
    {
#if USE_LARGE_WORLDS
        AsBlob.Length = sizeof(Ray);
        AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
        *(Ray*)AsBlob.Data = v;
#else
        *(Ray*)AsData = v;
#endif
    }

    Variant::Variant(const Matrix& v)
        : Type(VariantTypes::Matrix)
    {
        AsBlob.Length = sizeof(Matrix);
        AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
        *(Matrix*)AsBlob.Data = v;
    }

    Variant::Variant(List<Variant>&& v)
        : Type(VariantTypes::Array)
    {
        auto* array = reinterpret_cast<List<Variant, HeapAllocation>*>(AsData);
        new(array)List<Variant, HeapAllocation>(MoveTemp(v));
    }

    Variant::Variant(const List<Variant, HeapAllocation>& v)
        : Type(VariantTypes::Array)
    {
        auto* array = reinterpret_cast<List<Variant, HeapAllocation>*>(AsData);
        new(array)List<Variant, HeapAllocation>(v);
    }

    Variant::Variant(Dictionary<Variant, Variant>&& v)
        : Type(VariantTypes::Dictionary)
    {
        AsDictionary = New<Dictionary<Variant, Variant>>(MoveTemp(v));
    }

    Variant::Variant(const Dictionary<Variant, Variant>& v)
        : Type(VariantTypes::Dictionary)
    {
        AsDictionary = New<Dictionary<Variant, Variant>>(v);
    }

    Variant::Variant(const Span<byte>& v)
        : Type(VariantTypes::Blob)
    {
        AsBlob.Length = v.Length();
        if (AsBlob.Length > 0)
        {
            AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
            Platform::MemoryCopy(AsBlob.Data, v.Get(), AsBlob.Length);
        }
        else
        {
            AsBlob.Data = nullptr;
        }
    }

    Variant::~Variant()
    {
        switch (Type.Type)
        {
        case VariantTypes::Object:
            if (AsObject)
                // AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
                    break;
        case VariantTypes::Asset:
            if (AsAsset)
            {
                AsAsset->OnUnloadedEvent.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
                AsAsset->RemoveReference();
            }
            break;
        case VariantTypes::Structure:
            FreeStructure();
            break;
        case VariantTypes::String:
        case VariantTypes::Blob:
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
        case VariantTypes::Typename:
        case VariantTypes::Double4:
    #if USE_LARGE_WORLDS
        case MaterialVariantType::VariantTypes::BoundingSphere:
        case MaterialVariantType::VariantTypes::BoundingBox:
        case MaterialVariantType::VariantTypes::Ray:
    #endif
            PlatformAllocator::Free(AsBlob.Data);
            break;
        case VariantTypes::Array:
            reinterpret_cast<List<Variant, HeapAllocation>*>(AsData)->~List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            Delete(AsDictionary);
            break;
        default: ;
        }
    }

    Variant& Variant::operator=(Variant&& other)
    {
        ENGINE_ASSERT(this != &other);
        SetType(VariantTypeHandle());
        Type = MoveTemp(other.Type);
        switch (Type.Type)
        {
        case VariantTypes::String:
        case VariantTypes::Structure:
        case VariantTypes::Blob:
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
        case VariantTypes::Typename:
        case VariantTypes::Double4:
    #if USE_LARGE_WORLDS
        case MaterialVariantType::VariantTypes::BoundingSphere:
        case MaterialVariantType::VariantTypes::BoundingBox:
        case MaterialVariantType::VariantTypes::Ray:
    #endif
            AsBlob.Data = other.AsBlob.Data;
            AsBlob.Length = other.AsBlob.Length;
            break;
        case VariantTypes::Object:
            AsObject = other.AsObject;
            if (AsObject)
            {
                // AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(&other);
                // AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
            }
            break;
        case VariantTypes::Asset:
            AsAsset = other.AsAsset;
            if (AsAsset)
            {
                AsAsset->OnUnloadedEvent.Unbind<Variant, &Variant::OnAssetUnloaded>(&other);
                AsAsset->OnUnloadedEvent.Bind<Variant, &Variant::OnAssetUnloaded>(this);
            }
            break;
        case VariantTypes::Array:
            new(reinterpret_cast<List<Variant, HeapAllocation>*>(AsData))List<Variant, HeapAllocation>(MoveTemp(*reinterpret_cast<List<Variant, HeapAllocation>*>(other.AsData)));
            reinterpret_cast<List<Variant, HeapAllocation>*>(other.AsData)->~List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            AsDictionary = other.AsDictionary;
            other.AsDictionary = nullptr;
            break;
        case VariantTypes::Null:
        case VariantTypes::Void:
        case VariantTypes::MAX:
            break;
        default:
            Platform::MemoryCopy(AsData, other.AsData, sizeof(AsData));
            break;
        }
        return *this;
    }

    Variant& Variant::operator=(const Variant& other)
    {
        ENGINE_ASSERT(this != &other);
        SetType(other.Type);
        switch (Type.Type)
        {
        case VariantTypes::Structure:
            CopyStructure(other.AsBlob.Data);
            break;
        case VariantTypes::String:
        case VariantTypes::Blob:
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
        case VariantTypes::Typename:
        case VariantTypes::Double4:
    #if USE_LARGE_WORLDS
        case MaterialVariantType::VariantTypes::BoundingSphere:
        case MaterialVariantType::VariantTypes::BoundingBox:
        case MaterialVariantType::VariantTypes::Ray:
    #endif
            if (other.AsBlob.Data)
            {
                if (!AsBlob.Data || AsBlob.Length != other.AsBlob.Length)
                {
                    PlatformAllocator::Free(AsBlob.Data);
                    AsBlob.Data = PlatformAllocator::Allocate(other.AsBlob.Length);
                }
                Platform::MemoryCopy(AsBlob.Data, other.AsBlob.Data, other.AsBlob.Length);
            }
            else if (AsBlob.Data)
            {
                PlatformAllocator::Free(AsBlob.Data);
                AsBlob.Data = nullptr;
            }
            AsBlob.Length = other.AsBlob.Length;
            break;
        case VariantTypes::Object:
            AsObject = other.AsObject;
            if (other.AsObject)
                // AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);
                    break;
        case VariantTypes::Asset:
            AsAsset = other.AsAsset;
            if (other.AsAsset)
            {
                AsAsset->AddReference();
                AsAsset->OnUnloadedEvent.Bind<Variant, &Variant::OnAssetUnloaded>(this);
            }
            break;
        case VariantTypes::Array:
            *reinterpret_cast<List<Variant, HeapAllocation>*>(AsData) = *reinterpret_cast<const List<Variant, HeapAllocation>*>(other.AsData);
            break;
        case VariantTypes::Dictionary:
            if (AsDictionary)
                *AsDictionary = *other.AsDictionary;
            else if (other.AsDictionary)
                AsDictionary = New<Dictionary<Variant, Variant>>(*other.AsDictionary);
            break;
        case VariantTypes::Null:
        case VariantTypes::Void:
        case VariantTypes::MAX:
            break;
        default:
            Platform::MemoryCopy(AsData, other.AsData, sizeof(AsData));
            break;
        }
        return *this;
    }

    bool Variant::operator==(const Variant& other) const
    {
        if (Type == other.Type)
        {
            switch (Type.Type)
            {
            case VariantTypes::Null:
                return true;
            case VariantTypes::Bool:
                return AsBool == other.AsBool;
            case VariantTypes::Int16:
                return AsInt16 == other.AsInt16;
            case VariantTypes::Uint16:
                return AsUint16 == other.AsUint16;
            case VariantTypes::Int:
                return AsInt == other.AsInt;
            case VariantTypes::Uint:
                return AsUint == other.AsUint;
            case VariantTypes::Int64:
                return AsInt64 == other.AsInt64;
            case VariantTypes::Uint64:
            case VariantTypes::Enum:
                return AsUint64 == other.AsUint64;
            case VariantTypes::Float:
                return Math::IsNearEqual(AsFloat, other.AsFloat);
            case VariantTypes::Double:
                return Math::Abs(AsDouble - other.AsDouble) < Math::ZeroTolerance;
            case VariantTypes::Pointer:
                return AsPointer == other.AsPointer;
            case VariantTypes::String:
                if (AsBlob.Data == nullptr && other.AsBlob.Data == nullptr)
                    return true;
                if (AsBlob.Data == nullptr)
                    return false;
                if (other.AsBlob.Data == nullptr)
                    return false;
                return AsBlob.Length == other.AsBlob.Length && StringUtils::Compare(static_cast<const Char*>(AsBlob.Data), static_cast<const Char*>(other.AsBlob.Data), AsBlob.Length - 1) == 0;
            case VariantTypes::Object:
                return AsObject == other.AsObject;
            case VariantTypes::Structure:
            case VariantTypes::Blob:
            case VariantTypes::Transform:
            case VariantTypes::Matrix:
            case VariantTypes::Double4:
                return AsBlob.Length == other.AsBlob.Length && Platform::MemoryCompare(AsBlob.Data, other.AsBlob.Data, AsBlob.Length) == 0;
            case VariantTypes::Asset:
                return AsAsset == other.AsAsset;
            case VariantTypes::Float2:
                return *(Float2*)AsData == *(Float2*)other.AsData;
            case VariantTypes::Float3:
                return *(Float3*)AsData == *(Float3*)other.AsData;
            case VariantTypes::Float4:
                return *(Float4*)AsData == *(Float4*)other.AsData;
            case VariantTypes::Int2:
                return *(Int2*)AsData == *(Int2*)other.AsData;
            case VariantTypes::Int3:
                return *(Int3*)AsData == *(Int3*)other.AsData;
            case VariantTypes::Int4:
                return *(Int4*)AsData == *(Int4*)other.AsData;
            case VariantTypes::Double2:
                return *(Double2*)AsData == *(Double2*)other.AsData;
            case VariantTypes::Double3:
                return *(Double3*)AsData == *(Double3*)other.AsData;
            case VariantTypes::Color:
                return *(Color*)AsData == *(Color*)other.AsData;
            case VariantTypes::Quaternion:
                return *(Quaternion*)AsData == *(Quaternion*)other.AsData;
            case VariantTypes::Rectangle:
                return *(Rectangle*)AsData == *(Rectangle*)other.AsData;
            case VariantTypes::BoundingSphere:
                return AsBoundingSphere() == other.AsBoundingSphere();
            case VariantTypes::BoundingBox:
                return AsBoundingBox() == other.AsBoundingBox();
            case VariantTypes::Ray:
                return AsRay() == other.AsRay();
            case VariantTypes::Array:
            {
                const auto* array = reinterpret_cast<const List<Variant, HeapAllocation>*>(AsData);
                const List<Variant, HeapAllocation>* otherArray = reinterpret_cast<const List<Variant, HeapAllocation>*>(other.AsData);
                if (array->Count() != otherArray->Count())
                    return false;
                for (int32 i = 0; i < array->Count(); i++)
                {
                    if (array->At(i) != otherArray->At(i))
                        return false;
                }
                return true;
            }
            case VariantTypes::Dictionary:
                if (AsDictionary == nullptr && other.AsDictionary == nullptr)
                    return true;
                if (!AsDictionary || !other.AsDictionary)
                    return false;
                if (AsDictionary->Count() != other.AsDictionary->Count())
                    return false;
                for (auto& i : *AsDictionary)
                {
                    if (!other.AsDictionary->ContainsKey(i.Key) || other.AsDictionary->At(i.Key) != i.Value)
                        return false;
                }
                return true;
            case VariantTypes::Typename:
                if (AsBlob.Data == nullptr && other.AsBlob.Data == nullptr)
                    return true;
                if (AsBlob.Data == nullptr)
                    return false;
                if (other.AsBlob.Data == nullptr)
                    return false;
                return AsBlob.Length == other.AsBlob.Length && StringUtils::Compare(static_cast<const char*>(AsBlob.Data), static_cast<const char*>(other.AsBlob.Data), AsBlob.Length - 1) == 0;
            default:
                return false;
            }
        }
        if (CanCast(*this, other.Type))
        {
            return Cast(*this, other.Type) == other;
        }
        return false;
    }

    bool Variant::operator<(const Variant& other) const
    {
        if (Type == other.Type)
        {
            switch (Type.Type)
            {
            case VariantTypes::Null:
            case VariantTypes::Void:
                return true;
            case VariantTypes::Bool:
                return AsBool < other.AsBool;
            case VariantTypes::Int16:
                return AsInt16 < other.AsInt16;
            case VariantTypes::Uint16:
                return AsUint16 < other.AsUint16;
            case VariantTypes::Int:
                return AsInt < other.AsInt;
            case VariantTypes::Uint:
                return AsUint < other.AsUint;
            case VariantTypes::Int64:
                return AsInt64 < other.AsInt64;
            case VariantTypes::Uint64:
            case VariantTypes::Enum:
                return AsUint64 < other.AsUint64;
            case VariantTypes::Float:
                return AsFloat < other.AsFloat;
            case VariantTypes::Double:
                return AsDouble < other.AsDouble;
            case VariantTypes::Pointer:
                return AsPointer < other.AsPointer;
            case VariantTypes::String:
                return StringUtils::Compare(AsBlob.Data ? (const Char*)AsBlob.Data : SE_TEXT(""), other.AsBlob.Data ? (const Char*)other.AsBlob.Data : SE_TEXT("")) < 0;
            case VariantTypes::Typename:
                return StringUtils::Compare(AsBlob.Data ? (const char*)AsBlob.Data : "", other.AsBlob.Data ? (const char*)other.AsBlob.Data : "") < 0;
            default:
                return false;
            }
        }
        if (CanCast(*this, other.Type))
        {
            return Cast(*this, other.Type) < other;
        }
        return false;
    }

    Variant::operator bool() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return AsBool;
        case VariantTypes::Int16:
            return AsInt16 != 0;
        case VariantTypes::Uint16:
            return AsUint16 != 0;
        case VariantTypes::Int:
            return AsInt != 0;
        case VariantTypes::Uint:
            return AsUint != 0;
        case VariantTypes::Int64:
            return AsInt64 != 0;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return AsUint64 != 0;
        case VariantTypes::Float:
            return !Math::IsZero(AsFloat);
        case VariantTypes::Double:
            return !Math::IsZero(AsDouble);
        case VariantTypes::Pointer:
            return AsPointer != nullptr;
        case VariantTypes::String:
        case VariantTypes::Typename:
            return AsBlob.Length > 1;
        case VariantTypes::Object:
            return AsObject != nullptr;
        case VariantTypes::Asset:
            return AsAsset != nullptr;
        default:
            return false;
        }
    }

    Variant::operator Char() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return AsBool ? 1 : 0;
        case VariantTypes::Int16:
            return (Char)AsInt16;
        case VariantTypes::Uint16:
            return (Char)AsUint16;
        case VariantTypes::Int:
            return (Char)AsInt;
        case VariantTypes::Uint:
            return (Char)AsUint;
        case VariantTypes::Int64:
            return (Char)AsInt64;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return (Char)AsUint64;
        case VariantTypes::Float:
            return (Char)AsFloat;
        case VariantTypes::Double:
            return (Char)AsDouble;
        case VariantTypes::Pointer:
            return (Char)(intptr)AsPointer;
        default:
            return 0;
        }
    }

    Variant::operator int8() const
    {
        return (int8)operator int64();
    }

    Variant::operator int16() const
    {
        return (int16)operator int64();
    }

    Variant::operator int32() const
    {
        return (int32)operator int64();
    }

    Variant::operator int64() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return AsBool ? 1 : 0;
        case VariantTypes::Int16:
            return (int64)AsInt16;
        case VariantTypes::Uint16:
            return (int64)AsUint16;
        case VariantTypes::Int:
            return (int64)AsInt;
        case VariantTypes::Uint:
            return (int64)AsUint;
        case VariantTypes::Int64:
            return AsInt64;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return (int64)AsUint64;
        case VariantTypes::Float:
            return (int64)AsFloat;
        case VariantTypes::Double:
            return (int64)AsDouble;
        case VariantTypes::Pointer:
            return (int64)AsPointer;
        case VariantTypes::Float2:
            return (int64)AsFloat2().x;
        case VariantTypes::Float3:
            return (int64)AsFloat3().x;
        case VariantTypes::Float4:
            return (int64)AsFloat4().x;
        case VariantTypes::Double2:
            return (int64)AsDouble2().x;
        case VariantTypes::Double3:
            return (int64)AsDouble3().x;
        case VariantTypes::Double4:
            return (int64)AsDouble4().x;
        case VariantTypes::Int2:
            return (int64)AsInt2().x;
        case VariantTypes::Int3:
            return (int64)AsInt3().x;
        case VariantTypes::Int4:
            return (int64)AsInt4().x;
        default:
            return 0;
        }
    }

    Variant::operator uint8() const
    {
        return (uint8)operator uint64();
    }

    Variant::operator uint16() const
    {
        return (uint16)operator uint64();
    }

    Variant::operator uint32() const
    {
        return (uint32)operator uint64();
    }

    Variant::operator uint64() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return AsBool ? 1 : 0;
        case VariantTypes::Int16:
            return (uint64)AsInt16;
        case VariantTypes::Uint16:
            return (uint64)AsUint16;
        case VariantTypes::Int:
            return (uint64)AsInt;
        case VariantTypes::Uint:
            return (uint64)AsUint;
        case VariantTypes::Int64:
            return (uint64)AsInt64;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return AsUint64;
        case VariantTypes::Float:
            return (uint64)AsFloat;
        case VariantTypes::Double:
            return (uint64)AsDouble;
        case VariantTypes::Pointer:
            return (uint64)AsPointer;
        case VariantTypes::Float2:
            return (uint64)AsFloat2().x;
        case VariantTypes::Float3:
            return (uint64)AsFloat3().x;
        case VariantTypes::Float4:
            return (uint64)AsFloat4().x;
        case VariantTypes::Double2:
            return (uint64)AsDouble2().x;
        case VariantTypes::Double3:
            return (uint64)AsDouble3().x;
        case VariantTypes::Double4:
            return (uint64)AsDouble4().x;
        case VariantTypes::Int2:
            return (uint64)AsInt2().x;
        case VariantTypes::Int3:
            return (uint64)AsInt3().x;
        case VariantTypes::Int4:
            return (uint64)AsInt4().x;
        default:
            return 0;
        }
    }

    Variant::operator float() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return AsBool ? 1.0f : 0.0f;
        case VariantTypes::Int16:
            return (float)AsInt16;
        case VariantTypes::Uint16:
            return (float)AsUint16;
        case VariantTypes::Int:
            return (float)AsInt;
        case VariantTypes::Uint:
            return (float)AsUint;
        case VariantTypes::Int64:
            return (float)AsInt64;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return (float)AsUint64;
        case VariantTypes::Float:
            return AsFloat;
        case VariantTypes::Double:
            return (float)AsDouble;
        case VariantTypes::Float2:
            return AsFloat2().x;
        case VariantTypes::Float3:
            return AsFloat3().x;
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return AsFloat4().x;
        case VariantTypes::Double2:
            return (float)AsDouble2().x;
        case VariantTypes::Double3:
            return (float)AsDouble3().x;
        case VariantTypes::Double4:
            return (float)AsDouble4().x;
        case VariantTypes::Int2:
            return (float)AsInt2().x;
        case VariantTypes::Int3:
            return (float)AsInt3().x;
        case VariantTypes::Int4:
            return (float)AsInt4().x;
        case VariantTypes::Pointer:
            return AsPointer ? 1.0f : 0.0f;
        case VariantTypes::Object:
            return AsObject ? 1.0f : 0.0f;
        case VariantTypes::Asset:
            return AsAsset ? 1.0f : 0.0f;
        case VariantTypes::Blob:
            return AsBlob.Length > 0 ? 1.0f : 0.0f;
        default:
            return 0;
        }
    }

    Variant::operator double() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return AsBool ? 1.0 : 0.0;
        case VariantTypes::Int16:
            return (double)AsInt16;
        case VariantTypes::Uint16:
            return (double)AsUint16;
        case VariantTypes::Int:
            return (double)AsInt;
        case VariantTypes::Uint:
            return (double)AsUint;
        case VariantTypes::Int64:
            return (double)AsInt64;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return (double)AsUint64;
        case VariantTypes::Float:
            return (double)AsFloat;
        case VariantTypes::Double:
            return AsDouble;
        case VariantTypes::Float2:
            return (double)AsFloat2().x;
        case VariantTypes::Float3:
            return (double)AsFloat3().x;
        case VariantTypes::Float4:
            return (double)AsFloat4().x;
        case VariantTypes::Double2:
            return (double)AsDouble2().x;
        case VariantTypes::Double3:
            return (double)AsDouble3().x;
        case VariantTypes::Double4:
            return (double)AsDouble4().x;
        case VariantTypes::Int2:
            return (double)AsInt2().x;
        case VariantTypes::Int3:
            return (double)AsInt3().x;
        case VariantTypes::Int4:
            return (double)AsInt4().x;
        default:
            return 0;
        }
    }

    Variant::operator void*() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Pointer:
            return AsPointer;
        case VariantTypes::Object:
            return AsObject;
        case VariantTypes::Asset:
            return AsAsset;
        case VariantTypes::Structure:
        case VariantTypes::Blob:
            return AsBlob.Data;
        default:
            return nullptr;
        }
    }

    Variant::operator StringView() const
    {
        switch (Type.Type)
        {
        case VariantTypes::String:
            return StringView((const Char*)AsBlob.Data, AsBlob.Length != 0 ? AsBlob.Length / sizeof(Char) - 1 : 0);
        default:
            return StringView::Empty;
        }
    }

    Variant::operator StringAnsiView() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Typename:
            return StringAnsiView((const char*)AsBlob.Data, AsBlob.Length != 0 ? AsBlob.Length - 1 : 0);
        default:
            return StringAnsiView::Empty;
        }
    }

    Variant::operator Object*() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Object:
            return AsObject;
        default:
            return nullptr;
        }
    }

    /*Variant::operator ScriptingObject*() const
    {
        switch (Type.Type)
        {
        case MaterialVariantType::VariantTypes::Object:
            return AsObject;
        case MaterialVariantType::VariantTypes::Asset:
            return AsAsset;
        default:
            return nullptr;
        }
    }*/

    Variant::operator Asset*() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Asset:
            return AsAsset;
        default:
            return nullptr;
        }
    }

    Variant::operator Float2() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Float2(AsBool ? 1.0f : 0.0f);
        case VariantTypes::Int16:
            return Float2((float)AsInt16);
        case VariantTypes::Uint16:
            return Float2((float)AsUint16);
        case VariantTypes::Int:
            return Float2((float)AsInt);
        case VariantTypes::Uint:
            return Float2((float)AsUint);
        case VariantTypes::Int64:
            return Float2((float)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Float2((float)AsUint64);
        case VariantTypes::Float:
            return Float2(AsFloat);
        case VariantTypes::Double:
            return Float2((float)AsDouble);
        case VariantTypes::Pointer:
            return Float2((float)(intptr)AsPointer);
        case VariantTypes::Float2:
            return *(Float2*)AsData;
        case VariantTypes::Float3:
            return Float2(*(Float3*)AsData);
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return Float2(*(Float4*)AsData);
        case VariantTypes::Double2:
            return Float2(AsDouble2());
        case VariantTypes::Double3:
            return Float2(AsDouble3());
        case VariantTypes::Double4:
            return Float2(AsDouble4());
        case VariantTypes::Int2:
            return Float2(AsInt2());
        case VariantTypes::Int3:
            return Float2(AsInt3());
        case VariantTypes::Int4:
            return Float2(AsInt4());
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Float2::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Float2*)AsBlob.Data;*/
        default:
            return Float2::Zero;
        }
    }

    Variant::operator Float3() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Float3(AsBool ? 1.0f : 0.0f);
        case VariantTypes::Int16:
            return Float3((float)AsInt16);
        case VariantTypes::Uint16:
            return Float3((float)AsUint16);
        case VariantTypes::Int:
            return Float3((float)AsInt);
        case VariantTypes::Uint:
            return Float3((float)AsUint);
        case VariantTypes::Int64:
            return Float3((float)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Float3((float)AsUint64);
        case VariantTypes::Float:
            return Float3(AsFloat);
        case VariantTypes::Double:
            return Float3((float)AsDouble);
        case VariantTypes::Pointer:
            return Float3((float)(intptr)AsPointer);
        case VariantTypes::Float2:
            return Float3(*(Float2*)AsData, 0.0f);
        case VariantTypes::Float3:
            return *(Float3*)AsData;
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return Float3(*(Float4*)AsData);
        case VariantTypes::Double2:
            return Float3(AsDouble2());
        case VariantTypes::Double3:
            return Float3(AsDouble3());
        case VariantTypes::Double4:
            return Float3(AsDouble4());
        case VariantTypes::Int2:
            return Float3(AsInt2(), 0.0f);
        case VariantTypes::Int3:
            return Float3(AsInt3());
        case VariantTypes::Int4:
            return Float3(AsInt4());
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Float3::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Float3*)AsBlob.Data;*/
        default:
            return Float3::Zero;
        }
    }

    Variant::operator Float4() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Float4(AsBool ? 1.0f : 0.0f);
        case VariantTypes::Int16:
            return Float4((float)AsInt16);
        case VariantTypes::Uint16:
            return Float4((float)AsUint16);
        case VariantTypes::Int:
            return Float4((float)AsInt);
        case VariantTypes::Uint:
            return Float4((float)AsUint);
        case VariantTypes::Int64:
            return Float4((float)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Float4((float)AsUint64);
        case VariantTypes::Float:
            return Float4(AsFloat);
        case VariantTypes::Double:
            return Float4((float)AsDouble);
        case VariantTypes::Pointer:
            return Float4((float)(intptr)AsPointer);
        case VariantTypes::Float2:
            return Float4(*(Float2*)AsData, 0.0f, 0.0f);
        case VariantTypes::Float3:
            return Float4(*(Float3*)AsData, 0.0f);
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return *(Float4*)AsData;
        case VariantTypes::Double2:
            return Float4(AsDouble2(), 0.0f, 0.0f);
        case VariantTypes::Double3:
            return Float4(AsDouble3(), 0.0f);
        case VariantTypes::Double4:
            return Float4(AsDouble4());
        case VariantTypes::Int2:
            return Float4(AsInt2(), 0.0f, 0.0f);
        case VariantTypes::Int3:
            return Float4(AsInt3(), 0.0f);
        case VariantTypes::Int4:
            return Float4(AsInt4());
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Float4::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Float4*)AsBlob.Data;*/
        default:
            return Float4::Zero;
        }
    }

    Variant::operator Double2() const
    {
        return Double2(operator Double3());
    }

    Variant::operator Double3() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Double3(AsBool ? 1.0 : 0.0);
        case VariantTypes::Int16:
            return Double3((double)AsInt16);
        case VariantTypes::Uint16:
            return Double3((double)AsUint16);
        case VariantTypes::Int:
            return Double3((double)AsInt);
        case VariantTypes::Uint:
            return Double3((double)AsUint);
        case VariantTypes::Int64:
            return Double3((double)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Double3((double)AsUint64);
        case VariantTypes::Float:
            return Double3(AsFloat);
        case VariantTypes::Double:
            return Double3(AsDouble);
        case VariantTypes::Pointer:
            return Double3((double)(intptr)AsPointer);
        case VariantTypes::Float2:
            return Double3(AsFloat2(), 0.0);
        case VariantTypes::Float3:
            return Double3(AsFloat3());
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return Double3(AsFloat4());
        case VariantTypes::Double2:
            return Double3(AsDouble2());
        case VariantTypes::Double3:
            return AsDouble3();
        case VariantTypes::Double4:
            return Double3(AsDouble4());
        case VariantTypes::Int2:
            return Double3(AsInt2(), 0.0);
        case VariantTypes::Int3:
            return Double3(AsInt3());
        case VariantTypes::Int4:
            return Double3(AsInt4());
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Double3::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Double3*)AsBlob.Data;*/
        default:
            return Double3::Zero;
        }
    }

    Variant::operator Double4() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Double4(AsBool ? 1.0 : 0.0);
        case VariantTypes::Int16:
            return Double4((double)AsInt16);
        case VariantTypes::Uint16:
            return Double4((double)AsUint16);
        case VariantTypes::Int:
            return Double4((double)AsInt);
        case VariantTypes::Uint:
            return Double4((double)AsUint);
        case VariantTypes::Int64:
            return Double4((double)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Double4((double)AsUint64);
        case VariantTypes::Float:
            return Double4(AsFloat);
        case VariantTypes::Double:
            return Double4(AsDouble);
        case VariantTypes::Pointer:
            return Double4((double)(intptr)AsPointer);
        case VariantTypes::Float2:
            return Double4(*(Float2*)AsData, 0.0, 0.0);
        case VariantTypes::Float3:
            return Double4(*(Float3*)AsData, 0.0);
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return Double4(*(const Float4*)AsData);
        case VariantTypes::Double2:
            return Double4(AsDouble2(), 0.0, 0.0);
        case VariantTypes::Double3:
            return Double4(AsDouble3(), 0.0);
        case VariantTypes::Double4:
            return AsDouble4();
        case VariantTypes::Int2:
            return Double4(AsInt2(), 0.0, 0.0);
        case VariantTypes::Int3:
            return Double4(AsInt3(), 0.0);
        case VariantTypes::Int4:
            return Double4(AsInt4());
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Double4::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Double4*)AsBlob.Data;*/
        default:
            return Double4::Zero;
        }
    }

    Variant::operator Int2() const
    {
        return Int2(operator Int4());
    }

    Variant::operator Int3() const
    {
        return Int3(operator Int4());
    }

    Variant::operator Int4() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Int4((int32)(AsBool ? 1 : 0));
        case VariantTypes::Int16:
            return Int4(AsInt16);
        case VariantTypes::Uint16:
            return Int4((int32)AsUint16);
        case VariantTypes::Int:
            return Int4(AsInt);
        case VariantTypes::Uint:
            return Int4((int32)AsUint);
        case VariantTypes::Int64:
            return Int4((int32)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Int4((int32)AsUint64);
        case VariantTypes::Float:
            return Int4((int32)AsFloat);
        case VariantTypes::Double:
            return Int4((int32)AsDouble);
        case VariantTypes::Pointer:
            return Int4((int32)(intptr)AsPointer);
        case VariantTypes::Float2:
            return Int4(*(Float2*)AsData, 0, 0);
        case VariantTypes::Float3:
            return Int4(*(Float3*)AsData, 0);
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return Int4(*(Float4*)AsData);
        case VariantTypes::Int2:
            return Int4(*(Int2*)AsData, 0, 0);
        case VariantTypes::Int3:
            return Int4(*(Int3*)AsData, 0);
        case VariantTypes::Int4:
            return *(Int4*)AsData;
        case VariantTypes::Double2:
            return Int4(AsDouble2(), 0, 0);
        case VariantTypes::Double3:
            return Int4(AsDouble3(), 0);
        case VariantTypes::Double4:
            return Int4(AsDouble4());
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Int4::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Int4*)AsBlob.Data;*/
        default:
            return Int4::Zero;
        }
    }

    Variant::operator Color() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Bool:
            return Color(AsBool ? 1.0f : 0.0f);
        case VariantTypes::Int16:
            return Color((float)AsInt16);
        case VariantTypes::Uint16:
            return Color((float)AsUint16);
        case VariantTypes::Int:
            return Color((float)AsInt);
        case VariantTypes::Uint:
            return Color((float)AsUint);
        case VariantTypes::Int64:
            return Color((float)AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return Color((float)AsUint64);
        case VariantTypes::Float:
            return Color(AsFloat);
        case VariantTypes::Double:
            return Color((float)AsDouble);
        case VariantTypes::Pointer:
            return Color((float)(intptr)AsPointer);
        case VariantTypes::Float2:
            return Color((*(Float2*)AsData).x, (*(Float2*)AsData).y, 0.0f, 1.0f);
        case VariantTypes::Float3:
            return Color(*(Float3*)AsData, 1.0f);
        case VariantTypes::Float4:
        case VariantTypes::Color:
            return *(Color*)AsData;
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Color::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Color*)AsBlob.Data;*/
        default:
            return Colors::Black;
        }
    }

    Variant::operator Quaternion() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Float3:
            return Quaternion::Euler(*(Float3*)AsData);
        case VariantTypes::Double3:
            return Quaternion::Euler((Float3)*(Double3*)AsData);
        case VariantTypes::Quaternion:
            return *(Quaternion*)AsData;
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Quaternion::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Quaternion*)AsBlob.Data;*/
        default:
            return Quaternion::Identity;
        }
    }

    Variant::operator UID() const
    {
        switch (Type.Type)
        {
        case VariantTypes::UID:
            return *(UID*)AsData;
        case VariantTypes::Object:
            return UID::Empty; //AsObject ? AsObject->GetID() : UID::Empty;
        case VariantTypes::Asset:
            return AsAsset ? AsAsset->GetID() : UID::Empty;
        default:
            return UID::Empty;
        }
    }

    Variant::operator BoundingSphere() const
    {
        switch (Type.Type)
        {
        case VariantTypes::BoundingSphere:
            return AsBoundingSphere();
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, BoundingSphere::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(BoundingSphere*)AsBlob.Data;*/
        default:
            return BoundingSphere::Empty;
        }
    }

    Variant::operator BoundingBox() const
    {
        switch (Type.Type)
        {
        case VariantTypes::BoundingBox:
            return AsBoundingBox();
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, BoundingBox::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(BoundingBox*)AsBlob.Data;*/
        default:
            return BoundingBox::Empty;
        }
    }

    Variant::operator Transform() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Transform:
            return *(Transform*)AsBlob.Data;
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Transform::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Transform*)AsBlob.Data;*/
        default:
            return Transform::Identity;
        }
    }

    Variant::operator Matrix() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Matrix:
            return *(Matrix*)AsBlob.Data;
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Matrix::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Matrix*)AsBlob.Data;*/
        default:
            return Matrix::Identity;
        }
    }

    Variant::operator Ray() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Ray:
            return AsRay();
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Ray::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Ray*)AsBlob.Data;*/
        default:
            return Ray::Identity;
        }
    }

    Variant::operator Rectangle() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Rectangle:
            return *(Rectangle*)AsData;
            /*case MaterialVariantType::VariantTypes::Structure:
                if (StringUtils::Compare(Type.TypeName, Rectangle::TypeInitializer.GetType().Fullname.Get()) == 0)
                    return *(Rectangle*)AsBlob.Data;*/
        default:
            return Rectangle::Empty;
        }
    }

    const Float2& Variant::AsVector2() const
    {
        return *(const Float2*)AsData;
    }

    const Float3& Variant::AsVector3() const
    {
        return *(const Float3*)AsData;
    }

    const Float4& Variant::AsVector4() const
    {
#if USE_LARGE_WORLDS
        return *(const Vector4*)AsBlob.Data;
#else
        return *(const Float4*)AsData;
#endif
    }

    const Float2& Variant::AsFloat2() const
    {
        return *(const Float2*)AsData;
    }

    Float3& Variant::AsFloat3()
    {
        return *(Float3*)AsData;
    }

    const Float3& Variant::AsFloat3() const
    {
        return *(const Float3*)AsData;
    }

    const Float4& Variant::AsFloat4() const
    {
        return *(const Float4*)AsData;
    }

    const Double2& Variant::AsDouble2() const
    {
        return *(const Double2*)AsData;
    }

    const Double3& Variant::AsDouble3() const
    {
        return *(const Double3*)AsData;
    }

    const Double4& Variant::AsDouble4() const
    {
        return *(const Double4*)AsBlob.Data;
    }

    const Int2& Variant::AsInt2() const
    {
        return *(const Int2*)AsData;
    }

    const Int3& Variant::AsInt3() const
    {
        return *(const Int3*)AsData;
    }

    const Int4& Variant::AsInt4() const
    {
        return *(const Int4*)AsData;
    }

    const Color& Variant::AsColor() const
    {
        return *(const Color*)AsData;
    }

    const Quaternion& Variant::AsQuaternion() const
    {
        return *(const Quaternion*)AsData;
    }

    const Rectangle& Variant::AsRectangle() const
    {
        return *(const Rectangle*)AsData;
    }

    BoundingSphere& Variant::AsBoundingSphere()
    {
#if USE_LARGE_WORLDS
        return *(BoundingSphere*)AsBlob.Data;
#else
        return *(BoundingSphere*)AsData;
#endif
    }

    const BoundingSphere& Variant::AsBoundingSphere() const
    {
#if USE_LARGE_WORLDS
        return *(const BoundingSphere*)AsBlob.Data;
#else
        return *(const BoundingSphere*)AsData;
#endif
    }

    BoundingBox& Variant::AsBoundingBox()
    {
#if USE_LARGE_WORLDS
        return *(BoundingBox*)AsBlob.Data;
#else
        return *(BoundingBox*)AsData;
#endif
    }

    const BoundingBox& Variant::AsBoundingBox() const
    {
#if USE_LARGE_WORLDS
        return *(const BoundingBox*)AsBlob.Data;
#else
        return *(const BoundingBox*)AsData;
#endif
    }

    Ray& Variant::AsRay()
    {
#if USE_LARGE_WORLDS
        return *(Ray*)AsBlob.Data;
#else
        return *(Ray*)AsData;
#endif
    }

    const Ray& Variant::AsRay() const
    {
#if USE_LARGE_WORLDS
        return *(const Ray*)AsBlob.Data;
#else
        return *(const Ray*)AsData;
#endif
    }

    Transform& Variant::AsTransform()
    {
        return *(Transform*)AsBlob.Data;
    }

    const Transform& Variant::AsTransform() const
    {
        return *(const Transform*)AsBlob.Data;
    }

    const Matrix& Variant::AsMatrix() const
    {
        return *(const Matrix*)AsBlob.Data;
    }

    List<Variant>& Variant::AsArray()
    {
        return *reinterpret_cast<List<Variant, HeapAllocation>*>(AsData);
    }

    const List<Variant>& Variant::AsArray() const
    {
        return *reinterpret_cast<const List<Variant, HeapAllocation>*>(AsData);
    }

    void Variant::SetType(const VariantTypeHandle& type)
    {
        if (Type == type)
            return;

        switch (Type.Type)
        {
        case VariantTypes::Object:
            if (AsObject)
            {
                // AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
            }
            break;
        case VariantTypes::Asset:
            if (AsAsset)
            {
                AsAsset->OnUnloadedEvent.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
                AsAsset->RemoveReference();
            }
            break;
        case VariantTypes::Structure:
            FreeStructure();
            break;
        case VariantTypes::String:
        case VariantTypes::Blob:
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
        case VariantTypes::Typename:
        case VariantTypes::Double4:
    #if USE_LARGE_WORLDS
        case MaterialVariantType::VariantTypes::BoundingSphere:
        case MaterialVariantType::VariantTypes::BoundingBox:
        case MaterialVariantType::VariantTypes::Ray:
    #endif
            PlatformAllocator::Free(AsBlob.Data);
            break;
        case VariantTypes::Array:
            reinterpret_cast<List<Variant, HeapAllocation>*>(AsData)->~List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            if (AsDictionary)
                Delete(AsDictionary);
            break;
        default: ;
        }

        Type = type;

        switch (Type.Type)
        {
        case VariantTypes::String:
        case VariantTypes::Blob:
        case VariantTypes::Typename:
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
            break;
        case VariantTypes::Object:
            AsObject = nullptr;
            break;
        case VariantTypes::Asset:
            AsAsset = nullptr;
            break;
        case VariantTypes::Double4:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Double4));
            AsBlob.Length = sizeof(Double4);
            break;
#if USE_LARGE_WORLDS
        case MaterialVariantType::VariantTypes::BoundingSphere:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(BoundingSphere));
            AsBlob.Length = sizeof(BoundingSphere);
            break;
        case MaterialVariantType::VariantTypes::BoundingBox:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(BoundingBox));
            AsBlob.Length = sizeof(BoundingBox);
            break;
        case MaterialVariantType::VariantTypes::Ray:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Ray));
            AsBlob.Length = sizeof(Ray);
            break;
#endif
        case VariantTypes::Transform:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Transform));
            AsBlob.Length = sizeof(Transform);
            break;
        case VariantTypes::Matrix:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Matrix));
            AsBlob.Length = sizeof(Matrix);
            break;
        case VariantTypes::Array:
            new(reinterpret_cast<List<Variant, HeapAllocation>*>(AsData))List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            AsDictionary = New<Dictionary<Variant, Variant>>();
            break;
        case VariantTypes::Structure:
            AllocStructure();
            break;
        default: ;
        }
    }

    void Variant::SetType(VariantTypeHandle&& type)
    {
        if (Type == type)
            return;

        switch (Type.Type)
        {
        case VariantTypes::Object:
            if (AsObject)
            {
                // AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
            }
            break;
        case VariantTypes::Asset:
            if (AsAsset)
            {
                AsAsset->OnUnloadedEvent.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
                AsAsset->RemoveReference();
            }
            break;
        case VariantTypes::Structure:
            FreeStructure();
            break;
        case VariantTypes::String:
        case VariantTypes::Blob:
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
        case VariantTypes::Typename:
        case VariantTypes::Double4:
    #if USE_LARGE_WORLDS
        case VariantType::VariantTypes::BoundingSphere:
        case VariantType::VariantTypes::BoundingBox:
        case VariantType::VariantTypes::Ray:
    #endif
            PlatformAllocator::Free(AsBlob.Data);
            break;
        case VariantTypes::Array:
            reinterpret_cast<List<Variant, HeapAllocation>*>(AsData)->~List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            if (AsDictionary)
                Delete(AsDictionary);
            break;
        default: ;
        }

        Type = MoveTemp(type);

        switch (Type.Type)
        {
        case VariantTypes::String:
        case VariantTypes::Blob:
        case VariantTypes::Typename:
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
            break;
        case VariantTypes::Object:
            AsObject = nullptr;
            break;
        case VariantTypes::Asset:
            AsAsset = nullptr;
            break;
        case VariantTypes::Double4:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Double4));
            AsBlob.Length = sizeof(Double4);
            break;
#if USE_LARGE_WORLDS
        case VariantType::VariantTypes::BoundingSphere:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(BoundingSphere));
            AsBlob.Length = sizeof(BoundingSphere);
            break;
        case VariantType::VariantTypes::BoundingBox:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(BoundingBox));
            AsBlob.Length = sizeof(BoundingBox);
            break;
        case VariantType::VariantTypes::Ray:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Ray));
            AsBlob.Length = sizeof(Ray);
            break;
#endif
        case VariantTypes::Transform:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Transform));
            AsBlob.Length = sizeof(Transform);
            break;
        case VariantTypes::Matrix:
            AsBlob.Data = PlatformAllocator::Allocate(sizeof(Matrix));
            AsBlob.Length = sizeof(Matrix);
            break;
        case VariantTypes::Array:
            new(reinterpret_cast<List<Variant, HeapAllocation>*>(AsData))List<Variant, HeapAllocation>();
            break;
        case VariantTypes::Dictionary:
            AsDictionary = New<Dictionary<Variant, Variant>>();
            break;
        case VariantTypes::Structure:
            AllocStructure();
            break;
        default: ;
        }
    }

    void Variant::SetString(const StringView& str)
    {
        SetType(VariantTypeHandle(VariantTypes::String));
        if (str.Length() <= 0)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
            return;
        }
        const int32 length = str.Length() * sizeof(Char) + 2;
        if (AsBlob.Length != length)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = PlatformAllocator::Allocate(length);
            AsBlob.Length = length;
        }
        Platform::MemoryCopy(AsBlob.Data, str.Get(), length);
        ((Char*)AsBlob.Data)[str.Length()] = 0;
    }

    void Variant::SetString(const StringAnsiView& str)
    {
        SetType(VariantTypeHandle(VariantTypes::String));
        if (str.Length() <= 0)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
            return;
        }
        const int32 length = str.Length() * sizeof(Char) + 2;
        if (AsBlob.Length != length)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = PlatformAllocator::Allocate(length);
            AsBlob.Length = length;
        }
        int32 tmp;
        StringUtils::ConvertANSI2UTF16(str.Get(), (Char*)AsBlob.Data, str.Length(), tmp);
        ((Char*)AsBlob.Data)[str.Length()] = 0;
    }

    void Variant::SetTypename(const StringView& typeName)
    {
        SetType(VariantTypeHandle(VariantTypes::Typename));
        if (typeName.Length() <= 0)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
            return;
        }
        const int32 length = typeName.Length() + 1;
        if (AsBlob.Length != length)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = PlatformAllocator::Allocate(length);
            AsBlob.Length = length;
        }
        StringUtils::ConvertUTF162ANSI(typeName.Get(), (char*)AsBlob.Data, typeName.Length());
        ((char*)AsBlob.Data)[typeName.Length()] = 0;
    }

    void Variant::SetTypename(const StringAnsiView& typeName)
    {
        SetType(VariantTypeHandle(VariantTypes::Typename));
        if (typeName.Length() <= 0)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
            return;
        }
        const int32 length = typeName.Length() + 1;
        if (AsBlob.Length != length)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = PlatformAllocator::Allocate(length);
            AsBlob.Length = length;
        }
        Platform::MemoryCopy(AsBlob.Data, typeName.Get(), length);
        ((char*)AsBlob.Data)[typeName.Length()] = 0;
    }

    void Variant::SetBlob(int32 length)
    {
        SetType(VariantTypeHandle(VariantTypes::Blob));
        if (AsBlob.Length != length)
        {
            PlatformAllocator::Free(AsBlob.Data);
            AsBlob.Data = length > 0 ? PlatformAllocator::Allocate(length) : nullptr;
            AsBlob.Length = length;
        }
    }

    void Variant::SetBlob(const void* data, int32 length)
    {
        SetBlob(length);
        Platform::MemoryCopy(AsBlob.Data, data, length);
    }

    void Variant::SetObject(Object* object)
    {
        /*if (Type.Type != VariantType::VariantTypes::Object)
            SetType(VariantType(VariantType::VariantTypes::Object));
        if (AsObject)
            AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
        AsObject = object;
        if (object)
            object->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(this);*/
    }

    void Variant::SetAsset(Asset* asset)
    {
        if (Type.Type != VariantTypes::Asset)
            SetType(VariantTypeHandle(VariantTypes::Asset));
        if (AsAsset)
        {
            asset->OnUnloadedEvent.Unbind<Variant, &Variant::OnAssetUnloaded>(this);
            asset->RemoveReference();
        }
        AsAsset = asset;
        if (asset)
        {
            asset->AddReference();
            asset->OnUnloadedEvent.Bind<Variant, &Variant::OnAssetUnloaded>(this);
        }
    }

    String Variant::ToString() const
    {
        switch (Type.Type)
        {
        case VariantTypes::Null:
            return SE_TEXT("null");
        case VariantTypes::Bool:
            return AsBool ? SE_TEXT("true") : SE_TEXT("false");
        case VariantTypes::Int16:
            return StringUtils::ToString(AsInt16);
        case VariantTypes::Uint16:
            return StringUtils::ToString(AsUint16);
        case VariantTypes::Int:
            return StringUtils::ToString(AsInt);
        case VariantTypes::Uint:
            return StringUtils::ToString(AsUint);
        case VariantTypes::Enum:
            /*if (Type.TypeName)
            {
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(Type.TypeName));
                if (typeHandle && typeHandle.GetType().Type == ScriptingTypes::Enum)
                {
                    const auto items = typeHandle.GetType().Enum.Items;
                    for (int32 i = 0; items[i].Name; i++)
                    {
                        if (items[i].Value == AsUint)
                            return String(items[i].Name);
                    }
                }
            }*/
                return StringUtils::ToString(AsUint);
        case VariantTypes::Int64:
            return StringUtils::ToString(AsInt64);
        case VariantTypes::Uint64:
            return StringUtils::ToString(AsUint64);
        case VariantTypes::Float:
            return StringUtils::ToString(AsFloat);
        case VariantTypes::Double:
            return StringUtils::ToString(AsDouble);
        case VariantTypes::Pointer:
            return String::Format(SE_TEXT("{}"), AsPointer);
        case VariantTypes::String:
            return String((const Char*)AsBlob.Data, AsBlob.Length ? AsBlob.Length / sizeof(Char) - 1 : 0);
        case VariantTypes::Object:
            return AsObject ? AsObject->ToString() : SE_TEXT("null");
        case VariantTypes::Asset:
            return AsAsset ? AsAsset->ToString() : SE_TEXT("null");
        case VariantTypes::Structure:
        case VariantTypes::Blob:
        case VariantTypes::Dictionary:
        case VariantTypes::Array:
            return Type.ToString();
        case VariantTypes::Float2:
            return AsFloat2().ToString();
        case VariantTypes::Float3:
            return AsFloat3().ToString();
        case VariantTypes::Float4:
            return AsFloat4().ToString();
        case VariantTypes::Double2:
            return AsDouble2().ToString();
        case VariantTypes::Double3:
            return AsDouble3().ToString();
        case VariantTypes::Double4:
            return AsDouble4().ToString();
        case VariantTypes::Int2:
            return AsInt2().ToString();
        case VariantTypes::Int3:
            return AsInt3().ToString();
        case VariantTypes::Int4:
            return AsInt4().ToString();
        case VariantTypes::Color:
            return AsColor().ToString();
        case VariantTypes::BoundingSphere:
            return AsBoundingSphere().ToString();
        case VariantTypes::Quaternion:
            return AsQuaternion().ToString();
        case VariantTypes::Rectangle:
            return AsRectangle().ToString();
        case VariantTypes::BoundingBox:
            return AsBoundingBox().ToString();
        case VariantTypes::Transform:
            return AsTransform().ToString();
        case VariantTypes::Ray:
            return AsRay().ToString();
        case VariantTypes::Matrix:
            return AsMatrix().ToString();
        case VariantTypes::Typename:
            return String((const char*)AsBlob.Data, AsBlob.Length ? AsBlob.Length - 1 : 0);
        default:
            return String::Empty;
        }
    }

    void Variant::Inline()
    {
        VariantTypes type = VariantTypes::Null;
        byte data[sizeof(Matrix)];
        if (Type.Type == VariantTypes::Structure && AsBlob.Data && AsBlob.Length <= sizeof(Matrix))
        {
            for (int32 i = 2; i < (int)VariantTypes::MAX; i++)
            {
                if (StringUtils::Compare(Type.TypeName, InBuiltTypesTypeNames[i]) == 0)
                {
                    type = (VariantTypes)i;
                    break;
                }
            }
            if (type == VariantTypes::Null)
            {
                // Aliases
                if (StringUtils::Compare(Type.TypeName, "FlaxEngine.Vector2") == 0)
                    type = VariantTypes::Vector2;
                else if (StringUtils::Compare(Type.TypeName, "FlaxEngine.Float3") == 0)
                    type = VariantTypes::Float3;
                else if (StringUtils::Compare(Type.TypeName, "FlaxEngine.Vector4") == 0)
                    type = VariantTypes::Vector4;
            }
            if (type != VariantTypes::Null)
            {
                ENGINE_ASSERT(sizeof(data) >= AsBlob.Length);
                Platform::MemoryCopy(data, AsBlob.Data, AsBlob.Length);
            }
        }
        if (type != VariantTypes::Null)
        {
            switch (type)
            {
            case VariantTypes::Bool:
                *this = *(bool*)data;
                break;
            case VariantTypes::Int:
                *this = *(int32*)data;
                break;
            case VariantTypes::Uint:
                *this = *(uint32*)data;
                break;
            case VariantTypes::Int64:
                *this = *(int64*)data;
                break;
            case VariantTypes::Uint64:
                *this = *(uint64*)data;
                break;
            case VariantTypes::Float:
                *this = *(float*)data;
                break;
            case VariantTypes::Double:
                *this = *(double*)data;
                break;
            case VariantTypes::Float2:
                *this = *(Float2*)data;
                break;
            case VariantTypes::Float3:
                *this = *(Float3*)data;
                break;
            case VariantTypes::Float4:
                *this = *(Float4*)data;
                break;
            case VariantTypes::Color:
                *this = *(Color*)data;
                break;
            case VariantTypes::BoundingBox:
                *this = Variant(*(BoundingBox*)data);
                break;
            case VariantTypes::BoundingSphere:
                *this = *(BoundingSphere*)data;
                break;
            case VariantTypes::Quaternion:
                *this = *(Quaternion*)data;
                break;
            case VariantTypes::Transform:
                *this = Variant(*(Transform*)data);
                break;
            case VariantTypes::Rectangle:
                *this = *(Rectangle*)data;
                break;
            case VariantTypes::Ray:
                *this = Variant(*(Ray*)data);
                break;
            case VariantTypes::Matrix:
                *this = Variant(*(Matrix*)data);
                break;
            case VariantTypes::Int2:
                *this = *(Int2*)data;
                break;
            case VariantTypes::Int3:
                *this = *(Int3*)data;
                break;
            case VariantTypes::Int4:
                *this = *(Int4*)data;
                break;
            case VariantTypes::Int16:
                *this = *(int16*)data;
                break;
            case VariantTypes::Uint16:
                *this = *(uint16*)data;
                break;
            case VariantTypes::Double2:
                *this = *(Double2*)data;
                break;
            case VariantTypes::Double3:
                *this = *(Double3*)data;
                break;
            case VariantTypes::Double4:
                *this = *(Double4*)data;
                break;
            }
        }
    }

    void Variant::InvertInline()
    {
        byte data[sizeof(Matrix)];
        switch (Type.Type)
        {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Int64:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Pointer:
        case VariantTypes::String:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
    #if !USE_LARGE_WORLDS
        case VariantTypes::BoundingSphere:
        case VariantTypes::BoundingBox:
        case VariantTypes::Ray:
    #endif
        case VariantTypes::Quaternion:
        case VariantTypes::Rectangle:
        case VariantTypes::Int2:
        case VariantTypes::Int3:
        case VariantTypes::Int4:
        case VariantTypes::Int16:
        case VariantTypes::Uint16:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            static_assert(sizeof(data) >= sizeof(AsData), "Invalid memory size.");
            Platform::MemoryCopy(data, AsData, sizeof(AsData));
            break;
#if USE_LARGE_WORLDS
        case VariantType::VariantTypes::BoundingSphere:
        case VariantType::VariantTypes::BoundingBox:
        case VariantType::VariantTypes::Ray:
    #endif
        case VariantTypes::Transform:
        case VariantTypes::Matrix:
            ENGINE_ASSERT(sizeof(data) >= AsBlob.Length);
            Platform::MemoryCopy(data, AsBlob.Data, AsBlob.Length);
            break;
        default:
            return; // Not used
        }
        SetType(VariantTypeHandle(VariantTypes::Structure, InBuiltTypesTypeNames[(int)Type.Type]));
        CopyStructure(data);
    }

    Variant Variant::NewValue(const StringAnsiView& typeName)
    {
        Variant v;
        /*const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            switch (type.Type)
            {
            case ScriptingTypes::Script:
                v.SetType(VariantType(VariantType::VariantTypes::Object, typeName));
                v.AsObject = type.Script.Spawn(ScriptingObjectSpawnParams(Guid::New(), typeHandle));
                if (v.AsObject)
                    v.AsObject->Deleted.Bind<Variant, &Variant::OnObjectDeleted>(&v);
                break;
            case ScriptingTypes::Structure:
                v.SetType(VariantType(VariantType::VariantTypes::Structure, typeName));
                break;
            case ScriptingTypes::Enum:
                v.SetType(VariantType(VariantType::VariantTypes::Enum, typeName));
                v.AsUint64 = 0;
                break;
            default:
                LOG_ERROR("Render", "Unsupported scripting type '{}' for Variant", typeName.ToString());
                break;
            }
        }
#if USE_CSHARP
        else if (const auto mclass = Scripting::FindClass(typeName))
        {
            // Fallback to C#-only types
            if (mclass->IsEnum())
            {
                v.SetType(VariantType(VariantType::VariantTypes::Enum, typeName));
                v.AsUint64 = 0;
            }
            else if (mclass->IsValueType())
            {
                v.SetType(VariantType(VariantType::VariantTypes::Structure, typeName));
            }
            else
            {
                v.SetType(VariantType(VariantType::VariantTypes::ManagedObject, typeName));
                MObject* instance = mclass->CreateInstance();
                if (instance)
                {
                    v.MANAGED_GC_HANDLE = MCore::GCHandle::New(instance);
                }
            }
        }
#endif
        else */if (typeName.HasChars())
        {
            LOG_WARNING("Render", "Missing scripting type \'{0}\'", String(typeName));
        }
        v.Inline(); // Wrap in-built types
        return MoveTemp(v);
    }

    void Variant::DeleteValue()
    {
        // Delete any object owned by the Variant
        switch (Type.Type)
        {
        case VariantTypes::Object:
            /*if (AsObject)
            {
                AsObject->Deleted.Unbind<Variant, &Variant::OnObjectDeleted>(this);
                AsObject->DeleteObject();
                AsObject = nullptr;
            }*/
                break;
        }

        // Go back to null
        SetType(VariantTypeHandle(VariantTypes::Null));
    }

    bool Variant::CanCast(const Variant& v, const VariantTypeHandle& to)
    {
        if (v.Type == to)
            return true;
        switch (v.Type.Type)
        {
        case VariantTypes::Bool:
            switch (to.Type)
            {
        case VariantTypes::Int16:
        case VariantTypes::Uint16:
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
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Int16:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Int:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int16:
        case VariantTypes::Int64:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Uint16:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Uint:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Uint:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint16:
        case VariantTypes::Uint64:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Int64:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Uint64:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Float:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Int64:
        case VariantTypes::Uint64:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Double:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Uint16:
        case VariantTypes::Int64:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Float2:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Float3:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float4:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Float4:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Color:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        case VariantTypes::Color:
            switch (to.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint16:
        case VariantTypes::Uint:
        case VariantTypes::Int16:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return true;
        default:
            return false;
            }
        default:
            return false;
        }
    }

    Variant Variant::Cast(const Variant& v, const VariantTypeHandle& to)
    {
        if (v.Type == to)
            return v;
        switch (v.Type.Type)
        {
        case VariantTypes::Bool:
            switch (to.Type)
            {
        case VariantTypes::Int16: // No portable literal suffix for short ( Available in MSVC but Undocumented : i16 )
            return Variant((int16)(v.AsBool ? 1 : 0));
        case VariantTypes::Uint16: // No portable literal suffix for short ( Available in MSVC but Undocumented : ui16 )
            return Variant((uint16)(v.AsBool ? 1 : 0));
        case VariantTypes::Int:
            return Variant(v.AsBool ? 1 : 0);
        case VariantTypes::Uint:
            return Variant(v.AsBool ? 1u : 0u);
        case VariantTypes::Int64:
            return Variant(v.AsBool ? 1ll : 0ll);
        case VariantTypes::Uint64:
            return Variant(v.AsBool ? 1ull : 0ull);
        case VariantTypes::Float:
            return Variant(v.AsBool ? 1.0f : 0.0f);
        case VariantTypes::Double:
            return Variant(v.AsBool ? 1.0 : 0.0);
        case VariantTypes::Float2:
            return Variant(Float2(v.AsBool ? 1.0f : 0.0f));
        case VariantTypes::Float3:
            return Variant(Float3(v.AsBool ? 1.0f : 0.0f));
        case VariantTypes::Float4:
            return Variant(Float2(v.AsBool ? 1.0f : 0.0f));
        case VariantTypes::Color:
            return Variant(Color(v.AsBool ? 1.0f : 0.0f));
        case VariantTypes::Double2:
            return Variant(Double2(v.AsBool ? 1.0 : 0.0));
        case VariantTypes::Double3:
            return Variant(Double3(v.AsBool ? 1.0 : 0.0));
        case VariantTypes::Double4:
            return Variant(Double4(v.AsBool ? 1.0 : 0.0));
        default: ;
            }
            break;
        case VariantTypes::Int16:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(v.AsInt != 0);
        case VariantTypes::Int:
            return Variant((int32)v.AsInt16);
        case VariantTypes::Int64:
            return Variant((int64)v.AsInt16);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsInt16);
        case VariantTypes::Uint:
            return Variant((uint32)v.AsInt16);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsInt16);
        case VariantTypes::Float:
            return Variant((float)v.AsInt16);
        case VariantTypes::Double:
            return Variant((double)v.AsInt16);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsInt16));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsInt16));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsInt16));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsInt16));
        case VariantTypes::Double2:
            return Variant(Double2((double)v.AsInt16));
        case VariantTypes::Double3:
            return Variant(Double3((double)v.AsInt16));
        case VariantTypes::Double4:
            return Variant(Double4((double)v.AsInt16));
        default: ;
            }
            break;
        case VariantTypes::Int:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(v.AsInt != 0);
        case VariantTypes::Int16:
            return Variant((int16)v.AsInt);
        case VariantTypes::Int64:
            return Variant((int64)v.AsInt);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsInt);
        case VariantTypes::Uint:
            return Variant((uint32)v.AsInt);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsInt);
        case VariantTypes::Float:
            return Variant((float)v.AsInt);
        case VariantTypes::Double:
            return Variant((double)v.AsInt);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsInt));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsInt));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsInt));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsInt));
        default: ;
            }
            break;
        case VariantTypes::Uint16:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(v.AsUint16 != 0);
        case VariantTypes::Int16:
            return Variant((int16)v.AsUint16);
        case VariantTypes::Int:
            return Variant((int32)v.AsUint16);
        case VariantTypes::Int64:
            return Variant((int64)v.AsUint16);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsUint16);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsUint16);
        case VariantTypes::Float:
            return Variant((float)v.AsUint16);
        case VariantTypes::Double:
            return Variant((double)v.AsUint16);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsUint16));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsUint16));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsUint16));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsUint16));
        case VariantTypes::Double2:
            return Variant(Double2((double)v.AsUint16));
        case VariantTypes::Double3:
            return Variant(Double3((double)v.AsUint16));
        case VariantTypes::Double4:
            return Variant(Double4((double)v.AsUint16));
        default: ;
            }
            break;
        case VariantTypes::Uint:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(v.AsUint != 0);
        case VariantTypes::Int16:
            return Variant((int16)v.AsUint);
        case VariantTypes::Int:
            return Variant((int32)v.AsUint);
        case VariantTypes::Int64:
            return Variant((int64)v.AsUint);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsUint);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsUint);
        case VariantTypes::Float:
            return Variant((float)v.AsUint);
        case VariantTypes::Double:
            return Variant((double)v.AsUint);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsUint));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsUint));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsUint));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsUint));
        case VariantTypes::Double2:
            return Variant(Double2((double)v.AsUint));
        case VariantTypes::Double3:
            return Variant(Double3((double)v.AsUint));
        case VariantTypes::Double4:
            return Variant(Double4((double)v.AsUint));
        default: ;
            }
            break;
        case VariantTypes::Int64:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(v.AsInt64 != 0);
        case VariantTypes::Int16:
            return Variant((int16)v.AsInt64);
        case VariantTypes::Int:
            return Variant((int32)v.AsInt64);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsInt64);
        case VariantTypes::Uint:
            return Variant((uint32)v.AsInt64);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsInt64);
        case VariantTypes::Float:
            return Variant((float)v.AsInt64);
        case VariantTypes::Double:
            return Variant((double)v.AsInt64);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsInt64));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsInt64));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsInt64));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsInt64));
        case VariantTypes::Double2:
            return Variant(Double2((double)v.AsInt64));
        case VariantTypes::Double3:
            return Variant(Double3((double)v.AsInt64));
        case VariantTypes::Double4:
            return Variant(Double4((double)v.AsInt64));
        default: ;
            }
            break;
        case VariantTypes::Uint64:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(v.AsUint64 != 0);
        case VariantTypes::Int16:
            return Variant((int16)v.AsUint64);
        case VariantTypes::Int:
            return Variant((int32)v.AsUint64);
        case VariantTypes::Int64:
            return Variant((int64)v.AsUint64);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsUint16);
        case VariantTypes::Uint:
            return Variant((uint32)v.AsUint);
        case VariantTypes::Float:
            return Variant((float)v.AsUint64);
        case VariantTypes::Double:
            return Variant((double)v.AsUint64);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsInt));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsInt));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsInt));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsInt));
        case VariantTypes::Double2:
            return Variant(Double2((double)v.AsInt));
        case VariantTypes::Double3:
            return Variant(Double3((double)v.AsInt));
        case VariantTypes::Double4:
            return Variant(Double4((double)v.AsInt));
        default: ;
            }
            break;
        case VariantTypes::Float:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(Math::Abs(v.AsFloat) > Math::ZeroTolerance);
        case VariantTypes::Int16:
            return Variant((int16)v.AsFloat);
        case VariantTypes::Int:
            return Variant((int32)v.AsFloat);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsFloat);
        case VariantTypes::Uint:
            return Variant((uint32)v.AsFloat);
        case VariantTypes::Int64:
            return Variant((int64)v.AsFloat);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsFloat);
        case VariantTypes::Double:
            return Variant((double)v.AsFloat);
        case VariantTypes::Float2:
            return Variant(Float2(v.AsFloat));
        case VariantTypes::Float3:
            return Variant(Float3(v.AsFloat));
        case VariantTypes::Float4:
            return Variant(Float4(v.AsFloat));
        case VariantTypes::Color:
            return Variant(Color(v.AsFloat));
        case VariantTypes::Double2:
            return Variant(Double2(v.AsFloat));
        case VariantTypes::Double3:
            return Variant(Double3(v.AsFloat));
        case VariantTypes::Double4:
            return Variant(Double4(v.AsFloat));
        default: ;
            }
            break;
        case VariantTypes::Double:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(Math::Abs(v.AsDouble) > Math::ZeroTolerance);
        case VariantTypes::Int16:
            return Variant((int16)v.AsDouble);
        case VariantTypes::Int:
            return Variant((int32)v.AsDouble);
        case VariantTypes::Uint16:
            return Variant((uint16)v.AsDouble);
        case VariantTypes::Uint:
            return Variant((uint32)v.AsDouble);
        case VariantTypes::Int64:
            return Variant((int64)v.AsDouble);
        case VariantTypes::Uint64:
            return Variant((uint64)v.AsDouble);
        case VariantTypes::Float:
            return Variant((float)v.AsDouble);
        case VariantTypes::Float2:
            return Variant(Float2((float)v.AsDouble));
        case VariantTypes::Float3:
            return Variant(Float3((float)v.AsDouble));
        case VariantTypes::Float4:
            return Variant(Float4((float)v.AsDouble));
        case VariantTypes::Color:
            return Variant(Color((float)v.AsDouble));
        case VariantTypes::Double2:
            return Variant(Double2(v.AsDouble));
        case VariantTypes::Double3:
            return Variant(Double3(v.AsDouble));
        case VariantTypes::Double4:
            return Variant(Double4(v.AsDouble));
        default: ;
            }
            break;
        case VariantTypes::Float2:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(Math::Abs(((Float2*)v.AsData)->x) > Math::ZeroTolerance);
        case VariantTypes::Int16:
            return Variant((int16)((Float2*)v.AsData)->x);
        case VariantTypes::Int:
            return Variant((int32)((Float2*)v.AsData)->x);
        case VariantTypes::Uint16:
            return Variant((uint16)((Float2*)v.AsData)->x);
        case VariantTypes::Uint:
            return Variant((uint32)((Float2*)v.AsData)->x);
        case VariantTypes::Int64:
            return Variant((int64)((Float2*)v.AsData)->x);
        case VariantTypes::Uint64:
            return Variant((uint64)((Float2*)v.AsData)->x);
        case VariantTypes::Float:
            return Variant((float)((Float2*)v.AsData)->x);
        case VariantTypes::Double:
            return Variant((double)((Float2*)v.AsData)->x);
        case VariantTypes::Float3:
            return Variant(Float3(*(Float2*)v.AsData, 0.0f));
        case VariantTypes::Float4:
            return Variant(Float4(*(Float2*)v.AsData, 0.0f, 0.0f));
        case VariantTypes::Color:
            return Variant(Color(((Float2*)v.AsData)->x, ((Float2*)v.AsData)->y, 0.0f, 0.0f));
        case VariantTypes::Double2:
            return Variant(Double2(*(Float2*)v.AsData));
        case VariantTypes::Double3:
            return Variant(Double3(*(Float2*)v.AsData, 0.0));
        case VariantTypes::Double4:
            return Variant(Double4(*(Float2*)v.AsData, 0.0, 0.0));
        default: ;
            }
            break;
        case VariantTypes::Float3:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(Math::Abs(((Float3*)v.AsData)->x) > Math::ZeroTolerance);
        case VariantTypes::Int16:
            return Variant((int16)((Float3*)v.AsData)->x);
        case VariantTypes::Int:
            return Variant((int32)((Float3*)v.AsData)->x);
        case VariantTypes::Uint16:
            return Variant((uint16)((Float3*)v.AsData)->x);
        case VariantTypes::Uint:
            return Variant((uint32)((Float3*)v.AsData)->x);
        case VariantTypes::Int64:
            return Variant((int64)((Float3*)v.AsData)->x);
        case VariantTypes::Uint64:
            return Variant((uint64)((Float3*)v.AsData)->x);
        case VariantTypes::Float:
            return Variant((float)((Float3*)v.AsData)->x);
        case VariantTypes::Double:
            return Variant((double)((Float3*)v.AsData)->x);
        case VariantTypes::Float2:
            return Variant(Float2(*(Float3*)v.AsData));
        case VariantTypes::Float4:
            return Variant(Float4(*(Float3*)v.AsData, 0.0f));
        case VariantTypes::Color:
            return Variant(Color(((Float3*)v.AsData)->x, ((Float3*)v.AsData)->y, ((Float3*)v.AsData)->z, 0.0f));
        case VariantTypes::Double2:
            return Variant(Double2(*(Float3*)v.AsData));
        case VariantTypes::Double3:
            return Variant(Double3(*(Float3*)v.AsData));
        case VariantTypes::Double4:
            return Variant(Double4(*(Float3*)v.AsData, 0.0));
        default: ;
            }
            break;
        case VariantTypes::Float4:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(Math::Abs(((Float4*)v.AsData)->x) > Math::ZeroTolerance);
        case VariantTypes::Int16:
            return Variant((int16)((Float4*)v.AsData)->x);
        case VariantTypes::Int:
            return Variant((int32)((Float4*)v.AsData)->x);
        case VariantTypes::Uint16:
            return Variant((uint16)((Float4*)v.AsData)->x);
        case VariantTypes::Uint:
            return Variant((uint32)((Float4*)v.AsData)->x);
        case VariantTypes::Int64:
            return Variant((int64)((Float4*)v.AsData)->x);
        case VariantTypes::Uint64:
            return Variant((uint64)((Float4*)v.AsData)->x);
        case VariantTypes::Float:
            return Variant((float)((Float4*)v.AsData)->x);
        case VariantTypes::Double:
            return Variant((double)((Float4*)v.AsData)->x);
        case VariantTypes::Float2:
            return Variant(Float2(*(Float4*)v.AsData));
        case VariantTypes::Float3:
            return Variant(Float3(*(Float4*)v.AsData));
        case VariantTypes::Color:
            return Variant(*(Float4*)v.AsData);
        case VariantTypes::Double2:
            return Variant(Double2(*(Float4*)v.AsData));
        case VariantTypes::Double3:
            return Variant(Double3(*(Float4*)v.AsData));
        case VariantTypes::Double4:
            return Variant(Double4(*(Float4*)v.AsData));
        default: ;
            }
            break;
        case VariantTypes::Color:
            switch (to.Type)
            {
        case VariantTypes::Bool:
            return Variant(Math::Abs(((Color*)v.AsData)->r) > Math::ZeroTolerance);
        case VariantTypes::Int16:
            return Variant((int16)((Color*)v.AsData)->r);
        case VariantTypes::Int:
            return Variant((int32)((Color*)v.AsData)->r);
        case VariantTypes::Uint16:
            return Variant((uint16)((Color*)v.AsData)->r);
        case VariantTypes::Uint:
            return Variant((uint32)((Color*)v.AsData)->r);
        case VariantTypes::Int64:
            return Variant((int64)((Color*)v.AsData)->r);
        case VariantTypes::Uint64:
            return Variant((uint64)((Color*)v.AsData)->r);
        case VariantTypes::Float:
            return Variant((float)((Color*)v.AsData)->r);
        case VariantTypes::Double:
            return Variant((double)((Color*)v.AsData)->r);
        case VariantTypes::Float2:
            return Variant(Float2(*(Color*)v.AsData));
        case VariantTypes::Float3:
            return Variant(Float3(*(Color*)v.AsData));
        case VariantTypes::Float4:
            return Variant(*(Color*)v.AsData);
        case VariantTypes::Double2:
            return Variant(Double2(*(Color*)v.AsData));
        case VariantTypes::Double3:
            return Variant(Double3(*(Color*)v.AsData));
        case VariantTypes::Double4:
            return Variant(Double4(*(Color*)v.AsData));
        default: ;
            }
            break;
        default: ;
        }
        LOG_ERROR("Render", "Cannot cast Variant from {0} to {1}", v.Type.ToString(), to.ToString());
        return Null;
    }

    bool Variant::NearEqual(const Variant& a, const Variant& b, float epsilon)
    {
        if (a.Type != b.Type)
            return false;
        switch (a.Type.Type)
        {
        case VariantTypes::Int16:
            return Math::Abs(a.AsInt16 - b.AsInt16) <= (int32)epsilon;
        case VariantTypes::Int:
            return Math::Abs(a.AsInt - b.AsInt) <= (int32)epsilon;
        case VariantTypes::Int64:
            return Math::Abs(a.AsInt64 - b.AsInt64) <= (int64)epsilon;
        case VariantTypes::Float:
            return Math::IsNearEqual(a.AsFloat, b.AsFloat, epsilon);
        case VariantTypes::Double:
            return Math::IsNearEqual((float)a.AsDouble, (float)b.AsDouble, epsilon);
        case VariantTypes::Float2:
            return Float2::NearEqual(*(Float2*)a.AsData, *(Float2*)b.AsData, epsilon);
        case VariantTypes::Float3:
            return Float3::NearEqual(*(Float3*)a.AsData, *(Float3*)b.AsData, epsilon);
        case VariantTypes::Float4:
            return Float4::NearEqual(*(Float4*)a.AsData, *(Float4*)b.AsData, epsilon);
        case VariantTypes::Double2:
            return Double2::NearEqual(*(Double2*)a.AsData, *(Double2*)b.AsData, epsilon);
        case VariantTypes::Double3:
            return Double3::NearEqual(*(Double3*)a.AsData, *(Double3*)b.AsData, epsilon);
        case VariantTypes::Double4:
            return Double4::NearEqual(*(Double4*)a.AsBlob.Data, *(Double4*)b.AsBlob.Data, epsilon);
        case VariantTypes::Color:
            return Color::NearEqual(*(Color*)a.AsData, *(Color*)b.AsData, epsilon);
        case VariantTypes::BoundingSphere:
            return BoundingSphere::NearEqual(a.AsBoundingSphere(), b.AsBoundingSphere(), epsilon);
        case VariantTypes::Quaternion:
            return Quaternion::NearEqual(*(Quaternion*)a.AsData, *(Quaternion*)b.AsData, epsilon);
        case VariantTypes::Rectangle:
            return Rectangle::NearEqual(*(Rectangle*)a.AsData, *(Rectangle*)b.AsData, epsilon);
        case VariantTypes::BoundingBox:
            return BoundingBox::NearEqual(a.AsBoundingBox(), b.AsBoundingBox(), epsilon);
        case VariantTypes::Transform:
            return Transform::NearEqual(*(Transform*)a.AsBlob.Data, *(Transform*)b.AsBlob.Data, epsilon);
        case VariantTypes::Ray:
            return Ray::NearEqual(a.AsRay(), b.AsRay(), epsilon);
        default:
            return a == b;
        }
    }

    Variant Variant::Lerp(const Variant& a, const Variant& b, float alpha)
    {
        if (a.Type != b.Type)
            return a;
        switch (a.Type.Type)
        {
        case VariantTypes::Bool:
            return alpha < 0.5f ? a : b;
        case VariantTypes::Int16:
            return Math::Lerp(a.AsInt16, b.AsInt16, alpha);
        case VariantTypes::Int:
            return Math::Lerp(a.AsInt, b.AsInt, alpha);
        case VariantTypes::Uint16:
            return Math::Lerp(a.AsUint16, b.AsUint16, alpha);
        case VariantTypes::Uint:
            return Math::Lerp(a.AsUint, b.AsUint, alpha);
        case VariantTypes::Int64:
            return Math::Lerp(a.AsInt64, b.AsInt64, alpha);
        case VariantTypes::Uint64:
            return Math::Lerp(a.AsUint64, b.AsUint64, alpha);
        case VariantTypes::Float:
            return Math::Lerp(a.AsFloat, b.AsFloat, alpha);
        case VariantTypes::Float2:
            return Float2::Lerp(*(Float2*)a.AsData, *(Float2*)b.AsData, alpha);
        case VariantTypes::Float3:
            return Float3::Lerp(*(Float3*)a.AsData, *(Float3*)b.AsData, alpha);
        case VariantTypes::Float4:
            return Float4::Lerp(*(Float4*)a.AsData, *(Float4*)b.AsData, alpha);
        case VariantTypes::Double2:
            return Double2::Lerp(*(Double2*)a.AsData, *(Double2*)b.AsData, alpha);
        case VariantTypes::Double3:
            return Double3::Lerp(*(Double3*)a.AsData, *(Double3*)b.AsData, alpha);
        case VariantTypes::Double4:
            return Double4::Lerp(*(Double4*)a.AsBlob.Data, *(Double4*)b.AsBlob.Data, alpha);
        case VariantTypes::Color:
            return Color::Lerp(*(Color*)a.AsData, *(Color*)b.AsData, alpha);
        case VariantTypes::Quaternion:
            return Quaternion::Lerp(*(Quaternion*)a.AsData, *(Quaternion*)b.AsData, alpha);
        case VariantTypes::BoundingSphere:
            return BoundingSphere(Float3::Lerp(a.AsBoundingSphere().Center, b.AsBoundingSphere().Center, alpha), Math::Lerp(a.AsBoundingSphere().Radius, b.AsBoundingSphere().Radius, alpha));
        case VariantTypes::Rectangle:
            return Rectangle(Float2::Lerp((*(Rectangle*)a.AsData).Location, (*(Rectangle*)b.AsData).Location, alpha), Float2::Lerp((*(Rectangle*)a.AsData).Size, (*(Rectangle*)b.AsData).Size, alpha));
        case VariantTypes::Transform:
            return Variant(Transform::Lerp(*(Transform*)a.AsBlob.Data, *(Transform*)b.AsBlob.Data, alpha));
        case VariantTypes::BoundingBox:
            return Variant(BoundingBox(Float3::Lerp(a.AsBoundingBox().Minimum, b.AsBoundingBox().Minimum, alpha), Float3::Lerp(a.AsBoundingBox().Maximum, b.AsBoundingBox().Maximum, alpha)));
        case VariantTypes::Ray:
            return Variant(Ray(Float3::Lerp(a.AsRay().Position, b.AsRay().Position, alpha), Float3::Normalize(Float3::Lerp(a.AsRay().Direction, b.AsRay().Direction, alpha))));
        default:
            return a;
        }
    }

    void Variant::OnObjectDeleted(Object* obj)
    {
        AsObject = nullptr;
    }

    void Variant::OnAssetUnloaded(Asset* obj)
    {
        AsAsset = nullptr;
    }

    void Variant::AllocStructure()
    {
        /*const StringAnsiView typeName(Type.TypeName);
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            AsBlob.Length = type.Size;
            AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
            Platform::MemoryClear(AsBlob.Data, AsBlob.Length);
            type.Struct.Ctor(AsBlob.Data);
        }
        else if (typeName == "System.Int16" || typeName == "System.UInt16")
        {
            // [Deprecated on 10.05.2021, expires on 10.05.2023]
            // Hack for 16bit int
            AsBlob.Length = 2;
            AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
            *((int16*)AsBlob.Data) = 0;
        }
#if USE_CSHARP
        else if (const auto mclass = Scripting::FindClass(typeName))
        {
            // Fallback to C#-only types
            MCore::Thread::Attach();
            MObject* instance = mclass->CreateInstance();
            if (instance)
            {
#if 0
                void* data = MCore::Object::Unbox(instance);
                int32 instanceSize = mclass->GetInstanceSize();
                AsBlob.Length = instanceSize - (int32)((uintptr)data - (uintptr)instance);
                AsBlob.Data = PlatformAllocator::Allocate(AsBlob.Length);
                Platform::MemoryCopy(AsBlob.Data, data, AsBlob.Length);
#else
                Type.Type = VariantType::VariantTypes::ManagedObject;
                MANAGED_GC_HANDLE = MCore::GCHandle::New(instance);
#endif
            }
            else
            {
                AsBlob.Data = nullptr;
                AsBlob.Length = 0;
            }
        }
#endif
        else
        {
            if (typeName.Length() != 0)
            {
                LOG_WARNING("Render", "Missing scripting type \'{0}\'", String(typeName));
            }
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
        }*/
    }

    void Variant::CopyStructure(void* src)
    {
        /*if (AsBlob.Data && src)
        {
            const StringAnsiView typeName(Type.TypeName);
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
            if (typeHandle)
            {
                auto& type = typeHandle.GetType();
                type.Struct.Copy(AsBlob.Data, src);
            }
#if USE_CSHARP
            else if (const auto mclass = Scripting::FindClass(typeName))
            {
                // Fallback to C#-only types
                MCore::Thread::Attach();
                if (MANAGED_GC_HANDLE && mclass->IsValueType())
                {
                    MObject* instance = MCore::GCHandle::GetTarget(MANAGED_GC_HANDLE);
                    void* data = MCore::Object::Unbox(instance);
                    Platform::MemoryCopy(data, src, mclass->GetInstanceSize());
                }
            }
#endif
            else
            {
                if (typeName.Length() != 0)
                {
                    LOG_WARNING("Render", "Missing scripting type \'{0}\'", String(typeName));
                }
            }
        }*/
    }

    void Variant::FreeStructure()
    {
        /*if (!AsBlob.Data)
            return;
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(Type.TypeName));
        if (typeHandle)
        {
            const ScriptingType& type = typeHandle.GetType();
            type.Struct.Dtor(AsBlob.Data);
        }
        PlatformAllocator::Free(AsBlob.Data);*/
    }

    uint32 GetHash(const Variant& key)
    {
        switch (key.Type.Type)
        {
        case VariantTypes::Bool:
            return GetHash(key.AsBool);
        case VariantTypes::Int16:
            return GetHash(key.AsInt16);
        case VariantTypes::Int:
            return GetHash(key.AsInt);
        case VariantTypes::Uint16:
            return GetHash(key.AsUint16);
        case VariantTypes::Uint:
            return GetHash(key.AsUint);
        case VariantTypes::Int64:
            return GetHash(key.AsInt64);
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            return GetHash(key.AsUint64);
        case VariantTypes::Float:
            return GetHash(key.AsFloat);
        case VariantTypes::Double:
            return GetHash(key.AsDouble);
        case VariantTypes::Pointer:
            return GetHash(key.AsPointer);
        case VariantTypes::String:
            return GetHash((const Char*)key.AsBlob.Data);
        case VariantTypes::Object:
            return GetHash((void*)key.AsObject);
        case VariantTypes::Structure:
        case VariantTypes::Blob:
            return GetHash(key.AsBlob.Data, key.AsBlob.Length);
        case VariantTypes::Asset:
            return GetHash((void*)key.AsAsset);
        case VariantTypes::Color:
            return GetHash(*(Color*)key.AsData);
        case VariantTypes::Typename:
            return GetHash((const char*)key.AsBlob.Data);

        default:
            return 0;
        }
    }

    void Variant::StreamRead(ReadStream* stream, VariantTypeHandle& data)
    {
        data = VariantTypeHandle((VariantTypes)stream->ReadByte());
        int32 typeNameLength;
        stream->ReadInt32(&typeNameLength);
        if (typeNameLength == Max_int32)
        {
            stream->ReadInt32(&typeNameLength);
            if (typeNameLength == 0)
                return;
            data.TypeName = static_cast<char*>(PlatformAllocator::Allocate(typeNameLength + 1));
            char* ptr = data.TypeName;
            stream->ReadBytes(ptr, typeNameLength);
            for (int32 i = 0; i < typeNameLength; i++)
            {
                *ptr = *ptr ^ 77;
                ptr++;
            }
            *ptr = 0;
        }
        else if (typeNameLength > 0)
        {
            // [Deprecated on 27.08.2020, expires on 27.08.2021]
            ENGINE_ASSERT(typeNameLength < STREAM_MAX_STRING_LENGTH);
            List<Char> chars;
            chars.Resize(typeNameLength + 1);
            Char* ptr = chars.Get();
            stream->ReadBytes(ptr, typeNameLength * sizeof(Char));
            for (int32 i = 0; i < typeNameLength; i++)
            {
                *ptr = *ptr ^ 77;
                ptr++;
            }
            *ptr = 0;
            data.TypeName = static_cast<char*>(PlatformAllocator::Allocate(typeNameLength + 1));
            StringUtils::ConvertUTF162ANSI(chars.Get(), data.TypeName, typeNameLength);
            data.TypeName[typeNameLength] = 0;
        }
    }

    void Variant::StreamRead(ReadStream* stream, Variant& data)
    {
        VariantTypeHandle type;
        StreamRead(stream, type);
        data.SetType(MoveTemp(type));
        switch (data.Type.Type)
        {
        case VariantTypes::Null:
        case VariantTypes::Void:
            break;
        case VariantTypes::Bool:
            data.AsBool = stream->ReadBool();
            break;
        case VariantTypes::Int16:
            stream->ReadInt16(&data.AsInt16);
            break;
        case VariantTypes::Uint16:
            stream->ReadUint16(&data.AsUint16);
            break;
        case VariantTypes::Int:
            stream->ReadInt32(&data.AsInt);
            break;
        case VariantTypes::Uint:
            stream->ReadUint32(&data.AsUint);
            break;
        case VariantTypes::Int64:
            stream->ReadInt64(&data.AsInt64);
            break;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            stream->ReadUint64(&data.AsUint64);
            break;
        case VariantTypes::Float:
            stream->ReadFloat(&data.AsFloat);
            break;
        case VariantTypes::Double:
            stream->ReadDouble(&data.AsDouble);
            break;
        case VariantTypes::Pointer:
        {
            uint64 asUint64;
            stream->ReadUint64(&asUint64);
            data.AsPointer = (void*)(uintptr)asUint64;
            break;
        }
        case VariantTypes::String:
        {
            int32 length;
            stream->ReadInt32(&length);
            ENGINE_ASSERT(length < STREAM_MAX_STRING_LENGTH);
            const int32 dataLength = length * sizeof(Char) + 2;
            if (data.AsBlob.Length != dataLength)
            {
                PlatformAllocator::Free(data.AsBlob.Data);
                data.AsBlob.Data = dataLength > 0 ? PlatformAllocator::Allocate(dataLength) : nullptr;
                data.AsBlob.Length = dataLength;
            }
            Char* ptr = (Char*)data.AsBlob.Data;
            stream->ReadBytes(ptr, length * sizeof(Char));
            for (int32 i = 0; i < length; i++)
            {
                *ptr = *ptr ^ -14;
                ptr++;
            }
            *ptr = 0;
            break;
        }
        /*case VariantType::VariantTypes::Object:
        {
            UID id;
            stream->Read(id);
            data.SetObject(FindObject(id, ScriptingObject::GetStaticClass()));
            break;
        }
        case VariantType::VariantTypes::ManagedObject:
        case VariantType::VariantTypes::Structure:
        {
            const byte format = stream->ReadByte();
            if (format == 0)
            {
                // No data
            }
            else if (format == 1)
            {
                // Json
                WString json;
                ReadStringAnsi(&json, -71);
#if USE_CSHARP
                MCore::Thread::Attach();
                MClass* klass = MUtils::GetClass(data.Type);
                if (!klass)
                {
                    LOG(Error, "Invalid variant type {0}", data.Type);
                    return;
                }
                MObject* obj = MCore::Object::New(klass);
                if (!obj)
                {
                    LOG(Error, "Failed to managed instance of the variant type {0}", data.Type);
                    return;
                }
                if (!klass->IsValueType())
                    MCore::Object::Init(obj);
                ManagedSerialization::Deserialize(json, obj);
                if (data.Type.Type == VariantType::VariantTypes::ManagedObject)
                    data.SetManagedObject(obj);
                else
                    data = MUtils::UnboxVariant(obj);
#endif
            }
            else
            {
                LOG(Error, "Invalid Variant {0) format {1}", data.Type.ToString(), format);
            }
            break;
        }*/
        case VariantTypes::Blob:
        {
            int32 length;
            stream->ReadInt32(&length);
            data.SetBlob(length);
            stream->ReadBytes(data.AsBlob.Data, length);
            break;
        }
        /*case VariantType::VariantTypes::Asset:
        {
            UID id;
            stream->Read(id);
            data.SetAsset(LoadAsset(id, Asset::TypeInitializer));
            break;
        }*/
        case VariantTypes::Float2:
            stream->ReadBytes(&data.AsData, sizeof(Float2));
            break;
        case VariantTypes::Float3:
            stream->ReadBytes(&data.AsData, sizeof(Float3));
            break;
        case VariantTypes::Float4:
            stream->ReadBytes(&data.AsData, sizeof(Float4));
            break;
        case VariantTypes::Double2:
            stream->ReadBytes(&data.AsData, sizeof(Double2));
            break;
        case VariantTypes::Double3:
            stream->ReadBytes(&data.AsData, sizeof(Double3));
            break;
        case VariantTypes::Double4:
            stream->ReadBytes(data.AsBlob.Data, sizeof(Double4));
            break;
        case VariantTypes::Color:
            stream->ReadBytes(&data.AsData, sizeof(Color));
            break;
        case VariantTypes::UID:
            stream->ReadBytes(&data.AsData, sizeof(UID));
            break;
        /*case VariantType::VariantTypes::BoundingBox:
            stream->ReadBoundingBox(&data.AsBoundingBox());
            break;
        case VariantType::VariantTypes::BoundingSphere:
            stream->ReadBoundingSphere(&data.AsBoundingSphere());
            break;*/
        case VariantTypes::Quaternion:
            stream->ReadBytes(&data.AsData, sizeof(Quaternion));
            break;
        /*case VariantType::VariantTypes::Transform:
            stream->ReadTransform(&data.AsTransform());
            break;*/
        case VariantTypes::Rectangle:
            stream->ReadBytes(&data.AsData, sizeof(Rectangle));
            break;
        /*case VariantType::VariantTypes::Ray:
            stream->ReadRay(&data.AsRay());
            break;*/
        case VariantTypes::Matrix:
            stream->ReadBytes(data.AsBlob.Data, sizeof(Matrix));
            break;
        case VariantTypes::Array:
        {
            int32 count;
            stream->ReadInt32(&count);
            auto& array = *(List<Variant>*)data.AsData;
            array.Resize(count);
            for (int32 i = 0; i < count; i++)
                StreamRead(stream, array[i]);
            break;
        }
        case VariantTypes::Dictionary:
        {
            int32 count;
            stream->ReadInt32(&count);
            auto& dictionary = *data.AsDictionary;
            dictionary.Clear();
            dictionary.EnsureCapacity(count);
            for (int32 i = 0; i < count; i++)
            {
                Variant key;
                StreamRead(stream, key);
                StreamRead(stream, dictionary[MoveTemp(key)]);
            }
            break;
        }
        case VariantTypes::Typename:
        {
            int32 length;
            stream->ReadInt32(&length);
            ENGINE_ASSERT(length < STREAM_MAX_STRING_LENGTH);
            const int32 dataLength = length + 1;
            if (data.AsBlob.Length != dataLength)
            {
                PlatformAllocator::Free(data.AsBlob.Data);
                data.AsBlob.Data = dataLength > 0 ? PlatformAllocator::Allocate(dataLength) : nullptr;
                data.AsBlob.Length = dataLength;
            }
            char* ptr = (char*)data.AsBlob.Data;
            stream->ReadBytes(ptr, length);
            for (int32 i = 0; i < length; i++)
            {
                *ptr = *ptr ^ -14;
                ptr++;
            }
            *ptr = 0;
            break;
        }
        default:
            // stream->_hasError = true;
            LOG_ERROR("Stream", "Invalid Variant type. Corrupted data.");
            break;
        }
    }

    void Variant::StreamWrite(WriteStream* stream, const VariantTypeHandle& data)
    {
        stream->WriteByte((byte)data.Type);
        stream->WriteInt32(Max_int32);
        stream->WriteStringAnsi(StringAnsiView(data.TypeName), 77);
    }

    void Variant::StreamWrite(WriteStream* stream, const Variant& data)
    {
        StreamWrite(stream, data.Type);
        UID id;
        switch (data.Type.Type)
        {
        case VariantTypes::Null:
        case VariantTypes::Void:
            break;
        case VariantTypes::Bool:
            stream->WriteBool(data.AsBool);
            break;
        case VariantTypes::Int16:
            stream->WriteInt16(data.AsInt16);
            break;
        case VariantTypes::Uint16:
            stream->WriteUint16(data.AsUint16);
            break;
        case VariantTypes::Int:
            stream->WriteInt32(data.AsInt);
            break;
        case VariantTypes::Uint:
            stream->WriteUint32(data.AsUint);
            break;
        case VariantTypes::Int64:
            stream->WriteInt64(data.AsInt64);
            break;
        case VariantTypes::Uint64:
        case VariantTypes::Enum:
            stream->WriteUint64(data.AsUint64);
            break;
        case VariantTypes::Float:
            stream->WriteFloat(data.AsFloat);
            break;
        case VariantTypes::Double:
            stream->WriteDouble(data.AsDouble);
            break;
        case VariantTypes::Pointer:
            stream->WriteUint64((uint64)(uintptr)data.AsPointer);
            break;
        case VariantTypes::String:
            stream->WriteString((StringView)data, -14);
            break;
        /*case VariantType::VariantTypes::Object:
            id = data.AsObject ? data.AsObject->GetID() : UID::Empty;
            stream->Write(id);
            break;*/
        case VariantTypes::Blob:
            stream->WriteInt32(data.AsBlob.Length);
            stream->WriteBytes(data.AsBlob.Data, data.AsBlob.Length);
            break;
        /*case VariantType::VariantTypes::BoundingBox:
            stream->WriteBoundingBox(data.AsBoundingBox());
            break;
        case VariantType::VariantTypes::Transform:
            stream->WriteTransform(data.AsTransform());
            break;
        case VariantType::VariantTypes::Ray:
            stream->WriteRay(data.AsRay());
            break;*/
        case VariantTypes::Matrix:
            stream->WriteBytes(data.AsBlob.Data, sizeof(Matrix));
            break;
        case VariantTypes::Asset:
            id = data.AsAsset ? data.AsAsset->GetID() : UID::Empty;
            stream->Write(id);
            break;
        case VariantTypes::Float2:
            stream->WriteBytes(data.AsData, sizeof(Float2));
            break;
        case VariantTypes::Float3:
            stream->WriteBytes(data.AsData, sizeof(Float3));
            break;
        case VariantTypes::Float4:
            stream->WriteBytes(data.AsData, sizeof(Float4));
            break;
        case VariantTypes::Double2:
            stream->WriteBytes(data.AsData, sizeof(Double2));
            break;
        case VariantTypes::Double3:
            stream->WriteBytes(data.AsData, sizeof(Double3));
            break;
        case VariantTypes::Double4:
            stream->WriteBytes(data.AsBlob.Data, sizeof(Double4));
            break;
        case VariantTypes::Color:
            stream->WriteBytes(data.AsData, sizeof(Color));
            break;
        case VariantTypes::UID:
            stream->WriteBytes(data.AsData, sizeof(UID));
            break;
        case VariantTypes::Quaternion:
            stream->WriteBytes(data.AsData, sizeof(Quaternion));
            break;
        case VariantTypes::Rectangle:
            stream->WriteBytes(data.AsData, sizeof(Rectangle));
            break;
        /*case VariantType::VariantTypes::BoundingSphere:
            stream->WriteBoundingSphere(data.AsBoundingSphere());
            break;*/
        case VariantTypes::Array:
            id.A = ((List<Variant>*)data.AsData)->Count();
            stream->WriteInt32(id.A);
            for (uint32 i = 0; i < id.A; i++)
                StreamWrite(stream, ((List<Variant>*)data.AsData)->At(i));
            break;
        case VariantTypes::Dictionary:
            stream->WriteInt32(data.AsDictionary->Count());
            for (auto i = data.AsDictionary->begin(); i.IsNotEnd(); ++i)
            {
                StreamWrite(stream, i->Key);
                StreamWrite(stream, i->Value);
            }
            break;
        case VariantTypes::Typename:
            stream->WriteStringAnsi((StringAnsiView)data, -14);
            break;
        /*case VariantType::VariantTypes::ManagedObject:
        case VariantType::VariantTypes::Structure:
        {
#if USE_CSHARP
            MObject* obj;
            if (data.Type.Type == VariantType::VariantTypes::Structure)
                obj = MUtils::BoxVariant(data);
            else
                obj = (MObject*)data;
            if (obj)
            {
                WriteByte(1);
                rapidjson_flax::StringBuffer json;
                CompactJsonWriter writerObj(json);
                MCore::Thread::Attach();
                ManagedSerialization::Serialize(writerObj, obj);
                WriteStringAnsi(StringAnsiView(json.GetString(), (int32)json.GetSize()), -71);
            }
            else
#endif
            {
                stream->WriteByte(0);
            }
            break;
        }*/
        default:
            ENGINE_UNREACHABLE_CODE();
        }
    }
} // SE