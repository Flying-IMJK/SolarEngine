#include <Runtime/ShaderCompilation/Slang/SlangShaderCompiler.h>

#include <Runtime/Core/Platform/Platform.h>
#include <Runtime/ShaderCompilation/Slang/Artifact/ShaderArtifactUtil.h>

#include <slang-com-ptr.h>

namespace SE
{
    namespace
    {
        // Default-value blobs use natural scalar order, not the target's padded uniform layout.
        bool GetPackedValueSize(slang::TypeReflection* pType, slang::ProgramLayout* pLayout, uint32& size)
        {
            if (pType == nullptr)
            {
                return false;
            }
            const auto kind = pType->getKind();
            if (kind == slang::TypeReflection::Kind::Struct)
            {
                uint64 total = 0;
                for (uint32 index = 0; index < pType->getFieldCount(); index++)
                {
                    uint32 fieldSize;
                    if (!GetPackedValueSize(pType->getFieldByIndex(index)->getType(), pLayout, fieldSize))
                    {
                        return false;
                    }
                    total += fieldSize;
                }
                if (total > Max_uint32)
                {
                    return false;
                }
                size = static_cast<uint32>(total);
                return true;
            }
            if (kind == slang::TypeReflection::Kind::Array)
            {
                uint32       elementSize;
                const size_t count = pType->getElementCount();
                if (count > Max_uint32 || !GetPackedValueSize(pType->getElementType(), pLayout, elementSize) || static_cast<uint64>(elementSize) * count > Max_uint32)
                {
                    return false;
                }
                size = static_cast<uint32>(elementSize * count);
                return true;
            }
            if (kind == slang::TypeReflection::Kind::Enum)
            {
                // An unmarked enum still occupies its underlying scalar bytes in an aggregate blob.
                slang::TypeLayoutReflection* layout   = pLayout->getTypeLayout(pType);
                const size_t                 enumSize = layout ? layout->getSize(slang::ParameterCategory::Uniform) : 0;
                if (enumSize != 1 && enumSize != 2 && enumSize != 4 && enumSize != 8)
                {
                    return false;
                }
                size = static_cast<uint32>(enumSize);
                return true;
            }
            uint32 scalarSize;
            switch (pType->getScalarType())
            {
                case slang::TypeReflection::ScalarType::Int8:
                case slang::TypeReflection::ScalarType::UInt8:
                case slang::TypeReflection::ScalarType::FloatE4M3:
                case slang::TypeReflection::ScalarType::FloatE5M2:
                    scalarSize = 1;
                    break;
                case slang::TypeReflection::ScalarType::Int16:
                case slang::TypeReflection::ScalarType::UInt16:
                case slang::TypeReflection::ScalarType::Float16:
                case slang::TypeReflection::ScalarType::BFloat16:
                    scalarSize = 2;
                    break;
                case slang::TypeReflection::ScalarType::Bool:
                case slang::TypeReflection::ScalarType::Int32:
                case slang::TypeReflection::ScalarType::UInt32:
                case slang::TypeReflection::ScalarType::Float32:
                    scalarSize = 4;
                    break;
                case slang::TypeReflection::ScalarType::Int64:
                case slang::TypeReflection::ScalarType::UInt64:
                case slang::TypeReflection::ScalarType::IntPtr:
                case slang::TypeReflection::ScalarType::UIntPtr:
                case slang::TypeReflection::ScalarType::Float64:
                    scalarSize = 8;
                    break;
                default:
                    return false;
            }
            if (kind == slang::TypeReflection::Kind::Scalar)
            {
                size = scalarSize;
            }
            else if (kind == slang::TypeReflection::Kind::Vector)
            {
                size = scalarSize * static_cast<uint32>(pType->getElementCount());
            }
            else if (kind == slang::TypeReflection::Kind::Matrix)
            {
                size = scalarSize * pType->getRowCount() * pType->getColumnCount();
            }
            else
            {
                return false;
            }
            return true;
        }

