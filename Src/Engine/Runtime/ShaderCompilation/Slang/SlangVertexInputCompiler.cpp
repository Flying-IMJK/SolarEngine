#include "SlangVertexInputCompiler.h"

namespace SE
{
    namespace
    {
        bool BuildShaderType(slang::TypeLayoutReflection* typeLayout, SLC2VertexInputType& shaderType)
        {
            if (typeLayout == nullptr)
            {
                return false;
            }

            const slang::TypeReflection::Kind kind = typeLayout->getKind();
            const slang::TypeReflection::ScalarType scalar = typeLayout->getScalarType();
            uint32 componentCount = 1;
            if (kind == slang::TypeReflection::Kind::Vector)
            {
                componentCount = typeLayout->getColumnCount();
            }
            else if (kind != slang::TypeReflection::Kind::Scalar)
            {
                return false;
            }
            if (componentCount < 1 || componentCount > 4)
            {
                return false;
            }

            const bool isFloat = scalar == slang::TypeReflection::ScalarType::Float32;
            const bool isInt = scalar == slang::TypeReflection::ScalarType::Int32;
            const bool isUInt = scalar == slang::TypeReflection::ScalarType::UInt32;
            if (!isFloat && !isInt && !isUInt)
            {
                return false;
            }
            if (isFloat)
            {
                shaderType = static_cast<SLC2VertexInputType>(static_cast<byte>(SLC2VertexInputType::Float) + componentCount - 1);
            }
            else if (isInt)
            {
                shaderType = static_cast<SLC2VertexInputType>(static_cast<byte>(SLC2VertexInputType::Int) + componentCount - 1);
            }
            else
            {
                shaderType = static_cast<SLC2VertexInputType>(static_cast<byte>(SLC2VertexInputType::UInt) + componentCount - 1);
            }
            return true;
        }

        bool IsDuplicate(const List<SLC2VertexInputSignatureElement>& signature,
                         const SLC2VertexInputSignatureElement& element)
        {
            for (int32 index = 0; index < signature.Count(); index++)
            {
                const SLC2VertexInputSignatureElement& existing = signature[index];
                if ((existing.Semantic == element.Semantic && existing.SemanticIndex == element.SemanticIndex) ||
                    existing.Location == element.Location)
                {
                    return true;
                }
            }
            return false;
        }
    }

    bool SLC2VertexInputCompiler::AppendVertexInput(
        slang::VariableLayoutReflection* variable,
        List<SLC2VertexInputSignatureElement>& signature,
        String& error)
    {
        if (variable == nullptr || variable->getTypeLayout() == nullptr)
        {
            error = SE_TEXT("Vertex input reflection is missing a variable layout.");
            return false;
        }

        const char* semanticName = variable->getSemanticName();
        if (semanticName != nullptr && semanticName[0] != '\0')
        {
            String semantic = String(semanticName);
            semantic.ToUpper();
            // SV_* 是系统值输入，由固定功能提供，不需要 VertexFactory 数据。
            if (semantic.StartsWith(SE_TEXT("SV_")))
            {
                return true;
            }

            SLC2VertexInputSignatureElement element;
            element.Semantic = semantic;
            const size_t semanticIndex = variable->getSemanticIndex();
            const size_t location = variable->getOffset(slang::ParameterCategory::VaryingInput);
            if (semanticIndex > static_cast<size_t>(0xffffffffu) || location > static_cast<size_t>(0xffffffffu))
            {
                error = SE_TEXT("Vertex input semantic index or location exceeds the SLC2 range.");
                return false;
            }
            element.SemanticIndex = static_cast<uint32>(semanticIndex);
            element.Location = static_cast<uint32>(location);
            if (!BuildShaderType(variable->getTypeLayout(), element.ShaderType))
            {
                error = SE_TEXT("Vertex input uses an unsupported scalar or vector ShaderType.");
                return false;
            }
            if (IsDuplicate(signature, element))
            {
                error = SE_TEXT("Vertex input signature contains a duplicated semantic or location.");
                return false;
            }
            signature.Add(element);
            return true;
        }

        slang::TypeLayoutReflection* typeLayout = variable->getTypeLayout();
        if (typeLayout->getKind() != slang::TypeReflection::Kind::Struct)
        {
            error = SE_TEXT("Vertex input parameter has no semantic.");
            return false;
        }
        for (uint32 fieldIndex = 0; fieldIndex < typeLayout->getFieldCount(); fieldIndex++)
        {
            if (!AppendVertexInput(typeLayout->getFieldByIndex(fieldIndex), signature, error))
            {
                return false;
            }
        }
        return true;
    }

