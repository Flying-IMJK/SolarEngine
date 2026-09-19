// BindingsCSharpGenerator.cpp
// Generates C# binding declarations using direct string building.

#include "CodeGenerator_BindingsCSharp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Core/FileSystem.h"
#include "Core/Utils.h"

#include <string>

#include "CodeGenerator_Utils.h"

namespace SE::BuildTool
{
    // Keep binding flow below focused on declarations. Shared C# source
    // composition helpers live in CodeGeneratorUtils.
    using CodeGeneratorUtils::AppendCSharpComment;
    using CodeGeneratorUtils::AppendCSharpLibraryImport;
    using CodeGeneratorUtils::GetPropertyName;
    using CodeGeneratorUtils::IsValidCSharpAttributeList;
    using CodeGeneratorUtils::MakeCSharpIdentifier;
    using CodeGeneratorUtils::WithoutArray;

    CSharpTypeConversion BindingsCSharpGenerator::ResolveConversion(const TypeInfo& cppType, std::string_view marshalAs,
                                                                    BindingUseSite useSite, BindingDirection direction) const
    {
        const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(m_Database, cppType, marshalAs);
        return ResolveCSharpTypeConversion(m_Database, semantics, useSite, direction);
    }

    std::string BindingsCSharpGenerator::GetCSharpPublicType(const TypeInfo& cppType, std::string_view marshalAs) const
    {
        return ResolveConversion(cppType, marshalAs).publicType;
    }

    std::string BindingsCSharpGenerator::GetCSharpFullTypeName(const TypeID& typeID) const
    {
        TypeInfoBase const* declaration = m_Database.GetType(typeID);
        ENGINE_ASSERT(declaration);
        // Native scripting roots are implementation details. Managed binding
        // classes share the single public SE.Object root.
        if (declaration->name == "ScriptingObject" || declaration->name == "ManagedScriptingObject")
            return "SE.Object";
        return GetManagedTypeName(*declaration);
    }

