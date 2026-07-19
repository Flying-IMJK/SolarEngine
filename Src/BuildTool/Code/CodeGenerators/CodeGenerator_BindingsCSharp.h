#pragma once

// BindingsCSharpGenerator.h
// Generates C# binding declarations from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsDataTypes.h"

namespace SE::BuildTool
{
    class BindingsCSharpGenerator
    {
    public:
        BindingsCSharpGenerator() = default;

        /// Generate C# bindings for all items in the header.
        bool Generate(const BindingsHeaderInfo& headerInfo,
                      const std::string& solutionRoot);

        bool GenerateAll(const std::vector<BindingsHeaderInfo>& headers,
                         const std::string& solutionRoot);

        /// Generate minimal C# placeholders for native types referenced by API
        /// signatures but not generated as first-class binding types yet.
        bool GenerateNativeTypeStubs(const std::vector<BindingsHeaderInfo>& headers);

        /// Generate binary module assembly info (.Gen.cs).
        bool GenerateBinaryModuleAssemblyInfo(const BinaryModuleInfo& module);

        std::string_view GetErrorMessage() const { return m_errorMessage.c_str(); }

    private:
        // ---- Per-type generation methods ----

        void GenerateCSharpClass(const ApiClass& cls, const std::string& assemblyName,
                                 std::string& output);
        void GenerateCSharpStructure(const ApiClass& cls, const std::string& assemblyName,
                                     std::string& output);
        void GenerateCSharpEnum(const ApiEnum& en, std::string& output);
        void GenerateCSharpInterface(const ApiInterface& iface, std::string& output);

        // ---- Sub-generators ----

        void GenerateCSharpWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                           const std::string& assemblyName, std::string& output);
        void GenerateCSharpWrapperFunctionCall(const ApiClass& cls, const ApiFunction& fn,
                                               std::string& output);
        void GenerateCSharpPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                             const std::string& assemblyName, std::string& output);
        void GenerateCSharpFieldAccessors(const ApiClass& cls, const ApiField& field,
                                          const std::string& assemblyName, std::string& output);
        void GenerateCSharpEventAccessors(const ApiClass& cls, const ApiEvent& evt,
                                          const std::string& assemblyName, std::string& output);
        void GenerateCSharpClassMarshaller(const ApiClass& cls, std::string& output);
        void GenerateCSharpStructMarshaller(const ApiClass& cls, std::string& output);

        // ---- Helpers ----

        std::string GetAccessString(AccessLevel access) const;
        std::string BuildCSharpParams(const ApiFunction& fn, bool forPublic) const;
        std::string BuildCSharpInteropParams(const ApiClass& cls, const ApiFunction& fn) const;
        std::string BuildCSharpCallArgs(const ApiClass& cls, const ApiFunction& fn, bool isInterop) const;

        static bool SaveFile(const std::string& path, const std::string& content);

        mutable std::string m_errorMessage;
    };

} // namespace SE::BuildTool