    bool SLC2VertexInputCompiler::ReadVertexInputSignature(
        slang::ProgramLayout* layout,
        const int32 entryPointIndex,
        List<SLC2VertexInputSignatureElement>& signature,
        String& error)
    {
        signature.Clear();
        if (layout == nullptr)
        {
            error = SE_TEXT("Vertex input signature requires Slang reflection.");
            return false;
        }
        slang::EntryPointReflection* entryPoint = layout->getEntryPointByIndex(entryPointIndex);
        if (entryPoint == nullptr)
        {
            error = SE_TEXT("Vertex entry point reflection is missing.");
            return false;
        }
        for (uint32 parameterIndex = 0; parameterIndex < entryPoint->getParameterCount(); parameterIndex++)
        {
            if (!AppendVertexInput(entryPoint->getParameterByIndex(parameterIndex), signature, error))
            {
                return false;
            }
        }
        return ValidateVertexInputSignature(signature, error);
    }

    bool SLC2VertexInputCompiler::ValidateVertexInputSignature(
        const List<SLC2VertexInputSignatureElement>& signature, String& error)
    {
        for (int32 index = 0; index < signature.Count(); index++)
        {
            const SLC2VertexInputSignatureElement& element = signature[index];
            if (element.Semantic.IsEmpty() || element.Semantic.StartsWith(SE_TEXT("SV_")))
            {
                error = SE_TEXT("Vertex input signature contains an invalid system or empty semantic.");
                return false;
            }
            if (static_cast<byte>(element.ShaderType) > static_cast<byte>(SLC2VertexInputType::UInt4))
            {
                error = SE_TEXT("Vertex input signature contains an unknown ShaderType.");
                return false;
            }
            for (int32 other = index + 1; other < signature.Count(); other++)
            {
                if (signature[other].Semantic == element.Semantic &&
                    signature[other].SemanticIndex == element.SemanticIndex)
                {
                    error = SE_TEXT("Vertex input signature contains a duplicated semantic.");
                    return false;
                }
                if (signature[other].Location == element.Location)
                {
                    error = SE_TEXT("Vertex input signature contains a duplicated location.");
                    return false;
                }
            }
        }
        return true;
    }

    bool SLC2VertexInputCompiler::SameVertexInputSignature(
        const List<SLC2VertexInputSignatureElement>& a,
        const List<SLC2VertexInputSignatureElement>& b,
        const bool includeLocation)
    {
        if (a.Count() != b.Count())
        {
            return false;
        }
        for (int32 index = 0; index < a.Count(); index++)
        {
            const SLC2VertexInputSignatureElement& lhs = a[index];
            const SLC2VertexInputSignatureElement* rhs = nullptr;
            for (int32 otherIndex = 0; otherIndex < b.Count(); otherIndex++)
            {
                if (b[otherIndex].Semantic == lhs.Semantic && b[otherIndex].SemanticIndex == lhs.SemanticIndex)
                {
                    rhs = &b[otherIndex];
                    break;
                }
            }
            if (rhs == nullptr || lhs.ShaderType != rhs->ShaderType ||
                (includeLocation && lhs.Location != rhs->Location))
            {
                return false;
            }
        }
        return true;
    }
}
