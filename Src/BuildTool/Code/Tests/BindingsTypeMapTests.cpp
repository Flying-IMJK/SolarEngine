#include "CodeGenerators/CodeGenerator_BindingsTypeMap.h"
#include "Database/TypeDatabase.h"

#include <iostream>
#include <memory>
#include <vector>

using namespace SE::BuildTool;

namespace
{
    int g_failures = 0;

    void Check(bool condition, const char* message)
    {
        if (condition) return;
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

int main()
{
    TypeDatabase database;

    TypeInfo intType(TypeID("int32"));
    BindingTypeSemantics intSemantics = ResolveBindingTypeSemantics(database, intType);
    Check(intSemantics.kind == BindingTypeKind::Blittable, "int32 is direct blittable");
    Check(ResolveCppTypeConversion(database, intSemantics, BindingUseSite::Parameter, BindingDirection::In).exportType == "int32",
          "C++ int32 spelling");
    Check(ResolveCSharpTypeConversion(database, intSemantics, BindingUseSite::Parameter, BindingDirection::In).publicType == "int",
          "C# int32 spelling");

    TypeInfo stringType(TypeID("SE::String"));
    BindingTypeSemantics stringSemantics = ResolveBindingTypeSemantics(database, stringType);
    CSharpTypeConversion stringCSharp = ResolveCSharpTypeConversion(
        database, stringSemantics, BindingUseSite::Parameter, BindingDirection::In);
    Check(stringSemantics.kind == BindingTypeKind::String, "String semantic kind");
    Check(stringCSharp.publicType == "string" && stringCSharp.libraryImportManagedType == "string",
          "String separates managed spelling from CLRString storage");
    Check(stringCSharp.strategy == InteropStrategy::CustomMarshaller, "String uses exactly one custom-marshaller strategy");
    Check(ResolveCppTypeConversion(database, stringSemantics, BindingUseSite::Parameter, BindingDirection::In).exportType == "CLRString*",
          "String physical C++ storage");

    TypeInfo matrixType(TypeID("SE::Matrix"));
    BindingTypeSemantics matrixSemantics = ResolveBindingTypeSemantics(database, matrixType);
    Check(matrixSemantics.kind == BindingTypeKind::Blittable, "qualified Matrix is a known POD mapping");
    Check(ResolveCSharpTypeConversion(database, matrixSemantics, BindingUseSite::Parameter, BindingDirection::Ref).publicType == "Matrix",
          "qualified Matrix resolves to the existing managed POD");

    TypeInfo qualifiedChar(TypeID("SE::Char"));
    BindingTypeSemantics qualifiedCharSemantics = ResolveBindingTypeSemantics(database, qualifiedChar);
    CSharpTypeConversion qualifiedCharCSharp = ResolveCSharpTypeConversion(
        database, qualifiedCharSemantics, BindingUseSite::Field, BindingDirection::In);
    Check(qualifiedCharSemantics.kind == BindingTypeKind::Blittable, "qualified Char is a known builtin mapping");
    Check(qualifiedCharCSharp.publicType == "char" && qualifiedCharCSharp.libraryImportManagedType == "char",
          "qualified Char resolves to managed char");

    TypeInfo unqualifiedChar(TypeID("Char"));
    BindingTypeSemantics unqualifiedCharSemantics = ResolveBindingTypeSemantics(database, unqualifiedChar);
    Check(unqualifiedCharSemantics.kind == BindingTypeKind::Blittable, "unqualified Char remains supported");
    Check(ResolveCSharpTypeConversion(database, unqualifiedCharSemantics, BindingUseSite::Field,
                                      BindingDirection::In).publicType == "char",
          "unqualified Char resolves to managed char");

    TypeInfo unresolvedPointer(TypeID("InternalSlot"));
    unresolvedPointer.isPointer = true;
    unresolvedPointer.isConst = true;
    BindingTypeSemantics opaqueSemantics = ResolveBindingTypeSemantics(database, unresolvedPointer);
    CppTypeConversion opaqueCpp = ResolveCppTypeConversion(database, opaqueSemantics, BindingUseSite::Field,
                                                           BindingDirection::In);
    CSharpTypeConversion opaqueCSharp = ResolveCSharpTypeConversion(database, opaqueSemantics, BindingUseSite::Field,
                                                                    BindingDirection::In);
    Check(opaqueSemantics.kind == BindingTypeKind::OpaquePointer, "unresolved single-level pointer is opaque");
    Check(opaqueCpp.exportType == "void*" && opaqueCpp.nativeValueType == "InternalSlot*",
          "opaque pointer uses pointer-sized C++ ABI storage");
    Check(opaqueCSharp.publicType == "IntPtr" && opaqueCSharp.libraryImportManagedType == "IntPtr" &&
          opaqueCSharp.strategy == InteropStrategy::Direct,
          "opaque pointer uses direct IntPtr C# conversion");
    TypeInfo builtinPointer(TypeID("int32"));
    builtinPointer.isPointer = true;
    Check(ResolveBindingTypeSemantics(database, builtinPointer).kind == BindingTypeKind::OpaquePointer,
          "single-level pointers to builtin values use opaque semantics");
    TypeInfoStruct opaqueOwner(TypeID("SE::OpaqueOwner"), "OpaqueOwner");
    TypeInfoFunc opaqueFunction;
    opaqueFunction.name = "GetSlot";
    opaqueFunction.uniqueName = "GetSlot";
    opaqueFunction.entryPoint = "OpaqueOwner_GetSlot";
    opaqueFunction.isStatic = true;
    opaqueFunction.returnType = unresolvedPointer;
    FunctionAbiPlan opaquePlan = BuildFunctionAbiPlan(database, opaqueOwner, opaqueFunction);
    Check(opaquePlan.IsSupported() && opaquePlan.returnType.kind == AbiValueKind::OpaquePointer,
          "opaque pointer has an explicit ABI value kind");

    auto reflectedPointee = std::make_unique<TypeInfoStruct>(TypeID("SE::ReflectedSlot"), "ReflectedSlot");
    reflectedPointee->isStruct = true;
    reflectedPointee->isPod = true;
    database.RegisterType(std::move(reflectedPointee), false);
    TypeInfo reflectedPointer(TypeID("SE::ReflectedSlot"));
    reflectedPointer.isPointer = true;
    BindingTypeSemantics reflectedPointerSemantics = ResolveBindingTypeSemantics(database, reflectedPointer);
    Check(reflectedPointerSemantics.kind == BindingTypeKind::OpaquePointer,
          "reflected struct pointer uses opaque semantics");

    auto scriptingObject = std::make_unique<TypeInfoStruct>(TypeID("SE::BindingsObject"), "BindingsObject");
    scriptingObject->isStruct = false;
    scriptingObject->isScriptingObject = true;
    database.RegisterType(std::move(scriptingObject), false);
    TypeInfo scriptingObjectPointer(TypeID("SE::BindingsObject"));
    scriptingObjectPointer.isPointer = true;
    BindingTypeSemantics scriptingObjectSemantics = ResolveBindingTypeSemantics(database, scriptingObjectPointer);
    Check(scriptingObjectSemantics.kind == BindingTypeKind::ScriptingObject,
          "scripting object pointer retains object semantics");

    TypeInfo multiPointer = unresolvedPointer;
    multiPointer.pointerDepth = 2;
    BindingTypeSemantics multiPointerSemantics = ResolveBindingTypeSemantics(database, multiPointer);
    Check(!multiPointerSemantics.IsSupported() && multiPointerSemantics.diagnosticCode == "SEBIND004",
          "multi-level pointers remain rejected");

    TypeInfo unresolvedValue(TypeID("MissingValue"));
    BindingTypeSemantics unresolvedValueSemantics = ResolveBindingTypeSemantics(database, unresolvedValue);
    Check(!unresolvedValueSemantics.IsSupported() && unresolvedValueSemantics.diagnosticCode == "SEBIND005",
          "unresolved non-pointer values remain rejected");

    TypeInfoStruct mixedOwner(TypeID("SE::MixedBindingFixture"), "MixedBindingFixture");
    mixedOwner.isStruct = true;
    mixedOwner.isAPI = true;
    TypeInfoField characterField;
    characterField.name = "Character";
    characterField.type = qualifiedChar;
    characterField.isAPI = true;
    mixedOwner.fields.push_back(characterField);
    TypeInfoField slotField;
    slotField.name = "Slot";
    slotField.type = unresolvedPointer;
    slotField.isAPI = true;
    mixedOwner.fields.push_back(slotField);
    TypeInfoField objectField;
    objectField.name = "Object";
    objectField.type = scriptingObjectPointer;
    objectField.isAPI = true;
    mixedOwner.fields.push_back(objectField);
    BindingsHeaderInfo mixedHeader;
    mixedHeader.classes.push_back(&mixedOwner);
    std::vector<std::string> mixedFingerprints;
    std::vector<std::string> mixedDiagnostics;
    Check(ValidateBindingsHeader(database, mixedHeader, mixedDiagnostics, &mixedFingerprints),
          "mixed character, opaque pointer, and object pointer fields pass validation");
    Check(mixedDiagnostics.empty() && mixedFingerprints.size() == 6,
          "mixed field getter/setter ABI plans are emitted for every field");

    TypeInfo typeIdType(TypeID("SE::TypeID"));
    BindingTypeSemantics typeIdSemantics = ResolveBindingTypeSemantics(database, typeIdType);
    Check(typeIdSemantics.kind == BindingTypeKind::Blittable, "TypeID has an explicit ABI mapping");
    Check(ResolveCppTypeConversion(database, typeIdSemantics, BindingUseSite::Parameter, BindingDirection::In).exportType == "uint32",
          "TypeID crosses the native ABI as uint32");
    Check(ResolveCSharpTypeConversion(database, typeIdSemantics, BindingUseSite::Parameter, BindingDirection::In).publicType == "uint",
          "TypeID crosses the managed ABI as uint");

    TypeInfo arrayType(TypeID("SE::Array"));
    arrayType.genericityArgs.push_back(intType);
    BindingTypeSemantics arraySemantics = ResolveBindingTypeSemantics(database, arrayType);
    CSharpTypeConversion arrayCSharp = ResolveCSharpTypeConversion(
        database, arraySemantics, BindingUseSite::Parameter, BindingDirection::In);
    Check(arraySemantics.kind == BindingTypeKind::Collection, "Array<T> is structural collection");
    Check(arrayCSharp.publicType == "int[]" && arrayCSharp.libraryImportManagedType == "int[]",
          "Array<T> public and LibraryImport managed types");

    TypeInfoStruct owner(TypeID("SE::BindingsFixture"), "BindingsFixture");
    TypeInfoFunc function;
    function.name = "RoundTrip";
    function.uniqueName = "RoundTrip";
    function.entryPoint = "BindingsFixture_RoundTrip";
    function.isStatic = true;
    function.returnType = stringType;
    TypeInfoParam value;
    value.name = "value";
    value.type = intType;
    value.direction = ApiParameterDirection::Ref;
    function.params.push_back(value);
    TypeInfoParam values;
    values.name = "values";
    values.type = arrayType;
    function.params.push_back(values);

    FunctionAbiPlan first = BuildFunctionAbiPlan(database, owner, function);
    FunctionAbiPlan second = BuildFunctionAbiPlan(database, owner, function);
    Check(first.IsSupported(), "P0 fixture plan is supported");
    Check(first.parameters.size() == 3, "ref value, array, and hidden array count are ordered ABI parameters");
    Check(first.parameters[0].type.passMode == AbiPassMode::Pointer, "ref direction becomes ABI pointer mode");
    Check(first.parameters[2].role == AbiParameterRole::HiddenCount, "array count role is explicit");
    Check(first.returnType.kind == AbiValueKind::ClrString, "String return has CLR string ABI identity");
    Check(first.fingerprint == second.fingerprint && !first.fingerprint.empty(), "ABI fingerprint is deterministic");

    TypeInfoFunc refCollectionFunction;
    refCollectionFunction.name = "Replace";
    refCollectionFunction.uniqueName = "Replace";
    refCollectionFunction.entryPoint = "BindingsFixture_Replace";
    refCollectionFunction.isStatic = true;
    refCollectionFunction.returnType = TypeInfo::Void;
    values.direction = ApiParameterDirection::Ref;
    refCollectionFunction.params.push_back(values);
    FunctionAbiPlan refCollectionPlan = BuildFunctionAbiPlan(database, owner, refCollectionFunction);
    Check(refCollectionPlan.parameters.size() == 2, "ref collection expands to data and count ABI parameters");
    Check(refCollectionPlan.parameters[0].type.passMode == AbiPassMode::Pointer &&
          refCollectionPlan.parameters[1].type.passMode == AbiPassMode::Pointer,
          "ref collection data and count are both writable");

    TypeInfo rvalue = intType;
    rvalue.isRef = true;
    rvalue.isMoveRef = true;
    BindingTypeSemantics rvalueSemantics = ResolveBindingTypeSemantics(database, rvalue);
    Check(!rvalueSemantics.IsSupported() && rvalueSemantics.diagnosticCode == "SEBIND010",
          "rvalue reference fails with stable diagnostic");

    TypeInfo dictionary(TypeID("SE::Dictionary"));
    dictionary.genericityArgs.push_back(intType);
    dictionary.genericityArgs.push_back(intType);
    Check(!ResolveBindingTypeSemantics(database, dictionary).IsSupported(), "Dictionary is explicit P0 Unsupported");

    if (g_failures != 0)
        return 1;
    std::cout << "BindingsTypeMapTests: all checks passed\n";
    return 0;
}
