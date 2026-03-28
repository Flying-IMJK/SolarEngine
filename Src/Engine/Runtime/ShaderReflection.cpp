
#include "ShaderReflection.h"

#include "Graphics/Shaders/Config.h"

namespace SE
{
    namespace
    {
        const char* kRootDescriptorAttribute = "root";
    }

    TypedShaderVarOffset::TypedShaderVarOffset(const ReflectionType* pType, ShaderVarOffset offset) : ShaderVarOffset(offset), mpType(pType) {}

    TypedShaderVarOffset TypedShaderVarOffset::operator[](StringView name) const
    {
        if (!isValid())
            return *this;

        auto pType = getType();

        if (auto pStructType = pType->asStructType())
        {
            if (auto pMember = pStructType->findMember(name))
            {
                return TypedShaderVarOffset(pMember->getType(), (*this) + pMember->getBindLocation());
            }
        }

        LOG_ERROR("Shader", "No member named '{}' found.", name);
        return TypedShaderVarOffset();
    }

    TypedShaderVarOffset TypedShaderVarOffset::operator[](int index) const
    {
        throw 99;
    }

    TypedShaderVarOffset ReflectionType::getZeroOffset() const
    {
        return TypedShaderVarOffset(this, ShaderVarOffset::kZero);
    }

    TypedShaderVarOffset ReflectionType::getMemberOffset(StringView& name) const
    {
        return getZeroOffset()[name];
    }


    // Represents one link in a "breadcrumb trail" leading from a particular variable
    // back through the path of member-access operations that led to it.
    // E.g., when trying to construct information for `foo.bar.baz`
    // we might have a path that consists of:
    //
    // - An link for the field `baz` in type `Bar` (which knows its offset)
    // - An link for the field `bar` in type `Foo`
    // - An link for the top-level shader parameter `foo`
    //
    // To compute the correct offset for `baz` we can walk up this chain
    // and add up offsets.
    //
    // In simple cases, one can track this info top-down, by simply keeping
    // a "running total" offset, but that doesn't account for the fact that
    // `baz` might be a texture, UAV, sampler, or uniform, and the offset
    // we'd need to track for each case is different.
    //
    struct ReflectionPathLink
    {
        const ReflectionPathLink* pParent = nullptr;
        slang::VariableLayoutReflection* pVar = nullptr;
    };

    // Represents a full"breadcrumb trail" leading from a particular variable
    // back through the path of member-access operations that led to it.
    //
    // The `pPrimary` field represents the main path that gets used for
    // ordinary uniform, texture, buffer, etc. variables. In the 99% case
    // this is all that ever gets used.
    //
    // The `pDeferred` field represents a secondary path that describes
    // where the data that arose due to specialization ended up.
    //
    // E.g., if we have a program like:
    //
    //     struct MyStuff { IFoo f; Texture2D t; }
    //     MyStuff gStuff;
    //     Texture2D gOther;
    //
    // Then `gStuff` will be assigned a starting `t` register of `t0`,
    // and the *primary* path for `gStuff.t` will show that offset.
    //
    // However, if `gStuff.f` gets specialized to some type `Bar`:
    //
    //     struct Bar { Texture2D b; }
    //
    // Then the `gStuff.f.b` field also needs a texture register to be
    // assigned. It can't use registers `t0` or `t1` since those were
    // already allocated in the unspecialized program (to `gStuff.t`
    // and `gOther`, respectively), so it needs to use `t2`.
    //
    // But that means that the allocation for `gStuff` is split into two
    // pieces: a "primary" allocation for `gStuff.t`, and then a secondary
    // allocation for `gStuff.f` that got "deferred" until after specialization
    // (which means it comes after all the un-specialized parameters).
    //
    // The Slang reflection information lets us query both the primary
    // and deferred allocation/layout for a shader parameter, and we
    // need to handle both in order to support specialization.
    //
    struct ReflectionPath
    {
        ReflectionPathLink* pPrimary = nullptr;
        ReflectionPathLink* pDeferred = nullptr;
    };

    // A helper RAII type to extend a `ReflectionPath` with
    // additional links as needed based on the reflection
    // information from a Slang `VariableLayoutReflection`.
    //
    struct ExtendedReflectionPath : ReflectionPath
    {
        ExtendedReflectionPath(ReflectionPath const* pParent, slang::VariableLayoutReflection* pVar)
        {
            // If there is any path stored in `pParent`,
            // then that will be our starting point.
            //
            if (pParent)
            {
                pPrimary = pParent->pPrimary;
                pDeferred = pParent->pDeferred;
            }

            // Next, if `pVar` has a primary layout (and/or
            // an optional pending/deferred layout), then
            // we will extend the appropriate breadcrumb
            // trail with its information.
            //
            if (pVar)
            {
                primaryLinkStorage.pParent = pPrimary;
                primaryLinkStorage.pVar = pVar;
                pPrimary = &primaryLinkStorage;

                if (auto pDeferredVar = pVar->getPendingDataLayout())
                {
                    deferredLinkStorage.pParent = pDeferred;
                    deferredLinkStorage.pVar = pDeferredVar;
                    pDeferred = &deferredLinkStorage;
                }
            }
        }

        // These "storage" fields are used in the constructor
        // when it needs to allocate additional links. By pre-allocating
        // them here in the body of the type we avoid having to do
        // heap allocation when constructing an extended path.
        //
        ReflectionPathLink primaryLinkStorage;
        ReflectionPathLink deferredLinkStorage;
    };

    static ReflectionResourceType::Type getResourceType(slang::TypeReflection* pSlangType)
    {
        switch (pSlangType->unwrapArray()->getKind())
        {
        case slang::TypeReflection::Kind::ParameterBlock:
        case slang::TypeReflection::Kind::ConstantBuffer:
            return ReflectionResourceType::Type::ConstantBuffer;
        case slang::TypeReflection::Kind::SamplerState:
            return ReflectionResourceType::Type::Sampler;
        case slang::TypeReflection::Kind::ShaderStorageBuffer:
            return ReflectionResourceType::Type::StructuredBuffer;
        case slang::TypeReflection::Kind::TextureBuffer:
            return ReflectionResourceType::Type::TypedBuffer;
        case slang::TypeReflection::Kind::Resource:
            switch (pSlangType->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK)
            {
            case SLANG_STRUCTURED_BUFFER:
                return ReflectionResourceType::Type::StructuredBuffer;
            case SLANG_BYTE_ADDRESS_BUFFER:
                return ReflectionResourceType::Type::RawBuffer;
            case SLANG_TEXTURE_BUFFER:
                return ReflectionResourceType::Type::TypedBuffer;
            case SLANG_ACCELERATION_STRUCTURE:
                return ReflectionResourceType::Type::AccelerationStructure;
            case SLANG_TEXTURE_1D:
            case SLANG_TEXTURE_2D:
            case SLANG_TEXTURE_3D:
            case SLANG_TEXTURE_CUBE:
                return ReflectionResourceType::Type::Texture;
            default:
                return ReflectionResourceType::Type(-1);
            }
        default:
            return ReflectionResourceType::Type(-1);
        }
    }

    static ReflectionResourceType::ShaderAccess getShaderAccess(slang::TypeReflection* pSlangType)
    {
        // Compute access for an array using the underlying type...
        pSlangType = pSlangType->unwrapArray();

        switch (pSlangType->getKind())
        {
        case slang::TypeReflection::Kind::SamplerState:
        case slang::TypeReflection::Kind::ConstantBuffer:
            return ReflectionResourceType::ShaderAccess::Read;
            break;

        case slang::TypeReflection::Kind::Resource:
        case slang::TypeReflection::Kind::ShaderStorageBuffer:
            switch (pSlangType->getResourceAccess())
            {
        case SLANG_RESOURCE_ACCESS_NONE:
            return ReflectionResourceType::ShaderAccess::Undefined;

        case SLANG_RESOURCE_ACCESS_READ:
            return ReflectionResourceType::ShaderAccess::Read;

        default:
            return ReflectionResourceType::ShaderAccess::ReadWrite;
            }
            break;

        default:
            return ReflectionResourceType::ShaderAccess::Undefined;
        }
    }

    static ReflectionResourceType::ReturnType getReturnType(slang::TypeReflection* pType)
    {
        // Could be a resource that doesn't have a specific element type (e.g., a raw buffer)
        if (!pType)
            return ReflectionResourceType::ReturnType::Unknown;

        switch (pType->getScalarType())
        {
        case slang::TypeReflection::ScalarType::Float32:
            return ReflectionResourceType::ReturnType::Float;
        case slang::TypeReflection::ScalarType::Int32:
            return ReflectionResourceType::ReturnType::Int;
        case slang::TypeReflection::ScalarType::UInt32:
            return ReflectionResourceType::ReturnType::Uint;
        case slang::TypeReflection::ScalarType::Float64:
            return ReflectionResourceType::ReturnType::Double;

            // Could be a resource that uses an aggregate element type (e.g., a structured buffer)
        case slang::TypeReflection::ScalarType::None:
            return ReflectionResourceType::ReturnType::Unknown;

        default:
            return ReflectionResourceType::ReturnType::Unknown;
        }
    }

