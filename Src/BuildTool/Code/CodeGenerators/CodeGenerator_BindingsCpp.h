#pragma once

// BindingsCppGenerator.h
// Generates C++ InternalCall registration code from parsed API annotations.
// Uses direct string building instead of Mustache templates for complex generation logic.

#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "../Database/TypeDatabase.h"

#include <string_view>

namespace SE::BuildTool
{
    struct CollectionInfo;

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

        // ---- Per-type generation methods (public for unified pipeline) ----

        void GenerateCppClass(const TypeInfoStruct& cls, const std::string& assemblyType, std::string& output);
        void GenerateCppStruct(const TypeInfoStruct& cls, const std::string& assemblyType, std::string& output);
        void GenerateCppEnum(const TypeInfoEnum& en, const std::string& assemblyType, std::string& output);
        void GenerateCppInterface(const TypeInfoStruct& iface, const std::string& assemblyType,
                                  std::string& output);

    private:

        // ---- Sub-generators ----

        void GenerateCppMethodWrapperFunction(const TypeInfoStruct& cls,
                                              const TypeInfoFunc&   fn,
                                              std::string& bodyOut, std::string& endOut);
        void GenerateCppFieldWrapperFunction(const TypeInfoStruct& cls,
                                             const TypeInfoFunc&   fn,
                                             BindingInvocationKind invocation,
                                             std::string& bodyOut, std::string& endOut);

        void GenerateCppEventWrappers(const TypeInfoStruct& cls,
                                      const TypeInfoEvent&  evt,
                                      const std::string& assemblyType, std::string& bodyOut, std::string& endOut);

        void GenerateCppInitRuntime(const TypeInfoStruct& cls, std::string& output);

        // ---- Helpers ----
        TypeInfoBase const* GetRegisteredType(TypeID typeID) const;
        CppTypeConversion ResolveConversion(TypeInfo const& type, std::string_view marshalAs = {},
                                            BindingUseSite useSite = BindingUseSite::Parameter,
                                            BindingDirection direction = BindingDirection::In) const;
        std::string GetInteropValueType(TypeInfo const& type, std::string_view marshalAs = {}) const;
        bool CanGenerateVariantFieldAccess(TypeInfo const& type) const;
        bool GetNativeToManagedConvert(TypeInfo const& type, std::string& expr, std::string_view marshalAs = {}) const;
        bool GetManagedToNativeConvert(TypeInfo const& type, std::string& expr, std::string_view marshalAs = {}) const;
        std::string GetNativeToVariantConvert(TypeInfo const& type, const std::string& expr) const;
        std::string GetVariantToNativeConvert(TypeInfo const& type, const std::string& expr) const;
        std::string GetReturnTypeConver(const TypeInfoFunc& fn) const;
        std::string BuildWrapperParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool forExport) const;
        std::string BuildForwardArgs(const TypeInfoFunc& fn) const;
        std::string BuildCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                  std::string& setupOut, std::string& postCallOut) const;
        void GenerateCollectionReturn(const TypeInfoFunc& fn, const CollectionInfo& collection,
                                      const std::string& nativeExpression, const std::string& postCall,
                                      std::string& output) const;

        TypeDatabase const& m_Database;
    };

} // namespace SE::BuildTool
