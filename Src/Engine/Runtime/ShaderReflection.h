/*
#pragma once
#include <slang-com-ptr.h>
#include <slang.h>

#include "Core/Math/Vector3.h"
#include "Core/Types/Variable.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Strings/String.h"

namespace SE
{
    class ReflectionVar;
    class ReflectionType;
    class ReflectionResourceType;
    class ReflectionBasicType;
    class ReflectionStructType;
    class ReflectionArrayType;
    class ReflectionInterfaceType;
    class ParameterBlockReflection;
    struct ShaderContent;

    enum class ShaderResourceType
    {
        TextureSrv,
        TextureUav,
        RawBufferSrv,
        RawBufferUav,
        TypedBufferSrv,
        TypedBufferUav,
        Cbv,
        StructuredBufferUav,
        StructuredBufferSrv,
        AccelerationStructureSrv,
        Dsv,
        Rtv,
        Sampler,

        Count
    };

    /**
     * Represents the offset of a uniform shader variable relative to its enclosing type/buffer/block.
     *
     * A `UniformShaderVarOffset` is a simple wrapper around a byte offset for a uniform shader variable.
     * It is used to make API signatures less ambiguous (e.g., about whether an integer represents an
     * index, an offset, a count, etc.
     *
     * A `UniformShaderVarOffset` can also encode an invalid offset (represented as an all-ones bit pattern),
     * to indicate that a particular uniform variable is not present.
     *
     * A `UniformShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * UniformShaderVarOffset aOffset = pSomeType["a"]; // get offset of field `a` inside `pSomeType`
     * UniformShaderVarOffset bOffset = pBlock["b"]; // get offset of parameter `b` inside parameter block
     #1#
    struct UniformShaderVarOffset
    {
        /**
         * Type used to store the underlying byte offset.
         #1#
        typedef uint32 ByteOffset;

        /**
         * Construct from an explicit byte offset.
         #1#
        explicit UniformShaderVarOffset(size_t offset) : mByteOffset(ByteOffset(offset)) {}

        /**
         * Custom enumeration type used to represent a zero offset.
         *
         * Can be used to initialize a `UniformShaderVarOffset` when an explicit zero offset is desired:
         *
         * UniformShaderVarOffset myOffset = UniformShaderVarOffset::kZero;
         *
         #1#
        enum Zero
        {
            kZero = 0
        };

        /**
         * Construct an explicit zero offset.
         #1#
        UniformShaderVarOffset(Zero) : mByteOffset(0) {}

        /**
         * Custom enumeration type used to represent an invalid offset.
         *
         * Can be used to explicitly initialize a `UniformShaderVarOffset` to an invalid offset
         *
         * UniformShaderVarOffset myOffset = UniformShaderVarOffset::kInvalid;
         *
         * Note that the default constructor also creates an invalid offset, so this could instead
         * be written more simply as:
         *
         * UniformShaderVarOffset myOffset;
         #1#
        enum Invalid
        {
            kInvalid = -1
        };

        /**
         * Default constructor: creates an invalid offset.
         #1#
        UniformShaderVarOffset(Invalid _ = kInvalid) : mByteOffset(ByteOffset(-1)) {}

        /**
         * Get the raw byte offset.
         #1#
        ByteOffset getByteOffset() const { return mByteOffset; }

        /**
         * Check whether this offset is valid.
         *
         * An invalid offset has an all-ones bit pattern (`ByteOffset(-1)`).
         #1#
        bool isValid() const { return mByteOffset != ByteOffset(-1); }

        /**
         * Compare this offset to another offset.
         #1#
        bool operator==(const UniformShaderVarOffset& other) const { return mByteOffset == other.mByteOffset; }

        /**
         * Compare this offset to another offset.
         #1#
        bool operator!=(const UniformShaderVarOffset& other) const { return mByteOffset != other.mByteOffset; }

        /**
         * Compare this offset to an invalid offset.
         *
         * This operator allows for checks like:
         *
         * if(myOffset == UniformShaderVarOffset::kInvalid) { ... }
         #1#
        bool operator==(Invalid _) const { return !isValid(); }

        /**
         * Compare this offset to an invalid offset.
         *
         * This operator allows for checks like:
         *
         * if(myOffset != UniformShaderVarOffset::kInvalid) { ... }
         #1#
        bool operator!=(Invalid _) const { return isValid(); }

        /**
         * Add an additional byte offset to this offset.
         *
         * If this offset is invalid, returns an invalid offset.
         #1#
        UniformShaderVarOffset operator+(size_t offset) const
        {
            if (!isValid())
                return kInvalid;

            return UniformShaderVarOffset(mByteOffset + offset);
        }

        /**
         * Add an additional byte offset to this offset.
         *
         * If either `this` or `other` is an invalid offset, returns an invalid offset.
         #1#
        UniformShaderVarOffset operator+(UniformShaderVarOffset other) const
        {
            if (!isValid())
                return kInvalid;
            if (!other.isValid())
                return kInvalid;

            return UniformShaderVarOffset(mByteOffset + other.mByteOffset);
        }

    private:
        // The underlying raw byte offset.
        ByteOffset mByteOffset = ByteOffset(-1);
    };

    /**
     * Represents the offset of a resource-type shader variable relative to its enclosing type/buffer/block.
     *
     * A `ResourceShaderVarOffset` records the index of a descriptor range and an array index within that range.
     *
     * A `ResourceShaderVarOffset` can also encode an invalid offset (represented as an all-ones bit pattern
     * for both the range and array indices), to indicate that a particular resource variable is not present.
     *
     * A `ResourceShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * ResourceShaderVarOffset texOffset = pSomeType["tex"]; // get offset of texture `tex` inside `pSomeType`
     * ResourceShaderVarOffset sampOffset = pBlock["samp"]; // get offset of sampler `samp` inside block
     *
     * Please note that the concepts of resource "ranges" are largely an implementation detail of
     * the `ParameterBlock` type, and most user code should not attempt to explicitly work with
     * or reason about resource ranges. In particular, there is *no* correspondence between resource
     * range indices and the `register`s or `binding`s assigned to shader parameters.
     #1#
    struct ResourceShaderVarOffset
    {
    public:
        /**
         * Custom enumeration type used to represent a zero offset.
         *
         * Can be used to initialize a `ResourceShaderVarOffset` when an explicit zero offset is desired:
         *
         * ResourceShaderVarOffset myOffset = ResourceShaderVarOffset::kZero;
         *
         #1#
        enum Zero
        {
            kZero = 0
        };

        /**
         * Construct an explicit zero offset.
         #1#
        ResourceShaderVarOffset(Zero) : mRangeIndex(0), mArrayIndex(0) {}

        /**
         * Custom enumeration type used to represent an invalid offset.
         *
         * Can be used to initialize a `ResourceShaderVarOffset` when an explicit invalid is desired:
         *
         * ResourceShaderVarOffset myOffset = ResourceShaderVarOffset::kInvalid;
         *
         * Note that the default constructor also constructs an invalid offset, so this
         * could be written more simply as:
         *
         * ResourceShaderVarOffset myOffset;
         *
         #1#
        enum Invalid
        {
            kInvalid = -1
        };

        /**
         * Default constructor: constructs an invalid offset.
         #1#
        ResourceShaderVarOffset(Invalid _ = kInvalid) : mRangeIndex(RangeIndex(-1)), mArrayIndex(ArrayIndex(-1)) {}

        /**
         * Check if this is a valid offset.
         #1#
        bool isValid() const { return mRangeIndex != RangeIndex(-1); }

        /**
         * Add a further offset to this offset.
         *
         * If either `this` or `other` is invalid, returns an invalid offset.
         #1#
        ResourceShaderVarOffset operator+(const ResourceShaderVarOffset& other) const
        {
            if (!isValid())
                return kInvalid;
            if (!other.isValid())
                return kInvalid;

            return ResourceShaderVarOffset(mRangeIndex + other.mRangeIndex, mArrayIndex + other.mArrayIndex);
        }

        /**
         * Compare with another offset.
         #1#
        bool operator==(const ResourceShaderVarOffset& other) const
        {
            return mRangeIndex == other.mRangeIndex && mArrayIndex == other.mArrayIndex;
        }

        /**
         * Compare with another offset.
         #1#
        bool operator!=(const ResourceShaderVarOffset& other) const { return !(*this == other); }

        /**
         * Type used to store the resource/descriptor range.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        typedef uint32 RangeIndex;

        /**
         * Type used to store the array index within a range.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        typedef uint32 ArrayIndex;

        /**
         * Get the underlying resource/descriptor range index.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        RangeIndex getRangeIndex() const { return mRangeIndex; }

        /**
         * Get the underlying array index into the resource/descriptor range.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        ArrayIndex getArrayIndex() const { return mArrayIndex; }

        /**
         * Construct an offset representing an explicit resource range and array index.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        ResourceShaderVarOffset(RangeIndex rangeIndex, ArrayIndex arrayIndex) : mRangeIndex(rangeIndex), mArrayIndex(arrayIndex) {}

        /**
         * Construct an offset representing an explicit resource range.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        explicit ResourceShaderVarOffset(RangeIndex rangeIndex) : mRangeIndex(rangeIndex), mArrayIndex(0) {}

    private:
        RangeIndex mRangeIndex;
        ArrayIndex mArrayIndex;
    };

    /**
     * Represents the offset of a shader variable relative to its enclosing type/buffer/block.
     *
     * A `ShaderVarOffset` can be used to store the offset of a shader variable that might use
     * ordinary/uniform data, resources like textures/buffers/samplers, or some combination.
     * It effectively stores both a `UniformShaderVarOffset` and a `ResourceShaderVarOffset`
     *
     * A `ShaderVarOffset` can also encode an invalid offset, to indicate that a particular
     * shader variable is not present.
     *
     * A `ShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * ShaderVarOffset lightOffset = pSomeType["light"]; // get offset of variable `light` inside `pSomeType`
     * ShaderVarOffset materialOffset = pBlock["material"]; // get offset of variable `material` inside block
     *
     #1#
    struct ShaderVarOffset
    {
    public:
        /**
         * Construct a shader variable offset from its underlying uniform and resource offsets.
         #1#
        ShaderVarOffset(UniformShaderVarOffset uniform, ResourceShaderVarOffset resource) : mUniform(uniform), mResource(resource) {}

        /**
         * Custom enumeration type used to represent an invalid offset.
         *
         * Can be used to initialize a `ShaderVarOffset` when an explicit invalid is desired:
         *
         * ShaderVarOffset myOffset = ShaderVarOffset::kInvalid;
         *
         * Note that the default constructor also constructs an invalid offset, so this
         * could be written more simply as:
         *
         * ShaderVarOffset myOffset;
         *
         #1#
        enum Invalid
        {
            kInvalid = -1
        };

        /**
         * Default constructor: constructs an invalid offset.
         #1#
        ShaderVarOffset(Invalid _ = kInvalid) : mUniform(UniformShaderVarOffset::kInvalid), mResource(ResourceShaderVarOffset::kInvalid) {}

        /**
         * Custom enumeration type used to represent a zero offset.
         *
         * Can be used to initialize a `ShaderVarOffset` when an explicit zero offset is desired:
         *
         * ShaderVarOffset myOffset = ShaderVarOffset::kZero;
         #1#
        enum Zero
        {
            kZero = 0
        };

        /**
         * Construct an explicit zero offset.
         #1#
        ShaderVarOffset(Zero) : mUniform(UniformShaderVarOffset::kZero), mResource(ResourceShaderVarOffset::kZero) {}

        /**
         * Check if this is a valid offset.
         #1#
        bool isValid() const { return mUniform.isValid(); }

        /**
         * Get the underlying uniform offset.
         #1#
        UniformShaderVarOffset getUniform() const { return mUniform; }

        /**
         * Get the underlying uniform offset.
         *
         * This implicit conversion allows a `ShaderVarOffset` to be
         * passed to functions that expect a `UniformShaderVarOffset`.
         #1#
        operator UniformShaderVarOffset() const { return mUniform; }

        /**
         * Get the underlying resource offset.
         #1#
        ResourceShaderVarOffset getResource() const { return mResource; }

        /**
         * Get the underlying resource offset.
         *
         * This implicit conversion allows a `ShaderVarOffset` to be
         * passed to functions that expect a `ResourceShaderVarOffset`.
         #1#
        operator ResourceShaderVarOffset() const { return mResource; }

        /**
         * Add an additional offset.
         *
         * If either `this` or `other` is invalid, returns an invalid offset.
         #1#
        ShaderVarOffset operator+(const ShaderVarOffset& other) const
        {
            if (!isValid())
                return kInvalid;
            if (!other.isValid())
                return kInvalid;

            return ShaderVarOffset(mUniform + other.mUniform, mResource + other.mResource);
        }

        /**
         * Compare to another offset.
         #1#
        bool operator==(const ShaderVarOffset& other) const { return mUniform == other.mUniform && mResource == other.mResource; }

        /**
         * Compare to another offset.
         #1#
        bool operator!=(const ShaderVarOffset& other) const { return !(*this == other); }

        /**
         * Type used to store the underlying uniform byte offset.
         #1#
        using ByteOffset = UniformShaderVarOffset::ByteOffset;

        /**
         * Get the uniform byte offset.
         #1#
        ByteOffset getByteOffset() const { return mUniform.getByteOffset(); }

        /**
         * Type used to store the resource/descriptor range.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        using RangeIndex = ResourceShaderVarOffset::RangeIndex;

        /**
         * Type used to store the array index within a range.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        using ArrayIndex = ResourceShaderVarOffset::ArrayIndex;

        /**
         * Get the underlying resource range index.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        RangeIndex getResourceRangeIndex() const { return mResource.getRangeIndex(); }

        /**
         * Get the underlying resource array index.
         *
         * Note: most user code should *not* need to work with explicit range/array indices.
         #1#
        ArrayIndex getResourceArrayIndex() const { return mResource.getArrayIndex(); }

    protected:
        UniformShaderVarOffset mUniform;
        ResourceShaderVarOffset mResource;
    };

    /**
     * Represents the type of a shader variable and its offset relative to its enclosing type/buffer/block.
     *
     * A `TypedShaderVarOffset` is just a `ShaderVarOffset` plus a `ReflectionType` for
     * the variable at the given offset.
     *
     * A `TypedShaderVarOffset` can also encode an invalid offset, to indicate that a particular
     * shader variable is not present.
     *
     * A `TypedShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * TypedShaderVarOffset lightOffset = pSomeType["light"]; // get type and offset of texture `light` inside `pSomeType`
     * TypedShaderVarOffset materialOffset = pBlock["material"]; // get type and offset of sampler `material` inside block
     *
     * In addition, a `TypedShaderVarOffset` can be used to look up offsets for
     * sub-fields/-elements of shader variables with structure or array types:
     *
     * UniformShaderVarOffset lightPosOffset = lightOffset["position"];
     * ResourceShaderVarOffset diffuseMapOffset = materialOffset["diffuseMap"];
     *
     * Such offsets are always relative to the root type or block where lookup started.
     * For example, in the above code `lightPosOffset` would be the offset of the
     * field `light.position` relative to the enclosing type `pSomeType` and *not*
     * the offset of the `position` field relative to the immediately enclosing `light` field.
     *
     * Because `TypedShaderVarOffset` inherits from `ShaderVarOffset` it can be used
     * in all the same places, and also implicitly converts to both
     * `UniformShaderVarOffset` and `ResourceShaderVarOffset`.
     *
     * This struct has a non-owning pointer to the type information.
     * The caller is responsible for ensuring that the type information remains valid,
     * which is typically owned by the `ParameterBlockReflection` object.
     #1#
    struct TypedShaderVarOffset : ShaderVarOffset
    {
    public:
        /**
         * Default constructor: constructs an invalid offset.
         #1#
        TypedShaderVarOffset(Invalid _ = kInvalid) {}

        /**
         * Get the type of the shader variable.
         #1#
        const ReflectionType* getType() const { return mpType; }

        /**
         * Check if `this` represents a valid offset.
         #1#
        bool isValid() const { return mpType != nullptr; }

        /**
         * Look up type and offset of a sub-field with the given `name`.
         #1#
        TypedShaderVarOffset operator[](StringView name) const;

        /**
         * Look up type and offset of a sub-element or sub-field with the given `index`.
         #1#
        TypedShaderVarOffset operator[](int index) const;

        /**
         * Construct a typed shader variable offset from an explicit type and offset.
         *
         * The caller takes responsibility for ensuring that `pType` is a valid type
         * for the data at `offset`.
         #1#
        TypedShaderVarOffset(const ReflectionType* pType, ShaderVarOffset offset);

    private:
        const ReflectionType* mpType{nullptr};
    };


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * Reflection and layout information for a type in shader code.
     #1#
    class ReflectionType
    {
    public:
        virtual ~ReflectionType() = default;

        /**
         * The kind of a type.
         *
         * Every type has a kind, which specifies which subclass of `ReflectionType` it uses.
         *
         * When adding new derived classes, this enumeration should be updated.
         #1#
        enum class Kind
        {
            Array,     ///< ReflectionArrayType
            Struct,    ///< ReflectionStructType
            Basic,     ///< ReflectionBasicType
            Resource,  ///< ReflectionResourceType
            Interface, ///< ReflectionInterfaceType
        };

        /**
         * Get the kind of this type.
         *
         * The kind tells us if we have an array, structure, etc.
         #1#
        Kind getKind() const { return mKind; }

        /**
         * Dynamic-cast the current object to ReflectionResourceType
         #1#
        const ReflectionResourceType* asResourceType() const;

        /**
         * Dynamic-cast the current object to ReflectionBasicType
         #1#
        const ReflectionBasicType* asBasicType() const;

        /**
         * Dynamic-cast the current object to ReflectionStructType
         #1#
        const ReflectionStructType* asStructType() const;

        /**
         * Dynamic-cast the current object to ReflectionArrayType
         #1#
        const ReflectionArrayType* asArrayType() const;

        /**
         * Dynamic cast to ReflectionInterfaceType
         #1#
        const ReflectionInterfaceType* asInterfaceType() const;

        /**
         * "Unwrap" any array types to get to the non-array type underneath.
         *
         * If `this` is not an array, then returns `this`.
         * If `this` is an array, then applies `unwrapArray` to its element type.
         #1#
        const ReflectionType* unwrapArray() const;

        /**
         * Get the total number of array elements represented by this type.
         *
         * If `this` is not an array, then returns 1.
         * If `this` is an array, returns the number of elements times `getTotalArraySize()` for the element type.
         #1#
        uint32 getTotalArrayElementCount() const;

        /**
         * Get the size in bytes of instances of this type.
         *
         * This function only counts uniform/ordinary data, and not resources like textures/buffers/samplers.
         #1#
        int getByteSize() const { return mByteSize; }

        /**
         * Find a field/member of this type with the given `name`.
         *
         * If this type doesn't have fields/members, or doesn't have a field/member matching `name`, then returns null.
         #1#
        const ReflectionVar* findMember(StringView name) const;

        /**
         * Get the (type and) offset of a field/member with the given `name`.
         *
         * If this type doesn't have fields/members, or doesn't have a field/member matching `name`,
         * then logs an error and returns an invalid offset.
         #1#
        TypedShaderVarOffset getMemberOffset(StringView& name) const;

        /**
         * Find a typed member/element offset corresponding to the given byte offset.
         #1#
        TypedShaderVarOffset findMemberByOffset(int byteOffset) const;

        /**
         * Get an offset that is zero bytes into this type.
         *
         * Useful for turning a `ReflectionType` into a `TypedShaderVarOffset` so
         * that the `[]` operator can be used to look up members/elements.
         #1#
        TypedShaderVarOffset getZeroOffset() const;

        /**
         * Compare types for equality.
         *
         * It is possible for two distinct `ReflectionType` instances to represent
         * the same type with the same layout. The `==` operator must be used to
         * tell if two types have the same structure.
         #1#
        virtual bool operator==(const ReflectionType& other) const = 0;

        /**
         * Compare types for inequality.
         #1#
        bool operator!=(const ReflectionType& other) const { return !(*this == other); }

        /**
         * A range of resources contained (directly or indirectly) in this type.
         *
         * Different types will contain different numbers of resources, and those
         * resources will always be grouped into contiguous "ranges" that must be
         * allocated together in descriptor sets to allow them to be indexed.
         *
         * Some examples:
         *
         * * A basic type like `float2` has zero resource ranges.
         *
         * * A resource type like `Texture2D` will have one resource range,
         * with a corresponding descriptor type and an array count of one.
         *
         * * An array type like `float2[3]` or `Texture2D[4]` will have
         * the same number of ranges as its element type, but the count
         * of each range will be multiplied by the array element count.
         *
         * * A structure type like `struct { Texture2D a; Texture2D b[3]; }`
         * will concatenate the resource ranges from its fields, in order.
         *
         * The `ResourceRange` type is mostly an implementation detail
         * of `ReflectionType` that supports `ParameterBlock` and users
         * should probably not rely on this information.
         #1#
        struct ResourceRange
        {
            // TODO(tfoley) consider renaming this to `DescriptorRange`.

            /**
             * The type of descriptors that are stored in the range
             #1#
            ShaderResourceType descriptorType;

            /**
             * The total number of descriptors in the range.
             #1#
            uint32 count;

            /**
             * If the enclosing type had its descriptors stored in
             * flattened arrays, where would this range start?
             *
             * This is entirely an implementation detail of `ParameterBlock`.
             #1#
            uint32 baseIndex;
        };

        /**
         * Get the number of descriptor ranges contained in this type.
         #1#
        uint32 getResourceRangeCount() const { return mResourceRanges.Count(); }

        /**
         * Get information on a contained descriptor range.
         #1#
        const ResourceRange& getResourceRange(uint32 index) const { return mResourceRanges[index]; }

        slang::TypeLayoutReflection* getSlangTypeLayout() const { return mpSlangTypeLayout; }

    protected:
        ReflectionType(Kind kind, int byteSize, slang::TypeLayoutReflection* pSlangTypeLayout)
            : mKind(kind), mByteSize(byteSize), mpSlangTypeLayout(pSlangTypeLayout)
        {}

        Kind mKind;
        int mByteSize = 0;
        List<ResourceRange> mResourceRanges;
        slang::TypeLayoutReflection* mpSlangTypeLayout = nullptr;
    };

    /**
     * Represents an array type in shader code.
     #1#
    class ReflectionArrayType : public ReflectionType
    {
    public:
        /**
         * Create a new object
         #1#
        static ReflectionArrayType* create(
            uint32 elementCount,
            uint32 elementByteStride,
            const ReflectionType* pElementType,
            int byteSize,
            slang::TypeLayoutReflection* pSlangTypeLayout
        );

        /**
         * Get the number of elements in the array.
         #1#
        uint32 getElementCount() const { return mElementCount; }

        /**
         * Get the "stride" in bytes of the array.
         *
         * The stride is the number of bytes between consecutive
         * array elements. It is *not* necessarily the same as
         * the size of the array elements. For example an array
         * of `float3`s in a constant buffer may have a stride
         * of 16 bytes, but each element is only 12 bytes.
         #1#
        uint32 getElementByteStride() const { return mElementByteStride; }

        /**
         * Get the type of the array elements.
         #1#
        const ReflectionType* getElementType() const { return mpElementType; }

        bool operator==(const ReflectionArrayType& other) const;
        bool operator==(const ReflectionType& other) const override;

    private:
        ReflectionArrayType(
            uint32 elementCount,
            uint32 elementByteStride,
            const ReflectionType* pElementType,
            int totalByteSize,
            slang::TypeLayoutReflection* pSlangTypeLayout
        );

        uint32 mElementCount = 0;
        uint32 mElementByteStride = 0;
        const ReflectionType* mpElementType;
    };

    /**
     * Represents a `struct` type in shader code.
     #1#
    class ReflectionStructType : public ReflectionType
    {
    public:
        /**
         * Get the name of the struct type
         #1#
        const String& getName() const { return mName; }

        /**
         * Get the total number members
         #1#
        uint32 getMemberCount() const { return mMembers.Count(); }

        /**
         * Get member by index
         #1#
        const ReflectionVar* getMember(int index) const { return mMembers[index]; }

        /**
         * Get member by name
         #1#
        const ReflectionVar* getMember(StringView name) const;

        /**
         * Constant used to indicate that member lookup failed.
         #1#
        static constexpr int32 kInvalidMemberIndex = -1;

        /**
         * Get the index of a member
         *
         * Returns `kInvalidMemberIndex` if no such member exists.
         #1#
        int32 getMemberIndex(StringView name) const;

        /**
         * Find a member based on a byte offset.
         #1#
        TypedShaderVarOffset findMemberByOffset(size_t offset) const;

        bool operator==(const ReflectionStructType& other) const;
        bool operator==(const ReflectionType& other) const override;

        // TODO(tfoley): The following members are only needed to construct a type.

        /**
         * Create a new structure type
         * @param[in] size The size of the struct in bytes
         * @param[in] name The name of the struct
         #1#
        static ReflectionStructType* create(int byteSize, const String& name, slang::TypeLayoutReflection* pSlangTypeLayout);

        struct BuildState
        {
            uint32 cbCount = 0;
            uint32 srvCount = 0;
            uint32 uavCount = 0;
            uint32 samplerCount = 0;
        };

        /**
         * Add a new member
         #1#
        int32 addMember(const ReflectionVar* pVar, BuildState& ioBuildState);

        int32 addMemberIgnoringNameConflicts(const ReflectionVar* pVar, BuildState& ioBuildState);

    private:
        ReflectionStructType(int size, const String& name, slang::TypeLayoutReflection* pSlangTypeLayout);
        List<const ReflectionVar*> mMembers;           // Struct members
        Dictionary<String, int32> mNameToIndex; // Translates from a name to an index in mMembers
        String mName;
    };

    /**
     * Reflection object for scalars, vectors and matrices
     #1#
    class ReflectionBasicType : public ReflectionType
    {
    public:
        /**
         * The type of the object
         #1#
        enum class Type
        {
            Bool,
            Bool2,
            Bool3,
            Bool4,

            Uint8,
            Uint8_2,
            Uint8_3,
            Uint8_4,

            Uint16,
            Uint16_2,
            Uint16_3,
            Uint16_4,

            Uint,
            Uint2,
            Uint3,
            Uint4,

            Uint64,
            Uint64_2,
            Uint64_3,
            Uint64_4,

            Int8,
            Int8_2,
            Int8_3,
            Int8_4,

            Int16,
            Int16_2,
            Int16_3,
            Int16_4,

            Int,
            Int2,
            Int3,
            Int4,

            Int64,
            Int64_2,
            Int64_3,
            Int64_4,

            Float16,
            Float16_2,
            Float16_3,
            Float16_4,

            Float16_2x2,
            Float16_2x3,
            Float16_2x4,
            Float16_3x2,
            Float16_3x3,
            Float16_3x4,
            Float16_4x2,
            Float16_4x3,
            Float16_4x4,

            Float,
            Float2,
            Float3,
            Float4,

            Float2x2,
            Float2x3,
            Float2x4,
            Float3x2,
            Float3x3,
            Float3x4,
            Float4x2,
            Float4x3,
            Float4x4,

            Float64,
            Float64_2,
            Float64_3,
            Float64_4,

            Unknown = -1
        };

        /**
         * Create a new object
         * @param[in] offset The offset of the variable relative to the parent variable
         * @param[in] type The type of the object
         * @param[in] isRowMajor For matrices, true means row-major, otherwise it's column-major
         * @param[in] size The size of the object
         #1#
        static ReflectionBasicType* create(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout);

        /**
         * Get the object's type
         #1#
        Type getType() const { return mType; }

        /**
         * Check if this is a row-major matrix or not. The result is only valid for matrices
         #1#
        bool isRowMajor() const { return mIsRowMajor; }

        bool operator==(const ReflectionBasicType& other) const;
        bool operator==(const ReflectionType& other) const override;

    private:
        ReflectionBasicType(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout);
        Type mType;
        bool mIsRowMajor;
    };

    /**
     * Reflection object for resources
     #1#
    class ReflectionResourceType : public ReflectionType
    {
    public:
        /**
         * Describes how the shader will access the resource
         #1#
        enum class ShaderAccess
        {
            Undefined,
            Read,
            ReadWrite
        };

        /**
         * The expected return type
         #1#
        enum class ReturnType
        {
            Unknown,
            Float,
            Double,
            Int,
            Uint
        };

        /**
         * The resource dimension
         #1#
        enum class Dimensions
        {
            Unknown,
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
            Texture1DArray,
            Texture2DArray,
            Texture2DMS,
            Texture2DMSArray,
            TextureCubeArray,
            AccelerationStructure,
            Buffer,

            Count
        };

        /**
         * For structured-buffers, describes the type of the buffer
         #1#
        enum class StructuredType
        {
            Invalid, ///< Not a structured buffer
            Default, ///< Regular structured buffer
            Counter, ///< RWStructuredBuffer with counter
            Append,  ///< AppendStructuredBuffer
            Consume  ///< ConsumeStructuredBuffer
        };

        /**
         * The type of the resource
         #1#
        enum class Type
        {
            Texture,
            StructuredBuffer,
            RawBuffer,
            TypedBuffer,
            Sampler,
            ConstantBuffer,
            AccelerationStructure,
        };

        /**
         * Create a new object
         #1#
        static ReflectionResourceType* create(
            Type type,
            Dimensions dims,
            StructuredType structuredType,
            ReturnType retType,
            ShaderAccess shaderAccess,
            slang::TypeLayoutReflection* pSlangTypeLayout
        );

        /**
         * For structured- and constant-buffers, set a reflection-type describing the buffer's layout
         #1#
        void setStructType(const ReflectionType* pType);

        /**
         * Get the struct-type
         #1#
        const ReflectionType* getStructType() const { return mpStructType; }

        const ParameterBlockReflection* getParameterBlockReflector() const { return mpParameterBlockReflector; }
        void setParameterBlockReflector(const ParameterBlockReflection* pReflector) { mpParameterBlockReflector = pReflector; }

        /**
         * Get the dimensions
         #1#
        Dimensions getDimensions() const { return mDimensions; }

        /**
         * Get the structured-buffer type
         #1#
        StructuredType getStructuredBufferType() const { return mStructuredType; }

        /**
         * Get the resource return type
         #1#
        ReturnType getReturnType() const { return mReturnType; }

        /**
         * Get the required shader access
         #1#
        ShaderAccess getShaderAccess() const { return mShaderAccess; }

        /**
         * Get the resource type
         #1#
        Type getType() const { return mType; }

        /**
         * For structured- and constant-buffers, return the underlying type size, otherwise returns 0
         #1#
        int getSize() const { return mpStructType ? mpStructType->getByteSize() : 0; }

        bool operator==(const ReflectionResourceType& other) const;
        bool operator==(const ReflectionType& other) const override;

    private:
        ReflectionResourceType(
            Type type,
            Dimensions dims,
            StructuredType structuredType,
            ReturnType retType,
            ShaderAccess shaderAccess,
            slang::TypeLayoutReflection* pSlangTypeLayout
        );

        Dimensions mDimensions;
        StructuredType mStructuredType;
        ReturnType mReturnType;
        ShaderAccess mShaderAccess;
        Type mType;
        const ReflectionType* mpStructType;                        // For constant- and structured-buffers
        const ParameterBlockReflection* mpParameterBlockReflector; // For constant buffers and parameter blocks
    };

    /**
     * Reflection object for resources
     #1#
    class ReflectionInterfaceType : public ReflectionType
    {
    public:
        static ReflectionInterfaceType* create(slang::TypeLayoutReflection* pSlangTypeLayout);

        bool operator==(const ReflectionInterfaceType& other) const;
        bool operator==(const ReflectionType& other) const override;

        const ParameterBlockReflection* getParameterBlockReflector() const { return mpParameterBlockReflector; }
        void setParameterBlockReflector(const ParameterBlockReflection* pReflector) { mpParameterBlockReflector = pReflector; }

    private:
        ReflectionInterfaceType(slang::TypeLayoutReflection* pSlangTypeLayout);

        const ParameterBlockReflection* mpParameterBlockReflector; // For interface types that have been specialized
    };

    /**
     * An object describing a variable
     #1#
    class ReflectionVar
    {
    public:
        /**
         * Create a new object
         * @param[in] name The name of the variable
         * @param[in] pType The type of the variable
         * @param[in] bindLocation The offset of the variable relative to the parent object
         #1#
        static ReflectionVar* create(const String& name, const ReflectionType* pType, const ShaderVarOffset& bindLocation);

        /**
         * Get the variable name
         #1#
        const String& getName() const { return mName; }

        /**
         * Get the variable type
         #1#
        const ReflectionType* getType() const { return mpType; }

        /**
         * Get the variable offset
         #1#
        ShaderVarOffset getBindLocation() const { return mBindLocation; }
        int getByteOffset() const { return mBindLocation.getByteOffset(); }
        int getOffset() const { return mBindLocation.getByteOffset(); }

        bool operator==(const ReflectionVar& other) const;
        bool operator!=(const ReflectionVar& other) const { return !(*this == other); }

    private:
        ReflectionVar(const String& name, const ReflectionType* pType, const ShaderVarOffset& bindLocation);

        String mName;
        const ReflectionType* mpType;
        ShaderVarOffset mBindLocation;
    };


    class ShaderReflection;

    /**
     * A reflection object describing a parameter block
     #1#
    class ParameterBlockReflection
    {
    public:
        static constexpr uint32 kInvalidIndex = 0xffffffff;

        /**
         * Create a new parameter block reflector, for the given element type.
         #1#
        static ParameterBlockReflection* create(ShaderContent const* pProgramVersion, const ReflectionType* pElementType);

        /**
         * Create a new shader object reflector, for the given element type.
         #1#
        static ParameterBlockReflection* create(ShaderContent const* pProgramVersion, slang::TypeLayoutReflection* pElementType);

        /**
         * Get the type of the contents of the parameter block.
         #1#
        const ReflectionType* getElementType() const { return mpElementType; }

        using BindLocation = TypedShaderVarOffset;

        // TODO(tfoley): The following two functions really pertain to members, not just resources.

        /**
         * Get the variable for a resource in the block
         #1#
        const ReflectionVar* getResource(StringView name) const;

        /**
         * Get the bind-location for a resource in the block
         #1#
        BindLocation getResourceBinding(StringView name) const;

        /**
         * Describes binding information for a resource range.
         *
         * The resource ranges of a parameter block mirror those of its element type 1-to-1.
         * Things like the descriptor type and count for a range can thus be queried on
         * the element type, while the `ParameterBlockReflection` stores additional information
         * pertinent to how resource ranges are bound to the pipeline state.
         #1#
        struct ResourceRangeBindingInfo
        {
            enum class Flavor
            {
                Simple,         ///< A simple resource range (texture/sampler/etc.)
                RootDescriptor, ///< A resource root descriptor (buffers only)
                ConstantBuffer, ///< A sub-object for a constant buffer
                ParameterBlock, ///< A sub-object for a parameter block
                Interface,      ///< A sub-object for an interface-type parameter
            };

            Flavor flavor = Flavor::Simple;
            ReflectionResourceType::Dimensions dimension = ReflectionResourceType::Dimensions::Unknown;

            uint32 regIndex = 0; ///< The register index
            uint32 regSpace = 0; ///< The register space

            uint32 descriptorSetIndex = kInvalidIndex; ///< The index of the descriptor set to be bound into, when flavor is Flavor::Simple.

            /// The reflection object for a sub-object range.
            const ParameterBlockReflection* pSubObjectReflector;

            bool isDescriptorSet() const { return flavor == Flavor::Simple; }
            bool isRootDescriptor() const { return flavor == Flavor::RootDescriptor; }
        };

        struct DefaultConstantBufferBindingInfo
        {
            uint32 regIndex = 0;                       ///< The register index
            uint32 regSpace = 0;                       ///< The register space
            uint32 descriptorSetIndex = kInvalidIndex; ///< The index of the descriptor set to be bound into
            bool useRootConstants = false;
        };

        void setElementType(const ReflectionType* pElementType);

        void addResourceRange(const ResourceRangeBindingInfo& bindingInfo);

        friend struct ParameterBlockReflectionFinalizer;
        void finalize();

        bool hasDefaultConstantBuffer() const;
        void setDefaultConstantBufferBindingInfo(const DefaultConstantBufferBindingInfo& info);
        const DefaultConstantBufferBindingInfo& getDefaultConstantBufferBindingInfo() const;

        /**
         * Get the number of descriptor ranges contained in this type.
         #1#
        uint32 getResourceRangeCount() const { return mResourceRanges.Count(); }

        const ReflectionType::ResourceRange& getResourceRange(uint32 index) const { return getElementType()->getResourceRange(index); }

        /**
         * Get binding information on a contained descriptor range.
         #1#
        const ResourceRangeBindingInfo& getResourceRangeBindingInfo(uint32 index) const { return mResourceRanges[index]; }

        uint32 getRootDescriptorRangeCount() const { return mRootDescriptorRangeIndices.Count(); }
        uint32 getRootDescriptorRangeIndex(uint32 index) const { return mRootDescriptorRangeIndices[index]; }

        uint32 getParameterBlockSubObjectRangeCount() const { return mParameterBlockSubObjectRangeIndices.Count(); }
        uint32 getParameterBlockSubObjectRangeIndex(uint32 index) const { return mParameterBlockSubObjectRangeIndices[index]; }

        const ReflectionVar* findMember(StringView name) const { return getElementType()->findMember(name); }

    private:
        /// The element type of the parameter block
        ///
        /// For a `ConstantBuffer<T>` or `ParameterBlock<T>`,
        /// this will be the type `T`.
        ///
        const ReflectionType* mpElementType;

        /// Binding information for the "default" constant buffer, if needed.
        ///
        DefaultConstantBufferBindingInfo mDefaultConstantBufferBindingInfo;

        /// Binding information for the resource ranges in the element type.
        ///
        /// For something like a `Texture2D` in the element type,
        /// this will record the corresponding `register` and `space`.
        ///
        List<ResourceRangeBindingInfo> mResourceRanges;


        /// Indices of the resource ranges that represent root descriptors,
        /// and which therefore need their resources to be bound to the root signature.
        ///
        /// Note: this array does *not* include information for root descriptors
        /// that correspond to `ParameterBlock` and `ConstantBuffer` sub-objects, since they
        /// are required to allocate and maintain their own root descriptor range indices.
        ///
        List<uint32> mRootDescriptorRangeIndices;

        /// Indices of the resource ranges that represent parameter blocks,
        /// and which therefore need their descriptor sets to be bound
        /// along with the descriptor sets directly stored on the parameter block
        ///
        List<uint32> mParameterBlockSubObjectRangeIndices;
    };

    class EntryPointGroupReflection : public ParameterBlockReflection
    {
    public:
        static EntryPointGroupReflection* create(
            ShaderContent const* pProgramVersion,
            uint32 groupIndex,
            const List<slang::EntryPointLayout*>& pSlangEntryPointReflectors
        );
    };

    /**
     * Reflection object for an entire program. Essentially, it's a collection of ParameterBlocks
     #1#
    class ShaderReflection
    {
    public:

        /// 描述着色器 input 和 output 变量的数据结构。主要用于 VS 输入和 PS 输出
        struct ShaderVariable
        {
            uint32 bindLocation = 0;                                             ///> The bind-location of the variable
            String semanticName;                                                 ///> The semantic name of the variable
            ReflectionBasicType::Type type = ReflectionBasicType::Type::Unknown; ///> The type of the variable
        };
        using VariableMap = Dictionary<String, ShaderVariable>;
        using BindLocation = ParameterBlockReflection::BindLocation;

        /**
         * Data structure describing a hashed string used in the program.
         #1#
        struct HashedString
        {
            uint32 hash;
            String string;
        };

        /**
         * Create a new object for a Shader reflector object
         #1#
        static const ShaderReflection* Create(ShaderContent const* pProgramVersion,
            slang::ShaderReflection* pSlangReflector,
            const List<slang::EntryPointLayout*>& pSlangEntryPointReflectors
        );

        void finalize();

        /**
         * Get parameter block by name
         #1#
        const ParameterBlockReflection* getParameterBlock(StringView name) const;

        /**
         * Get the default (unnamed) parameter block.
         #1#
        const ParameterBlockReflection* getDefaultParameterBlock() const { return mpDefaultBlock; }

        /**
         * For compute-shaders, return the required thread-group size
         #1#
        Int3 getThreadGroupSize() const { return mThreadGroupSize; }

        /**
         * For pixel-shaders, check if we need to run the shader at sample frequency
         #1#
        bool isSampleFrequency() const { return mIsSampleFrequency; }

        /**
         * Get a resource from the default parameter block
         #1#
        const ReflectionVar* getResource(StringView name) const;

        /**
         * Search for a vertex attribute by its semantic name
         #1#
        const ShaderVariable* getVertexAttributeBySemantic(StringView semantic) const;

        /**
         * Search for a vertex attribute by the variable name
         #1#
        const ShaderVariable* getVertexAttribute(StringView name) const;

        /**
         * Get a pixel shader output variable
         #1#
        const ShaderVariable* getPixelShaderOutput(StringView name) const;

        /**
         * Look up a type by name.
         * @return nullptr if the type does not exist.
         #1#
        ReflectionType* findType(StringView name) const;

        const ReflectionVar* findMember(StringView name) const;

        const List<EntryPointGroupReflection*>& getEntryPointGroups() const { return mEntryPointGroups; }

        const EntryPointGroupReflection* getEntryPointGroup(uint32 index) const { return mEntryPointGroups[index]; }

        const List<HashedString>& getHashedStrings() const { return mHashedStrings; }

    private:
        void setDefaultParameterBlock(ParameterBlockReflection* pBlock);

        // ProgramVersion const* mpProgramVersion;

        ParameterBlockReflection* mpDefaultBlock;
        Int3 mThreadGroupSize;
        bool mIsSampleFrequency = false;

        VariableMap mPsOut;
        VariableMap mVertAttr;
        VariableMap mVertAttrBySemantic;

        mutable Dictionary<String, ReflectionType*> mMapNameToType;

        List<EntryPointGroupReflection*> mEntryPointGroups;

        List<HashedString> mHashedStrings;
    };


    struct ShaderContent
    {
        Slang::ComPtr<slang::IComponentType> slangGlobalScope;
        List<Slang::ComPtr<slang::IComponentType>> slangEntryPoints;
    };

} // SE
*/
