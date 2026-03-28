#pragma once
#include "Core/Types/Variable.h"
#include "Runtime/API.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
    class WriteStream;
    class ReadStream;
    struct Ray;
    struct BoundingBox;
    struct BoundingSphere;
    struct UID;
    class Object;
    class Asset;
    class StringAnsiView;
    class StringView;


    enum class VariantTypes
    {
        Null = 0,
        Void,

        Bool,
        Int,
        Uint,
        Int64,
        Uint64,
        Float,
        Double,
        Pointer,

        String,
        Object,
        Structure,
        Asset,
        Blob,
        Enum,

        Float2,
        Float3,
        Float4,
        Color,
        BoundingBox,
        BoundingSphere,
        Quaternion,
        Transform,
        Rectangle,
        Ray,
        Matrix,

        Array,
        Dictionary,
        Typename,

        Int2,
        Int3,
        Int4,

        Int16,
        Uint16,

        Double2,
        Double3,
        Double4,

        UID,

        MAX,
#if USE_LARGE_WORLDS
        Vector2 = Double2,
        Vector3 = Double3,
        Vector4 = Double4,
#else
        Vector2 = Float2,
        Vector3 = Float3,
        Vector4 = Float4,
#endif
    };

    /// <summary>
    /// Represents an object type that can be interpreted as more than one type.
    /// </summary>
    struct SE_API_RUNTIME VariantTypeHandle
    {
    public:
        /// <summary>
        /// The type of the variant.
        /// </summary>
        VariantTypes Type;

        /// <summary>
        /// The optional additional full name of the scripting type. Used for Asset, Object, Enum, Structure types to describe type precisely.
        /// </summary>
        char* TypeName;

    public:
        FORCE_INLINE VariantTypeHandle()
        {
            Type = VariantTypes::Null;
            TypeName = nullptr;
        }

        FORCE_INLINE explicit VariantTypeHandle(VariantTypes type)
        {
            Type = type;
            TypeName = nullptr;
        }

        explicit VariantTypeHandle(VariantTypes type, const StringView& typeName);
        explicit VariantTypeHandle(VariantTypes type, const StringAnsiView& typeName);
        explicit VariantTypeHandle(const StringAnsiView& typeName);
        VariantTypeHandle(const VariantTypeHandle& other);
        VariantTypeHandle(VariantTypeHandle&& other) noexcept;

        ~VariantTypeHandle();

    public:
        VariantTypeHandle& operator=(const VariantTypes& type);
        VariantTypeHandle& operator=(VariantTypeHandle&& other);
        VariantTypeHandle& operator=(const VariantTypeHandle& other);
        bool operator==(const VariantTypes& type) const;
        bool operator==(const VariantTypeHandle& other) const;

        FORCE_INLINE bool operator!=(const VariantTypeHandle& other) const
        {
            return !operator==(other);
        }

    public:
        void SetTypeName(const StringView& typeName);
        void SetTypeName(const StringAnsiView& typeName);
        const char* GetTypeName() const;
        VariantTypeHandle GetElementType() const;
        String ToString() const;
    };

    SE_API_RUNTIME uint32 GetHash(const VariantTypeHandle& key);


    /// <summary>
    /// Represents an object that can be interpreted as more than one type.
    /// </summary>
    struct SE_API_RUNTIME Variant
    {
        friend class Stream;

        /// <summary>
        /// Thee value type.
        /// </summary>
        VariantTypeHandle Type;

        union
        {
            bool AsBool;
            int16 AsInt16;
            uint16 AsUint16;
            int32 AsInt;
            uint32 AsUint;
            int64 AsInt64;
            uint64 AsUint64;
            float AsFloat;
            double AsDouble;
            void* AsPointer;

            Object* AsObject;
            Asset* AsAsset;

            struct
            {
                void* Data;
                int32 Length;
            } AsBlob;

            Dictionary<Variant, Variant, HeapAllocation>* AsDictionary;

            byte AsData[24];
        };

    public:
        // 0.0f (floating-point value type)
        static const Variant Zero;

        // 1.0f (floating-point value type)
        static const Variant One;

        // nullptr (pointer value type)
        static const Variant Null;

        // false (boolean value type)
        static const Variant False;

        // true (boolean value type)
        static const Variant True;

    public:
        FORCE_INLINE Variant()
        {
        }

        Variant(const Variant& other);
        Variant(Variant&& other) noexcept;

        explicit Variant(decltype(nullptr));
        explicit Variant(const VariantTypeHandle& type);
        explicit Variant(VariantTypeHandle&& type);
        Variant(bool v);
        Variant(int16 v);
        Variant(uint16 v);
        Variant(int32 v);
        Variant(uint32 v);
        Variant(int64 v);
        Variant(uint64 v);
        Variant(float v);
        Variant(double v);
        Variant(void* v);
        Variant(Object* v);
        Variant(Asset* v);
        Variant(const StringView& v);
        Variant(const StringAnsiView& v);
        Variant(const Char* v);
        Variant(const char* v);
        Variant(const UID& v);
        Variant(const Float2& v);
        Variant(const Float3& v);
        Variant(const Float4& v);
        Variant(const Double2& v);
        Variant(const Double3& v);
        Variant(const Double4& v);
        Variant(const Int2& v);
        Variant(const Int3& v);
        Variant(const Int4& v);
        Variant(const Color& v);
        Variant(const Quaternion& v);
        Variant(const BoundingSphere& v);
        Variant(const Rectangle& v);
        explicit Variant(const BoundingBox& v);
        explicit Variant(const Transform& v);
        explicit Variant(const Ray& v);
        explicit Variant(const Matrix& v);
        Variant(List<Variant, HeapAllocation>&& v);
        Variant(const List<Variant, HeapAllocation>& v);
        explicit Variant(Dictionary<Variant, Variant, HeapAllocation>&& v);
        explicit Variant(const Dictionary<Variant, Variant, HeapAllocation>& v);
        explicit Variant(const Span<byte>& v);

        template<typename T>
        Variant(const class AssetRef<T>& v) : Variant(v.Get())
        {
        }

        ~Variant();

    public:
        Variant& operator=(Variant&& other);
        Variant& operator=(const Variant& other);
        bool operator==(const Variant& other) const;
        bool operator<(const Variant& other) const;

        FORCE_INLINE bool operator>(const Variant& other) const
        {
            return !operator==(other) && !operator<(other);
        }

        FORCE_INLINE bool operator>=(const Variant& other) const
        {
            return !operator<(other);
        }

        FORCE_INLINE bool operator<=(const Variant& other) const
        {
            return !operator>(other);
        }

        FORCE_INLINE bool operator!=(const Variant& other) const
        {
            return !operator==(other);
        }

    public:
        explicit operator bool() const;
        explicit operator Char() const;
        explicit operator int8() const;
        explicit operator int16() const;
        explicit operator int32() const;
        explicit operator int64() const;
        explicit operator uint8() const;
        explicit operator uint16() const;
        explicit operator uint32() const;
        explicit operator uint64() const;
        explicit operator float() const;
        explicit operator double() const;
        explicit operator void*() const;
        explicit operator StringView() const; // Returned StringView, if not empty, is guaranteed to point to a null terminated buffer.
        explicit operator StringAnsiView() const; // Returned StringView, if not empty, is guaranteed to point to a null terminated buffer.
        explicit operator Object*() const;
        explicit operator Asset*() const;
        explicit operator Float2() const;
        explicit operator Float3() const;
        explicit operator Float4() const;
        explicit operator Double2() const;
        explicit operator Double3() const;
        explicit operator Double4() const;
        explicit operator Int2() const;
        explicit operator Int3() const;
        explicit operator Int4() const;
        explicit operator Color() const;
        explicit operator Quaternion() const;
        explicit operator UID() const;
        explicit operator BoundingSphere() const;
        explicit operator BoundingBox() const;
        explicit operator Transform() const;
        explicit operator Matrix() const;
        explicit operator Ray() const;
        explicit operator Rectangle() const;

        const Float2& AsVector2() const;
        const Float3& AsVector3() const;
        const Float4& AsVector4() const;
        const Float2& AsFloat2() const;
        Float3& AsFloat3();
        const Float3& AsFloat3() const;
        const Float4& AsFloat4() const;
        const Double2& AsDouble2() const;
        const Double3& AsDouble3() const;
        const Double4& AsDouble4() const;
        const Int2& AsInt2() const;
        const Int3& AsInt3() const;
        const Int4& AsInt4() const;
        const Color& AsColor() const;
        const Quaternion& AsQuaternion() const;
        const Rectangle& AsRectangle() const;
        const UID& AsGuid() const;
        BoundingSphere& AsBoundingSphere();
        const BoundingSphere& AsBoundingSphere() const;
        BoundingBox& AsBoundingBox();
        const BoundingBox& AsBoundingBox() const;
        Ray& AsRay();
        const Ray& AsRay() const;
        Transform& AsTransform();
        const Transform& AsTransform() const;
        const Matrix& AsMatrix() const;
        List<Variant, HeapAllocation>& AsArray();
        const List<Variant, HeapAllocation>& AsArray() const;

        template<typename T>
        const T* AsStructure() const
        {
            if (Type.Type == VariantTypes::Structure && Type == T::TypeInitializer)
                return (const T*)AsBlob.Data;
            return nullptr;
        }

    public:
        void SetType(const VariantTypeHandle& type);
        void SetType(VariantTypeHandle&& type);
        void SetString(const StringView& str);
        void SetString(const StringAnsiView& str);
        void SetTypename(const StringView& typeName);
        void SetTypename(const StringAnsiView& typeName);
        void SetBlob(int32 length);
        void SetBlob(const void* data, int32 length);
        void SetObject(Object* object);
        void SetAsset(Asset* asset);
        String ToString() const;

        // Inlines potential value type into in-built format (eg. Vector3 stored as Structure, or String stored as ManagedObject).
        void Inline();

        // Inverts the inlined value from in-built format into generic storage (eg. Float3 from inlined format into Structure).
        void InvertInline();

        // Allocates the Variant of the specific type (eg. structure or object or value).
        static Variant NewValue(const StringAnsiView& typeName);

        // Frees the object or data owned by this Variant container (eg. structure or object).
        void DeleteValue();

        FORCE_INLINE Variant Cast(const VariantTypeHandle& to) const
        {
            return Cast(*this, to);
        }

    public:
        template<typename T>
        static typename TEnableIf<TIsEnum<T>::Value, Variant>::Type Enum(VariantTypeHandle&& type, const T value)
        {
            ASSERT_LOW_LAYER(type.Type == VariantType::Enum);
            Variant v;
            v.SetType(MoveTemp(type));
            v.AsUint64 = (uint64)value;
            return MoveTemp(v);
        }

        template<typename T>
        static typename TEnableIf<!TIsEnum<T>::Value && !TIsPointer<T>::Value, Variant>::Type Structure(VariantTypeHandle&& type, const T& value)
        {
            ASSERT_LOW_LAYER(type.Type == VariantType::Structure);
            Variant v;
            v.SetType(MoveTemp(type));
            v.CopyStructure((void*)&value);
            return MoveTemp(v);
        }

        static bool CanCast(const Variant& v, const VariantTypeHandle& to);
        static Variant Cast(const Variant& v, const VariantTypeHandle& to);
        static bool NearEqual(const Variant& a, const Variant& b, float epsilon = 1e-6f);
        static Variant Lerp(const Variant& a, const Variant& b, float alpha);

    public:
        static void StreamRead(ReadStream* stream, VariantTypeHandle& data);
        static void StreamRead(ReadStream* stream, Variant& data);
        static void StreamWrite(WriteStream* stream, const VariantTypeHandle& data);
        static void StreamWrite(WriteStream* stream, const Variant& data);

    private:
        void OnObjectDeleted(Object* obj);
        void OnAssetUnloaded(Asset* obj);
        void AllocStructure();
        void CopyStructure(void* src);
        void FreeStructure();
    };


} // SE