    std::string BindingsCSharpGenerator::GetCSharpFromInterop(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs) const
    {
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs);
        switch (conversion.kind)
        {
        case BindingTypeKind::String:
        case BindingTypeKind::StringView:
        case BindingTypeKind::Collection:
            return expression;
        case BindingTypeKind::ScriptingObject:
        case BindingTypeKind::ObjectRef:
        case BindingTypeKind::VariantFamily:
            return Utils::String::Format("({0})SE.Interop.ManagedHandleMarshaller.NativeToManaged.ConvertToManaged({1})",
                conversion.publicType, expression);
        case BindingTypeKind::TypeHandle:
            return Utils::String::Format("SE.Interop.SystemTypeMarshaller.ConvertToManaged({0})", expression);
        case BindingTypeKind::NativeObject:
            return Utils::String::Format("{0}.FromUnmanaged({1})", conversion.publicType, expression);
        case BindingTypeKind::InteropStruct:
            return Utils::String::Format("{0}.ConvertToManaged({1})", conversion.marshaller, expression);
        default:
            return expression;
        }
    }

    std::string BindingsCSharpGenerator::GetCSharpToInterop(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs) const
    {
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs);
        switch (conversion.kind)
        {
        case BindingTypeKind::String:
        case BindingTypeKind::StringView:
        case BindingTypeKind::Collection:
            return expression;
        case BindingTypeKind::ScriptingObject:
            return Utils::String::Format("Object.GetUnmanagedPtr({0})", expression);
        case BindingTypeKind::ObjectRef:
        case BindingTypeKind::VariantFamily:
            return Utils::String::Format("SE.Interop.ManagedHandleMarshaller.ManagedToNative.ConvertToUnmanaged({0})", expression);
        case BindingTypeKind::TypeHandle:
            return Utils::String::Format("SE.Interop.SystemTypeMarshaller.ConvertToUnmanaged({0})", expression);
        case BindingTypeKind::NativeObject:
            return Utils::String::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", expression);
        case BindingTypeKind::InteropStruct:
            return Utils::String::Format("{0}.ConvertToUnmanaged({1})", conversion.marshaller, expression);
        default:
            return expression;
        }
    }

    bool BindingsCSharpGenerator::UsePassByReference(const TypeInfo& cppType, std::string_view marshalAs) const
    {
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs);
        if (cppType.isPointer || conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView ||
            conversion.kind == BindingTypeKind::ScriptingObject || conversion.kind == BindingTypeKind::NativeObject ||
            conversion.kind == BindingTypeKind::ObjectRef || conversion.kind == BindingTypeKind::Collection ||
            conversion.kind == BindingTypeKind::VariantFamily || conversion.kind == BindingTypeKind::TypeHandle)
        {
            return false;
        }
        return cppType.isRef;
    }

    std::string BindingsCSharpGenerator::GetCSharpParamMarshalAttribute(const TypeInfo& cppType, const std::string& paramName,
                                                                        std::string_view marshalAs, BindingDirection direction) const
    {
        const CollectionInfo collection = GetCollectionInfo(cppType);
        if (collection.IsCollection())
        {
            if (collection.kind == CollectionKind::Fixed)
            {
                return Utils::String::Format("[MarshalUsing(typeof(SE.Interop.ArrayMarshaller<,>), ConstantElementCount = {0})]", collection.fixedElementCount);
            }
            return Utils::String::Format("[MarshalUsing(typeof(SE.Interop.ArrayMarshaller<,>), CountElementName = nameof(__{0}Count))]", paramName);
        }

        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs);
        if (cppType.typeID == TypeID("bool"))
        {
            return "[MarshalAs(UnmanagedType.U1)]";
        }
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
        {
            return "[MarshalUsing(typeof(SE.Interop.StringMarshaller))]";
        }
        return {};
    }

    std::string BindingsCSharpGenerator::GetCSharpReturnMarshalAttribute(const TypeInfo& cppType, std::string_view marshalAs) const
    {
        const CollectionInfo collection = GetCollectionInfo(cppType);
        if (collection.kind == CollectionKind::Fixed)
            return Utils::String::Format("[return: MarshalUsing(typeof(SE.Interop.ArrayMarshaller<,>), ConstantElementCount = {0})]", collection.fixedElementCount);
        if (collection.HasRuntimeCount())
            return "[return: MarshalUsing(typeof(SE.Interop.ArrayMarshaller<,>), CountElementName = nameof(__returnCount))]";
        if (cppType.typeID == TypeID("bool"))
        {
            return "[return: MarshalAs(UnmanagedType.U1)]";
        }
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs, BindingUseSite::Return, BindingDirection::Out);
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
        {
            return "[return: MarshalUsing(typeof(SE.Interop.StringMarshaller))]";
        }
        return {};
    }

    // -------------------------------------------------------------------------
    // C# ABI layout translation for interop-struct fields
    // -------------------------------------------------------------------------

    std::string BindingsCSharpGenerator::GetCSharpStructAbiFieldType(const TypeInfo& cppType, std::string_view marshalAs) const
    {
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs, BindingUseSite::Field);
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView ||
            conversion.kind == BindingTypeKind::Collection)
        {
            return "IntPtr";
        }
        if (cppType.typeID == TypeID("bool"))
        {
            return "byte";
        }

        if (conversion.kind != BindingTypeKind::InteropStruct)
        {
            return conversion.libraryImportManagedType;
        }

        const std::string publicType = conversion.publicType;
        const int separator = Utils::String::FindLast(publicType, '.');
        const std::string simpleName = separator == INVALID_INDEX ? publicType : publicType.substr(separator + 1);
        return Utils::String::Format("{0}.{1}Internal", conversion.marshaller, simpleName);
    }

    std::string BindingsCSharpGenerator::GetCSharpCollectionCountExpression(const TypeInfo& cppType, const std::string& expression) const
    {
        const std::string baseType = cppType.typeID.ToString();
        const bool usesCount = baseType == "Dictionary" || baseType == "HashSet";
        return Utils::String::Format("{0} != null ? {0}.{1} : 0", expression, usesCount ? "Count" : "Length");
    }

    std::string BindingsCSharpGenerator::GetCSharpStructFieldFromAbi(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs) const
    {
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs, BindingUseSite::Field);
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
        {
            return Utils::String::Format("Interop.StringMarshaller.ToManaged({0})", expression);
        }
        if (conversion.kind == BindingTypeKind::Collection)
        {
            const std::string elementType = GetCSharpPublicType(GetCollectionInfo(cppType).elementType);
            return Utils::String::Format(
                "{0} != IntPtr.Zero ? Unsafe.As<ManagedArray>(ManagedHandle.FromIntPtr({0}).Target).ToArray<{1}>() : null",
                expression, elementType);
        }
        if (cppType.typeID == TypeID("bool"))
        {
            return Utils::String::Format("{0} != 0", expression);
        }

        return conversion.kind == BindingTypeKind::InteropStruct
            ? Utils::String::Format("{0}.ConvertToManaged({1})", conversion.marshaller, expression)
            : GetCSharpFromInterop(cppType, expression, marshalAs);
    }

    std::string BindingsCSharpGenerator::GetCSharpStructFieldToAbi(const TypeInfo& cppType, const std::string& expression, std::string_view marshalAs) const
    {
        const CSharpTypeConversion conversion = ResolveConversion(cppType, marshalAs, BindingUseSite::Field);
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
            return Utils::String::Format("Interop.StringMarshaller.ManagedToNative.ConvertToUnmanaged({0})", expression);
        if (conversion.kind == BindingTypeKind::Collection)
        {
            return Utils::String::Format(
                "{0}?.Length > 0 ? ManagedHandle.ToIntPtr(ManagedArray.WrapNewArray({0}), GCHandleType.Weak) : IntPtr.Zero",
                expression);
        }
        if (cppType.typeID == TypeID("bool"))
            return Utils::String::Format("{0} ? (byte)1 : (byte)0", expression);

        return conversion.kind == BindingTypeKind::InteropStruct
            ? Utils::String::Format("{0}.ConvertToUnmanaged({1})", conversion.marshaller, expression)
            : GetCSharpToInterop(cppType, expression, marshalAs);
    }

    std::string BindingsCSharpGenerator::NormalizeCSharpDefaultValue(const TypeInfoParam& param)
    {
        std::string value = param.defaultValue;
        Utils::String::TrimStart(value);
        Utils::String::TrimEnd(value);
        if (value.empty())
        {
            return value;
        }

        // Clang joins source tokens with spaces, including scoped names.
        Utils::String::ReplaceAll(value, " :: ", "::");
        Utils::String::ReplaceAll(value, ":: ", "::");
        Utils::String::ReplaceAll(value, " ::", "::");

        if (value == "nullptr" || value == "NULL" ||
            value == "String::Empty" || value == "StringAnsi::Empty" ||
            value == "StringAnsiView::Empty" || value == "StringView::Empty" ||
            value == "SE::StringView::Empty" || value == "::SE::StringView::Empty")
        {
            return "null";
        }

        if (value == "Max_int8") return "sbyte.MaxValue";
        if (value == "Max_uint8") return "byte.MaxValue";
        if (value == "Max_int16") return "short.MaxValue";
        if (value == "Max_uint16") return "ushort.MaxValue";
        if (value == "Max_int32") return "int.MaxValue";
        if (value == "Max_uint32") return "uint.MaxValue";
        if (value == "Max_int64") return "long.MaxValue";
        if (value == "Max_uint64") return "ulong.MaxValue";
        if (value == "Max_float") return "float.MaxValue";
        if (value == "Max_double") return "double.MaxValue";


        // clang tokenization may separate the macro tokens as SE_TEXT ( "..." ).
        constexpr size_t textMacroLength = sizeof("SE_TEXT") - 1;
        if (value.compare(0, textMacroLength, "SE_TEXT") == 0)
        {
            const size_t openParen = value.find_first_not_of(" \t\r\n", textMacroLength);
            if (openParen != std::string::npos && value[openParen] == '(')
            {
                const size_t literalStart = value.find_first_not_of(" \t\r\n", openParen + 1);
                if (literalStart != std::string::npos && value[literalStart] == '"')
                {
                    size_t literalEnd = literalStart + 1;
                    while (literalEnd < value.size())
                    {
                        if (value[literalEnd] == '\\')
                        {
                            literalEnd += 2;
                            continue;
                        }
                        if (value[literalEnd] == '"')
                            break;
                        ++literalEnd;
                    }
                    if (literalEnd < value.size() && value[literalEnd] == '"')
                    {
                        const size_t closeParen = value.find_first_not_of(" \t\r\n", literalEnd + 1);
                        if (closeParen != std::string::npos && value[closeParen] == ')' &&
                            value.find_first_not_of(" \t\r\n", closeParen + 1) == std::string::npos)
                        {
                            return value.substr(literalStart, literalEnd - literalStart + 1);
                        }
                    }
                }
            }
        }

        int pos;
        while ((pos = Utils::String::Find(value, "::")) != INVALID_INDEX)
        {
            value = value.substr(0, pos) + "." + value.substr(pos + 2);
        }

        if (Utils::String::StartsWith(value, "Colors."))
        {
            value = "Color." + value.substr(7);
        }
        return value;
    }

    std::string BindingsCSharpGenerator::BuildCSharpParams(const TypeInfoFunc& fn, bool forPublic)
    {
        std::string params;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0) params += ", ";

            TypeInfo const paramType = fn.params[i].type;
            const std::string_view marshalAs = fn.params[i].marshalAs;
            const BindingDirection direction = GetBindingDirection(fn.params[i]);
            std::string type = forPublic ? GetCSharpPublicType(paramType, marshalAs)
                                         : ResolveConversion(paramType, marshalAs, BindingUseSite::Parameter, direction).libraryImportManagedType;

            // Pass by ref for non-interop
            if (forPublic && direction == BindingDirection::Ref)
            {
                params += Utils::String::Format("ref {0} {1}", type, MakeCSharpIdentifier(fn.params[i].name));
            }
            else if (forPublic && direction == BindingDirection::Out)
            {
                params += Utils::String::Format("out {0} {1}", type, MakeCSharpIdentifier(fn.params[i].name));
            }
            else
            {
                // Add marshal attribute for interop params
                if (!forPublic)
                {
                    std::string marshalAttr = GetCSharpParamMarshalAttribute(paramType, fn.params[i].name, marshalAs, direction);
                    if (!marshalAttr.empty())
                        params += Utils::String::Format("{0} ", marshalAttr);
                }
                std::string defaultValue;
                if (forPublic && !fn.params[i].defaultValue.empty() && direction == BindingDirection::In && IsCSharpOptionalConstant(fn.params[i]))
                {
                    defaultValue = Utils::String::Format(" = {0}", NormalizeCSharpDefaultValue(fn.params[i]));
                }
                params += Utils::String::Format("{0} {1}{2}", type, MakeCSharpIdentifier(fn.params[i].name), defaultValue);
            }
        }
        return params;
    }

    bool BindingsCSharpGenerator::IsCSharpOptionalConstant(const TypeInfoParam& param) const
    {
        if (param.defaultValue.empty())
            return false;

        std::string nativeValue = param.defaultValue;
        Utils::String::TrimStart(nativeValue);
        Utils::String::TrimEnd(nativeValue);


        if (nativeValue == "MAX_int8" || 
            nativeValue == "MAX_int16" || 
            nativeValue == "MAX_uint16" || 
            nativeValue == "MAX_int32" || 
            nativeValue == "MAX_uint32" || 
            nativeValue == "MAX_int64" ||
            nativeValue == "MAX_uint64" ||
            nativeValue == "MAX_float" ||
            nativeValue == "MAX_double")
        {
            return true;
        }

        // Preserve the existing overload for native string macro defaults.
        if (Utils::String::StartsWith(param.defaultValue, "SE_TEXT"))
        {
            return false;
        }

        const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(m_Database, param.type, param.marshalAs);
        if (semantics.isEnum)
            return true;

        const std::string value = NormalizeCSharpDefaultValue(param);
        if (value == "null" || value == "true" || value == "false" || value == "default")
            return true;
        if (!value.empty() && (value.front() == '\"' || value.front() == '\'' ||
            (value.front() >= '0' && value.front() <= '9') || value.front() == '+' || value.front() == '-'))
        {
            return true;
        }
        return false;
    }

    std::string BindingsCSharpGenerator::BuildCSharpInteropParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn)
    {
        std::string params;
        if (!fn.isStatic)
        {
            params += "IntPtr __obj";
        }

        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (params.length() > 0) params += ", ";
            TypeInfo const paramType = fn.params[i].type;
            const std::string_view marshalAs = fn.params[i].marshalAs;
            const BindingDirection direction = GetBindingDirection(fn.params[i]);
            std::string interopType = ResolveConversion(paramType, marshalAs, BindingUseSite::Parameter, direction).libraryImportManagedType;
            std::string marshalAttr = GetCSharpParamMarshalAttribute(paramType, fn.params[i].name, marshalAs, direction);

            if (!marshalAttr.empty())
            {
                params += Utils::String::Format("{0} ", marshalAttr);
            }

            if (direction == BindingDirection::Out)
            {
                params += "out ";
            }
            else if (direction == BindingDirection::Ref)
            {
                params += "ref ";
            }
            params += Utils::String::Format("{0} {1}", interopType, MakeCSharpIdentifier(fn.params[i].name));
            const CollectionInfo collection = GetCollectionInfo(paramType);
            if (collection.HasRuntimeCount())
            {
                const char* countDirection = direction == BindingDirection::Out ? "out "
                    : direction == BindingDirection::Ref ? "ref " : "";
                params += Utils::String::Format(", {0}int __{1}Count", countDirection, MakeCSharpIdentifier(fn.params[i].name));
            }
        }
        return params;
    }

    std::string BindingsCSharpGenerator::BuildCSharpCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool isInterop,
                                                             std::string* preCall, std::string* postCall, std::string* cleanup)
    {
        std::string args;
        if (!fn.isStatic && isInterop)
            args += "__unmanagedPtr";

        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (args.length() > 0) args += ", ";

            if (isInterop)
            {
                // Convert to interop representation
                std::string paramName = MakeCSharpIdentifier(fn.params[i].name);
                TypeInfo const paramType = fn.params[i].type;
                const CollectionInfo collection = GetCollectionInfo(paramType);
                const BindingDirection direction = GetBindingDirection(fn.params[i]);
                const CSharpTypeConversion conversion = ResolveConversion(
                    paramType, fn.params[i].marshalAs, BindingUseSite::Parameter, direction);
                const bool needsManualTemporary = direction != BindingDirection::In &&
                    conversion.strategy == InteropStrategy::ManualWrapper &&
                    conversion.publicType != conversion.libraryImportManagedType;
                std::string abiArgument = paramName;
                if (needsManualTemporary)
                {
                    abiArgument = Utils::String::Format("__{0}Abi", paramName);
                    if (preCall)
                    {
                        if (direction == BindingDirection::Out)
                            *preCall += Utils::String::Format("            {0} {1};\n", conversion.libraryImportManagedType, abiArgument);
                        else
                            *preCall += Utils::String::Format("            var {0} = {1};\n", abiArgument,
                                GetCSharpToInterop(paramType, paramName, fn.params[i].marshalAs));
                    }
                    if (postCall)
                        *postCall += Utils::String::Format("            {0} = {1};\n", paramName,
                            GetCSharpFromInterop(paramType, abiArgument, fn.params[i].marshalAs));
                }
                else if (direction == BindingDirection::In && !collection.IsCollection())
                {
                    abiArgument = GetCSharpToInterop(paramType, paramName, fn.params[i].marshalAs);
                }

                if (direction == BindingDirection::Out)
                    args += Utils::String::Format("out {0}", abiArgument);
                else if (direction == BindingDirection::Ref)
                    args += Utils::String::Format("ref {0}", abiArgument);
                else
                    args += abiArgument;
                if (collection.HasRuntimeCount())
                {
                    if (direction == BindingDirection::In)
                    {
                        args += Utils::String::Format(", {0}", GetCSharpCollectionCountExpression(paramType, paramName));
                    }
                    else
                    {
                        const std::string countName = Utils::String::Format("__{0}Count", paramName);
                        if (preCall)
                        {
                            if (direction == BindingDirection::Out)
                                *preCall += Utils::String::Format("            int {0};\n", countName);
                            else
                                *preCall += Utils::String::Format("            int {0} = {1};\n", countName,
                                    GetCSharpCollectionCountExpression(paramType, paramName));
                        }
                        args += Utils::String::Format(", {0} {1}",
                            direction == BindingDirection::Out ? "out" : "ref", countName);
                    }
                }
            }
            else
            {
                // Public call - forward as-is with ref/out keywords
                const BindingDirection direction = GetBindingDirection(fn.params[i]);
                if (direction == BindingDirection::Out)
                    args += Utils::String::Format("out {0}", MakeCSharpIdentifier(fn.params[i].name));
                else if (direction == BindingDirection::Ref)
                    args += Utils::String::Format("ref {0}", MakeCSharpIdentifier(fn.params[i].name));
                else
                    args += MakeCSharpIdentifier(fn.params[i].name);
            }
        }
        (void)cleanup;
        return args;
    }

    // -------------------------------------------------------------------------
    // [DllImport] declaration for a single function
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunction(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                                                const std::string& assemblyName, std::string& output)
    {
        TypeInfo const returnTypeInfo = fn.returnType;
        const CollectionInfo returnCollection = GetCollectionInfo(returnTypeInfo);
        const FunctionAbiPlan abiPlan = BuildFunctionAbiPlan(m_Database, cls, fn);
        std::string interopRetType = ResolveConversion(returnTypeInfo, fn.marshalAs, BindingUseSite::Return, BindingDirection::Out).libraryImportManagedType;
        std::string interopParams = BuildCSharpInteropParams(cls, fn);
        std::string returnMarshalAttr = GetCSharpReturnMarshalAttribute(returnTypeInfo, fn.marshalAs);
        std::string internalName = Utils::String::Format("Internal_{0}", fn.uniqueName);
        const bool useOutResult = abiPlan.usesHiddenResult;
        if (useOutResult)
        {
            if (!interopParams.empty())
            {
                interopParams += ", ";
            }
            std::string marshal = GetCSharpParamMarshalAttribute(returnTypeInfo, "__resultAsRef", fn.marshalAs, BindingDirection::Out);
            if (!marshal.empty())
            {
                interopParams += marshal + " ";
            }
            interopParams += Utils::String::Format("out {0} __resultAsRef", interopRetType);
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!interopParams.empty()) interopParams += ", ";
            interopParams += "out int __returnCount";
        }

        output += Utils::String::Format("        // SE ABI: {0}\n", abiPlan.fingerprint);
        AppendCSharpLibraryImport(output, assemblyName, fn.entryPoint);
        if (!useOutResult && !returnMarshalAttr.empty())
        {
            output += Utils::String::Format("        {0}\n", returnMarshalAttr);
        }
        output += Utils::String::Format("        internal static partial {0} {1}({2});\n\n", useOutResult ? "void" : interopRetType, internalName, interopParams);
    }

    // -------------------------------------------------------------------------
    // Public wrapper function call (calls the InternalCall)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunctionCall(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                                                     std::string& output)
    {
        TypeInfo const returnTypeInfo = fn.returnType;
        const CollectionInfo returnCollection = GetCollectionInfo(returnTypeInfo);
        std::string publicRetType = GetCSharpPublicType(returnTypeInfo, fn.marshalAs);
        std::string publicParams = BuildCSharpParams(fn, true);
        bool retIsVoid = fn.returnType.typeID == TypeInfo::Void.typeID;
        std::string access = CodeGeneratorUtils::GetAccessString(AccessLevel::Public);
        std::string staticKeyword = fn.isStatic ? "static " : "";

        // C# optional parameters only accept compile-time constants. Preserve
        // native defaults such as Colors::White with an overload that supplies
        // the value in the method body, while keeping the full ABI wrapper
        // explicit and unambiguous.
        int firstNonConstantDefault = INVALID_INDEX;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (!fn.params[i].defaultValue.empty() && !IsCSharpOptionalConstant(fn.params[i]))
            {
                firstNonConstantDefault = i;
                break;
            }
        }
        if (firstNonConstantDefault != INVALID_INDEX)
        {
            bool allTrailingHaveDefaults = true;
            for (int i = firstNonConstantDefault; i < fn.params.size(); ++i)
                allTrailingHaveDefaults &= !fn.params[i].defaultValue.empty();

            if (allTrailingHaveDefaults)
            {
                TypeInfoFunc overload = fn;
                overload.params.resize(firstNonConstantDefault);
                std::string overloadArgs;
                for (int i = 0; i < fn.params.size(); ++i)
                {
                    if (i > 0)
                        overloadArgs += ", ";
                    if (i < firstNonConstantDefault)
                    {
                        const BindingDirection direction = GetBindingDirection(fn.params[i]);
                        if (direction == BindingDirection::Ref)
                            overloadArgs += "ref ";
                        else if (direction == BindingDirection::Out)
                            overloadArgs += "out ";
                        overloadArgs += MakeCSharpIdentifier(fn.params[i].name);
                    }
                    else
                    {
                        overloadArgs += NormalizeCSharpDefaultValue(fn.params[i]);
                    }
                }

                output += Utils::String::Format("        {0} {1}{2} {3}({4})\n        {{\n",
                    access, staticKeyword, retIsVoid ? "void" : publicRetType,
                    MakeCSharpIdentifier(fn.name), BuildCSharpParams(overload, true));
                if (retIsVoid)
                    output += Utils::String::Format("            {0}({1});\n", MakeCSharpIdentifier(fn.name), overloadArgs);
                else
                    output += Utils::String::Format("            return {0}({1});\n", MakeCSharpIdentifier(fn.name), overloadArgs);
                output += "        }\n\n";
            }
        }

        AppendCSharpComment(output, "        ", fn.comment);
        if (IsValidCSharpAttributeList(fn.attributes))
            output += Utils::String::Format("        {0}\n", fn.attributes);
        output += Utils::String::Format("        {0} {1}{2}{3} {4}({5})\n",
            access, staticKeyword,
            retIsVoid ? "void" : publicRetType,
            std::string(" "), MakeCSharpIdentifier(fn.name), publicParams);
        output += "        {\n";

        std::string preCall;
        std::string postCall;
        std::string cleanup;
        std::string interopCallArgs = BuildCSharpCallArgs(cls, fn, true, &preCall, &postCall, &cleanup);
        if (returnCollection.HasRuntimeCount())
        {
            if (!interopCallArgs.empty())
                interopCallArgs += ", ";
            interopCallArgs += "out _";
        }
        std::string interopCall = Utils::String::Format("Internal_{0}({1})", fn.uniqueName, interopCallArgs);

        const bool useOutResult = BuildFunctionAbiPlan(m_Database, cls, fn).usesHiddenResult;
        output += preCall;
        if (useOutResult)
        {
            std::string callArgs = interopCallArgs;
            if (!callArgs.empty()) callArgs += ", ";
            callArgs += "out __resultAsRef";
            output += Utils::String::Format("            {0} __resultAsRef;\n", ResolveConversion(returnTypeInfo, fn.marshalAs, BindingUseSite::Return, BindingDirection::Out).libraryImportManagedType);
            output += Utils::String::Format("            Internal_{0}({1});\n", fn.uniqueName, callArgs);
            output += postCall;
            std::string fromInterop = GetCSharpFromInterop(returnTypeInfo, "__resultAsRef", fn.marshalAs);
            output += Utils::String::Format("            return {0};\n", fromInterop);
            output += "        }\n\n";
            return;
        }

        if (retIsVoid)
        {
            output += Utils::String::Format("            {0};\n", interopCall);
            output += postCall;
        }
        else
        {
            if (postCall.empty())
            {
                std::string fromInterop = GetCSharpFromInterop(returnTypeInfo, interopCall, fn.marshalAs);
                output += Utils::String::Format("            return {0};\n", fromInterop);
            }
            else
            {
                output += Utils::String::Format("            var __abiReturn = {0};\n", interopCall);
                output += postCall;
                output += Utils::String::Format("            return {0};\n",
                    GetCSharpFromInterop(returnTypeInfo, "__abiReturn", fn.marshalAs));
            }
        }
        output += cleanup;
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Shared accessor property generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpAccessorProperty(const TypeInfoStruct& cls, const BindingCallable* getter,
                                                                   const BindingCallable* setter, const std::string& publicName,
                                                                   const std::string& publicCppType, AccessLevel getterAccess,
                                                                   AccessLevel setterAccess, bool isStatic,
                                                                   const std::string& attributes, const std::string& comment,
                                                                   const std::string& assemblyName,
                                                                   std::string& output)
    {
        if (getter)
        {
            GenerateCSharpWrapperFunction(cls, getter->function, assemblyName, output);
        }
        if (setter)
        {
            GenerateCSharpWrapperFunction(cls, setter->function, assemblyName, output);
        }

        // NoProxy suppresses only the public facade. The interop declarations
        // above remain available to handwritten code in the generated assembly.
        const BindingCallable* publicGetter = getter && !getter->function.APINoProxy ? getter : nullptr;
        const BindingCallable* publicSetter = setter && !setter->function.APINoProxy ? setter : nullptr;
        if (!publicGetter && !publicSetter)
        {
            return;
        }

        const std::string publicType = publicGetter
            ? GetCSharpPublicType(publicGetter->function.returnType, publicGetter->function.marshalAs)
            : GetCSharpPublicType(publicSetter->function.params[0].type, publicSetter->function.params[0].marshalAs);
        const AccessLevel propertyAccess = publicGetter ? getterAccess : setterAccess;
        const std::string propertyAccessText = CodeGeneratorUtils::GetAccessString(propertyAccess);
        AppendCSharpComment(output, "        ", comment);
        if (IsValidCSharpAttributeList(attributes))
            output += Utils::String::Format("        {0}\n", attributes);
        output += Utils::String::Format("        {0} {1}{2} {3}\n        {{\n", propertyAccessText,
            isStatic ? "static " : "", publicType, MakeCSharpIdentifier(publicName));

        if (publicGetter)
        {
            const TypeInfoFunc& getterFunction = publicGetter->function;
            TypeInfo const getterType = getterFunction.returnType;
            const CollectionInfo getterCollection = GetCollectionInfo(getterType);
            const bool usesOutResult = BuildFunctionAbiPlan(m_Database, cls, getterFunction).usesHiddenResult;
            std::string callArgs = BuildCSharpCallArgs(cls, getterFunction, true);
            if (getterCollection.HasRuntimeCount())
            {
                if (!callArgs.empty())
                    callArgs += ", ";
                callArgs += "out _";
            }
            if (usesOutResult)
            {
                if (!callArgs.empty())
                    callArgs += ", ";
                callArgs += "out __resultAsRef";
                output += Utils::String::Format("            get {{ {0} __resultAsRef; Internal_{1}({2}); return {3}; }}\n",
                    ResolveConversion(getterType, getterFunction.marshalAs, BindingUseSite::Return, BindingDirection::Out).libraryImportManagedType, getterFunction.uniqueName, callArgs,
                    GetCSharpFromInterop(getterType, "__resultAsRef", getterFunction.marshalAs));
            }
            else
            {
                const std::string getterCall = Utils::String::Format("Internal_{0}({1})", getterFunction.uniqueName, callArgs);
                output += Utils::String::Format("            get {{ return {0}; }}\n", GetCSharpFromInterop(getterType, getterCall, getterFunction.marshalAs));
            }
        }

        if (publicSetter)
        {
            const TypeInfoFunc& setterFunction = publicSetter->function;
            TypeInfo setterType = setterFunction.params[0].type;

            const bool usesPointer = GetBindingDirection(setterFunction.params[0]) == BindingDirection::Ref;
            const std::string setterModifier = publicGetter && setterAccess != propertyAccess
                ? CodeGeneratorUtils::GetAccessString(setterAccess) + " " : "";
            const std::string instanceArg = setterFunction.isStatic ? "" : "__unmanagedPtr";
            const CollectionInfo setterCollection = GetCollectionInfo(setterType);
            const std::string toInterop = setterCollection.IsCollection() ? "value"
                : GetCSharpToInterop(setterType, "value", setterFunction.params[0].marshalAs);
            if (usesPointer)
            {
                output += Utils::String::Format("            {0}set {{ var __valueAsRef = {1}; Internal_{2}({3}{4}ref __valueAsRef); }}\n",
                    setterModifier, toInterop, setterFunction.uniqueName, instanceArg,
                    instanceArg.empty() ? "" : ", ");
            }
            else
            {
                std::string setterArgs = instanceArg;
                if (!setterArgs.empty()) setterArgs += ", ";
                setterArgs += toInterop;
                if (setterCollection.HasRuntimeCount())
                {
                    setterArgs += Utils::String::Format(", {0}", GetCSharpCollectionCountExpression(setterType, "value"));
                }
                if (setterCollection.kind == CollectionKind::Fixed)
                {
                    output += Utils::String::Format(
                        "            {0}set {{ if (value == null || value.Length != {1}) throw new ArgumentException(\"Expected exactly {1} elements.\", nameof(value)); Internal_{2}({3}); }}\n",
                        setterModifier, setterCollection.fixedElementCount, setterFunction.uniqueName, setterArgs);
                }
                else
                {
                    output += Utils::String::Format("            {0}set {{ Internal_{1}({2}); }}\n",
                        setterModifier, setterFunction.uniqueName, setterArgs);
                }
            }
        }
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Property and field adapters
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpPropertyAccessors(const TypeInfoStruct& cls, const TypeInfoFunc& prop,
                                                                   std::vector<bool>& consumedFunctions,
                                                                   int functionIndex, const std::string& assemblyName,
                                                                   std::string& output)
    {
        BindingCallable getter;
        BindingCallable setter;
        const BindingCallable* getterPtr = nullptr;
        const BindingCallable* setterPtr = nullptr;

        for (int i = functionIndex; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (consumedFunctions[i] || !fn.APIIsPropertie || GetPropertyName(fn) != GetPropertyName(prop))
            {
                continue;
            }

            consumedFunctions[i] = true;
            if (fn.returnType.typeID == TypeInfo::Void.typeID && fn.params.size() == 1)
            {
                setter = MakePropertySetter(cls, fn);
                setterPtr = &setter;
            }
            else
            {
                getter = MakePropertyGetter(cls, fn);
                getterPtr = &getter;
            }
        }

        if (getterPtr == nullptr && setterPtr == nullptr)
        {
            return;
        }

        TypeInfoFunc const& accessSource = getterPtr != nullptr ? getterPtr->function : setterPtr->function;
        std::string const publicCppType = getterPtr != nullptr ? getterPtr->function.returnType.ToString()
            : setterPtr->function.params[0].type.ToString();
        GenerateCSharpAccessorProperty(cls, getterPtr, setterPtr, GetPropertyName(prop), publicCppType, accessSource.access,
            accessSource.access, accessSource.isStatic, accessSource.attributes, accessSource.comment, assemblyName, output);
    }

    void BindingsCSharpGenerator::GenerateCSharpFieldAccessors(const TypeInfoStruct& cls,
                                                               const TypeInfoField&  field,
                                                               const std::string&    assemblyName,
                                                               std::string&          output)
    {
        TypeInfo const fieldTypeInfo = field.type;
        std::string const fieldCppType = fieldTypeInfo.ToString();
        const std::string publicType   = GetCSharpPublicType(fieldTypeInfo, field.marshalAs);

        // Classes own native state, so every exposed field is a native accessor
        // property. Struct instance fields remain direct layout members.
        if (field.isStatic || !cls.isStruct)
        {
            BindingCallable        getter = MakeBindingFieldGetter(cls, field);
            BindingCallable        setter;
            const BindingCallable* setterPtr = nullptr;
            if (!field.APIIsReadOnly)
            {
                setter    = MakeBindingFieldSetter(cls, field);
                setterPtr = &setter;
            }
            const AccessLevel fieldAccess = AccessLevel::Public;
            GenerateCSharpAccessorProperty(cls,
                                           &getter,
                                           setterPtr,
                                           field.name,
                                           fieldCppType,
                                           fieldAccess,
                                           fieldAccess,
                                           field.isStatic,
                                           field.attributes,
                                           field.comment,
                                           assemblyName,
                                           output);
            return;
        }

        if (fieldTypeInfo.arraySize > 0)
        {
            AppendCSharpComment(output, "        ", field.comment);
            output += Utils::String::Format(
                "        public {0} {1};\n\n", publicType, MakeCSharpIdentifier(field.name));
            return;
        }

        const std::string fieldAccess = CodeGeneratorUtils::GetAccessString(AccessLevel::Public);
        AppendCSharpComment(output, "        ", field.comment);
        if (IsValidCSharpAttributeList(field.attributes))
        {
            output += Utils::String::Format("        {0}\n", field.attributes);
        }

        std::string fieldDecl =
            Utils::String::Format("        {0} {1} {2}", fieldAccess, publicType, MakeCSharpIdentifier(field.name));
        if (fieldTypeInfo.typeID == TypeID("bool"))
        {
            fieldDecl = Utils::String::Format("        [MarshalAs(UnmanagedType.U1)]\n        {0} {1} {2}",
                                              fieldAccess,
                                              publicType,
                                              MakeCSharpIdentifier(field.name));
        }
        else if (ResolveConversion(fieldTypeInfo, field.marshalAs, BindingUseSite::Field).kind == BindingTypeKind::String)
        {
            fieldDecl = Utils::String::Format("        [MarshalAs(UnmanagedType.LPUTF8Str)]\n        {0} {1} {2}",
                                              fieldAccess,
                                              publicType,
                                              MakeCSharpIdentifier(field.name));
        }

        fieldDecl += ";\n\n";
        output += fieldDecl;
    }

    // -------------------------------------------------------------------------
    // Event accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpEventAccessors(const TypeInfoStruct& cls, const TypeInfoEvent& evt,
                                                                const std::string& assemblyName, std::string& output)
    {
        // Build the C# delegate type for the event
        std::string delegateParams;
        std::string delegateTypes;
        for (int i = 0; i < evt.params.size(); ++i)
        {
            if (i > 0) delegateParams += ", ";
            if (i > 0) delegateTypes += ", ";
            TypeInfo const paramType = evt.params[i].type;
            std::string publicType = GetCSharpPublicType(paramType, evt.params[i].marshalAs);
            std::string parameterName = MakeCSharpIdentifier(evt.params[i].name);
            delegateTypes += publicType;
            if (paramType.isRef && !paramType.isConst)
            {
                delegateParams += Utils::String::Format("ref {0} {1}", publicType, parameterName);
            }
            else
            {
                delegateParams += Utils::String::Format("{0} {1}", publicType, parameterName);
            }
        }

        // Native bool occupies one byte. Keep this signature consistent with
        // normal generated API calls instead of P/Invoke's default BOOL.
        AppendCSharpLibraryImport(output, assemblyName, Utils::String::Format("{0}_{1}_ManagedBind", cls.name, evt.name));
        if (cls.APIIsStatic)
        {
            output += Utils::String::Format("        internal static partial void Internal_{0}_Bind([MarshalAs(UnmanagedType.U1)] bool bind);\n\n", evt.name);
        }
        else
        {
            output += Utils::String::Format("        internal static partial void Internal_{0}_Bind(IntPtr __obj, [MarshalAs(UnmanagedType.U1)] bool bind);\n\n", evt.name);
        }

        bool useCustomDelegate = false;
        for (auto const& param : evt.params)
        {
            TypeInfo const paramType = param.type;
            if (paramType.isRef && !paramType.isConst)
            {
                useCustomDelegate = true;
                break;
            }
        }

        std::string actionType;
        if (useCustomDelegate)
        {
            actionType = Utils::String::Format("{0}Delegate", MakeCSharpIdentifier(evt.name));
        }
        else
        {
            actionType = "Action";
            if (!delegateTypes.empty())
                actionType += Utils::String::Format("<{0}>", delegateTypes);
        }
        std::string eventModifier = cls.APIIsStatic ? " static" : "";
        std::string managedArguments;
        std::string invokeParams;
        std::string invokePreparation;
        std::string invokeWriteBack;
        for (int i = 0; i < evt.params.size(); ++i)
        {
            if (i > 0)
            {
                managedArguments += ", ";
                invokeParams += ", ";
            }
            std::string parameterName = MakeCSharpIdentifier(evt.params[i].name);
            TypeInfo const paramType = evt.params[i].type;
            if (paramType.isRef && !paramType.isConst)
                invokeParams += "ref ";
            const BindingDirection direction = GetBindingDirection(evt.params[i]);
            std::string interopType = ResolveConversion(paramType, evt.params[i].marshalAs, BindingUseSite::Parameter, direction).libraryImportManagedType;
            std::string publicType = GetCSharpPublicType(paramType, evt.params[i].marshalAs);
            invokeParams += Utils::String::Format("{0} {1}", interopType, parameterName);
            if (paramType.isRef && !paramType.isConst)
            {
                if (publicType == interopType)
                {
                    managedArguments += Utils::String::Format("ref {0}", parameterName);
                }
                else
                {
                    std::string managedParameterName = Utils::String::Format("__managed_{0}", parameterName);
                    invokePreparation += Utils::String::Format("                {0} {1} = {2};\n", publicType, managedParameterName,
                        GetCSharpFromInterop(paramType, parameterName, evt.params[i].marshalAs));
                    invokeWriteBack += Utils::String::Format("                {0} = {1};\n", parameterName,
                        GetCSharpToInterop(paramType, managedParameterName, evt.params[i].marshalAs));
                    managedArguments += Utils::String::Format("ref {0}", managedParameterName);
                }
            }
            else
            {
                managedArguments += GetCSharpFromInterop(paramType, parameterName, evt.params[i].marshalAs);
            }
        }

        // Native bridge invokes this method with interop representations.

        // Action<T> keeps the common event API concise. A dedicated delegate is
        // required only for writable native references because Action<T> cannot
        // express C# ref parameters.
        if (useCustomDelegate)
        {
            output += Utils::String::Format("        public delegate void {0}({1});\n\n", actionType, delegateParams);
        }

        std::string evtAccess = CodeGeneratorUtils::GetAccessString(evt.access);
        output += Utils::String::Format("        private{1} int _{0}BindCount;\n", evt.name, eventModifier);
        output += Utils::String::Format("        private{1} readonly object _{0}Sync = new object();\n", evt.name, eventModifier);
        output += Utils::String::Format("        private{0} {1} _{2};\n\n", eventModifier, actionType, evt.name);

        // Event declaration with add/remove
        AppendCSharpComment(output, "        ", evt.comment);
        if (IsValidCSharpAttributeList(evt.attributes))
        {
            output += Utils::String::Format("        {0}\n", evt.attributes);
        }
        output += Utils::String::Format("        {0}{1} event {2} {3}\n        {{\n", evtAccess, eventModifier, actionType, evt.name);
        output += "            add\n            {\n";
        output += "                if (value == null) return;\n";
        output += Utils::String::Format("                lock (_{0}Sync)\n                {{\n", evt.name);
        output += Utils::String::Format("                    _{0} += value;\n", evt.name);
        output += Utils::String::Format("                    if (_{0}BindCount++ != 0) return;\n", evt.name);
        if (cls.APIIsStatic)
        {
            output += Utils::String::Format("                    Internal_{0}_Bind(true);\n", evt.name);
        }
        else
        {
            output += Utils::String::Format("                    Internal_{0}_Bind(__unmanagedPtr, true);\n", evt.name);
        }
        output += "                }\n";
        output += "            }\n";
        output += "            remove\n            {\n";
        output += "                if (value == null) return;\n";
        output += Utils::String::Format("                lock (_{0}Sync)\n                {{\n", evt.name);
        output += Utils::String::Format("                    var updated = ({0})Delegate.Remove(_{1}, value);\n", actionType, evt.name);
        output += Utils::String::Format("                    if (updated == _{0}) return;\n", evt.name);
        output += Utils::String::Format("                    _{0} = updated;\n", evt.name);
        output += Utils::String::Format("                    if (--_{0}BindCount != 0) return;\n", evt.name);
        if (cls.APIIsStatic)
        {
            output += Utils::String::Format("                    Internal_{0}_Bind(false);\n", evt.name);
        }
        else
        {
            output += Utils::String::Format("                    Internal_{0}_Bind(__unmanagedPtr, false);\n", evt.name);
        }
        output += "                }\n";
        output += "            }\n";
        output += "        }\n\n";

        output += Utils::String::Format("        internal{0} void Internal_{1}_Invoke({2})\n        {{\n",
            eventModifier, evt.name, invokeParams);
        output += Utils::String::Format("            {0} handler;\n", actionType);
        output += Utils::String::Format("            lock (_{0}Sync) handler = _{0};\n", evt.name);
        output += "            if (handler != null)\n            {\n";
        output += invokePreparation;
        output += Utils::String::Format("                handler.Invoke({0});\n", managedArguments);
        output += invokeWriteBack;
        output += "            }\n";
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Class marshaller (ManagedHandleMarshaller for ScriptingObject types)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpClassMarshaller(std::string& name,
                                                                std::string& marshallerName,
                                                                std::string& output)
    {
        // Mode wiring mirrors the struct marshaller: in-modes convert managed to
        // native, out-modes convert native to managed, ref-modes are bidirectional.
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedIn, typeof({1}.ManagedToNative))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedOut, typeof({1}.NativeToManaged))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementIn, typeof({1}.ManagedToNative))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedOut, typeof({1}.NativeToManaged))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedIn, typeof({1}.NativeToManaged))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementOut, typeof({1}.NativeToManaged))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedRef, typeof({1}.Bidirectional))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedRef, typeof({1}.Bidirectional))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementRef, typeof({1}))]\n", name, marshallerName);

        output += Utils::String::Format("        internal struct {0}Marshaller\n        {{\n", name);

        output += "            #pragma warning disable 1591\n";
        output += "            public static class NativeToManaged\n";
        output += "            {\n";
        output += Utils::String::Format("                public static {0} ConvertToManaged(IntPtr unmanaged) => Unsafe.As<{0}>(ManagedHandleMarshaller.NativeToManaged.ConvertToManaged(unmanaged));\n", name);
        output += Utils::String::Format("                public static IntPtr ConvertToUnmanaged({0} managed) => ManagedHandleMarshaller.ManagedToNative.ConvertToUnmanaged(managed);\n", name);
        output += Utils::String::Format("                public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.NativeToManaged.Free(unmanaged);\n", name);
        output += "            }\n";


        output += "            public static class ManagedToNative\n";
        output += "            {\n";
        output += Utils::String::Format("                public static {0} ConvertToManaged(IntPtr unmanaged) => Unsafe.As<{0}>(ManagedHandleMarshaller.NativeToManaged.ConvertToManaged(unmanaged));\n", name);
        output += Utils::String::Format("                public static IntPtr ConvertToUnmanaged({0} managed) => ManagedHandleMarshaller.ManagedToNative.ConvertToUnmanaged(managed);\n", name);
        output += Utils::String::Format("                public static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.NativeToManaged.Free(unmanaged);\n", name);
        output += "            }\n";

        output += "            public struct Bidirectional\n";
        output += "            {\n";
        output += "                ManagedHandleMarshaller.Bidirectional marsh;\n";
        output += Utils::String::Format("                public void FromManaged({0} managed) => marsh.FromManaged(managed);\n", name);
        output += "                public IntPtr ToUnmanaged() => marsh.ToUnmanaged();\n";
        output += "                public void FromUnmanaged(IntPtr unmanaged) => marsh.FromUnmanaged(unmanaged);\n";
        output += Utils::String::Format("                public {0} ToManaged() => Unsafe.As<{0}>(marsh.ToManaged());\n", name);
        output += "                public void Free() => marsh.Free();\n";
        output += "            }\n";
        output += "        #pragma warning restore 1591\n";
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Struct marshaller (CustomMarshaller with blittable internal)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpStructMarshaller(const TypeInfoStruct& cls, std::string& output)
    {
        // Match the .NET source-generated P/Invoke marshalling modes used by
        // Flax. MarshalMode.Default alone requires runtime marshalling and is
        // insufficient once the generated assembly disables it.
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedIn, typeof({0}Marshaller.ManagedToNative))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedOut, typeof({0}Marshaller.ManagedToNative))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementIn, typeof({0}Marshaller.ManagedToNative))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedOut, typeof({0}Marshaller.NativeToManaged))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedIn, typeof({0}Marshaller.NativeToManaged))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementOut, typeof({0}Marshaller.NativeToManaged))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedRef, typeof({0}Marshaller.Bidirectional))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedRef, typeof({0}Marshaller.Bidirectional))]\n", cls.name);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementRef, typeof({0}Marshaller))]\n", cls.name);
        output += Utils::String::Format("        internal static partial class {0}Marshaller\n        {{\n", cls.name);

        // ABI representation. It intentionally never embeds native C++ String
        // objects; strings are managed CLR handles (IntPtr) at this boundary.
        output += "            [StructLayout(LayoutKind.Sequential)]\n";
        output += Utils::String::Format("            internal unsafe struct {0}Internal\n            {{\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            TypeInfo const fieldTypeInfo = field.type;
            TypeInfo const fieldElementType = WithoutArray(fieldTypeInfo);
            std::string fieldType = GetCSharpStructAbiFieldType(fieldElementType, field.marshalAs);
            if (fieldTypeInfo.arraySize > 0)
                output += Utils::String::Format("                public fixed {0} {1}[{2}];\n", fieldType, field.name, fieldTypeInfo.arraySize);
            else
                output += Utils::String::Format("                public {0} {1};\n", fieldType, field.name);
        }
        output += "            }\n\n";

        // Unmanaged -> managed
        output += Utils::String::Format("            public static {0} ConvertToManaged({0}Internal unmanaged)\n            {{\n", cls.name);
        output += Utils::String::Format("                var result = new {0}();\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            TypeInfo const fieldTypeInfo = field.type;
            TypeInfo const fieldElementType = WithoutArray(fieldTypeInfo);
            if (fieldTypeInfo.arraySize > 0)
            {
                output += Utils::String::Format("                result.{0} = new {1}[{2}];\n", field.name,
                    GetCSharpPublicType(fieldElementType, field.marshalAs), fieldTypeInfo.arraySize);
                output += Utils::String::Format("                for (int i = 0; i < {0}; i++) result.{1}[i] = {2};\n",
                    fieldTypeInfo.arraySize, field.name, GetCSharpStructFieldFromAbi(fieldElementType,
                        Utils::String::Format("unmanaged.{0}[i]", field.name), field.marshalAs));
            }
            else
            {
                std::string fromInterop = GetCSharpStructFieldFromAbi(fieldTypeInfo, Utils::String::Format("unmanaged.{0}", field.name), field.marshalAs);
                output += Utils::String::Format("                result.{0} = {1};\n", field.name, fromInterop);
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        // Managed -> unmanaged
        output += Utils::String::Format("            public static {0}Internal ConvertToUnmanaged({0} managed)\n            {{\n", cls.name);
        output += Utils::String::Format("                var result = new {0}Internal();\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            TypeInfo const fieldTypeInfo = field.type;
            TypeInfo const fieldElementType = WithoutArray(fieldTypeInfo);
            if (fieldTypeInfo.arraySize > 0)
            {
                output += Utils::String::Format("                if (managed.{0} != null)\n                {{\n", field.name);
                output += Utils::String::Format("                    int __{0}Count = Math.Min(managed.{0}.Length, {1});\n", field.name, fieldTypeInfo.arraySize);
                output += Utils::String::Format("                    for (int i = 0; i < __{0}Count; i++) result.{0}[i] = {1};\n",
                    field.name, GetCSharpStructFieldToAbi(fieldElementType,
                        Utils::String::Format("managed.{0}[i]", field.name), field.marshalAs));
                output += "                }\n";
            }
            else
            {
                std::string toInterop = GetCSharpStructFieldToAbi(fieldTypeInfo, Utils::String::Format("managed.{0}", field.name), field.marshalAs);
                output += Utils::String::Format("                result.{0} = {1};\n", field.name, toInterop);
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        output += Utils::String::Format("            public static void Free({0}Internal unmanaged)\n            {{\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(m_Database, field.type, field.marshalAs);
            if (semantics.kind == BindingTypeKind::Collection && semantics.collection.kind != CollectionKind::Fixed)
            {
                output += Utils::String::Format(
                    "                if (unmanaged.{0} != IntPtr.Zero)\n"
                    "                {{\n"
                    "                    ManagedHandle handle = ManagedHandle.FromIntPtr(unmanaged.{0});\n"
                    "                    Unsafe.As<ManagedArray>(handle.Target).Free();\n"
                    "                    handle.Free();\n"
                    "                }}\n",
                    field.name);
            }
        }
        output += "            }\n\n";

        output += "            public static class ManagedToNative\n            {\n";
        output += Utils::String::Format("                public static {0} ConvertToManaged({0}Internal unmanaged) => {0}Marshaller.ConvertToManaged(unmanaged);\n", cls.name);
        output += Utils::String::Format("                public static {0}Internal ConvertToUnmanaged({0} managed) => {0}Marshaller.ConvertToUnmanaged(managed);\n", cls.name);
        output += Utils::String::Format("                public static void Free({0}Internal unmanaged) => {0}Marshaller.Free(unmanaged);\n", cls.name);
        output += "            }\n\n";

        output += "            public static class NativeToManaged\n            {\n";
        output += Utils::String::Format("                public static {0} ConvertToManaged({0}Internal unmanaged) => {0}Marshaller.ConvertToManaged(unmanaged);\n", cls.name);
        output += Utils::String::Format("                public static {0}Internal ConvertToUnmanaged({0} managed) => {0}Marshaller.ConvertToUnmanaged(managed);\n", cls.name);
        output += Utils::String::Format("                public static void Free({0}Internal unmanaged) => {0}Marshaller.Free(unmanaged);\n", cls.name);
        output += "            }\n\n";

        output += "            public ref struct Bidirectional\n            {\n";
        output += Utils::String::Format("                private {0}Internal unmanaged;\n", cls.name);
        output += Utils::String::Format("                private {0} managed;\n\n", cls.name);
        output += Utils::String::Format("                public Bidirectional({0} managed)\n                {{\n                    this.managed = managed;\n                    unmanaged = {0}Marshaller.ConvertToUnmanaged(managed);\n                }}\n\n", cls.name);
        output += Utils::String::Format("                public void FromManaged({0} managed) => this.managed = managed;\n", cls.name);
        output += Utils::String::Format("                public {0}Internal ToUnmanaged() {{ unmanaged = {0}Marshaller.ConvertToUnmanaged(managed); return unmanaged; }}\n", cls.name);
        output += Utils::String::Format("                public void FromUnmanaged({0}Internal unmanaged) => this.unmanaged = unmanaged;\n", cls.name);
        output += Utils::String::Format("                public {0} ToManaged()\n                {{\n                    managed = {0}Marshaller.ConvertToManaged(unmanaged);\n                    return managed;\n                }}\n", cls.name);
        output += Utils::String::Format("                public void Free() => {0}Marshaller.Free(unmanaged);\n            }}\n", cls.name);

        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpClass(const TypeInfoStruct& cls, const std::string& assemblyName, std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceScopeList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        // Class declaration
        std::string className       = cls.APIName.empty() ? cls.name : cls.APIName;
        std::string classKeyword = cls.APIIsStatic ? "static " : "";
        std::string sealedKeyword = cls.APIIsSealed ? "sealed " : "";
        std::string abstractKeyword = cls.APIIsAbstract ? "abstract " : "";

        AppendCSharpComment(output, "    ", cls.comment);
        // User attributes
        if (IsValidCSharpAttributeList(cls.APIAttributes))
        {
            output += Utils::String::Format("    {0}\n", cls.APIAttributes);
        }

        std::string marshallerName = "";
        if (!cls.APIIsStatic)
        {
            marshallerName = className + "Marshaller";
            output += Utils::String::Format("    [NativeMarshalling(typeof({0}))]\n", marshallerName);
        }

        output += Utils::String::Format("    public unsafe {0}{1}{2}partial class {3}",
                                        abstractKeyword,
                                        classKeyword,
                                        sealedKeyword,
                                        MakeCSharpIdentifier(className));

        // Base class
        if (cls.parentTypeID != TypeID::Invalid)
        {
            output += Utils::String::Format(" : {0}", GetCSharpFullTypeName(cls.parentTypeID));
        }

        // Interface implementations
        if (!cls.interfaces.empty())
        {
            if (cls.baseClassName.empty())
                output += " : ";
            else
                output += ", ";
            for (int i = 0; i < cls.interfaces.size(); ++i)
            {
                if (i > 0) output += ", ";
                output += cls.interfaces[i]->name;
            }
        }

        output += "\n    {\n";

        // Constructor (if not abstract, not static, not noConstructor)
        if (!cls.APIIsStatic && !cls.APINoConstructor)
        {
            output += "        /// <summary>\n";
            output += Utils::String::Format("        /// Initializes a new instance of the <see cref=\"{0}\"/>.\n",
                                            className);
            output += "        /// </summary>\n";
            output += Utils::String::Format("        {0} {1}() : base()\n",
                                            cls.APIIsAbstract ? "protected" : "public",
                                            MakeCSharpIdentifier(className));
            output += "        {\n";
            output += "        }\n\n\n";
        }

        // Events
        for (auto& evt : cls.events)
        {
            GenerateCSharpEventAccessors(cls, evt, assemblyName, output);
        }

        std::vector<bool> consumedFunctions(cls.functions.size(), false);
        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (!fn.APIIsPropertie || consumedFunctions[i])
            {
                continue;
            }
            GenerateCSharpPropertyAccessors(cls, fn, consumedFunctions, i, assemblyName, output);
        }

        // Functions - first generate all [LibraryImport] declarations, then public wrappers
        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (fn.APIIsPropertie) continue;
            GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);
        }

        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (fn.APINoProxy || fn.APIIsPropertie) continue;

            GenerateCSharpWrapperFunctionCall(cls, fn, output);
        }

        // Fields
        for (auto& field : cls.fields)
        {
            GenerateCSharpFieldAccessors(cls, field, assemblyName, output);
        }

        // Marshaller
        if (!marshallerName.empty())
        {
            GenerateCSharpClassMarshaller(className, marshallerName, output);
        }

        output += "    }\n";

        if (!nsName.empty())
        {
            output += "}\n";
        }
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Structure generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::OpenCSharpContainingTypeScopes(const TypeInfoBase& type, std::string& output) const
    {
        std::vector<std::string> parentScopes;
        parentScopes.reserve(type.structScopeList.size());
        for (const std::string& scopeName : type.structScopeList)
        {
            const std::string fullNativeName = CodeGeneratorUtils::GetFullNativeName(
                type.namespaceScopeList, parentScopes, scopeName);
            TypeInfoBase const* declaration = m_Database.GetType(TypeID(fullNativeName));
            ENGINE_ASSERT(declaration && declaration->IsFlag(TypeInfoBase::Flag::IsClassStruct));

            auto const* containingType = static_cast<TypeInfoStruct const*>(declaration);
            const std::string managedName = containingType->APIName.empty() ? containingType->name : containingType->APIName;
            const std::string typeKeyword = containingType->APIIsInterface
                ? "interface" : (containingType->isStruct ? "struct" : "class");
            const std::string abstractKeyword = !containingType->APIIsInterface && containingType->APIIsAbstract ? "abstract " : "";
            const std::string staticKeyword = !containingType->APIIsInterface && containingType->APIIsStatic ? "static " : "";
            const std::string sealedKeyword = !containingType->APIIsInterface && containingType->APIIsSealed ? "sealed " : "";

            output += Utils::String::Format("    public unsafe {0}{1}{2}partial {3} {4}\n    {{\n",
                abstractKeyword,
                staticKeyword,
                sealedKeyword,
                typeKeyword,
                MakeCSharpIdentifier(managedName));
            parentScopes.push_back(scopeName);
        }
    }

    void BindingsCSharpGenerator::CloseCSharpContainingTypeScopes(const TypeInfoBase& type, std::string& output)
    {
        for (size_t i = 0; i < type.structScopeList.size(); ++i)
            output += "    }\n";
    }

    void BindingsCSharpGenerator::GenerateCSharpStructure(const TypeInfoStruct& cls, const std::string& assemblyName,
                                                            std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceScopeList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        OpenCSharpContainingTypeScopes(cls, output);

        // Marshaller (must come before the struct for CustomMarshaller attribute)
        GenerateCSharpStructMarshaller(cls, output);

        // Struct declaration
        AppendCSharpComment(output, "    ", cls.comment);
        output += Utils::String::Format("    [StructLayout(LayoutKind.Sequential)]\n");
        output += Utils::String::Format("    [NativeMarshalling(typeof({0}Marshaller))]\n", MakeCSharpIdentifier(cls.name));
        if (IsValidCSharpAttributeList(cls.APIAttributes))
        {
            output += Utils::String::Format("    {0}\n", cls.APIAttributes);
        }

        output += Utils::String::Format("    public unsafe partial struct {0}", MakeCSharpIdentifier(cls.name));

        if (cls.parentTypeID != TypeID::Invalid)
        {
            TypeInfoBase const* parent = m_Database.GetType(cls.parentTypeID);
            if (parent && parent->isAPI && parent->IsFlag(TypeInfoBase::Flag::IsClassStruct) &&
                static_cast<TypeInfoStruct const*>(parent)->APIIsInterface)
            {
                output += Utils::String::Format(" : {0}", GetCSharpFullTypeName(cls.parentTypeID));
            }
        }

        output += "\n    {\n";

        // Fields
        for (auto& field : cls.fields)
        {
            GenerateCSharpFieldAccessors(cls, field, assemblyName, output);
        }

        std::vector<bool> consumedFunctions(cls.functions.size(), false);
        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (!fn.APIIsPropertie || consumedFunctions[i])
            {
                continue;
            }
            GenerateCSharpPropertyAccessors(cls, fn, consumedFunctions, i, assemblyName, output);
        }

        // Functions - LibraryImport + public wrappers
        for (auto& fn : cls.functions)
        {
            if (!fn.APIIsPropertie)
                GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);
        }

        for (auto& fn : cls.functions)
        {
            if (!fn.APINoProxy && !fn.APIIsPropertie)
                GenerateCSharpWrapperFunctionCall(cls, fn, output);
        }

        output += "    }\n";

        CloseCSharpContainingTypeScopes(cls, output);

        if (!nsName.empty())
        {
            output += "}\n";
        }
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Enum generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpEnum(const TypeInfoEnum& en, std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(en.namespaceScopeList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        // Enum declaration
        AppendCSharpComment(output, "    ", en.comment);
        if (IsValidCSharpAttributeList(en.APIAttributes))
        {
            output += Utils::String::Format("    {0}\n", en.APIAttributes);
        }

        // Map underlying type
        std::string csUnderlyingType;
        std::string stripped = GetEnumUnderlyingTypeName(en.underlyingType);
        if (stripped == "uint8")   csUnderlyingType = "byte";
        else if (stripped == "int8")    csUnderlyingType = "sbyte";
        else if (stripped == "uint16")  csUnderlyingType = "ushort";
        else if (stripped == "int16")   csUnderlyingType = "short";
        else if (stripped == "uint32")  csUnderlyingType = "uint";
        else if (stripped == "int32")   csUnderlyingType = "int";
        else if (stripped == "uint64")  csUnderlyingType = "ulong";
        else if (stripped == "int64")   csUnderlyingType = "long";
        else                            csUnderlyingType = "int"; // default

        output += Utils::String::Format("    public enum {0} : {1}\n    {{\n", MakeCSharpIdentifier(en.name), csUnderlyingType);

        for (int i = 0; i < en.enumConstants.size(); ++i)
        {
            if (!en.enumConstants[i].description.empty())
                AppendCSharpComment(output, "        ", en.enumConstants[i].description);
            output += Utils::String::Format("        {0} = {1}", MakeCSharpIdentifier(en.enumConstants[i].label), en.enumConstants[i].value);
            if (i < en.enumConstants.size() - 1)
                output += ",";
            output += "\n";
        }

        output += "    }\n";

        if (!nsName.empty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Interface generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpInterface(const TypeInfoStruct& iface, std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(iface.namespaceScopeList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        AppendCSharpComment(output, "    ", iface.comment);
        if (IsValidCSharpAttributeList(iface.APIAttributes))
        {
            output += Utils::String::Format("    {0}\n", iface.APIAttributes);
        }

        output += Utils::String::Format("    public unsafe partial interface {0}\n    {{\n", MakeCSharpIdentifier(iface.name));

        // Function signatures
        for (auto& fn : iface.functions)
        {
            TypeInfo const returnTypeInfo = fn.returnType;
            std::string publicRetType = GetCSharpPublicType(returnTypeInfo, fn.marshalAs);
            std::string publicParams = BuildCSharpParams(fn, true);
            AppendCSharpComment(output, "        ", fn.comment);
            output += Utils::String::Format("        {0} {1}({2});\n",
                returnTypeInfo.typeID == TypeInfo::Void.typeID ? "void" : publicRetType,
                MakeCSharpIdentifier(fn.name), publicParams);
        }

        output += "    }\n\n";

        // Interface Marshaller
        output += Utils::String::Format("    internal struct {0}Marshaller\n    {{\n", iface.name);
        output += Utils::String::Format("        public static {0} ConvertToManaged(IntPtr unmanaged) => default;\n", iface.name);
        output += Utils::String::Format("        public static IntPtr ConvertToUnmanaged({0} managed) => IntPtr.Zero;\n", iface.name);
        output += "    }\n";

        if (!nsName.empty())
        {
            output += "}\n";
        }
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Generate - entry point for a single header
    // -------------------------------------------------------------------------

    bool BindingsCSharpGenerator::Generate(const BindingsHeaderInfo& headerInfo,
                                            const std::string& solutionRoot)
    {
        std::vector<TypeInfoInjectedCode*> injectCodes;
        for (auto const& code : headerInfo.injectedCode)
        {
            if (code != nullptr && code->lang == InjectEnum::CS)
            {
                injectCodes.emplace_back(code);
            }
        }

        if (headerInfo.classes.empty() && headerInfo.enums.empty() && headerInfo.interfaces.empty())
        {
            return true;
        }

        std::string t;
        if (Utils::String::Contains(headerInfo.filePath, "WindowBase"))
        {
            t = "AssetContent";
        }

        std::string output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - do not edit manually.\n";
        output += Utils::String::Format("// Source: {0}\n", headerInfo.filePath);
        output += "//-------------------------------------------------------------------------\n";
        output += "#pragma warning disable CS0108\n";
        output += "#pragma warning disable CS8603\n";
        output += "#pragma warning disable CS8625\n";
        output += "using System;\n";
        output += "using System.ComponentModel;\n";
        output += "using System.Runtime.CompilerServices;\n";
        output += "using System.Runtime.InteropServices;\n";
        output += "using System.Runtime.InteropServices.Marshalling;\n";
        output += "using SE.Interop;\n";

        for (auto const& code : injectCodes)
        {
            output += code->code;
            if (!Utils::String::EndsWith(output, '\n'))
            {
                output += "\n";
            }
        }
        output += "\n";

        // Generate enums first
        for (auto& en : headerInfo.enums)
        {
            GenerateCSharpEnum(*en, output);
        }

        // Generate interfaces
        for (auto& iface : headerInfo.interfaces)
        {
            if (!iface->APIInBuildMapType.empty())
            {
                continue;
            }
            GenerateCSharpInterface(*iface, output);
        }

        // Generate classes/structs
        for (auto& cls : headerInfo.classes)
        {
            if (!cls->APIInBuildMapType.empty())
            {
                continue;
            }
            if (cls->isStruct)
            {
                GenerateCSharpStructure(*cls, headerInfo.assemblyName, output);
            }
            else
            {
                GenerateCSharpClass(*cls, headerInfo.assemblyName, output);
            }
        }

        std::string baseName = FileSystem::GetFileNameWithoutExtension(headerInfo.filePath);
        std::string assemblyDir = headerInfo.assemblyDir;
        std::string outDir = Utils::String::Format("{0}/{1}", assemblyDir, Settings::g_autogeneratedDirectory);
        FileSystem::NormalizePath(outDir);
        if (!m_GeneratedFiles && !FileSystem::DirectoryExists(outDir))
        {
            FileSystem::CreateDirectory(outDir);
        }

        std::string outPath = outDir + "/" + baseName + ".CSharp.cs";
        if (m_GeneratedFiles)
        {
            m_GeneratedFiles->push_back({ outPath, std::move(output) });
            return true;
        }
        return CodeGeneratorUtils::SaveFile(outPath, std::string(output.c_str()));
    }

} // namespace SE::BuildTool
