#include "Utils.h"

#include <fstream>

#include "FileSystem.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    //-------------------------------------------------------------------------

    char const* GetMarkMacroText( ReflectionMacroType macro )
    {
        switch ( macro )
        {
        case ReflectionMacroType::ReflectMeta:
            return "SE_META";
        case ReflectionMacroType::SEClass:
            return "SE_CLASS";
        case ReflectionMacroType::SEStruct:
            return "SE_STRUCT";
        case ReflectionMacroType::SEInterface:
            return "SE_INTERFACE";
        case ReflectionMacroType::SEEnum:
            return "SE_ENUM";
        case ReflectionMacroType::SEProperty:
            return "SE_PROPERTY";
        case ReflectionMacroType::SEFunction:
            return "SE_FUNCTION";
        case ReflectionMacroType::SEEvent:
            return "SE_EVENT";
        case ReflectionMacroType::SETypeDef:
            return "SE_TYPEDEF";
        case ReflectionMacroType::APIInjectCode:
            return "API_INJECT_CODE";
        case ReflectionMacroType::NumMacros:
            break;
        }
        return "";
    }

    int32 Utils::String::Length(const char* text)
    {
        return text ? (int32)std::char_traits<char>::length(text) : 0;
    }

    int32 Utils::String::Length(std::string_view text)
    {
        return (int32)text.length();
    }

    const char* Utils::String::CStr(const char* text)
    {
        return text ? text : "";
    }

    const char* Utils::String::CStr(const std::string& text)
    {
        return text.c_str();
    }

    std::string Utils::String::ToString(std::string_view text)
    {
        return std::string(text);
    }

    std::string Utils::String::ToLowerCopy(std::string_view text)
    {
        std::string result(text);
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return (char)std::tolower(c);
        });
        return result;
    }

    bool Utils::String::StartsWith(std::string_view text, std::string_view prefix)
    {
        return prefix.size() <= text.size() && text.compare(0, prefix.size(), prefix) == 0;
    }

    bool Utils::String::EndsWith(std::string_view text, std::string_view suffix)
    {
        return suffix.size() <= text.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool Utils::String::EndsWith(std::string_view text, char suffix)
    {
        return !text.empty() && text.back() == suffix;
    }

    int32 Utils::String::Find(std::string_view text, std::string_view needle, int32 start)
    {
        const auto found = text.find(needle, (size_t)std::max(start, 0));
        return found == std::string_view::npos ? INVALID_INDEX : (int32)found;
    }

    int32 Utils::String::Find(std::string_view text, char needle, int32 start)
    {
        const auto found = text.find(needle, (size_t)std::max(start, 0));
        return found == std::string_view::npos ? INVALID_INDEX : (int32)found;
    }

    int32 Utils::String::FindLast(std::string_view text, char needle)
    {
        const auto found = text.find_last_of(needle);
        return found == std::string_view::npos ? INVALID_INDEX : (int32)found;
    }

    bool Utils::String::Contains(std::string_view text, std::string_view needle)
    {
        return Find(text, needle) != INVALID_INDEX;
    }

    void Utils::String::ReplaceAll(std::string& text, std::string_view from, std::string_view to)
    {
        if (from.empty())
            return;

        size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos)
        {
            text.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    void Utils::String::TrimStart(std::string& text)
    {
        text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    void Utils::String::TrimEnd(std::string& text)
    {
        text.erase(std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), text.end());
    }

    void Utils::String::Split(std::string_view text, std::string_view delimiter, std::vector<std::string>& out)
    {
        out.clear();
        if (delimiter.empty())
        {
            out.push_back(std::string(text));
            return;
        }

        size_t start = 0;
        size_t pos = 0;
        while ((pos = text.find(delimiter, start)) != std::string_view::npos)
        {
            out.push_back(std::string(text.substr(start, pos - start)));
            start = pos + delimiter.length();
        }
        out.push_back(std::string(text.substr(start)));
    }

    void Utils::String::Split(std::string_view text, char delimiter, std::vector<std::string>& out)
    {
        Split(text, std::string_view(&delimiter, 1), out);
    }

    bool Utils::String::TryReadFormatArgumentIndex(std::string_view fmt, size_t openBraceIndex, size_t& closeBraceIndex, size_t& argumentIndex)
    {
        size_t cursor = openBraceIndex + 1;
        if (cursor >= fmt.size() || !std::isdigit(static_cast<unsigned char>(fmt[cursor])))
        {
            return false;
        }

        size_t value = 0;
        do
        {
            value = (value * 10) + static_cast<size_t>(fmt[cursor] - '0');
            ++cursor;
        } while (cursor < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[cursor])));

        if (cursor < fmt.size() && fmt[cursor] == '}')
        {
            closeBraceIndex = cursor;
            argumentIndex = value;
            return true;
        }

        return false;
    }

    void Utils::String::AppendEscapedFormatLiteral(std::string& result, std::string_view text, size_t start, size_t length)
    {
        for (size_t i = start; i < start + length; ++i)
        {
            if (text[i] == '{')
            {
                result += "{{";
            }
            else if (text[i] == '}')
            {
                result += "}}";
            }
            else
            {
                result += text[i];
            }
        }
    }

    bool Utils::IsFileUnderToolsProject(std::string const& filePath)
    {
        if (String::Find(filePath, "\\EngineTools\\") != INVALID_INDEX)
        {
            return true;
        }

        if (String::Find(filePath, "\\GameTools\\") != INVALID_INDEX)
        {
            return true;
        }

        return false;
    }

    std::vector<std::string> Utils::SplitString(const char* nameSpaces, const char* combine)
    {
        std::string temp(nameSpaces);
        std::vector<std::string> result;
        Utils::String::Split(temp, combine, result);
        return result;
    }

    std::string Utils::CombineStringList(const std::vector<std::string>& nameSpaceList, const char* combine)
    {
        std::string combineNameSpace;
        for (int i = 0; i < nameSpaceList.size(); i++)
        {
            const std::string& stage = nameSpaceList[i];
            combineNameSpace.append(stage);

            if (i != nameSpaceList.size() - 1)
            {
                combineNameSpace.append(combine);
            }
        }

        return combineNameSpace;
    }

    bool Utils::ReadAllText(const std::string& path, std::string& outText)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        outText = data;
        return true;
    }

    bool Utils::WriteAllText(const std::string& path, const std::string& text)
    {
        std::string parent = FileSystem::GetParentDirectory(path);
        if (!parent.empty())
            FileSystem::CreateDirectory(parent);
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file)
            return false;
        file << text;
        return true;
    }




    TypeID coreTypeID[] = {
        TypeID("bool"),
        TypeID("uint8"),
        TypeID("int8"),
        TypeID("uint16"),
        TypeID("int16"),
        TypeID("uint32"),
        TypeID("int32"),
        TypeID("uint64"),
        TypeID("int64"),
        TypeID("float"),
        TypeID("double"),
        TypeID("SE::UID"),
        TypeID("SE::StringID"),
        TypeID("SE::TypeID"),
        TypeID("SE::String"),
        TypeID("SE::Color32"),
        TypeID("SE::Float2"),
        TypeID("SE::Float3"),
        TypeID("SE::Float4"),
        TypeID("SE::Int2"),
        TypeID("SE::Int3"),
        TypeID("SE::Int4"),
        TypeID("SE::Double2"),
        TypeID("SE::Double3"),
        TypeID("SE::Double4"),
        TypeID("SE::Quaternion"),
        TypeID("SE::Transform"),
        TypeID("SE::Matrix"),
        TypeID("SE::Microseconds"),
        TypeID("SE::Milliseconds"),
        TypeID("SE::Seconds"),
        TypeID("SE::FloatRange"),
        TypeID("SE::FloatCurve"),
    };


    TypeID Utils::GetCoreTypeID(TypeIDCore type)
    {
        return coreTypeID[(int)type];
    }

    bool Utils::IsCoreType(TypeID type)
    {
        for (int i = 0; i < (int)TypeIDCore::NumTypes; i++)
        {
            if (type == coreTypeID[i])
            {
                return true;
            }
        }
        return false;
    }
}
