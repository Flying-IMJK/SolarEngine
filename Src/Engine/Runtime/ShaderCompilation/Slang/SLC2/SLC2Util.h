#pragma once
#include <slang.h>
#include <Runtime/Core/Types/Variable.h>
#include <Runtime/Graphics/Base/GPUEnums.h>

namespace SE
{
    class SLC2Util
    {
    public:
        static bool ReadAttributeInt(slang::Attribute* attribute, const uint32 argumentIndex, int32& value);

        static bool ReadAttributeString(slang::Attribute* attribute, const uint32 argumentIndex, String& value);

        static bool IsAttribute(slang::Attribute* attribute, const char* name);

        static bool HasAttribute(slang::TypeReflection* type, const char* name);

        static SlangStage ToSlangStage(const ShaderStage stage);
    };
}