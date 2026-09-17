#pragma once

#include "CodeGenerator_BindingsDataTypes.h"

#include <string>
#include <string_view>
#include <vector>

namespace SE::BuildTool
{
    class TypeDatabase;

    enum class CollectionKind { None, Variable, Fixed };

    struct CollectionInfo
    {
        CollectionKind kind = CollectionKind::None;
        TypeInfo elementType;
        int fixedElementCount = 0;

        bool IsCollection() const { return kind != CollectionKind::None; }
        bool HasRuntimeCount() const { return kind == CollectionKind::Variable; }
    };

    enum class BindingTypeKind
    {
        Unsupported,
        Blittable,
        String,
        StringView,
        ScriptingObject,
        NativeObject,
        ObjectRef,
        Collection,
        InteropStruct,
        VariantFamily,
        TypeHandle,
        OpaquePointer,
    };

    enum class BindingUseSite { Parameter, Return, Field, ArrayElement, Variant };
    enum class BindingDirection { In, Ref, Out };
    enum class InteropStrategy { Direct, CustomMarshaller, ManualWrapper, Unsupported };

    struct ReferenceSemantics
    {
        int pointerDepth = 0;
        bool isConst = false;
        bool isLValueReference = false;
        bool isRValueReference = false;
    };

    // Language-neutral facts only. No target-language spelling is stored here.
    struct BindingTypeSemantics
    {
        TypeInfo sourceType;
        TypeID canonicalType;
        BindingTypeKind kind = BindingTypeKind::Unsupported;
        bool isEnum = false;
        TypeInfoBase const* declaration = nullptr;
        CollectionInfo collection;
        ReferenceSemantics reference;
        std::string diagnosticCode;
        std::string diagnostic;

        bool IsSupported() const { return kind != BindingTypeKind::Unsupported; }
    };

    enum class AbiValueKind
    {
        Void,
        Integer,
        Float,
        Enum,
        BlittableStruct,
        InteropStruct,
        ClrString,
        ClrArray,
        ClrObject,
        ClrTypeObject,
        OpaquePointer,
    };

    enum class AbiPassMode { Value, Pointer, OutPointer };

    struct AbiType
    {
        AbiValueKind kind = AbiValueKind::Void;
        TypeID canonicalType;
        AbiPassMode passMode = AbiPassMode::Value;
        BindingTypeKind sourceKind = BindingTypeKind::Unsupported;
    };

    enum class AbiParameterRole { This, PublicParameter, HiddenCount, HiddenResult, Context };

    struct AbiParameterPlan
    {
        AbiParameterRole role = AbiParameterRole::PublicParameter;
        int publicParameterIndex = -1;
        AbiType type;
    };

    struct PublicToAbiMapping
    {
        int publicParameterIndex = -1;
        std::vector<int> abiParameterIndices;
    };

    struct FunctionAbiPlan
    {
        std::string entryPoint;
        AbiType returnType;
        std::vector<AbiParameterPlan> parameters;
        std::vector<PublicToAbiMapping> publicMappings;
        bool usesHiddenResult = false;
        std::string fingerprint;
        std::vector<std::string> diagnostics;

        bool IsSupported() const { return diagnostics.empty(); }
    };

    struct GeneratedFile
    {
        std::string path;
        std::string content;
    };

    enum class ConversionOp { None, ManagedToAbi, AbiToManaged, ReleaseTemporary };

    struct StatementPlan
    {
        ConversionOp operation = ConversionOp::None;
        std::string source;
        std::string target;
    };

    struct CallArgumentPlan
    {
        std::string expression;
        AbiPassMode passMode = AbiPassMode::Value;
    };

    // Ownership/write-back/cleanup lives here, outside FunctionAbiPlan.
    struct ArgumentMarshallingPlan
    {
        std::vector<StatementPlan> preCall;
        CallArgumentPlan callArgument;
        std::vector<StatementPlan> postCall;
        std::vector<StatementPlan> cleanup;

        bool RequiresConversion() const
        {
            return !preCall.empty() || !postCall.empty() || !cleanup.empty();
        }
    };

    // C++ and C# lower independently from the shared semantics and ABI plan.
    struct CppTypeConversion
    {
        BindingTypeKind kind = BindingTypeKind::Unsupported;
        std::string exportType;
        std::string nativeValueType;
        InteropStrategy strategy = InteropStrategy::Unsupported;
        std::string diagnostic;
    };

    struct CSharpTypeConversion
    {
        BindingTypeKind kind = BindingTypeKind::Unsupported;
        std::string publicType;
        std::string libraryImportManagedType;
        std::string marshaller;
        InteropStrategy strategy = InteropStrategy::Unsupported;
        std::string diagnostic;
    };

    BindingTypeSemantics ResolveBindingTypeSemantics(TypeDatabase const& database, TypeInfo const& type,
                                                     std::string_view marshalAs = {});
    CppTypeConversion ResolveCppTypeConversion(TypeDatabase const& database, BindingTypeSemantics const& semantics,
                                               BindingUseSite useSite, BindingDirection direction);
    CSharpTypeConversion ResolveCSharpTypeConversion(TypeDatabase const& database, BindingTypeSemantics const& semantics,
                                                     BindingUseSite useSite, BindingDirection direction);
    FunctionAbiPlan BuildFunctionAbiPlan(TypeDatabase const& database, TypeInfoStruct const& owner,
                                         TypeInfoFunc const& function);
    bool ValidateBindingsHeader(TypeDatabase const& database, BindingsHeaderInfo const& header,
                                std::vector<std::string>& diagnostics,
                                std::vector<std::string>* fingerprints = nullptr);

    BindingDirection GetBindingDirection(TypeInfoParam const& parameter);
    std::string GetManagedTypeName(TypeInfoBase const& declaration);
    bool IsKnownBlittableBuiltin(TypeInfo const& type);
    CollectionInfo GetCollectionInfo(TypeInfo const& type);
} // namespace SE::BuildTool
