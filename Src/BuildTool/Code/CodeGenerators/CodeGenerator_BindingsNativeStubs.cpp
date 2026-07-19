#include "CodeGenerator_BindingsNativeStubs.h"

#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Core/FileSystem.h"
#include "Core/Utils.h"

#include <string>

#include "CodeGenerator_Utils.h"

namespace SE::BuildTool
{
    struct NativeTypeStub
    {
        std::string fullName;
        bool       isClass = false;
        bool       isEnum = false;
        std::vector<std::string> enumMembers;
    };

    static bool IsBuiltinCSharpType(const std::string& type)
    {
        return type.empty()
            || type == "void"
            || type == "bool"
            || type == "byte"
            || type == "sbyte"
            || type == "short"
            || type == "ushort"
            || type == "int"
            || type == "uint"
            || type == "long"
            || type == "ulong"
            || type == "float"
            || type == "double"
            || type == "char"
            || type == "string"
            || type == "object"
            || type == "IntPtr"
            || type == "System.IntPtr"
            || type == "System.Guid"
            || type == "System.Type"
            || Utils::String::StartsWith(type, "System.");
    }

    static std::string NormalizeStubTypeName(const std::string& rawType)
    {
        std::string type = rawType;
        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);

        while (type.length() >= 2)
        {
            const char* data = type.c_str();
            int len = (int)type.length();
            if (data[len - 2] == '[' && data[len - 1] == ']')
                type = type.substr(0, len - 2);
            else
                break;
        }

