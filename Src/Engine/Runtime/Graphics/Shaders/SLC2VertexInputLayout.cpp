#include "SLC2VertexInputLayout.h"

namespace SE
{
    namespace
    {
        enum class ValueCategory : byte
        {
            Invalid,
            Float,
            UNorm,
            SNorm,
            UInt,
            SInt,
        };

        struct ValueInfo
        {
            ValueCategory Category = ValueCategory::Invalid;
            int32 Components = 0;
        };

        ValueInfo GetShaderInfo(const SLC2VertexInputType type)
        {
            switch (type)
            {
            case SLC2VertexInputType::Float:  return {ValueCategory::Float, 1};
            case SLC2VertexInputType::Float2: return {ValueCategory::Float, 2};
            case SLC2VertexInputType::Float3: return {ValueCategory::Float, 3};
            case SLC2VertexInputType::Float4: return {ValueCategory::Float, 4};
            case SLC2VertexInputType::Int:    return {ValueCategory::SInt, 1};
            case SLC2VertexInputType::Int2:   return {ValueCategory::SInt, 2};
            case SLC2VertexInputType::Int3:   return {ValueCategory::SInt, 3};
            case SLC2VertexInputType::Int4:   return {ValueCategory::SInt, 4};
            case SLC2VertexInputType::UInt:   return {ValueCategory::UInt, 1};
            case SLC2VertexInputType::UInt2:  return {ValueCategory::UInt, 2};
            case SLC2VertexInputType::UInt3:  return {ValueCategory::UInt, 3};
            case SLC2VertexInputType::UInt4:  return {ValueCategory::UInt, 4};
            default:                          return {};
            }
        }

        ValueInfo GetPixelInfo(const PixelFormat format)
        {
            ValueCategory category = ValueCategory::Invalid;
            switch (format)
            {
            case PixelFormat::R32G32B32A32_Float:
            case PixelFormat::R32G32B32_Float:
            case PixelFormat::R32G32_Float:
            case PixelFormat::R16G16B16A16_Float:
            case PixelFormat::R16G16_Float:
            case PixelFormat::R11G11B10_Float:
            case PixelFormat::R32_Float:
            case PixelFormat::R16_Float:
                category = ValueCategory::Float;
                break;
            case PixelFormat::R32G32B32A32_UInt:
            case PixelFormat::R32G32B32_UInt:
            case PixelFormat::R32G32_UInt:
            case PixelFormat::R16G16B16A16_UInt:
            case PixelFormat::R16G16_UInt:
            case PixelFormat::R10G10B10A2_UInt:
            case PixelFormat::R8G8B8A8_UInt:
            case PixelFormat::R8G8_UInt:
            case PixelFormat::R32_UInt:
            case PixelFormat::R16_UInt:
            case PixelFormat::R8_UInt:
                category = ValueCategory::UInt;
                break;
            case PixelFormat::R32G32B32A32_SInt:
            case PixelFormat::R32G32B32_SInt:
            case PixelFormat::R32G32_SInt:
            case PixelFormat::R16G16B16A16_SInt:
            case PixelFormat::R16G16_SInt:
            case PixelFormat::R8G8B8A8_SInt:
            case PixelFormat::R8G8_SInt:
            case PixelFormat::R32_SInt:
            case PixelFormat::R16_SInt:
            case PixelFormat::R8_SInt:
                category = ValueCategory::SInt;
                break;
            case PixelFormat::R16G16B16A16_UNorm:
            case PixelFormat::R10G10B10A2_UNorm:
            case PixelFormat::R8G8B8A8_UNorm:
            case PixelFormat::R16G16_UNorm:
            case PixelFormat::R8G8_UNorm:
            case PixelFormat::R16_UNorm:
            case PixelFormat::R8_UNorm:
                category = ValueCategory::UNorm;
                break;
            case PixelFormat::R16G16B16A16_SNorm:
            case PixelFormat::R8G8B8A8_SNorm:
            case PixelFormat::R16G16_SNorm:
            case PixelFormat::R8G8_SNorm:
            case PixelFormat::R16_SNorm:
            case PixelFormat::R8_SNorm:
                category = ValueCategory::SNorm;
                break;
            default:
                return {};
            }
            return {category, PixelFormatComputeComponentsCount(format)};
        }
    }

    bool SLC2VertexInputLayoutMatcher::IsCompatible(
        const SLC2VertexInputType shaderType,
        const PixelFormat format)
    {
        const ValueInfo shader = GetShaderInfo(shaderType);
        const ValueInfo physical = GetPixelInfo(format);
        if (shader.Category == ValueCategory::Invalid || physical.Category == ValueCategory::Invalid ||
            physical.Components < shader.Components)
        {
            return false;
        }
        if (shader.Category == ValueCategory::Float)
        {
            return physical.Category == ValueCategory::Float ||
                physical.Category == ValueCategory::UNorm ||
                physical.Category == ValueCategory::SNorm;
        }
        return shader.Category == physical.Category;
    }

    bool SLC2VertexInputLayoutMatcher::Match(
        const List<SLC2VertexInputSignatureElement>& signature,
        const VertexFactoryLayout& layout,
        SLC2VertexInputMatchResult& output,
        String& error)
    {
        if (!layout.IsValid())
        {
            error = SE_TEXT("VertexFactory layout is invalid.");
            return false;
        }

        SLC2VertexInputMatchResult candidate;
        candidate.Elements.EnsureCapacity(signature.Count());
        for (int32 index = 0; index < signature.Count(); index++)
        {
            const SLC2VertexInputSignatureElement& input = signature[index];
            const VertexFactoryInputElement* element = layout.FindElement(input.Semantic, input.SemanticIndex);
            if (element == nullptr)
            {
                error = String::Format(SE_TEXT("Missing VertexFactory element {0}{1}."), input.Semantic, input.SemanticIndex);
                return false;
            }
            if (!IsCompatible(input.ShaderType, element->Format))
            {
                error = String::Format(SE_TEXT("VertexFactory element {0}{1} has incompatible format {2}."),
                    input.Semantic, input.SemanticIndex, PixelFormatGetString(element->Format));
                return false;
            }
            candidate.Elements.Add(element);
            candidate.RequiredVertexBufferSlotsMask |= (1u << element->Slot);
        }

        output = MoveTemp(candidate);
        return true;
    }
}
