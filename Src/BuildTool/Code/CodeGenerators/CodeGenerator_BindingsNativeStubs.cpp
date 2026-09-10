#include "CodeGenerator_BindingsCSharp.h"

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
        uint32_t   genericArity = 0;
        std::vector<std::string> enumMembers;
        TypeInfoStruct const* declaration = nullptr;
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

    static bool TryParseGenericType(const std::string& type, std::string& outBaseName,
                                    std::vector<std::string>& outArguments)
    {
        int32 const openIndex = Utils::String::Find(type, '<');
        if (openIndex == INVALID_INDEX)
        {
            return false;
        }

        int32 depth = 0;
        int32 closeIndex = INVALID_INDEX;
        for (int32 i = openIndex; i < type.length(); ++i)
        {
            if (type[(size_t)i] == '<')
            {
                ++depth;
            }
            else if (type[(size_t)i] == '>')
            {
                if (--depth == 0)
                {
                    closeIndex = i;
                    break;
                }
            }
        }

        if (closeIndex == INVALID_INDEX || closeIndex != type.length() - 1)
        {
            return false;
        }

        outBaseName = type.substr(0, (size_t)openIndex);
        Utils::String::TrimStart(outBaseName);
        Utils::String::TrimEnd(outBaseName);
        if (outBaseName.empty() || closeIndex == openIndex + 1)
        {
            return false;
        }

        int32 argumentStart = openIndex + 1;
        depth = 0;
        for (int32 i = argumentStart; i < closeIndex; ++i)
        {
            Char const c = type[(size_t)i];
            if (c == '<')
            {
                ++depth;
            }
            else if (c == '>')
            {
                --depth;
            }
            else if (c == ',' && depth == 0)
            {
                std::string argument = type.substr((size_t)argumentStart, (size_t)(i - argumentStart));
                Utils::String::TrimStart(argument);
                Utils::String::TrimEnd(argument);
                if (argument.empty())
                {
                    return false;
                }
                outArguments.push_back(std::move(argument));
                argumentStart = i + 1;
            }
        }

        std::string argument = type.substr((size_t)argumentStart, (size_t)(closeIndex - argumentStart));
        Utils::String::TrimStart(argument);
        Utils::String::TrimEnd(argument);
        if (argument.empty())
        {
            return false;
        }
        outArguments.push_back(std::move(argument));
        return true;
    }

    static std::string NormalizeStubBaseName(std::string type)
    {
        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);
        if (!type.empty() && !IsBuiltinCSharpType(type) && Utils::String::Find(type, ".") == INVALID_INDEX)
        {
            type = std::string("SE.") + type;
        }
        return type;
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

        std::string baseName;
        std::vector<std::string> arguments;
        if (TryParseGenericType(type, baseName, arguments))
        {
            int32 const openIndex = Utils::String::Find(type, '<');
            return NormalizeStubBaseName(baseName) + type.substr((size_t)openIndex);
        }
        return NormalizeStubBaseName(type);
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

    static int FindStub(std::vector<NativeTypeStub>& stubs, const std::string& fullName, uint32_t genericArity)
    {
        return Utils::Vector::FindIndexIf(stubs, [&](const NativeTypeStub& stub) {
            return stub.fullName == fullName && stub.genericArity == genericArity;
        });
    }

    static void AddAvailableType(std::vector<std::string>& availableTypes, const std::string& nsName, const std::string& name)
    {
        std::string fullName = nsName.empty() ? name : nsName + "." + name;
        if (!Utils::Vector::Contains(availableTypes, fullName))
            availableTypes.push_back(fullName);
    }

    static void AddAvailableFullType(std::vector<std::string>& availableTypes, const std::string& fullName)
    {
        if (!fullName.empty() && !Utils::Vector::Contains(availableTypes, fullName))
            availableTypes.push_back(fullName);
    }

    static NativeTypeStub& AddStub(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                   const std::string& fullName, bool isClass, uint32_t genericArity = 0)
    {
        int existing = FindStub(stubs, fullName, genericArity);
        if (existing != INVALID_INDEX)
        {
            if (isClass)
                stubs[existing].isClass = true;
            return stubs[existing];
        }

        NativeTypeStub& stub = Utils::Vector::AddOne(stubs);
        stub.fullName = fullName;
        stub.isClass = isClass;
        stub.genericArity = genericArity;
        if (Utils::Vector::Contains(availableTypes, fullName))
            stub.fullName = std::string();
        return stub;
    }

    static void AddStubForCSharpType(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                     const std::string& rawType, bool isClass)
    {
        std::string const publicType = NormalizeStubTypeName(rawType);
        std::string baseName;
        std::vector<std::string> arguments;
        if (TryParseGenericType(publicType, baseName, arguments))
        {
            for (auto const& argument : arguments)
            {
                AddStubForCSharpType(stubs, availableTypes, argument, false);
            }
        }
        else
        {
            baseName = publicType;
        }

        if (IsBuiltinCSharpType(baseName) || Utils::Vector::Contains(availableTypes, baseName))
        {
            return;
        }

        AddStub(stubs, availableTypes, baseName, isClass, (uint32_t)arguments.size());
    }

    static void AddStubForCppType(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                  TypeDatabase const& database, TypeInfo const& cppType,
                                  std::string_view marshalAs = {})
    {
        const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(database, cppType, marshalAs);
        const CSharpTypeConversion conversion = ResolveCSharpTypeConversion(
            database, semantics, BindingUseSite::Parameter, BindingDirection::In);
        const bool isClass = conversion.kind == BindingTypeKind::ScriptingObject ||
                             conversion.kind == BindingTypeKind::NativeObject ||
                             conversion.kind == BindingTypeKind::ObjectRef ||
                             conversion.kind == BindingTypeKind::OpaquePointer;
        AddStubForCSharpType(stubs, availableTypes, conversion.publicType, isClass);

        if (semantics.isEnum)
        {
            std::string const fullName = NormalizeStubTypeName(conversion.publicType);
            NativeTypeStub& stub = AddStub(stubs, availableTypes, fullName, false);
            if (!stub.fullName.empty())
                stub.isEnum = true;
        }

        // Referenced reflected structs are allowed to be non-API types. A
        // placeholder is insufficient for non-blittable layouts because the
        // LibraryImport declaration needs their generated custom marshaller.
        if (conversion.kind == BindingTypeKind::InteropStruct && semantics.declaration &&
            semantics.declaration->IsFlag(TypeInfoBase::Flag::IsClassStruct))
        {
            std::string const fullName = NormalizeStubTypeName(conversion.publicType);
            NativeTypeStub& stub = AddStub(stubs, availableTypes, fullName, false);
            if (!stub.fullName.empty())
                stub.declaration = static_cast<TypeInfoStruct const*>(semantics.declaration);
        }
    }

    static void AddEnumMemberFromDefault(std::vector<NativeTypeStub>& stubs, const std::vector<std::string>& availableTypes,
                                         TypeDatabase const& database, const TypeInfoParam& param)
    {
        std::string defaultValue = BindingsCSharpGenerator::NormalizeCSharpDefaultValue(param);
        int dotPos = Utils::String::Find(defaultValue, ".");
        if (dotPos == INVALID_INDEX)
            return;

        const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(database, param.type, param.marshalAs);
        if (!semantics.isEnum)
            return;
        std::string enumType = NormalizeStubTypeName(ResolveCSharpTypeConversion(
            database, semantics, BindingUseSite::Parameter, GetBindingDirection(param)).publicType);
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
                                     TypeDatabase const& database, const TypeInfoFunc& fn)
    {
        AddStubForCppType(stubs, availableTypes, database, fn.returnType, fn.marshalAs);
        for (auto& param : fn.params)
        {
            AddStubForCppType(stubs, availableTypes, database, param.type, param.marshalAs);
            AddEnumMemberFromDefault(stubs, availableTypes, database, param);
        }
    }

    static std::string GetGenericParameterList(uint32_t genericArity)
    {
        if (genericArity == 0)
        {
            return std::string();
        }

        std::string parameters = "<";
        for (uint32_t i = 0; i < genericArity; ++i)
        {
            if (i > 0)
            {
                parameters += ", ";
            }
            parameters += genericArity == 1 ? "T" : Utils::String::Format("T{0}", i);
        }
        parameters += ">";
        return parameters;
    }

    bool BindingsCSharpGenerator::GenerateNativeTypeStubs(const std::vector<BindingsHeaderInfo>& headers)
    {
        if (headers.empty())
            return true;

        std::vector<std::string> availableTypes;
        availableTypes.push_back("SE.Object");

        for (auto& header : headers)
        {
            for (auto& cls : header.classes)
            {
                if (!cls->APIInBuildMapType.empty())
                    AddAvailableFullType(availableTypes, cls->APIInBuildMapType);
                else
                    AddAvailableType(availableTypes, CodeGeneratorUtils::GetFullCSNameSpaceName(cls->namespaceScopeList),
                        cls->APIName.empty() ? cls->name : cls->APIName);
            }
            for (auto& en : header.enums)
            {
                AddAvailableType(availableTypes, CodeGeneratorUtils::GetFullCSNameSpaceName(en->namespaceScopeList), en->name);
            }
            for (auto& iface : header.interfaces)
            {
                if (!iface->APIInBuildMapType.empty())
                    AddAvailableFullType(availableTypes, iface->APIInBuildMapType);
                else
                    AddAvailableType(availableTypes, CodeGeneratorUtils::GetFullCSNameSpaceName(iface->namespaceScopeList), iface->name);
            }
        }

        std::vector<NativeTypeStub> stubs;
        for (auto& header : headers)
        {
            for (auto& cls : header.classes)
            {
                if (!cls->APIInBuildMapType.empty())
                {
                    continue;
                }
                for (auto& field : cls->fields)
                {
                    AddStubForCppType(stubs, availableTypes, m_Database, field.type, field.marshalAs);
                }
                for (auto& fn : cls->functions)
                {
                    CollectFunctionStubs(stubs, availableTypes, m_Database, fn);
                }
                for (auto& evt : cls->events)
                {
                    for (auto& param : evt.params)
                    {
                        AddStubForCppType(stubs, availableTypes, m_Database, param.type, param.marshalAs);
                    }
                }
            }
            for (auto& iface : header.interfaces)
            {
                if (!iface->APIInBuildMapType.empty())
                {
                    continue;
                }
                for (auto& fn : iface->functions)
                    CollectFunctionStubs(stubs, availableTypes, m_Database, fn);
            }
        }

        std::string output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - native type placeholders.\n";
        output += "//-------------------------------------------------------------------------\n";
        output += "using System;\n";
        output += "using System.Runtime.CompilerServices;\n";
        output += "using System.Runtime.InteropServices;\n";
        output += "using System.Runtime.InteropServices.Marshalling;\n";
        output += "using SE.Interop;\n\n";

        bool hasStubs = false;
        for (auto& stub : stubs)
        {
            if (stub.fullName.empty() || Utils::Vector::Contains(availableTypes, stub.fullName))
                continue;

            hasStubs = true;
            if (stub.declaration)
            {
                GenerateCSharpStructure(*stub.declaration, headers[0].assemblyName, output);
                continue;
            }

            std::string nsName = GetStubNamespace(stub.fullName);
            std::string simpleName = CodeGeneratorUtils::MakeCSharpIdentifier(GetStubSimpleName(stub.fullName)) + GetGenericParameterList(stub.genericArity);
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
                        output += Utils::String::Format("        {0} = {1}", CodeGeneratorUtils::MakeCSharpIdentifier(stub.enumMembers[i]), i);
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
        if (!m_GeneratedFiles && !FileSystem::DirectoryExists(outDir))
            FileSystem::CreateDirectory(outDir);

        std::string outPath = outDir + "/NativeTypeStubs.CSharp.cs";
        if (!hasStubs)
        {
            output += "namespace SE { }\n";
        }
        if (m_GeneratedFiles)
        {
            m_GeneratedFiles->push_back({ outPath, std::move(output) });
            return true;
        }
        return CodeGeneratorUtils::SaveFile(outPath, std::string(output.c_str()));
    }
}
