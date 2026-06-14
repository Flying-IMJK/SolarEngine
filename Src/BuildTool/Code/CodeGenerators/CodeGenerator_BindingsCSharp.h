#pragma once

// BindingsCSharpGenerator.h
// Generates C# binding declarations from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsDataTypes.h"
#include "Core/Types/Strings/String.h"

namespace SE::ReflectTool
{
    class BindingsCSharpGenerator
    {
    public:
        BindingsCSharpGenerator() = default;

        /// Generate C# bindings for all items in the header.
        bool Generate(const BindingsHeaderInfo& headerInfo,
                      const String& solutionRoot);

        bool GenerateAll(const List<BindingsHeaderInfo>& headers,
                         const String& solutionRoot);

        /// Generate binary module assembly info (.Gen.cs).
        bool GenerateBinaryModuleAssemblyInfo(const BinaryModuleInfo& module);

        StringAnsiView GetErrorMessage() const { return m_errorMessage.Get(); }

    private:
        // ---- Per-type generation methods ----

        void GenerateCSharpClass(const ApiClass& cls, const StringAnsi& assemblyName,
                                 StringAnsi& output);
        void GenerateCSharpStructure(const ApiClass& cls, const StringAnsi& assemblyName,
                                     StringAnsi& output);
        void GenerateCSharpEnum(const ApiEnum& en, StringAnsi& output);
        void GenerateCSharpInterface(const ApiInterface& iface, StringAnsi& output);

        // ---- Sub-generators ----

        void GenerateCSharpWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                           const StringAnsi& assemblyName, StringAnsi& output);
        void GenerateCSharpWrapperFunctionCall(const ApiClass& cls, const ApiFunction& fn,
                                               StringAnsi& output);
        void GenerateCSharpPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                             const StringAnsi& assemblyName, StringAnsi& output);
        void GenerateCSharpFieldAccessors(const ApiClass& cls, const ApiField& field,
                                          const StringAnsi& assemblyName, StringAnsi& output);
        void GenerateCSharpEventAccessors(const ApiClass& cls, const ApiEvent& evt,
                                          const StringAnsi& assemblyName, StringAnsi& output);
        void GenerateCSharpClassMarshaller(const ApiClass& cls, StringAnsi& output);
        void GenerateCSharpStructMarshaller(const ApiClass& cls, StringAnsi& output);

        // ---- Helpers ----

        StringAnsi GetAccessString(AccessLevel access) const;
        StringAnsi BuildCSharpParams(const ApiFunction& fn, bool forPublic) const;
        StringAnsi BuildCSharpInteropParams(const ApiClass& cls, const ApiFunction& fn) const;
        StringAnsi BuildCSharpCallArgs(const ApiClass& cls, const ApiFunction& fn, bool isInterop) const;
        StringAnsi GetCSharpNamespace(const StringAnsi& namespaceName) const;

        static bool SaveFile(const String& path, const std::string& content);

        mutable StringAnsi m_errorMessage;
    };

} // namespace SE::ReflectTool