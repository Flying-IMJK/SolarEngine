#pragma once

// BindingsCppGenerator.h
// Generates C++ InternalCall registration code from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsModel.h"
#include "../Database/TypeDatabase.h"

namespace SE::BuildTool
{
    struct CollectionAbiInfo;

    class BindingsCppGenerator
    {
    public:
        BindingsCppGenerator(TypeDatabase const& database) : m_Database(database) {}

        /// Generate C++ InternalCall registration code for all items in the header.
        /// The caller owns placement of the generated text. Solar keeps this code in
        /// the matching .typeinfo.h file instead of emitting separate binding .cpp files.
        bool GenerateSource(const BindingsHeaderInfo& headerInfo, std::string& output);

        /// Generates the module-wide ABI bridge for non-blittable API structs.
        /// It deliberately uses managed handles for strings rather than exposing
        /// native String object layout to C#.
        bool GenerateInteropHeader(const std::vector<BindingsHeaderInfo>& headers, std::string& output);

        std::string_view GetErrorMessage() const { return m_errorMessage.c_str(); }

        // ---- Per-type generation methods (public for unified pipeline) ----

        void GenerateCppClass(const TypeInfoStruct& cls, const std::string& assemblyType, std::string& output);
        void GenerateCppStruct(const TypeInfoStruct& cls, const std::string& assemblyType, std::string& output);
        void GenerateCppEnum(const TypeInfoEnum& en, const std::string& assemblyType, std::string& output);
        void GenerateCppInterface(const TypeInfoStruct& iface, const std::string& assemblyType,
                                  std::string& output);

    private:

        // ---- Sub-generators ----

        void GenerateCppWrapperFunction(const TypeInfoStruct& cls,
                                        const TypeInfoFunc&    fn,
                                        BindingInvocationKind invocation,
                                        std::string& bodyOut, std::string& endOut);
        void GenerateCppPropertyAccessors(const TypeInfoStruct& cls,
                                          const TypeInfoFunc&   prop,
                                          std::vector<bool>&    consumedFunctions,
                                          int                   functionIndex,
                                          std::string&          bodyOut,
                                          std::string&          endOut);
        void GenerateCppEventWrappers(const TypeInfoStruct& cls,
                                      const TypeInfoEvent&  evt,
                                      const std::string& assemblyType, std::string& bodyOut, std::string& endOut);
        void GenerateCppFieldAccessors(const TypeInfoStruct& cls,
                                       const TypeInfoField&  field,
                                       std::string&          bodyOut,
                                       std::string&          endOut);
        void GenerateCppInitRuntime(const TypeInfoStruct& cls, std::string& output);

        // ---- Helpers ----
        TypeInfoBase const* GetRegisteredType(TypeID typeID) const;
        bool CanGenerateVariantFieldAccess(TypeID typeID) const;
        std::string GetNativeToManagedConvert(TypeID typeID, const std::string& expr) const;
        std::string GetManagedToNativeConvert(TypeID typeID, const std::string& expr) const;
        std::string GetNativeToVariantConvert(TypeID typeID, const std::string& expr) const;
        std::string GetVariantToNativeConvert(TypeID typeID, const std::string& expr) const;
        std::string GetInteropReturnType(const TypeInfoFunc& fn) const;
        std::string GetInteropParamType(const TypeInfoParam& param) const;
        bool ShouldUseOutResult(const std::string& cppType) const;
        bool NeedsInteropPointer(const TypeInfoParam& param) const;
        std::string BuildWrapperParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool forExport) const;
        std::string BuildForwardArgs(const TypeInfoFunc& fn) const;
        std::string BuildCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn, std::string& setupOut) const;
        void GenerateCollectionReturn(const TypeInfoFunc& fn, const CollectionAbiInfo& collection,
                                      const std::string& nativeExpression, std::string& output) const;

        mutable std::string m_errorMessage;

        TypeDatabase const& m_Database;
    };

} // namespace SE::BuildTool
