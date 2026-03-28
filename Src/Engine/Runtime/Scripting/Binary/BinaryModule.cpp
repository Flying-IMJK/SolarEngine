// BinaryModule.cpp — Base script assembly container.
// Requirements: 4.3

#include "BinaryModule.h"
#include "Core/Logging/Logging.h"

namespace SE
{
    int32 BinaryModule::RegisterType(ScriptingType& type)
    {
        if (type.Fullname.IsEmpty())
        {
            LOG_ERROR("Scripting", "BinaryModule::RegisterType — type has empty fullname.");
            return -1;
        }

        const std::string key(type.Fullname.Get());

        // Prevent duplicate registration.
        if (TypeNameToTypeIndex.count(key))
        {
            LOG_WARNING("Scripting", "BinaryModule::RegisterType — type '{}' is already registered.", key.c_str());
            return TypeNameToTypeIndex[key];
        }

        const int32 index = static_cast<int32>(Types.size());
        type.Module    = this;
        type.TypeIndex = index;

        Types.push_back(type);
        TypeNameToTypeIndex[key] = index;

        return index;
    }

    bool BinaryModule::FindScriptingType(const char* fullname, int32& outIndex) const
    {
        if (!fullname)
            return false;

        auto it = TypeNameToTypeIndex.find(fullname);
        if (it == TypeNameToTypeIndex.end())
            return false;

        outIndex = it->second;
        return true;
    }

    ScriptingTypeHandle BinaryModule::FindScriptingType(const char* fullname) const
    {
        int32 index = -1;
        if (!FindScriptingType(fullname, index))
            return ScriptingTypeHandle{};

        return ScriptingTypeHandle{ const_cast<BinaryModule*>(this), index };
    }

    ScriptingTypeHandle BinaryModule::GetTypeHandle(int32 index)
    {
        if (index < 0 || index >= static_cast<int32>(Types.size()))
            return ScriptingTypeHandle{};

        return ScriptingTypeHandle{ this, index };
    }

} // namespace SE