        bool ReadMaterialParameter(slang::VariableReflection* pVariable, slang::ProgramLayout* pLayout, const String& path, const byte* pInheritedValue, bool inheritedValueUnavailable, List<ShaderMaterialParameter>& parameters, String& error)
        {
            if (pVariable == nullptr || pVariable->getType() == nullptr)
            {
                error = SE_TEXT("Material parameter source declaration is null.");
                return false;
            }

            slang::Attribute* marker  = nullptr;
            bool              isColor = false;
            for (uint32 index = 0; index < pVariable->getUserAttributeCount(); index++)
            {
                slang::Attribute* attribute = pVariable->getUserAttributeByIndex(index);
                const bool        color     = ShaderArtifactUtil::IsAttribute(attribute, "ShadeColorParam");
                if (color || ShaderArtifactUtil::IsAttribute(attribute, "SolarShadeParam"))
                {
                    if (marker != nullptr)
                    {
                        error = String::Format(SE_TEXT("Material parameter has multiple parameter attributes: {0}"), path);
                        return false;
                    }
                    marker  = attribute;
                    isColor = color;
                }
            }

            slang::TypeReflection* type = pVariable->getType();
            const auto             kind = type->getKind();
            if (marker != nullptr)
            {
                ShaderMaterialParameter parameter;
                parameter.ShaderName = StringID(path);
                if (path.IsEmpty() || marker->getArgumentCount() != 1 || !ShaderArtifactUtil::ReadAttributeString(marker, 0, parameter.Name))
                {
                    error = String::Format(SE_TEXT("Material parameter requires one display name: {0}"), path);
                    return false;
                }
                if (pVariable->findModifier(slang::Modifier::Static) || pVariable->findModifier(slang::Modifier::Const))
                {
                    error = String::Format(SE_TEXT("Material parameter must be a bindable variable: {0}"), path);
                    return false;
                }

                if (kind == slang::TypeReflection::Kind::Scalar)
                {
                    switch (type->getScalarType())
                    {
                        case slang::TypeReflection::ScalarType::Bool:
                            parameter.Type = MaterialParameterType::Bool;
                            break;
                        case slang::TypeReflection::ScalarType::Int32:
                            parameter.Type = MaterialParameterType::Integer;
                            break;
                        case slang::TypeReflection::ScalarType::Float32:
                            parameter.Type = MaterialParameterType::Float;
                            break;
                        default:
                            break;
                    }
                }
                else if (kind == slang::TypeReflection::Kind::Vector && type->getScalarType() == slang::TypeReflection::ScalarType::Float32)
                {
                    switch (type->getElementCount())
                    {
                        case 2:
                            parameter.Type = MaterialParameterType::Vector2;
                            break;
                        case 3:
                            parameter.Type = MaterialParameterType::Vector3;
                            break;
                        case 4:
                            parameter.Type = MaterialParameterType::Vector4;
                            break;
                        default:
                            break;
                    }
                }
                else if (kind == slang::TypeReflection::Kind::Matrix && type->getScalarType() == slang::TypeReflection::ScalarType::Float32 && type->getRowCount() == 4 && type->getColumnCount() == 4)
                {
                    parameter.Type = MaterialParameterType::Matrix;
                }
                else if (kind == slang::TypeReflection::Kind::Resource && type->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ)
                {
                    // Exact shapes exclude arrays, multisampled textures and writable resources.
                    if (type->getResourceShape() == SLANG_TEXTURE_2D)
                    {
                        parameter.Type = MaterialParameterType::Texture;
                    }
                    else if (type->getResourceShape() == SLANG_TEXTURE_CUBE)
                    {
                        parameter.Type = MaterialParameterType::CubeTexture;
                    }
                }
                if (isColor)
                {
                    if (parameter.Type != MaterialParameterType::Vector4)
                    {
                        error = String::Format(SE_TEXT("ShadeColorParam requires float4: {0}"), path);
                        return false;
                    }
                    parameter.Type = MaterialParameterType::Color;
                }
                if (parameter.Type == MaterialParameterType::Invalid)
                {
                    error = String::Format(SE_TEXT("Unsupported public material parameter type: {0}"), path);
                    return false;
                }

                const uint32 size = parameter.GetValueSize();
                if (size != 0)
                {
                    Slang::ComPtr<slang::IBlob> initialValue;
                    if (inheritedValueUnavailable || (pInheritedValue == nullptr && !SLANG_SUCCEEDED(pVariable->getDefaultValueBlob(initialValue.writeRef()))))
                    {
                        error = String::Format(SE_TEXT("Cannot reflect material parameter initializer: {0}"), path);
                        return false;
                    }
                    parameter.DefaultValue.Allocate(size);
                    if (pInheritedValue != nullptr)
                    {
                        Platform::MemoryCopy(parameter.DefaultValue.Get(), pInheritedValue, size);
                    }
                    else if (initialValue)
                    {
                        if (initialValue->getBufferSize() != size)
                        {
                            error = String::Format(SE_TEXT("Material parameter initializer size does not match its type: {0}"), path);
                            return false;
                        }
                        Platform::MemoryCopy(parameter.DefaultValue.Get(), initialValue->getBufferPointer(), size);
                    }
                    else
                    {
                        Platform::MemoryClear(parameter.DefaultValue.Get(), size);
                    }
                }

                // One Shader exposes one public collection, including declarations unused by some variants.
                for (const ShaderMaterialParameter& existing : parameters)
                {
                    if (existing.ShaderName.ToString() == path)
                    {
                        if (existing.Name != parameter.Name || existing.Type != parameter.Type)
                        {
                            error = String::Format(SE_TEXT("Material parameter declaration differs between programs, variants or targets: {0}"), path);
                            return false;
                        }
                        return true;
                    }
                }
                parameters.Add(MoveTemp(parameter));
                return true;
            }

            if (kind == slang::TypeReflection::Kind::ConstantBuffer || kind == slang::TypeReflection::Kind::ParameterBlock)
            {
                type = type->getElementType();
            }

            if (type != nullptr && type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                Slang::ComPtr<slang::IBlob> aggregateValue;
                const byte*                 aggregateData = pInheritedValue;
                if (aggregateData == nullptr && !inheritedValueUnavailable && kind == slang::TypeReflection::Kind::Struct)
                {
                    inheritedValueUnavailable = !SLANG_SUCCEEDED(pVariable->getDefaultValueBlob(aggregateValue.writeRef()));
                    if (aggregateValue)
                    {
                        uint32 packedSize;
                        if (!GetPackedValueSize(type, pLayout, packedSize) || aggregateValue->getBufferSize() != packedSize)
                        {
                            inheritedValueUnavailable = true;
                        }
                        else
                        {
                            aggregateData = static_cast<const byte*>(aggregateValue->getBufferPointer());
                        }
                    }
                }
                uint32 offset = 0;
                for (uint32 index = 0; index < type->getFieldCount(); index++)
                {
                    slang::VariableReflection* field = type->getFieldByIndex(index);
                    const String           fieldName(field->getName());
                    const String           fieldPath = path.IsEmpty() ? fieldName : path + SE_TEXT(".") + fieldName;
                    uint32                 fieldSize = 0;
                    if (aggregateData != nullptr && !GetPackedValueSize(field->getType(), pLayout, fieldSize))
                    {
                        error = String::Format(SE_TEXT("Cannot locate material parameter in aggregate initializer: {0}"), fieldPath);
                        return false;
                    }
                    if (!ReadMaterialParameter(field, pLayout, fieldPath, aggregateData ? aggregateData + offset : nullptr, inheritedValueUnavailable, parameters, error))
                    {
                        return false;
                    }
                    offset += fieldSize;
                }
            }
            return true;
        }
    } // namespace

    bool SlangShaderCompiler::MaterialParameters(slang::ProgramLayout* pLayout, List<ShaderMaterialParameter>& parameters, String& error)
    {
        for (uint32 index = 0; index < pLayout->getParameterCount(); index++)
        {
            slang::VariableReflection* variable = pLayout->getParameterByIndex(index)->getVariable();
            if (!ReadMaterialParameter(variable, pLayout, String(variable->getName()), nullptr, false, parameters, error))
            {
                return false;
            }
        }
        return true;
    }
} // namespace SE
