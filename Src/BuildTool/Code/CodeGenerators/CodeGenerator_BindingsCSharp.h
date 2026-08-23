#pragma once

// BindingsCSharpGenerator.h
// Generates C# binding declarations from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsModel.h"

namespace SE::BuildTool
{
    class BindingsCSharpGenerator
    {
    public:
        BindingsCSharpGenerator() = default;

        /// Generate C# bindings for all items in the header.
        bool Generate(const BindingsHeaderInfo& headerInfo,
                      const std::string& solutionRoot);

        /// Generate minimal C# placeholders for native types referenced by API
        /// signatures but not generated as first-class binding types yet.
        bool GenerateNativeTypeStubs(const std::vector<BindingsHeaderInfo>& headers);

        /// Generate binary module assembly info (.Gen.cs).
        bool GenerateBinaryModuleAssemblyInfo(const BinaryModuleInfo& module);

        std::string_view GetErrorMessage() const { return m_errorMessage.c_str(); }

    private:
        // ---- Per-type generation methods ----

        void GenerateCSharpClass(const TypeInfoStruct& cls, const std::string& assemblyName,
                                 std::string& output);
        void GenerateCSharpStructure(const TypeInfoStruct& cls, const std::string& assemblyName,
                                     std::string& output);
        void GenerateCSharpEnum(const TypeInfoEnum& en, std::string& output);
        void GenerateCSharpInterface(const TypeInfoStruct& iface, std::string& output);

        // ---- Sub-generators ----

        void GenerateCSharpWrapperFunction(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                           const std::string& assemblyName, std::string& output);
        void GenerateCSharpWrapperFunctionCall(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                               std::string& output);
        void GenerateCSharpAccessorProperty(const TypeInfoStruct& cls, const BindingCallable* getter,
                                            const BindingCallable* setter, const std::string& publicName,
                                            const std::string& publicCppType, AccessLevel getterAccess,
                                            AccessLevel setterAccess, bool isStatic, const std::string& attributes,
                                            const std::string& comment,
                                            const std::string& assemblyName, std::string& output);
        void GenerateCSharpPropertyAccessors(const TypeInfoStruct& cls, const TypeInfoFunc& prop,
                                             std::vector<bool>& consumedFunctions,
                                             int functionIndex, const std::string& assemblyName,
                                             std::string& output);
        void GenerateCSharpFieldAccessors(const TypeInfoStruct& cls, const TypeInfoField& field,
                                          const std::string& assemblyName, std::string& output);
        void GenerateCSharpEventAccessors(const TypeInfoStruct& cls, const TypeInfoEvent& evt,
                                          const std::string& assemblyName, std::string& output);
        void GenerateCSharpClassMarshaller(std::string& name, std::string& marshallerName, std::string& output);
        void GenerateCSharpStructMarshaller(const TypeInfoStruct& cls, std::string& output);

        // ---- Helpers ----


        std::string BuildCSharpParams(const TypeInfoFunc& fn, bool forPublic) const;
        std::string BuildCSharpInteropParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn) const;
        std::string BuildCSharpCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool isInterop) const;

        mutable std::string m_errorMessage;
    };

} // namespace SE::BuildTool