    static ReflectionResourceType::Dimensions getResourceDimensions(SlangResourceShape shape)
    {
        switch (shape)
        {
        case SLANG_TEXTURE_1D:
            return ReflectionResourceType::Dimensions::Texture1D;
        case SLANG_TEXTURE_1D_ARRAY:
            return ReflectionResourceType::Dimensions::Texture1DArray;
        case SLANG_TEXTURE_2D:
            return ReflectionResourceType::Dimensions::Texture2D;
        case SLANG_TEXTURE_2D_ARRAY:
            return ReflectionResourceType::Dimensions::Texture2DArray;
        case SLANG_TEXTURE_2D_MULTISAMPLE:
            return ReflectionResourceType::Dimensions::Texture2DMS;
        case SLANG_TEXTURE_2D_MULTISAMPLE_ARRAY:
            return ReflectionResourceType::Dimensions::Texture2DMSArray;
        case SLANG_TEXTURE_3D:
            return ReflectionResourceType::Dimensions::Texture3D;
        case SLANG_TEXTURE_CUBE:
            return ReflectionResourceType::Dimensions::TextureCube;
        case SLANG_TEXTURE_CUBE_ARRAY:
            return ReflectionResourceType::Dimensions::TextureCubeArray;
        case SLANG_ACCELERATION_STRUCTURE:
            return ReflectionResourceType::Dimensions::AccelerationStructure;

        case SLANG_TEXTURE_BUFFER:
        case SLANG_STRUCTURED_BUFFER:
        case SLANG_BYTE_ADDRESS_BUFFER:
            return ReflectionResourceType::Dimensions::Buffer;

        default:
            return ReflectionResourceType::Dimensions::Unknown;
        }
    }

