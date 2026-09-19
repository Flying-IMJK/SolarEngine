#pragma once

// BindingsCSharpGenerator.h
// Generates C# binding declarations from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "../Database/TypeDatabase.h"

namespace SE::BuildTool
{
    class BindingsCSharpGenerator
    {
    public:
        BindingsCSharpGenerator(TypeDatabase const& database, std::vector<GeneratedFile>* generatedFiles = nullptr)
            : m_Database(database), m_GeneratedFiles(generatedFiles) {}

        /// Generate C# bindings for all items in the header.
        bool Generate(const BindingsHeaderInfo& headerInfo,
                      const std::string& solutionRoot);

        /// Generate minimal C# placeholders for native types referenced by API
        /// signatures but not generated as first-class binding types yet.
        bool GenerateNativeTypeStubs(const std::vector<BindingsHeaderInfo>& headers);

        /// Normalizes a C++ default-value expression (namespaces, nullptr) to C#.
        static std::string NormalizeCSharpDefaultValue(const TypeInfoParam& param);

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

        void OpenCSharpContainingTypeScopes(const TypeInfoBase& type, std::string& output) const;
        static void CloseCSharpContainingTypeScopes(const TypeInfoBase& type, std::string& output);

        std::string BuildCSharpParams(const TypeInfoFunc& fn, bool forPublic);
        bool IsCSharpOptionalConstant(const TypeInfoParam& param) const;
        std::string BuildCSharpInteropParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn);
        std::string BuildCSharpCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool isInterop,
                                        std::string* preCall = nullptr, std::string* postCall = nullptr,
                                        std::string* cleanup = nullptr);

        // C# ABI layout translation for interop-struct fields.
        std::string GetCSharpStructAbiFieldType(const TypeInfo& cppType, std::string_view marshalAs = {}) const;
        std::string GetCSharpCollectionCountExpression(const TypeInfo& cppType, const std::string& expression) const;
        std::string GetCSharpStructFieldFromAbi(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs = {}) const;
        std::string GetCSharpStructFieldToAbi(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs = {}) const;

        CSharpTypeConversion ResolveConversion(const TypeInfo& cppType, std::string_view marshalAs = {},
                                               BindingUseSite useSite = BindingUseSite::Parameter,
                                               BindingDirection direction = BindingDirection::In) const;
        std::string GetCSharpPublicType(const TypeInfo& cppType, std::string_view marshalAs = {}) const;
        std::string GetCSharpFullTypeName(const TypeID& typeID) const;
        std::string GetCSharpFromInterop(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs = {}) const;
        std::string GetCSharpToInterop(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs = {}) const;
        bool UsePassByReference(const TypeInfo& cppType, std::string_view marshalAs = {}) const;
        std::string GetCSharpParamMarshalAttribute(const TypeInfo& cppType, const std::string& paramName,
                                                   std::string_view marshalAs = {},
                                                   BindingDirection direction = BindingDirection::In) const;
        std::string GetCSharpReturnMarshalAttribute(const TypeInfo& cppType, std::string_view marshalAs = {}) const;

        TypeDatabase const& m_Database;
        std::vector<GeneratedFile>* m_GeneratedFiles;
    };

} // namespace SE::BuildTool