        if (!type.empty() && !IsBuiltinCSharpType(type) && Utils::String::Find(type, ".") == INVALID_INDEX)
            type = std::string("SE.") + type;
        return type;
    }

    static int FindLastDot(const std::string& fullName)
    {
        int pos = INVALID_INDEX;
        int searchStart = 0;
        while (true)
        {
            std::string tail = fullName.substr(searchStart);
            int found = Utils::String::Find(tail, ".");
            if (found == INVALID_INDEX)
                break;
            pos = searchStart + found;
            searchStart = pos + 1;
        }
        return pos;
    }

    static std::string GetStubNamespace(const std::string& fullName)
    {
        int pos = FindLastDot(fullName);
        return pos == INVALID_INDEX ? std::string() : fullName.substr(0, pos);
    }

    static std::string GetStubSimpleName(const std::string& fullName)
    {
        int pos = FindLastDot(fullName);
        return pos == INVALID_INDEX ? fullName : fullName.substr(pos + 1);
    }

    static int FindStub(std::vector<NativeTypeStub>& stubs, const std::string& fullName)
    {
        return Utils::Vector::FindIndexIf(stubs, [&](const NativeTypeStub& stub) {
            return stub.fullName == fullName;
        });
    }

    static void AddAvailableType(std::vector<std::string>& availableTypes, const std::string& nsName, const std::string& name)
    {
        std::string fullName = nsName.empty() ? name : nsName + "." + name;
        if (!Utils::Vector::Contains(availableTypes, fullName))
            availableTypes.push_back(fullName);
    }

    static NativeTypeStub& AddStub(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                   const std::string& fullName, bool isClass)
    {
        int existing = FindStub(stubs, fullName);
        if (existing != INVALID_INDEX)
        {
            if (isClass)
                stubs[existing].isClass = true;
            return stubs[existing];
        }

        NativeTypeStub& stub = Utils::Vector::AddOne(stubs);
        stub.fullName = fullName;
        stub.isClass = isClass;
        if (Utils::Vector::Contains(availableTypes, fullName))
            stub.fullName = std::string();
        return stub;
    }

    static void AddStubForCppType(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                  const std::string& cppType)
    {
        std::string publicType = NormalizeStubTypeName(GetCSharpPublicType(cppType));
        if (IsBuiltinCSharpType(publicType) || Utils::Vector::Contains(availableTypes, publicType))
            return;

        bool isClass = IsScriptingObjectPointer(cppType) || IsObjectTypeRef(cppType)
            || (!cppType.empty() && cppType.find('*') != std::string::npos);
        AddStub(stubs, availableTypes, publicType, isClass);
    }

    static std::string NormalizeStubDefaultValue(const ApiParam& param)
    {
        std::string value = param.defaultValue;
        Utils::String::TrimStart(value);
        Utils::String::TrimEnd(value);
        if (value.empty())
            return value;

        Utils::String::ReplaceAll(value, " :: ", "::");
        Utils::String::ReplaceAll(value, ":: ", "::");
        Utils::String::ReplaceAll(value, " ::", "::");
        Utils::String::ReplaceAll(value, "nullptr", "null");
        Utils::String::ReplaceAll(value, "NULL", "null");

        int pos;
        while ((pos = Utils::String::Find(value, "::")) != INVALID_INDEX)
        {
            value = value.substr(0, pos) + "." + value.substr(pos + 2);
        }
        return value;
    }

    static void AddEnumMemberFromDefault(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                         const ApiParam& param)
    {
        std::string defaultValue = NormalizeStubDefaultValue(param);
        int dotPos = Utils::String::Find(defaultValue, ".");
        if (dotPos == INVALID_INDEX)
            return;

        std::string enumType = NormalizeStubTypeName(GetCSharpPublicType(param.cppType));
        if (IsBuiltinCSharpType(enumType) || Utils::Vector::Contains(availableTypes, enumType))
            return;

        std::string valuePrefix = defaultValue.substr(0, dotPos);
        std::string enumSimpleName = GetStubSimpleName(enumType);
        if (valuePrefix != enumSimpleName && valuePrefix != enumType)
            return;

        std::string member = defaultValue.substr(dotPos + 1);
        if (member.empty())
            return;

        NativeTypeStub& stub = AddStub(stubs, availableTypes, enumType, false);
        if (stub.fullName.empty())
            return;
        stub.isEnum = true;
        if (!Utils::Vector::Contains(stub.enumMembers, member))
            stub.enumMembers.push_back(member);
    }

    static void CollectFunctionStubs(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                     const ApiFunction& fn)
    {
        AddStubForCppType(stubs, availableTypes, fn.returnType);
        for (auto& param : fn.params)
        {
            AddStubForCppType(stubs, availableTypes, param.cppType);
            AddEnumMemberFromDefault(stubs, availableTypes, param);
        }
    }

    bool BindingsCSharpGenerator::GenerateNativeTypeStubs(const std::vector<BindingsHeaderInfo>& headers)
    {
        if (headers.empty())
            return true;

        std::vector<std::string> availableTypes;
        availableTypes.push_back("SE.Object");
        availableTypes.push_back("SE.Scripting.ScriptingObject");

        for (auto& header : headers)
        {
            for (auto& cls : header.classes)
            {
                AddAvailableType(availableTypes, CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceNameList), cls.name);
            }
            for (auto& en : header.enums)
            {
                AddAvailableType(availableTypes, CodeGeneratorUtils::GetFullCSNameSpaceName(en.namespaceScopeList), en.name);
            }
            for (auto& iface : header.interfaces)
            {
                AddAvailableType(availableTypes, CodeGeneratorUtils::GetFullCSNameSpaceName(iface.namespaceNameList), iface.name);
            }
        }

        std::vector<NativeTypeStub> stubs;
        for (auto& header : headers)
        {
            for (auto& cls : header.classes)
            {
                for (auto& field : cls.fields)
                {
                    AddStubForCppType(stubs, availableTypes, field.cppType);
                }
                for (auto& prop : cls.properties)
                {
                    AddStubForCppType(stubs, availableTypes, prop.cppType);
                }
                for (auto& fn : cls.functions)
                {
                    CollectFunctionStubs(stubs, availableTypes, fn);
                }
                for (auto& evt : cls.events)
                {
                    for (auto& param : evt.params)
                        AddStubForCppType(stubs, availableTypes, param.cppType);
                }
            }
            for (auto& iface : header.interfaces)
            {
                for (auto& fn : iface.functions)
                    CollectFunctionStubs(stubs, availableTypes, fn);
            }
        }

        std::string output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - native type placeholders.\n";
        output += "//-------------------------------------------------------------------------\n";
        output += "using System;\n";
        output += "using System.Runtime.InteropServices;\n\n";

        bool hasStubs = false;
        for (auto& stub : stubs)
        {
            if (stub.fullName.empty() || Utils::Vector::Contains(availableTypes, stub.fullName))
                continue;

            hasStubs = true;
            std::string nsName = GetStubNamespace(stub.fullName);
            std::string simpleName = MakeCSharpIdentifier(GetStubSimpleName(stub.fullName));
            if (!nsName.empty())
                output += Utils::String::Format("namespace {0}\n{{\n", nsName);

            if (stub.isEnum)
            {
                output += Utils::String::Format("    public enum {0} : int\n    {{\n", simpleName);
                if (stub.enumMembers.empty())
                {
                    output += "        _ = 0\n";
                }
                else
                {
                    for (int i = 0; i < stub.enumMembers.size(); ++i)
                    {
                        output += Utils::String::Format("        {0} = {1}", MakeCSharpIdentifier(stub.enumMembers[i]), i);
                        if (i < stub.enumMembers.size() - 1)
                            output += ",";
                        output += "\n";
                    }
                }
                output += "    }\n";
            }
            else if (stub.isClass)
            {
                output += Utils::String::Format("    public unsafe partial class {0}\n    {{\n", simpleName);
                output += "        internal IntPtr __unmanagedPtr = IntPtr.Zero;\n";
                output += Utils::String::Format("        internal static {0} FromUnmanaged(IntPtr nativePtr) => nativePtr != IntPtr.Zero ? new {0} {{ __unmanagedPtr = nativePtr }} : null;\n", simpleName);
                output += "    }\n";
            }
            else
            {
                output += "    [StructLayout(LayoutKind.Sequential)]\n";
                output += Utils::String::Format("    public unsafe partial struct {0}\n    {{\n", simpleName);
                output += "    }\n";
            }

            if (!nsName.empty())
                output += "}\n";
            output += "\n";
        }

        std::string outDir = Utils::String::Format("{0}/{1}", headers[0].assemblyDir, Settings::g_autogeneratedDirectory);
        FileSystem::NormalizePath(outDir);
        if (!FileSystem::DirectoryExists(outDir))
            FileSystem::CreateDirectory(outDir);

        std::string outPath = outDir + "/NativeTypeStubs.CSharp.cs";
        if (!hasStubs)
        {
            output += "namespace SE { }\n";
        }
        return SaveFile(outPath, std::string(output.c_str()));
    }
}
