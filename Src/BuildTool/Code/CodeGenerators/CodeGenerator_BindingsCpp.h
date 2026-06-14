#pragma once

// BindingsCppGenerator.h
// Generates C++ InternalCall registration code from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsDataTypes.h"
#include "Core/Types/Strings/String.h"

namespace SE::ReflectTool
{
    class BindingsCppGenerator
    {
    public:
        BindingsCppGenerator() = default;

        /// Generate C++ InternalCall registration code for all items in the header.
        bool Generate(const BindingsHeaderInfo& headerInfo,
                      const String& solutionRoot,
                      bool compilerIsMSVC);

        bool GenerateAll(const List<BindingsHeaderInfo>& headers,
                         const String& solutionRoot,
                         bool compilerIsMSVC);

        /// Generate binary module .Gen.h and .Gen.cpp files.
        bool GenerateBinaryModule(const BinaryModuleInfo& module,
                                  const String& solutionRoot);

        StringAnsiView GetErrorMessage() const { return m_errorMessage.Get(); }

        // ---- Per-type generation methods (public for unified pipeline) ----

        void GenerateCppClass(const ApiClass& cls, const StringAnsi& assemblyType,
                              bool compilerIsMSVC, StringAnsi& output);
        void GenerateCppStruct(const ApiClass& cls, const StringAnsi& assemblyType,
                               bool compilerIsMSVC, StringAnsi& output);
        void GenerateCppEnum(const ApiEnum& en, const StringAnsi& assemblyType,
                             StringAnsi& output);
        void GenerateCppInterface(const ApiInterface& iface, const StringAnsi& assemblyType,
                                  StringAnsi& output);

    private:

        // ---- Sub-generators ----

        void GenerateCppWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                        bool compilerIsMSVC, StringAnsi& bodyOut, StringAnsi& endOut);
        void GenerateCppPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                          bool compilerIsMSVC, StringAnsi& bodyOut, StringAnsi& endOut);
        void GenerateCppEventWrappers(const ApiClass& cls, const ApiEvent& evt,
                                       StringAnsi& bodyOut);
        void GenerateCppFieldAccessors(const ApiClass& cls, const ApiField& field,
                                        bool compilerIsMSVC, StringAnsi& bodyOut, StringAnsi& endOut);
        void GenerateCppInitRuntime(const ApiClass& cls, StringAnsi& output);

        // ---- Helpers ----

        StringAnsi GetNativeName(const StringAnsi& nameSpaceName, const StringAnsi& name) const;
    	StringAnsi GetFullCSTypeName(const StringAnsi& nameSpaceName, const StringAnsi& name) const;
        StringAnsi GetInternalClassName(const ApiClass& cls) const;
        StringAnsi GetNativeToManagedConvert(const StringAnsi& cppType, const StringAnsi& expr) const;
        StringAnsi GetManagedToNativeConvert(const StringAnsi& cppType, const StringAnsi& expr) const;
        StringAnsi GetNativeToVariantConvert(const StringAnsi& cppType, const StringAnsi& expr) const;
        StringAnsi GetVariantToNativeConvert(const StringAnsi& cppType, const StringAnsi& expr) const;
        StringAnsi GetInteropReturnType(const ApiFunction& fn) const;
        StringAnsi GetInteropParamType(const ApiParam& param) const;
        StringAnsi BuildWrapperParams(const ApiClass& cls, const ApiFunction& fn, bool forExport) const;
        StringAnsi BuildForwardArgs(const ApiFunction& fn) const;
        StringAnsi BuildCallArgs(const ApiClass& cls, const ApiFunction& fn) const;

        static bool SaveFile(const String& path, const StringAnsi& content);

        mutable StringAnsi m_errorMessage;
    };

} // namespace SE::ReflectTool