    ReflectionBasicType::Type getVariableType(slang::TypeReflection::ScalarType slangScalarType, uint32 rows, uint32 columns)
    {
        switch (slangScalarType)
        {
        case slang::TypeReflection::ScalarType::Bool:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Bool;
            case 2:
                return ReflectionBasicType::Type::Bool2;
            case 3:
                return ReflectionBasicType::Type::Bool3;
            case 4:
                return ReflectionBasicType::Type::Bool4;
            }
            break;
        case slang::TypeReflection::ScalarType::UInt8:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Uint8;
            case 2:
                return ReflectionBasicType::Type::Uint8_2;
            case 3:
                return ReflectionBasicType::Type::Uint8_3;
            case 4:
                return ReflectionBasicType::Type::Uint8_4;
            }
            break;
        case slang::TypeReflection::ScalarType::UInt16:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Uint16;
            case 2:
                return ReflectionBasicType::Type::Uint16_2;
            case 3:
                return ReflectionBasicType::Type::Uint16_3;
            case 4:
                return ReflectionBasicType::Type::Uint16_4;
            }
            break;
        case slang::TypeReflection::ScalarType::UInt32:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Uint;
            case 2:
                return ReflectionBasicType::Type::Uint2;
            case 3:
                return ReflectionBasicType::Type::Uint3;
            case 4:
                return ReflectionBasicType::Type::Uint4;
            }
            break;
        case slang::TypeReflection::ScalarType::UInt64:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Uint64;
            case 2:
                return ReflectionBasicType::Type::Uint64_2;
            case 3:
                return ReflectionBasicType::Type::Uint64_3;
            case 4:
                return ReflectionBasicType::Type::Uint64_4;
            }
            break;
        case slang::TypeReflection::ScalarType::Int8:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Int8;
            case 2:
                return ReflectionBasicType::Type::Int8_2;
            case 3:
                return ReflectionBasicType::Type::Int8_3;
            case 4:
                return ReflectionBasicType::Type::Int8_4;
            }
            break;
        case slang::TypeReflection::ScalarType::Int16:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Int16;
            case 2:
                return ReflectionBasicType::Type::Int16_2;
            case 3:
                return ReflectionBasicType::Type::Int16_3;
            case 4:
                return ReflectionBasicType::Type::Int16_4;
            }
            break;
        case slang::TypeReflection::ScalarType::Int32:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Int;
            case 2:
                return ReflectionBasicType::Type::Int2;
            case 3:
                return ReflectionBasicType::Type::Int3;
            case 4:
                return ReflectionBasicType::Type::Int4;
            }
            break;
        case slang::TypeReflection::ScalarType::Int64:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Int64;
            case 2:
                return ReflectionBasicType::Type::Int64_2;
            case 3:
                return ReflectionBasicType::Type::Int64_3;
            case 4:
                return ReflectionBasicType::Type::Int64_4;
            }
            break;
        case slang::TypeReflection::ScalarType::Float16:
            switch (rows)
            {
        case 1:
            switch (columns)
            {
        case 1:
            return ReflectionBasicType::Type::Float16;
        case 2:
            return ReflectionBasicType::Type::Float16_2;
        case 3:
            return ReflectionBasicType::Type::Float16_3;
        case 4:
            return ReflectionBasicType::Type::Float16_4;
            }
                break;
        case 2:
            switch (columns)
            {
        case 2:
            return ReflectionBasicType::Type::Float16_2x2;
        case 3:
            return ReflectionBasicType::Type::Float16_2x3;
        case 4:
            return ReflectionBasicType::Type::Float16_2x4;
            }
                break;
        case 3:
            switch (columns)
            {
        case 2:
            return ReflectionBasicType::Type::Float16_3x2;
        case 3:
            return ReflectionBasicType::Type::Float16_3x3;
        case 4:
            return ReflectionBasicType::Type::Float16_3x4;
            }
                break;
        case 4:
            switch (columns)
            {
        case 2:
            return ReflectionBasicType::Type::Float16_4x2;
        case 3:
            return ReflectionBasicType::Type::Float16_4x3;
        case 4:
            return ReflectionBasicType::Type::Float16_4x4;
            }
                break;
            }
            break;
        case slang::TypeReflection::ScalarType::Float32:
            switch (rows)
            {
        case 1:
            switch (columns)
            {
        case 1:
            return ReflectionBasicType::Type::Float;
        case 2:
            return ReflectionBasicType::Type::Float2;
        case 3:
            return ReflectionBasicType::Type::Float3;
        case 4:
            return ReflectionBasicType::Type::Float4;
            }
                break;
        case 2:
            switch (columns)
            {
        case 2:
            return ReflectionBasicType::Type::Float2x2;
        case 3:
            return ReflectionBasicType::Type::Float2x3;
        case 4:
            return ReflectionBasicType::Type::Float2x4;
            }
                break;
        case 3:
            switch (columns)
            {
        case 2:
            return ReflectionBasicType::Type::Float3x2;
        case 3:
            return ReflectionBasicType::Type::Float3x3;
        case 4:
            return ReflectionBasicType::Type::Float3x4;
            }
                break;
        case 4:
            switch (columns)
            {
        case 2:
            return ReflectionBasicType::Type::Float4x2;
        case 3:
            return ReflectionBasicType::Type::Float4x3;
        case 4:
            return ReflectionBasicType::Type::Float4x4;
            }
                break;
            }
            break;
        case slang::TypeReflection::ScalarType::Float64:
            ENGINE_ASSERT(rows == 1);
            switch (columns)
            {
            case 1:
                return ReflectionBasicType::Type::Float64;
            case 2:
                return ReflectionBasicType::Type::Float64_2;
            case 3:
                return ReflectionBasicType::Type::Float64_3;
            case 4:
                return ReflectionBasicType::Type::Float64_4;
            }
            break;
        }
        
        return ReflectionBasicType::Type::Unknown;
    }

    static ReflectionResourceType::StructuredType getStructuredBufferType(slang::TypeReflection* pSlangType)
    {
        auto invalid = ReflectionResourceType::StructuredType::Invalid;

        if (pSlangType->getKind() != slang::TypeReflection::Kind::Resource)
            return invalid; // not a structured buffer

        if (pSlangType->getResourceShape() != SLANG_STRUCTURED_BUFFER)
            return invalid; // not a structured buffer

        switch (pSlangType->getResourceAccess())
        {
        default:
            return invalid;

        case SLANG_RESOURCE_ACCESS_READ:
            return ReflectionResourceType::StructuredType::Default;

        case SLANG_RESOURCE_ACCESS_READ_WRITE:
        case SLANG_RESOURCE_ACCESS_RASTER_ORDERED:
            return ReflectionResourceType::StructuredType::Counter;
        case SLANG_RESOURCE_ACCESS_APPEND:
            return ReflectionResourceType::StructuredType::Append;
        case SLANG_RESOURCE_ACCESS_CONSUME:
            return ReflectionResourceType::StructuredType::Consume;
        }
    };

    ReflectionVar* reflectVariable(
        slang::VariableLayoutReflection* pSlangLayout,
        ShaderVarOffset::RangeIndex rangeIndex,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    );

    ReflectionType* reflectType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    );

    // Determine if a Slang type layout consumes any storage/resources of the given kind
    static bool hasUsage(slang::TypeLayoutReflection* pSlangTypeLayout, SlangParameterCategory resourceKind)
    {
        auto kindCount = pSlangTypeLayout->getCategoryCount();
        for (unsigned int ii = 0; ii < kindCount; ++ii)
        {
            if (SlangParameterCategory(pSlangTypeLayout->getCategoryByIndex(ii)) == resourceKind)
            {
                return true;
            }
        }
        return false;
    }

    // Given a "breadcrumb trail" (reflection path), determine
    // the actual register/binding that will be used by a leaf
    // parameter for the given resource kind.
    static int getRegisterIndexFromPath(const ReflectionPathLink* pPath, SlangParameterCategory category)
    {
        uint32 offset = 0;
        for (auto pp = pPath; pp; pp = pp->pParent)
        {
            if (pp->pVar)
            {
                // We are in the process of walking up from a leaf
                // shader variable to the root (some global shader
                // parameter).
                //
                // If along the way we run into a parameter block,
                // *and* that parameter block has been allocated
                // into its own register space, then we should stop
                // adding contributions to the register/binding of
                // the leaf parameter, since any register offsets
                // coming from "above" this point shouldn't affect
                // the register/binding of a parameter inside of
                // the parameter block.
                //
                // TODO: This logic is really fiddly and doesn't
                // seem like something Falcor should have to do.
                // The Slang library should be provided utility
                // functions to handle this stuff.
                //
                if (pp->pVar->getType()->getKind() == slang::TypeReflection::Kind::ParameterBlock &&
                    hasUsage(pp->pVar->getTypeLayout(), SLANG_PARAMETER_CATEGORY_REGISTER_SPACE) &&
                    category != SLANG_PARAMETER_CATEGORY_REGISTER_SPACE)
                {
                    return offset;
                }

                offset += (uint32)pp->pVar->getOffset(category);
                continue;
            }
            LOG_ERROR("Shader", "Invalid reflection path");
        }
        return offset;
    }

    static uint32 getRegisterSpaceFromPath(const ReflectionPathLink* pPath, SlangParameterCategory category)
    {
        uint32 offset = 0;
        for (auto pp = pPath; pp; pp = pp->pParent)
        {
            if (pp->pVar)
            {
                // Similar to the case above in `getRegisterIndexFromPath`,
                // if we are walking from a member in a parameter block
                // up to the block itself, then the space for our parameter
                // should be offset by the register space assigned to
                // the block itself, and we should stop walking up
                // the breadcrumb trail.
                //
                // TODO: Just as in `getRegisterIndexFromPath` this is way
                // too subtle, and Slang should be providing a service
                // to compute this.
                //
                if (pp->pVar->getTypeLayout()->getKind() == slang::TypeReflection::Kind::ParameterBlock)
                {
                    return offset + (uint32)getRegisterIndexFromPath(pp, SLANG_PARAMETER_CATEGORY_REGISTER_SPACE);
                }
                offset += (uint32)pp->pVar->getBindingSpace(category);
                continue;
            }

            LOG_ERROR("Shader", "Invalid reflection path");
        }
        return offset;
    }

    static ShaderResourceType getShaderResourceType(const ReflectionResourceType* pType);

    static slang::ParameterCategory getParameterCategory(slang::TypeLayoutReflection* pTypeLayout);

    static void extractDefaultConstantBufferBinding(
        slang::TypeLayoutReflection* pSlangType,
        ReflectionPath* pPath,
        ParameterBlockReflection* pBlock,
        bool shouldUseRootConstants
    )
    {
        auto pContainerLayout = pSlangType->getContainerVarLayout();
        ENGINE_ASSERT(pContainerLayout);

        ExtendedReflectionPath containerPath(pPath, pContainerLayout);
        int32_t containerCategoryCount = pContainerLayout->getCategoryCount();
        for (int32_t containerCategoryIndex = 0; containerCategoryIndex < containerCategoryCount; ++containerCategoryIndex)
        {
            auto containerCategory = pContainerLayout->getCategoryByIndex(containerCategoryIndex);
            switch (containerCategory)
            {
            case slang::ParameterCategory::DescriptorTableSlot:
            case slang::ParameterCategory::ConstantBuffer:
            {
                ParameterBlockReflection::DefaultConstantBufferBindingInfo defaultConstantBufferInfo;
                defaultConstantBufferInfo.regIndex =
                    (uint32)getRegisterIndexFromPath(containerPath.pPrimary, SlangParameterCategory(containerCategory));
                defaultConstantBufferInfo.regSpace =
                    getRegisterSpaceFromPath(containerPath.pPrimary, SlangParameterCategory(containerCategory));
                defaultConstantBufferInfo.useRootConstants = shouldUseRootConstants;
                pBlock->setDefaultConstantBufferBindingInfo(defaultConstantBufferInfo);
            }
                break;

            default:
                break;
            }
        }
    }

    ReflectionType* reflectResourceType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        ReflectionResourceType::Type type = getResourceType(pSlangType->getType());
        ReflectionResourceType::Dimensions dims = getResourceDimensions(pSlangType->getResourceShape());

        ReflectionResourceType::ShaderAccess shaderAccess = getShaderAccess(pSlangType->getType());
        ReflectionResourceType::ReturnType retType = getReturnType(pSlangType->getType());
        ReflectionResourceType::StructuredType structuredType = getStructuredBufferType(pSlangType->getType());

        ENGINE_ASSERT(pPath->pPrimary && pPath->pPrimary->pVar);
        String name(pPath->pPrimary->pVar->getName());

        // Check if resource type represents a root descriptor.
        // In the shader we use a custom [root] attribute to flag resources to map to root descriptors.
        auto pVar = pPath->pPrimary->pVar->getVariable();
        bool isRootDescriptor = pVar->findUserAttributeByName(pProgramVersion->slangGlobalScope->getSession()->getGlobalSession(), kRootDescriptorAttribute) != nullptr;

        // Check that the root descriptor type is supported.
        if (isRootDescriptor)
        {
            // Check the resource type and shader access.
            if (type != ReflectionResourceType::Type::RawBuffer && type != ReflectionResourceType::Type::StructuredBuffer &&
                type != ReflectionResourceType::Type::AccelerationStructure)
            {
                LOG_ERROR("Shader",
                    "Resource '{}' cannot be bound as root descriptor. Only raw buffers, structured buffers, and acceleration structures are supported.",
                    name
                );
            }
            if (shaderAccess != ReflectionResourceType::ShaderAccess::Read && shaderAccess != ReflectionResourceType::ShaderAccess::ReadWrite)
            {
                LOG_ERROR("Shader", "Buffer '{}' cannot be bound as root descriptor. Only SRV/UAVs are supported.", name);
            }
            ENGINE_ASSERT(
                type != ReflectionResourceType::Type::AccelerationStructure || shaderAccess == ReflectionResourceType::ShaderAccess::Read
            );

            // Check that it's not an append/consume structured buffer, which is unsupported for root descriptors.
            // RWStructuredBuffer with counter is also not supported, but we cannot see that on the type declaration.
            // At bind time, we'll validate that the buffer has not been created with a UAV counter.
            if (type == ReflectionResourceType::Type::StructuredBuffer)
            {
                ENGINE_ASSERT(structuredType != ReflectionResourceType::StructuredType::Invalid);
                if (structuredType == ReflectionResourceType::StructuredType::Append ||
                    structuredType == ReflectionResourceType::StructuredType::Consume)
                {
                    LOG_ERROR("Shader", 
                        "StructuredBuffer '{}' cannot be bound as root descriptor. Only regular structured buffers are supported, not append/consume buffers.",
                        name
                    );
                }
            }
            ENGINE_ASSERT(
                dims == ReflectionResourceType::Dimensions::Buffer || dims == ReflectionResourceType::Dimensions::AccelerationStructure
            ); // We shouldn't get here otherwise
        }

        ReflectionResourceType* pType = ReflectionResourceType::create(type, dims, structuredType, retType, shaderAccess, pSlangType);

        slang::ParameterCategory category = getParameterCategory(pSlangType);
        ParameterBlockReflection::ResourceRangeBindingInfo bindingInfo;
        bindingInfo.regIndex = getRegisterIndexFromPath(pPath->pPrimary, SlangParameterCategory(category));
        bindingInfo.regSpace = getRegisterSpaceFromPath(pPath->pPrimary, SlangParameterCategory(category));
        bindingInfo.dimension = dims;

        if (isRootDescriptor)
            bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::RootDescriptor;

        switch (type)
        {
        default:
            break;

            case ReflectionResourceType::Type::StructuredBuffer:
            {
                const auto& pElementLayout = pSlangType->getElementTypeLayout();
                auto pBufferType = reflectType(pElementLayout, pBlock, pPath, pProgramVersion);
                pType->setStructType(pBufferType);
            }
            break;

            // TODO: The fact that constant buffers (and parameter blocks, since Falcor currently
            // pretends that parameter blocks are constant buffers in its reflection types) are
            // treated so differently from other resource types is a huge sign that they should
            // *not* be resource types to begin with (and they *aren't* resource types in Slang).
            //
            case ReflectionResourceType::Type::ConstantBuffer:
            {
                // We have a sub-parameter-block (whether a true parameter block, or just a constant buffer)
                auto pSubBlock = New<ParameterBlockReflection>();
                const auto& pElementLayout = pSlangType->getElementTypeLayout();
                auto pElementType = reflectType(pElementLayout, pSubBlock, pPath, pProgramVersion);
                pSubBlock->setElementType(pElementType);

                extractDefaultConstantBufferBinding(pSlangType, pPath, pSubBlock, /*shouldUseRootConstants:*/ false);

                pSubBlock->finalize();

                pType->setStructType(pElementType);
                pType->setParameterBlockReflector(pSubBlock);

                // TODO: `pSubBlock` should probably get stored on the
                // `ReflectionResourceType` somewhere, so that we can
                // retrieve it later without having to use a parent
                // `ParameterBlockReflection` to look it up.

                if (pSlangType->getKind() == slang::TypeReflection::Kind::ParameterBlock)
                {
                    bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::ParameterBlock;
                }
                else
                {
                    bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::ConstantBuffer;
                }
                bindingInfo.pSubObjectReflector = pSubBlock;
            }
            break;
        }

        if (pBlock)
        {
            pBlock->addResourceRange(bindingInfo);
        }

        return pType;
    }

    ReflectionType* reflectStructType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        // Note: not all types have names. In particular, the "element type" of
        // a `cbuffer` declaration is an anonymous `struct` type, and Slang
        // returns `nullptr` from `getName().
        //
        auto pSlangName = pSlangType->getName();
        StringAnsi name = pSlangName ? StringAnsi(pSlangName) : StringAnsi::Empty;

        ReflectionStructType* pType = ReflectionStructType::create(pSlangType->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM), name.ToString(), pSlangType);

        ReflectionStructType::BuildState buildState;

        for (uint32 i = 0; i < pSlangType->getFieldCount(); i++)
        {
            auto pSlangField = pSlangType->getFieldByIndex(i);
            ExtendedReflectionPath fieldPath(pPath, pSlangField);

            ReflectionVar* pVar = reflectVariable(pSlangField, pType->getResourceRangeCount(), pBlock, &fieldPath, pProgramVersion);
            if (pVar)
            {
                pType->addMember(pVar, buildState);
            }
        }
        return pType;
    }

    static int getByteSize(slang::TypeLayoutReflection* pSlangType)
    {
        return pSlangType->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM);
    }

    ReflectionType* reflectArrayType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        uint32 elementCount = (uint32)pSlangType->getElementCount();
        uint32 elementByteStride = (uint32)pSlangType->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);

        ReflectionType* pElementType = reflectType(pSlangType->getElementTypeLayout(), pBlock, pPath, pProgramVersion);
        ReflectionArrayType* pArrayType = ReflectionArrayType::create(elementCount, elementByteStride, pElementType, getByteSize(pSlangType), pSlangType);
        return pArrayType;
    }

    ReflectionType* reflectBasicType(slang::TypeLayoutReflection* pSlangType)
    {
        const bool isRowMajor = pSlangType->getMatrixLayoutMode() == SLANG_MATRIX_LAYOUT_ROW_MAJOR;
        ReflectionBasicType::Type type = getVariableType(pSlangType->getScalarType(), pSlangType->getRowCount(), pSlangType->getColumnCount());
        ReflectionType* pType = ReflectionBasicType::create(type, isRowMajor, pSlangType->getSize(), pSlangType);
        return pType;
    }

    ReflectionType* reflectInterfaceType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        auto pType = ReflectionInterfaceType::create(pSlangType);

        slang::ParameterCategory category = getParameterCategory(pSlangType);
        ParameterBlockReflection::ResourceRangeBindingInfo bindingInfo;
        bindingInfo.regIndex = (uint32)getRegisterIndexFromPath(pPath->pPrimary, SlangParameterCategory(category));
        bindingInfo.regSpace = getRegisterSpaceFromPath(pPath->pPrimary, SlangParameterCategory(category));

        bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::Interface;

        if (auto pSlangPendingTypeLayout = pSlangType->getPendingDataTypeLayout())
        {
            ReflectionPath subPath;
            subPath.pPrimary = pPath->pDeferred;
            subPath.pDeferred = nullptr;

            auto pPendingBlock = New<ParameterBlockReflection>();
            auto pPendingType = reflectType(pSlangPendingTypeLayout, pPendingBlock, &subPath, pProgramVersion);
            pPendingBlock->setElementType(pPendingType);

            // TODO: What to do if `pPendingType->getByteSize()` is non-zero?

            pPendingBlock->finalize();

            pType->setParameterBlockReflector(pPendingBlock);

            bindingInfo.pSubObjectReflector = pPendingBlock;

            category = slang::ParameterCategory::Uniform;
            bindingInfo.regIndex = (uint32)getRegisterIndexFromPath(pPath->pDeferred, SlangParameterCategory(category));
            bindingInfo.regSpace = getRegisterSpaceFromPath(pPath->pPrimary, SlangParameterCategory(category));
        }

        if (pBlock)
        {
            pBlock->addResourceRange(bindingInfo);
        }

        return pType;
    }

    ReflectionType* reflectSpecializedType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        auto pSlangBaseType = pSlangType->getElementTypeLayout();

        auto pSlangVarLayout = pSlangType->getSpecializedTypePendingDataVarLayout();

        ReflectionPathLink deferredLink;
        deferredLink.pParent = pPath->pPrimary;
        deferredLink.pVar = pSlangVarLayout;

        ReflectionPath path;
        path.pPrimary = pPath->pPrimary;
        path.pDeferred = &deferredLink;

        return reflectType(pSlangBaseType, pBlock, &path, pProgramVersion);
    }

    ReflectionType* reflectType(
        slang::TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        ENGINE_ASSERT(pSlangType);
        auto kind = pSlangType->getType()->getKind();
        switch (kind)
        {
        case slang::TypeReflection::Kind::ParameterBlock:
        case slang::TypeReflection::Kind::Resource:
        case slang::TypeReflection::Kind::SamplerState:
        case slang::TypeReflection::Kind::ConstantBuffer:
        case slang::TypeReflection::Kind::ShaderStorageBuffer:
        case slang::TypeReflection::Kind::TextureBuffer:
            return reflectResourceType(pSlangType, pBlock, pPath, pProgramVersion);
        case slang::TypeReflection::Kind::Struct:
            return reflectStructType(pSlangType, pBlock, pPath, pProgramVersion);
        case slang::TypeReflection::Kind::Array:
            return reflectArrayType(pSlangType, pBlock, pPath, pProgramVersion);
        case slang::TypeReflection::Kind::Interface:
            return reflectInterfaceType(pSlangType, pBlock, pPath, pProgramVersion);
        case slang::TypeReflection::Kind::Specialized:
            return reflectSpecializedType(pSlangType, pBlock, pPath, pProgramVersion);
        case slang::TypeReflection::Kind::Scalar:
        case slang::TypeReflection::Kind::Matrix:
        case slang::TypeReflection::Kind::Vector:
            return reflectBasicType(pSlangType);
        case slang::TypeReflection::Kind::None:
            return nullptr;
        case slang::TypeReflection::Kind::GenericTypeParameter:
            // TODO: How to handle this type? Let it generate an error for now.
            LOG_ERROR("Shader", "Unexpected Slang type");
        default:
            ENGINE_UNREACHABLE_CODE();
        }
        return nullptr;
    }

    static slang::ParameterCategory getParameterCategory(slang::TypeLayoutReflection* pTypeLayout)
    {
        slang::ParameterCategory category = pTypeLayout->getParameterCategory();
        if (category == slang::ParameterCategory::Mixed)
        {
            switch (pTypeLayout->getKind())
            {
            case slang::TypeReflection::Kind::ConstantBuffer:
            case slang::TypeReflection::Kind::ParameterBlock:
            case slang::TypeReflection::Kind::None:
                category = slang::ParameterCategory::ConstantBuffer;
                break;
            }
        }
        return category;
    }

    static slang::ParameterCategory getParameterCategory(slang::VariableLayoutReflection* pVarLayout)
    {
        return getParameterCategory(pVarLayout->getTypeLayout());
    }

    ReflectionVar* reflectVariable(
        slang::VariableLayoutReflection* pSlangLayout,
        ShaderVarOffset::RangeIndex rangeIndex,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ShaderContent const* pProgramVersion
    )
    {
        ENGINE_ASSERT(pPath);
        String name(pSlangLayout->getName());

        ReflectionType* pType = reflectType(pSlangLayout->getTypeLayout(), pBlock, pPath, pProgramVersion);
        auto byteOffset = (ShaderVarOffset::ByteOffset)pSlangLayout->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM);

        ReflectionVar* pVar = ReflectionVar::create(name, pType,
            ShaderVarOffset(UniformShaderVarOffset(byteOffset), ResourceShaderVarOffset(rangeIndex, 0)));

        return pVar;
    }

    ReflectionVar* reflectTopLevelVariable(
        slang::VariableLayoutReflection* pSlangLayout,
        ShaderVarOffset::RangeIndex rangeIndex,
        ParameterBlockReflection* pBlock,
        ShaderContent const* pProgramVersion
    )
    {
        ExtendedReflectionPath path(nullptr, pSlangLayout);

        return reflectVariable(pSlangLayout, rangeIndex, pBlock, &path, pProgramVersion);
    }

    static void storeShaderVariable(
        const ReflectionPath& path,
        SlangParameterCategory category,
        const String& name,
        ShaderReflection::VariableMap& varMap,
        ShaderReflection::VariableMap* pVarMapBySemantic,
        uint32 count,
        uint32 stride
    )
    {
        auto pVar = path.pPrimary->pVar;

        ShaderReflection::ShaderVariable var;
        const auto& pTypeLayout = pVar->getTypeLayout();
        var.type = getVariableType(pTypeLayout->getScalarType(), pTypeLayout->getRowCount(), pTypeLayout->getColumnCount());

        uint32 baseIndex = (uint32)getRegisterIndexFromPath(path.pPrimary, category);
        for (uint32 i = 0; i < std::max(count, 1u); i++)
        {
            var.bindLocation = baseIndex + (i * stride);
            var.semanticName = pVar->getSemanticName();
            if (count)
            {
                var.semanticName += String::Format(SE_TEXT("[{0}]"), i);
            }
            varMap[name] = var;
            if (pVarMapBySemantic)
            {
                (*pVarMapBySemantic)[var.semanticName] = var;
            }
        }
    }

    static void reflectVaryingParameter(
        const ReflectionPath& path,
        const String& name,
        SlangParameterCategory category,
        ShaderReflection::VariableMap& varMap,
        ShaderReflection::VariableMap* pVarMapBySemantic = nullptr
    )
    {
        auto pVar = path.pPrimary->pVar;
        slang::TypeLayoutReflection* pTypeLayout = pVar->getTypeLayout();
        // Skip parameters that don't consume space in the given category
        if (pTypeLayout->getSize(category) == 0)
            return;

        slang::TypeReflection::Kind kind = pTypeLayout->getKind();
        // If this is a leaf node, store it
        if ((kind == slang::TypeReflection::Kind::Matrix) || (kind == slang::TypeReflection::Kind::Vector) || (kind == slang::TypeReflection::Kind::Scalar))
        {
            storeShaderVariable(path, category, name, varMap, pVarMapBySemantic, 0, 0);
        }
        else if (kind == slang::TypeReflection::Kind::Array)
        {
            auto arrayKind = pTypeLayout->getElementTypeLayout()->getKind();
            ENGINE_ASSERT(
                (arrayKind == slang::TypeReflection::Kind::Matrix) || (arrayKind == slang::TypeReflection::Kind::Vector) ||
                (arrayKind == slang::TypeReflection::Kind::Scalar)
            );
            uint32 arraySize = (uint32)pTypeLayout->getTotalArrayElementCount();
            uint32 arrayStride = (uint32)pTypeLayout->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);
            storeShaderVariable(path, category, name, varMap, pVarMapBySemantic, arraySize, arrayStride);
        }
        else if (kind == slang::TypeReflection::Kind::Struct)
        {
            for (uint32 f = 0; f < pTypeLayout->getFieldCount(); f++)
            {
                auto pField = pTypeLayout->getFieldByIndex(f);
                ExtendedReflectionPath newPath(&path, pField);
                String memberName = name;
                memberName.Append(SE_TEXT('.'));
                memberName.Append(pField->getName());
                reflectVaryingParameter(newPath, memberName, category, varMap, pVarMapBySemantic);
            }
        }
        else
        {
            ENGINE_UNREACHABLE_CODE();
        }
    }

    static void reflectShaderIO(
        slang::EntryPointReflection* pEntryPoint,
        SlangParameterCategory category,
        ShaderReflection::VariableMap& varMap,
        ShaderReflection::VariableMap* pVarMapBySemantic = nullptr
    )
    {
        uint32 entryPointParamCount = pEntryPoint->getParameterCount();
        for (uint32 pp = 0; pp < entryPointParamCount; ++pp)
        {
            auto pVar = pEntryPoint->getParameterByIndex(pp);

            ExtendedReflectionPath path(nullptr, pVar);
            reflectVaryingParameter(path, String(pVar->getName()), category, varMap, pVarMapBySemantic);
        }
    }

    const ShaderReflection* ShaderReflection::Create(
        ShaderContent const* pProgramVersion,
        slang::ShaderReflection* pSlangReflector,
        const List<slang::EntryPointLayout*>& pSlangEntryPointReflectors
    )
    {
        ShaderReflection* shaderReflection = New<ShaderReflection>();

        // For Falcor's purposes, the global scope of a program can be treated
        // much like a user-defined `struct` type, where the fields are the
        // global shader parameters.
        //
        // Slang provides two ways to iterate over the parameters of a program:
        //
        // 1. We can directly query `getParameterCount()` and then `getParameterByIndex()`,
        //    to enumerate all the global shader parameters.
        //
        // 2. We can query `getGlobalParamsTypeLayout()` which returns a type layout
        //    that represents all of the global-scope parameters bundled together.
        //
        // Our code will mostly use option (1), but we will do a little bit of
        // option (2) to be able to get the total size of the global parameters,
        // for cases where a default constant buffer is needed for the global
        // scope.
        //
        auto pSlangGlobalParamsTypeLayout = pSlangReflector->getGlobalParamsTypeLayout();

        // The Slang type layout for the global scope either directly represents the
        // parameters as a struct type `G`, or it represents those parameters wrapped
        // up into a constant buffer like `ConstantBuffer<G>`. If we are in the latter
        // case, then we want to get the element type (`G`) from the constant buffer
        // type layout.
        //
        if (auto pElementTypeLayout = pSlangGlobalParamsTypeLayout->getElementTypeLayout())
        {
            pSlangGlobalParamsTypeLayout = pElementTypeLayout;
        }

        // Once we have the Slang type layout for the `struct` of global parameters,
        // we can easily query its size in bytes.
        //
        size_t slangGlobalParamsSize = pSlangGlobalParamsTypeLayout->getSize(SlangParameterCategory(slang::ParameterCategory::Uniform));

        ReflectionStructType* pGlobalStruct = ReflectionStructType::create(slangGlobalParamsSize, SE_TEXT(""), nullptr);
        ParameterBlockReflection* pDefaultBlock = New<ParameterBlockReflection>();
        pDefaultBlock->setElementType(pGlobalStruct);

        ReflectionStructType::BuildState buildState;
        for (uint32 i = 0; i < pSlangReflector->getParameterCount(); i++)
        {
            slang::VariableLayoutReflection* pSlangLayout = pSlangReflector->getParameterByIndex(i);

            ReflectionVar* pVar = reflectTopLevelVariable(pSlangLayout, pGlobalStruct->getResourceRangeCount(), pDefaultBlock, pProgramVersion);
            if (pVar)
            {
                pGlobalStruct->addMember(pVar, buildState);
            }
        }

        pDefaultBlock->finalize();
        shaderReflection->setDefaultParameterBlock(pDefaultBlock);


        auto entryPointGroupCount = 1;

        for (uint32 gg = 0; gg < entryPointGroupCount; ++gg)
        {
            EntryPointGroupReflection* pEntryPointGroup = EntryPointGroupReflection::create(pProgramVersion, gg, pSlangEntryPointReflectors);
            shaderReflection->mEntryPointGroups.Add(pEntryPointGroup);
        }

        // Reflect per-stage parameters
        for (auto pSlangEntryPoint : pSlangEntryPointReflectors)
        {
            switch (pSlangEntryPoint->getStage())
            {
            case SLANG_STAGE_COMPUTE:
            {
                SlangUInt sizeAlongAxis[3];
                pSlangEntryPoint->getComputeThreadGroupSize(3, &sizeAlongAxis[0]);
                shaderReflection->mThreadGroupSize.x = sizeAlongAxis[0];
                shaderReflection->mThreadGroupSize.y = sizeAlongAxis[1];
                shaderReflection->mThreadGroupSize.z = sizeAlongAxis[2];
            }
                break;
            case SLANG_STAGE_FRAGMENT:
                reflectShaderIO(pSlangEntryPoint, SLANG_PARAMETER_CATEGORY_FRAGMENT_OUTPUT, shaderReflection->mPsOut);
                break;
            case SLANG_STAGE_VERTEX:
                reflectShaderIO(pSlangEntryPoint, SLANG_PARAMETER_CATEGORY_VERTEX_INPUT, shaderReflection->mVertAttr, &shaderReflection->mVertAttrBySemantic);
                break;
            default:
                break;
            }
        }

        // Get hashed strings
        uint32 hashedStringCount = pSlangReflector->getHashedStringCount();
        shaderReflection->mHashedStrings.Resize(hashedStringCount);
        for (uint32 i = 0; i < hashedStringCount; ++i)
        {
            size_t stringSize;
            const char* stringData = pSlangReflector->getHashedString(i, &stringSize);
            uint32 stringHash = spComputeStringHash(stringData, stringSize);
            shaderReflection->mHashedStrings.Add(HashedString{stringHash, String(stringData, stringSize)});
        }

        return shaderReflection;
    }

    void ShaderReflection::finalize()
    {
        mpDefaultBlock->finalize();
    }

    static bool isVaryingParameter(slang::VariableLayoutReflection* pSlangParam)
    {
        // TODO: It is unfortunate that Falcor has to maintain this logic,
        // since there is nearly identical logic already in Slang.
        //
        // The basic problem is that we want to know whether a parameter
        // is logically "uniform" or logically "varying."
        //
        // In the common cases, we can tell by looking at the kind(s)
        // of resources the parameter consumes; if it uses any
        // kinds of resources that only make sense for varying
        // parameters, then it is varying.
        //
        unsigned int categoryCount = pSlangParam->getCategoryCount();
        for (unsigned int ii = 0; ii < categoryCount; ++ii)
        {
            switch (pSlangParam->getCategoryByIndex(ii))
            {
                // Varying cross-stage input/output obviously marks
                // a varying parameter, as do the special categories
                // of input used for ray-tracing shaders.
                //
            case slang::ParameterCategory::VaryingInput:
            case slang::ParameterCategory::VaryingOutput:
            case slang::ParameterCategory::RayPayload:
            case slang::ParameterCategory::HitAttributes:
                return true;

                // Everything else indicates a uniform parameter.
                //
            default:
                return false;
            }
        }

        // If we get to the end of the loop above, then it
        // means that there must have been *zero* categories
        // of resources consumed by the parameter.
        //
        // There are two cases where that could have happened:
        //
        // 1. A parameter of a zero-size type (an empty `struct`
        //   or a `void` parameter). In this case uniform-vs-varying
        //   is a meaningless distinction.
        //
        // 2. A varying "system value" parameter, which doesn't
        //   consume any application-bindable resources.
        //
        // Because case (1) is unimportant, we choose the default
        // behavior based on (2). If a parameter doesn't appear
        // to consume any resources, we assume it is varying.
        //
        return true;
    }

    static uint32 getUniformParameterCount(slang::EntryPointReflection* pSlangEntryPoint)
    {
        uint32 entryPointParamCount = pSlangEntryPoint->getParameterCount();
        uint32 uniformParamCount = 0;
        for (uint32 pp = 0; pp < entryPointParamCount; ++pp)
        {
            auto pVar = pSlangEntryPoint->getParameterByIndex(pp);

            if (isVaryingParameter(pVar))
                continue;

            uniformParamCount++;
        }
        return uniformParamCount;
    }

    EntryPointGroupReflection* EntryPointGroupReflection::create(
        ShaderContent const* pProgramVersion,
        uint32 groupIndex,
        const List<slang::EntryPointLayout*>& pSlangEntryPointReflectors
    )
    {
        // We are going to expect/require that all the entry points have the
        // same uniform parameters - or at least that for all the uniform
        // parameters they declare there is a match.
        //
        // We will start by finding out which of the entry points has the
        // most uniform parameters.
        //

        uint32 entryPointCount = 1;;
        ENGINE_ASSERT(entryPointCount != 0);

        slang::EntryPointLayout* pBestEntryPoint = pSlangEntryPointReflectors[0];
        for (uint32 ee = 0; ee < entryPointCount; ++ee)
        {
            slang::EntryPointReflection* pSlangEntryPoint = pSlangEntryPointReflectors[/*pProgram->getGroupEntryPointIndex(groupIndex, ee)*/0];

            if (getUniformParameterCount(pSlangEntryPoint) > getUniformParameterCount(pBestEntryPoint))
            {
                pBestEntryPoint = pSlangEntryPoint;
            }
        }

        auto pGroup = New<EntryPointGroupReflection>();

        // The layout for an entry point either represents a Slang `struct` type
        // for the entry-point parameters, or it represents a Slang `ConstantBuffer<X>`
        // where `X` is the `struct` type for the entry-point parameters.
        //
        auto pSlangEntryPointVarLayout = pBestEntryPoint->getVarLayout();
        auto pSlangEntryPointTypeLayout = pBestEntryPoint->getTypeLayout();
        ExtendedReflectionPath entryPointPath(nullptr, pSlangEntryPointVarLayout);

        // We need to detect the latter case, because it means that a "default" constant
        // buffer has been allocated for the parameters.
        //
        // Note: in recent Slang releases, we could just check if the "kind" of the
        // `pSlangEntryPointTypeLayout` is `ConstantBuffer`, but in some existing
        // releases that won't work (due to Slang bugs).
        //
        // Instead, we check whether the type layout has a "container" layout
        // associated with it, which should only happen for `ConstantBuffer<...>`
        // or `ParameterBlock<...>` types.
        //
        bool hasDefaultConstantBuffer = false;
        if (pSlangEntryPointTypeLayout->getContainerVarLayout() != nullptr)
        {
            hasDefaultConstantBuffer = true;
        }

        // In the case where theere is no default constant buffer, the variable
        // and type layouts for the entry point itself are what we want to reflect
        // as the "element type."
        //
        auto pSlangElementVarLayout = pSlangEntryPointVarLayout;
        auto pSlangElementTypeLayout = pSlangEntryPointTypeLayout;
        ReflectionPath* pElementPath = &entryPointPath;

        // If there is a default constant buffer, though, we need to drill down
        // to its element type to get the information we want.
        //
        if (hasDefaultConstantBuffer)
        {
            pSlangElementVarLayout = pSlangEntryPointTypeLayout->getElementVarLayout();
            pSlangElementTypeLayout = pSlangElementVarLayout->getTypeLayout();
        }
        ExtendedReflectionPath elementPath(&entryPointPath, pSlangElementVarLayout);
        if (hasDefaultConstantBuffer)
        {
            pElementPath = &elementPath;
        }

        ReflectionStructType::BuildState elementTypeBuildState;

        String name;
        if (entryPointCount == 1)
            name = pBestEntryPoint->getName();

        auto pElementType = ReflectionStructType::create(pSlangElementTypeLayout->getSize(), name, pSlangElementTypeLayout);
        pGroup->setElementType(pElementType);

        uint32 entryPointParamCount = pBestEntryPoint->getParameterCount();
        for (uint32 pp = 0; pp < entryPointParamCount; ++pp)
        {
            auto pSlangParam = pBestEntryPoint->getParameterByIndex(pp);

            // Note: Due to some quirks on the Slang reflection information,
            // we do not currently need to append the parameter to the
            // reflection path(s) we computed outside the loop.
            //
            // TODO: We probably need to revisit this choice if/when we
            // want to reflect all the parameters using the existing
            // logic that handles `struct` types.
            //
            ExtendedReflectionPath path(nullptr, pSlangParam);

            if (isVaryingParameter(pSlangParam))
                continue;

            auto pParam = reflectVariable(pSlangParam, pElementType->getResourceRangeCount(), pGroup, &path, pProgramVersion);

            pElementType->addMember(pParam, elementTypeBuildState);
        }

        // If the entry point had a default constant buffer allocated
        // for it, we need to extract its binding information. The
        // logic here is nearly identical to the logic for an explicit
        // constant buffer in user code. The main difference is that
        // entry-point `uniform` parameters should default to being
        // treated as a root constant buffer.
        //
        if (hasDefaultConstantBuffer)
        {
            extractDefaultConstantBufferBinding(pSlangEntryPointTypeLayout, &entryPointPath, pGroup, /*shouldUseRootConstants:*/ true);
        }

        pGroup->finalize();

        // TODO(tfoley): There is no guarantee that all the other entry
        // points in the group agree with the one we chose as the "best."
        // We should ideally iterate over the parameters of the other
        // entry points and perform a check to see if they match what
        // we extracted from the best entry point.
        //
        // TODO: alternatively, if Falcor could identify the entry point
        // groups more explicitly to Slang, we could skip the need for
        // this kind of matching/validation in the application layer.

        return pGroup;
    }

    static ShaderStage getShaderTypeFromSlangStage(SlangStage stage)
    {
        switch (stage)
        {
#define CASE(SLANG_NAME, FALCOR_NAME) \
case SLANG_STAGE_##SLANG_NAME:    \
return ShaderStage::FALCOR_NAME

            CASE(VERTEX, Vertex);
            CASE(HULL, Hull);
            CASE(DOMAIN, Domain);
            CASE(GEOMETRY, Geometry);
            CASE(PIXEL, Pixel);

            CASE(COMPUTE, Compute);

            /*CASE(RAY_GENERATION, RayGeneration);
            CASE(INTERSECTION, Intersection);
            CASE(ANY_HIT, AnyHit);
            CASE(CLOSEST_HIT, ClosestHit);
            CASE(MISS, Miss);
            CASE(CALLABLE, Callable);*/
#undef CASE

        default:
            ENGINE_UNREACHABLE_CODE();
            return ShaderStage::Max;
        }
    }

    void ShaderReflection::setDefaultParameterBlock(ParameterBlockReflection* pBlock)
    {
        mpDefaultBlock = pBlock;
    }

    int32 ReflectionStructType::addMemberIgnoringNameConflicts(
        const ReflectionVar* pVar,
        ReflectionStructType::BuildState& ioBuildState
    )
    {
        int memberIndex = mMembers.Count();
        mMembers.Add(pVar);

        auto pFieldType = pVar->getType();
        auto fieldRangeCount = pFieldType->getResourceRangeCount();
        for (uint32 rr = 0; rr < fieldRangeCount; ++rr)
        {
            auto fieldRange = pFieldType->getResourceRange(rr);

            switch (fieldRange.descriptorType)
            {
            case ShaderResourceType::Cbv:
                fieldRange.baseIndex = ioBuildState.cbCount;
                ioBuildState.cbCount += fieldRange.count;
                break;

            case ShaderResourceType::TextureSrv:
            case ShaderResourceType::RawBufferSrv:
            case ShaderResourceType::TypedBufferSrv:
            case ShaderResourceType::StructuredBufferSrv:
            case ShaderResourceType::AccelerationStructureSrv:
                fieldRange.baseIndex = ioBuildState.srvCount;
                ioBuildState.srvCount += fieldRange.count;
                break;

            case ShaderResourceType::TextureUav:
            case ShaderResourceType::RawBufferUav:
            case ShaderResourceType::TypedBufferUav:
            case ShaderResourceType::StructuredBufferUav:
                fieldRange.baseIndex = ioBuildState.uavCount;
                ioBuildState.uavCount += fieldRange.count;
                break;

            case ShaderResourceType::Sampler:
                fieldRange.baseIndex = ioBuildState.samplerCount;
                ioBuildState.samplerCount += fieldRange.count;
                break;

            case ShaderResourceType::Dsv:
            case ShaderResourceType::Rtv:
                break;

            default:
                ENGINE_UNREACHABLE_CODE();
                break;
            }

            mResourceRanges.Add(fieldRange);
        }

        return memberIndex;
    }

    int32 ReflectionStructType::addMember(const ReflectionVar* pVar, ReflectionStructType::BuildState& ioBuildState)
    {
        if (mNameToIndex.Find(pVar->getName()) != mNameToIndex.end())
        {
            int32 index = mNameToIndex[pVar->getName()];
            if (*pVar != *mMembers[index])
            {
                LOG_ERROR("Shader",
                    "Mismatch in variable declarations between different shader stages. Variable name is '{}', struct name is '{}'.",
                    pVar->getName(),
                    mName
                );
            }
            return -1;
        }
        auto memberIndex = addMemberIgnoringNameConflicts(pVar, ioBuildState);
        mNameToIndex[pVar->getName()] = memberIndex;
        return memberIndex;
    }

    ReflectionVar* ReflectionVar::create(
        const String& name,
        const ReflectionType* pType,
        const ShaderVarOffset& bindLocation
    )
    {
        return new ReflectionVar(name, pType, bindLocation);
    }

    ReflectionVar::ReflectionVar(const String& name, const ReflectionType* pType, const ShaderVarOffset& bindLocation)
        : mName(name), mpType(pType), mBindLocation(bindLocation)
    {}

    //
    void ParameterBlockReflection::setElementType(const ReflectionType* pElementType)
    {
        ENGINE_ASSERT(!mpElementType);
        mpElementType = pElementType;
    }

    ParameterBlockReflection* ParameterBlockReflection::create(ShaderContent const* pProgramVersion, const ReflectionType* pElementType)
    {
        auto pResult = New<ParameterBlockReflection>();
        pResult->setElementType(pElementType);

#if FALCOR_HAS_D3D12
        ReflectionStructType::BuildState counters;
        pResult->mBuildDescriptorSets = pProgramVersion->getProgram()->mpDevice->getType() == Device::Type::D3D12;
#endif

        auto rangeCount = pElementType->getResourceRangeCount();
        for (uint32 rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
        {
            const auto& rangeInfo = pElementType->getResourceRange(rangeIndex);

            ResourceRangeBindingInfo bindingInfo;

            uint32 regIndex = 0;
            uint32 regSpace = 0;

#if FALCOR_HAS_D3D12
            if (pResult->mBuildDescriptorSets)
            {
                switch (rangeInfo.descriptorType)
                {
                case ShaderResourceType::Cbv:
                    regIndex += counters.cbCount;
                    counters.cbCount += rangeInfo.count;
                    break;

                case ShaderResourceType::TextureSrv:
                case ShaderResourceType::RawBufferSrv:
                case ShaderResourceType::TypedBufferSrv:
                case ShaderResourceType::StructuredBufferSrv:
                case ShaderResourceType::AccelerationStructureSrv:
                    regIndex += counters.srvCount;
                    counters.srvCount += rangeInfo.count;
                    break;

                case ShaderResourceType::TextureUav:
                case ShaderResourceType::RawBufferUav:
                case ShaderResourceType::TypedBufferUav:
                case ShaderResourceType::StructuredBufferUav:
                    regIndex += counters.uavCount;
                    counters.uavCount += rangeInfo.count;
                    break;

                case ShaderResourceType::Sampler:
                    regIndex += counters.samplerCount;
                    counters.samplerCount += rangeInfo.count;
                    break;

                case ShaderResourceType::Dsv:
                case ShaderResourceType::Rtv:
                    break;

                default:
                    FALCOR_UNREACHABLE();
                    break;
                }
            }
#endif

            bindingInfo.regIndex = regIndex;
            bindingInfo.regSpace = regSpace;

            pResult->addResourceRange(bindingInfo);
        }

        pResult->finalize();
        return pResult;
    }

    ParameterBlockReflection* ParameterBlockReflection::create(ShaderContent const* pProgramVersion, slang::TypeLayoutReflection* pSlangElementType)
    {
        auto pResult = New<ParameterBlockReflection>();

        ReflectionPath path;

        auto pElementType = reflectType(pSlangElementType, pResult, &path, pProgramVersion);
        pResult->setElementType(pElementType);

        pResult->finalize();

        return pResult;
    }

    static ShaderResourceType getShaderResourceType(const ReflectionResourceType* pType)
    {
        auto shaderAccess = pType->getShaderAccess();
        switch (pType->getType())
        {
        case ReflectionResourceType::Type::ConstantBuffer:
            return ShaderResourceType::Cbv;
            break;
        case ReflectionResourceType::Type::Texture:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::TextureSrv : ShaderResourceType::TextureUav;
            break;
        case ReflectionResourceType::Type::RawBuffer:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::RawBufferSrv
                                                                              : ShaderResourceType::RawBufferUav;
            break;
        case ReflectionResourceType::Type::StructuredBuffer:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::StructuredBufferSrv
                                                                              : ShaderResourceType::StructuredBufferUav;
            break;
        case ReflectionResourceType::Type::TypedBuffer:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::TypedBufferSrv
                                                                              : ShaderResourceType::TypedBufferUav;
            break;
        case ReflectionResourceType::Type::AccelerationStructure:
            ENGINE_ASSERT(shaderAccess == ReflectionResourceType::ShaderAccess::Read);
            return ShaderResourceType::AccelerationStructureSrv;
            break;
        case ReflectionResourceType::Type::Sampler:
            return ShaderResourceType::Sampler;
            break;
        default:
            return ShaderResourceType::Count;
        }
    }

    const ReflectionVar* ParameterBlockReflection::getResource(StringView name) const
    {
        return getElementType()->findMember(name);
    }

    void ParameterBlockReflection::addResourceRange(const ResourceRangeBindingInfo& bindingInfo)
    {
        mResourceRanges.Add(bindingInfo);
    }

    bool ParameterBlockReflection::hasDefaultConstantBuffer() const
    {
        // A parameter block needs a "default" constant buffer whenever its element type requires it to store ordinary/uniform data
        return getElementType()->getByteSize() != 0;
    }

    void ParameterBlockReflection::setDefaultConstantBufferBindingInfo(const DefaultConstantBufferBindingInfo& info)
    {
        mDefaultConstantBufferBindingInfo = info;
    }

    const ParameterBlockReflection::DefaultConstantBufferBindingInfo& ParameterBlockReflection::getDefaultConstantBufferBindingInfo() const
    {
        return mDefaultConstantBufferBindingInfo;
    }

    void ParameterBlockReflection::finalize()
    {
        ENGINE_ASSERT(getElementType()->getResourceRangeCount() == mResourceRanges.Count());

        /*if (mBuildDescriptorSets)
        {
            ParameterBlockReflectionFinalizer finalizer;
            finalizer.finalize(this);
        }*/
    }

    const ParameterBlockReflection* ShaderReflection::getParameterBlock(StringView name) const
    {
        if (name == SE_TEXT(""))
            return mpDefaultBlock;

        return mpDefaultBlock->getElementType()->findMember(name)->getType()->asResourceType()->getParameterBlockReflector();
    }

    TypedShaderVarOffset ReflectionType::findMemberByOffset(int offset) const
    {
        if (auto pStructType = asStructType())
        {
            return pStructType->findMemberByOffset(offset);
        }

        return TypedShaderVarOffset::kInvalid;
    }

    TypedShaderVarOffset ReflectionStructType::findMemberByOffset(size_t offset) const
    {
        for (auto pMember : mMembers)
        {
            auto memberOffset = pMember->getBindLocation();
            auto memberUniformOffset = memberOffset.getUniform().getByteOffset();
            auto pMemberType = pMember->getType();
            auto memberByteSize = pMember->getType()->getByteSize();

            if (offset >= memberUniformOffset)
            {
                if (offset < memberUniformOffset + memberByteSize)
                {
                    return TypedShaderVarOffset(pMemberType, memberOffset);
                }
            }
        }

        return TypedShaderVarOffset::kInvalid;
    }

    const ReflectionVar* ReflectionType::findMember(StringView name) const
    {
        if (auto pStructType = asStructType())
        {
            size_t fieldIndex = pStructType->getMemberIndex(name);
            if (fieldIndex == ReflectionStructType::kInvalidMemberIndex)
                return nullptr;

            return pStructType->getMember(fieldIndex);
        }

        return nullptr;
    }

    int32 ReflectionStructType::getMemberIndex(StringView name) const
    {
        auto it = mNameToIndex.Find(name);
        if (it == mNameToIndex.end())
            return kInvalidMemberIndex;
        return it->Value;
    }

    const ReflectionVar* ReflectionStructType::getMember(StringView name) const
    {
        static const ReflectionVar* pNull;
        auto index = getMemberIndex(name);
        return (index == kInvalidMemberIndex) ? pNull : getMember(index);
    }

    const ReflectionResourceType* ReflectionType::asResourceType() const
    {
        // In the past, Falcor relied on undefined behavior checking `this` for nullptr, returning nullptr if `this` was nullptr.
        ENGINE_ASSERT(this);
        return this->getKind() == ReflectionType::Kind::Resource ? static_cast<const ReflectionResourceType*>(this) : nullptr;
    }

    const ReflectionBasicType* ReflectionType::asBasicType() const
    {
        // In the past, Falcor relied on undefined behavior checking `this` for nullptr, returning nullptr if `this` was nullptr.
        ENGINE_ASSERT(this);
        return this->getKind() == ReflectionType::Kind::Basic ? static_cast<const ReflectionBasicType*>(this) : nullptr;
    }

    const ReflectionStructType* ReflectionType::asStructType() const
    {
        // In the past, Falcor relied on undefined behavior checking `this` for nullptr, returning nullptr if `this` was nullptr.
        ENGINE_ASSERT(this);
        return this->getKind() == ReflectionType::Kind::Struct ? static_cast<const ReflectionStructType*>(this) : nullptr;
    }

    const ReflectionArrayType* ReflectionType::asArrayType() const
    {
        // In the past, Falcor relied on undefined behavior checking `this` for nullptr, returning nullptr if `this` was nullptr.
        ENGINE_ASSERT(this);
        return this->getKind() == ReflectionType::Kind::Array ? static_cast<const ReflectionArrayType*>(this) : nullptr;
    }

    const ReflectionInterfaceType* ReflectionType::asInterfaceType() const
    {
        // In the past, Falcor relied on undefined behavior checking `this` for nullptr, returning nullptr if `this` was nullptr.
        ENGINE_ASSERT(this);
        return this->getKind() == ReflectionType::Kind::Interface ? static_cast<const ReflectionInterfaceType*>(this) : nullptr;
    }

    const ReflectionType* ReflectionType::unwrapArray() const
    {
        const ReflectionType* pType = this;
        while (auto pArrayType = pType->asArrayType())
        {
            pType = pArrayType->getElementType();
        }
        return pType;
    }

    uint32 ReflectionType::getTotalArrayElementCount() const
    {
        uint32 result = 1;

        const ReflectionType* pType = this;
        while (auto pArrayType = pType->asArrayType())
        {
            result *= pArrayType->getElementCount();
            pType = pArrayType->getElementType();
        }
        return result;
    }

    ReflectionArrayType* ReflectionArrayType::create(
        uint32 arraySize,
        uint32 arrayStride,
        const ReflectionType* pType,
        int byteSize,
        slang::TypeLayoutReflection* pSlangTypeLayout
    )
    {
        return new ReflectionArrayType(arraySize, arrayStride, pType, byteSize, pSlangTypeLayout);
    }

    ReflectionArrayType::ReflectionArrayType(
        uint32 elementCount,
        uint32 elementByteStride,
        const ReflectionType* pElementType,
        int byteSize,
        slang::TypeLayoutReflection* pSlangTypeLayout
    )
        : ReflectionType(ReflectionType::Kind::Array, byteSize, pSlangTypeLayout)
        , mElementCount(elementCount)
        , mElementByteStride(elementByteStride)
        , mpElementType(pElementType)
    {
        auto rangeCount = pElementType->getResourceRangeCount();
        for (uint32 rr = 0; rr < rangeCount; ++rr)
        {
            auto range = pElementType->getResourceRange(rr);
            range.count *= elementCount;
            range.baseIndex *= elementCount;
            mResourceRanges.Add(range);
        }
    }

    ReflectionResourceType* ReflectionResourceType::create(
        Type type,
        Dimensions dims,
        StructuredType structuredType,
        ReturnType retType,
        ShaderAccess shaderAccess,
        slang::TypeLayoutReflection* pSlangTypeLayout
    )
    {
        return new ReflectionResourceType(type, dims, structuredType, retType, shaderAccess, pSlangTypeLayout);
    }

    ReflectionResourceType::ReflectionResourceType(
        Type type,
        Dimensions dims,
        StructuredType structuredType,
        ReturnType retType,
        ShaderAccess shaderAccess,
        slang::TypeLayoutReflection* pSlangTypeLayout
    )
        : ReflectionType(ReflectionType::Kind::Resource, 0, pSlangTypeLayout)
        , mDimensions(dims)
        , mStructuredType(structuredType)
        , mReturnType(retType)
        , mShaderAccess(shaderAccess)
        , mType(type)
    {
        ResourceRange range;
        range.descriptorType = getShaderResourceType(this);
        range.count = 1;
        range.baseIndex = 0;

        mResourceRanges.Add(range);
    }

    void ReflectionResourceType::setStructType(const ReflectionType* pType)
    {
        mpStructType = pType;
    }

    ReflectionBasicType* ReflectionBasicType::create(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout)
    {
        return new ReflectionBasicType(type, isRowMajor, size, pSlangTypeLayout);
    }

    ReflectionBasicType::ReflectionBasicType(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout)
        : ReflectionType(ReflectionType::Kind::Basic, size, pSlangTypeLayout), mType(type), mIsRowMajor(isRowMajor)
    {}

    ReflectionStructType* ReflectionStructType::create(int size, const String& name, slang::TypeLayoutReflection* pSlangTypeLayout)
    {
        return new ReflectionStructType(size, name, pSlangTypeLayout);
    }

    ReflectionStructType::ReflectionStructType(int size, const String& name, slang::TypeLayoutReflection* pSlangTypeLayout)
        : ReflectionType(ReflectionType::Kind::Struct, size, pSlangTypeLayout), mName(name)
    {}

    ParameterBlockReflection::BindLocation ParameterBlockReflection::getResourceBinding(StringView name) const
    {
        return getElementType()->getMemberOffset(name);
    }

    const ReflectionVar* ShaderReflection::getResource(StringView name) const
    {
        return mpDefaultBlock->getResource(name);
    }

    bool ReflectionArrayType::operator==(const ReflectionType& other) const
    {
        const ReflectionArrayType* pOther = other.asArrayType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionResourceType::operator==(const ReflectionType& other) const
    {
        const ReflectionResourceType* pOther = other.asResourceType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionStructType::operator==(const ReflectionType& other) const
    {
        const ReflectionStructType* pOther = other.asStructType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionBasicType::operator==(const ReflectionType& other) const
    {
        const ReflectionBasicType* pOther = other.asBasicType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionArrayType::operator==(const ReflectionArrayType& other) const
    {
        if (mElementCount != other.mElementCount)
            return false;
        if (mElementByteStride != other.mElementByteStride)
            return false;
        if (*mpElementType != *other.mpElementType)
            return false;
        return true;
    }

    bool ReflectionStructType::operator==(const ReflectionStructType& other) const
    {
        // We only care about the struct layout. Checking the members should be enough
        if (mMembers.Count() != other.mMembers.Count())
            return false;
        for (int i = 0; i < mMembers.Count(); i++)
        {
            // Theoretically, the order of the struct members should match
            if (*mMembers[i] != *other.mMembers[i])
                return false;
        }
        return true;
    }

    bool ReflectionBasicType::operator==(const ReflectionBasicType& other) const
    {
        if (mType != other.mType)
            return false;
        if (mIsRowMajor != other.mIsRowMajor)
            return false;
        return true;
    }

    bool ReflectionResourceType::operator==(const ReflectionResourceType& other) const
    {
        if (mDimensions != other.mDimensions)
            return false;
        if (mStructuredType != other.mStructuredType)
            return false;
        if (mReturnType != other.mReturnType)
            return false;
        if (mShaderAccess != other.mShaderAccess)
            return false;
        if (mType != other.mType)
            return false;
        bool hasStruct = (mpStructType != nullptr);
        bool otherHasStruct = (other.mpStructType != nullptr);
        if (hasStruct != otherHasStruct)
            return false;
        if (hasStruct && (*mpStructType != *other.mpStructType))
            return false;

        return true;
    }

    bool ReflectionVar::operator==(const ReflectionVar& other) const
    {
        if (*mpType != *other.mpType)
            return false;
        if (mBindLocation != other.mBindLocation)
            return false;
        if (mName != other.mName)
            return false;

        return true;
    }

    inline const ShaderReflection::ShaderVariable* getShaderAttribute(
        StringView name,
        const ShaderReflection::VariableMap& varMap,
        const String& funcName
    )
    {
        const auto& it = varMap.Find(name);
        return (it == varMap.end()) ? nullptr : &(it->Value);
    }

    const ShaderReflection::ShaderVariable* ShaderReflection::getVertexAttributeBySemantic(StringView semantic) const
    {
        return getShaderAttribute(semantic, mVertAttrBySemantic, SE_TEXT("getVertexAttributeBySemantic()"));
    }

    const ShaderReflection::ShaderVariable* ShaderReflection::getVertexAttribute(StringView name) const
    {
        return getShaderAttribute(name, mVertAttr, SE_TEXT("getVertexAttribute()"));
    }

    const ShaderReflection::ShaderVariable* ShaderReflection::getPixelShaderOutput(StringView name) const
    {
        return getShaderAttribute(name, mPsOut, SE_TEXT("getPixelShaderOutput()"));
    }

    ReflectionType* ShaderReflection::findType(StringView name) const
    {
        return nullptr;

        /*auto iter = mMapNameToType.Find(name);
        if (iter != mMapNameToType.end())
            return iter->Value;

        String nameStr{name};
        auto pSlangType = mpSlangReflector->findTypeByName(nameStr);
        if (!pSlangType)
            return nullptr;
        auto pSlangTypeLayout = mpSlangReflector->getTypeLayout(pSlangType);

        auto pFalcorTypeLayout = reflectType(pSlangTypeLayout, nullptr, nullptr, mpProgramVersion);
        if (!pFalcorTypeLayout)
            return nullptr;

        mMapNameToType.Add(std::move(nameStr), pFalcorTypeLayout);

        return pFalcorTypeLayout;*/
    }

    const ReflectionVar* ShaderReflection::findMember(StringView name) const
    {
        return mpDefaultBlock->findMember(name);
    }

    ReflectionInterfaceType* ReflectionInterfaceType::create(slang::TypeLayoutReflection* pSlangTypeLayout)
    {
        return new ReflectionInterfaceType(pSlangTypeLayout);
    }

    ReflectionInterfaceType::ReflectionInterfaceType(slang::TypeLayoutReflection* pSlangTypeLayout)
        : ReflectionType(Kind::Interface, 0, pSlangTypeLayout)
    {
        ResourceRange range;

        range.descriptorType = ShaderResourceType::Cbv;
        range.count = 1;
        range.baseIndex = 0;

        mResourceRanges.Add(range);
    }

    bool ReflectionInterfaceType::operator==(const ReflectionInterfaceType& other) const
    {
        // TODO: properly double-check this
        return true;
    }

    bool ReflectionInterfaceType::operator==(const ReflectionType& other) const
    {
        auto pOtherInterface = other.asInterfaceType();
        if (!pOtherInterface)
            return false;
        return (*this == *pOtherInterface);
    }
} // SE