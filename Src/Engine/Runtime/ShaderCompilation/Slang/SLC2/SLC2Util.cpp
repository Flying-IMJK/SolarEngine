#include "SLC2Util.h"
#include <Runtime/Core/Types/Strings/String.h>

namespace SE
{
    bool SLC2Util::ReadAttributeInt(slang::Attribute* attribute, const uint32 argumentIndex, int32& value)
    {
        int slangValue = 0;
        if (!SLANG_SUCCEEDED(attribute->getArgumentValueInt(argumentIndex, &slangValue)))
        {
            return false;
        }
        value = static_cast<int32>(slangValue);
        return true;
    }

    bool SLC2Util::ReadAttributeString(slang::Attribute* attribute, const uint32 argumentIndex, String& value)
    {
        size_t      length = 0;
        const char* text   = attribute->getArgumentValueString(argumentIndex, &length);
        if (text == nullptr)
        {
            return false;
        }
        value = String(text, length);
        return true;
    }

    bool SLC2Util::IsAttribute(slang::Attribute* attribute, const char* name)
    {
        if (attribute == nullptr)
        {
            return false;
        }
        const char* attributeName = attribute->getName();
        if (attributeName == nullptr)
        {
            return false;
        }
        const StringAnsiView actual   = StringAnsiView(attributeName);
        const StringAnsiView expected = StringAnsiView(name);
        const StringAnsi     expected2 = StringAnsi(name) + StringAnsi(SE_TEXT("Attribute"));
        return actual == expected || actual == expected2;
    }

    bool SLC2Util::HasAttribute(slang::TypeReflection* type, const char* name)
    {
        if (type == nullptr)
        {
            return false;
        }
        for (unsigned int i = 0; i < type->getUserAttributeCount(); i++)
        {
            if (IsAttribute(type->getUserAttributeByIndex(i), name))
            {
                return true;
            }
        }
        return false;
    }

    SlangStage SLC2Util::ToSlangStage(const ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:
                return SLANG_STAGE_VERTEX;
            case ShaderStage::Hull:
                return SLANG_STAGE_HULL;
            case ShaderStage::Domain:
                return SLANG_STAGE_DOMAIN;
            case ShaderStage::Geometry:
                return SLANG_STAGE_GEOMETRY;
            case ShaderStage::Pixel:
                return SLANG_STAGE_FRAGMENT;
            case ShaderStage::Compute:
                return SLANG_STAGE_COMPUTE;
            default:
                return SLANG_STAGE_NONE;
        }
    }
}