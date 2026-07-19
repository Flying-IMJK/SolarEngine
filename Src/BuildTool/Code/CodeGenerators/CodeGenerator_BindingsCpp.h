#pragma once

// BindingsCppGenerator.h
// Generates C++ InternalCall registration code from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsDataTypes.h"

namespace SE::BuildTool
{
    class BindingsCppGenerator
    {
    public:
        BindingsCppGenerator() = default;

        /// Generate C++ InternalCall registration code for all items in the header.
        /// The caller owns placement of the generated text. Solar keeps this code in
        /// the matching .typeinfo.h file instead of emitting separate binding .cpp files.
        bool GenerateSource(const BindingsHeaderInfo& headerInfo,
                            bool compilerIsMSVC,
                            std::string& output);

        std::string_view GetErrorMessage() const { return m_errorMessage.c_str(); }

        // ---- Per-type generation methods (public for unified pipeline) ----

        void GenerateCppClass(const ApiClass& cls, const std::string& assemblyType,
                              bool compilerIsMSVC, std::string& output);
        void GenerateCppStruct(const ApiClass& cls, const std::string& assemblyType,
                               bool compilerIsMSVC, std::string& output);
        void GenerateCppEnum(const ApiEnum& en, const std::string& assemblyType,
                             std::string& output);
        void GenerateCppInterface(const ApiInterface& iface, const std::string& assemblyType,
                                  std::string& output);

    private:

        // ---- Sub-generators ----

        void GenerateCppWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                        bool compilerIsMSVC, std::string& bodyOut, std::string& endOut);
        void GenerateCppPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                          bool compilerIsMSVC, std::string& bodyOut, std::string& endOut);
        void GenerateCppEventWrappers(const ApiClass& cls, const ApiEvent& evt,
                                       std::string& bodyOut);
        void GenerateCppFieldAccessors(const ApiClass& cls, const ApiField& field,
                                        bool compilerIsMSVC, std::string& bodyOut, std::string& endOut);
        void GenerateCppInitRuntime(const ApiClass& cls, std::string& output);

        // ---- Helpers ----
        std::string GetNativeToManagedConvert(const std::string& cppType, const std::string& expr) const;
        std::string GetManagedToNativeConvert(const std::string& cppType, const std::string& expr) const;
        std::string GetNativeToVariantConvert(const std::string& cppType, const std::string& expr) const;
        std::string GetVariantToNativeConvert(const std::string& cppType, const std::string& expr) const;
        std::string GetInteropReturnType(const ApiFunction& fn) const;
        std::string GetInteropParamType(const ApiParam& param) const;
        std::string BuildWrapperParams(const ApiClass& cls, const ApiFunction& fn, bool forExport) const;
        std::string BuildForwardArgs(const ApiFunction& fn) const;
        std::string BuildCallArgs(const ApiClass& cls, const ApiFunction& fn) const;
        mutable std::string m_errorMessage;
    };

} // namespace SE::BuildTool
