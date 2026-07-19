
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "CodeGenerator_Utils.h"

#include "Core/Utils.h"

namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static bool IsInjectedCppCode(ApiInjectedCode const& code)
    {
        return Utils::String::ToLowerCopy(code.lang) == "cpp";
    }

    static bool HasApiTag(const ApiClass& cls, const std::string& tag)
    {
        return !cls.tag.empty() && Utils::String::Find(cls.tag, tag) != INVALID_INDEX;
    }

    static std::string GetCppNativeSimpleName(const ApiClass& cls)
    {
        return cls.nativeName.empty() ? cls.name : cls.nativeName;
    }

    static std::string GetCppNativeTypeName(const ApiClass& cls)
    {
        return CodeGeneratorUtils::GetNativeName(cls.namespaceNameList, cls.structScopeList, GetCppNativeSimpleName(cls));
    }

    static std::string GetCppNativeInvokeTypeName(const ApiClass& cls)
    {
        std::string nativeName = HasApiTag(cls, "NativeInvokeUseName") ? cls.name : GetCppNativeSimpleName(cls);
        return CodeGeneratorUtils::GetNativeName(cls.namespaceNameList, cls.structScopeList, nativeName);
    }

    static bool IsNativePointer(const std::string& cppType)
    {
        std::string type = cppType;
        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);
        return !type.empty() && type.back() == '*';
    }

    std::string BindingsCppGenerator::GetInteropReturnType(const ApiFunction& fn) const
    {
        std::string stripped = StripTypeQualifiers(fn.returnType);
        const TypeMapping* map = FindTypeMapping(stripped.c_str());
        if (fn.returnType == "void")
            return "void";
        if (map && map->isString)
            return "void*";
        if (IsScriptingObjectPointer(fn.returnType))
            return "void*";
        return CodeGeneratorUtils::QualifyCppType(fn.returnType);
    }

    std::string BindingsCppGenerator::GetInteropParamType(const ApiParam& param) const
    {
        std::string stripped = StripTypeQualifiers(param.cppType);
        const TypeMapping* map = FindTypeMapping(stripped.c_str());
        if (map && map->isString)
            return "void*";
        if (IsScriptingObjectPointer(param.cppType))
            return "void*";
        return CodeGeneratorUtils::QualifyCppType(param.cppType);
    }

    std::string BindingsCppGenerator::GetNativeToManagedConvert(const std::string& cppType, const std::string& expr) const
    {
        std::string stripped = StripTypeQualifiers(cppType);
        const TypeMapping* map = FindTypeMapping(stripped.c_str());
        if (map && map->isString)
        {
            return Utils::String::Format("SEUtils::ToManagedString({0})", expr);
        }
        if (IsScriptingObjectPointer(cppType))
        {
            // API headers frequently forward-declare a scripting object return
            // type. Use an explicit base-pointer reinterpret cast so the stub
            // does not require the concrete type definition merely to emit the
            // managed handle conversion.
            return Utils::String::Format("ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>({0}))", expr);
        }
        return expr;
    }

    std::string BindingsCppGenerator::GetManagedToNativeConvert(const std::string& cppType, const std::string& expr) const
    {
        std::string stripped = StripTypeQualifiers(cppType);
        const TypeMapping* map = FindTypeMapping(stripped.c_str());
        if (map && map->isString)
            return Utils::String::Format("SEUtils::ToString({0})", expr);
        if (IsScriptingObjectPointer(cppType))
            return Utils::String::Format("({0}*){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        if (IsNativePointer(cppType))
            return Utils::String::Format("({0}*){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        return expr;
    }

    std::string BindingsCppGenerator::GetNativeToVariantConvert(const std::string& cppType, const std::string& expr) const
    {
        std::string stripped = StripTypeQualifiers(cppType);
        if (stripped == "bool" || stripped == "int32" || stripped == "uint32"
            || stripped == "int64" || stripped == "uint64" || stripped == "float"
            || stripped == "double")
            return Utils::String::Format("Variant({0})", expr);
        if (IsScriptingObjectPointer(cppType))
            return Utils::String::Format("Variant((ScriptingObject*){0})", expr);
        return Utils::String::Format("Variant({0})", expr);
    }

    std::string BindingsCppGenerator::GetVariantToNativeConvert(const std::string& cppType, const std::string& expr) const
    {
        std::string stripped = StripTypeQualifiers(cppType);
        if (stripped == "Variant" || stripped == "VariantType")
            return expr;
        if (stripped == "String")
            return Utils::String::Format("(StringView){0}", expr);
        if (stripped == "StringAnsi")
            return Utils::String::Format("(StringAnsiView){0}", expr);
        const TypeMapping* map = FindTypeMapping(stripped.c_str());
        if (map && map->isString)
            return Utils::String::Format("(StringView){0}", expr);
        if (IsScriptingObjectPointer(cppType))
        {
            return Utils::String::Format("({0}*)ScriptingObject::Cast((ScriptingObject*){1})", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        }
        // Enum types
        if (FindTypeMapping(stripped.c_str()) == nullptr && !IsPodType(stripped))
        {
            return Utils::String::Format("({0})(uint64){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        }
        return Utils::String::Format("({0}){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
    }

    std::string BindingsCppGenerator::BuildWrapperParams(const ApiClass& cls, const ApiFunction& fn, bool forExport) const
    {
        std::string params;
        if (!fn.isStatic)
        {
            std::string nativeTypeName = GetCppNativeTypeName(cls);
            if (forExport)
            {
                params += Utils::String::Format("::{0}* __obj", nativeTypeName);
            }
            else
            {
                params += Utils::String::Format("::{0}* __obj", nativeTypeName);
            }
        }
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (params.length() > 0)
            {
                params += ", ";
            }
            std::string interopType = GetInteropParamType(fn.params[i]);
            params += Utils::String::Format("{0} {1}", interopType, fn.params[i].name);
        }
        return params;
    }

    std::string BindingsCppGenerator::BuildForwardArgs(const ApiFunction& fn) const
    {
        std::string args;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0)
                args += ", ";
            args += fn.params[i].name;
        }
        return args;
    }

    std::string BindingsCppGenerator::BuildCallArgs(const ApiClass& cls, const ApiFunction& fn) const
    {
        std::string args;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0)
                args += ", ";
            std::string converted = GetManagedToNativeConvert(fn.params[i].cppType, fn.params[i].name);
            args += converted;
        }
        return args;
    }

    // -------------------------------------------------------------------------
    // Wrapper function generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                                           bool compilerIsMSVC, std::string& bodyOut, std::string& endOut)
    {
        std::string retType = GetInteropReturnType(fn);
        std::string params = BuildWrapperParams(cls, fn, true);
        std::string callArgs = BuildCallArgs(cls, fn);
        std::string callExpr;

        if (fn.isStatic)
        {
            std::string nativeName = GetCppNativeInvokeTypeName(cls);
            callExpr = Utils::String::Format("::{0}::{1}({2})", nativeName, fn.name, callArgs);
        }
        else
        {
            callExpr = Utils::String::Format("__obj->{0}({1})", fn.name, callArgs);
        }

        std::string retConvert = GetNativeToManagedConvert(fn.returnType, callExpr);
        bool retIsVoid = (fn.returnType == "void");

        if (compilerIsMSVC)
        {
            // MSVC: DLLEXPORT + MSVC_FUNC_EXPORT
            bodyOut += Utils::String::Format("    DLLEXPORT static {0} {1}({2})\n", retType, fn.uniqueName, params);
            bodyOut += "    {\n";
            if (!fn.isStatic)
            {
                bodyOut += "        if (__obj == nullptr) return";
                if (!retIsVoid) bodyOut += " {}";
                bodyOut += ";\n";
            }
            bodyOut += retIsVoid
                ? Utils::String::Format("        {0};\n", retConvert)
                : Utils::String::Format("        return {0};\n", retConvert);
            bodyOut += "    }\n";
        }
        else
        {
            // Clang: Internal class method + plain-C export at file end
            bodyOut += Utils::String::Format("    static {0} {1}({2})\n", retType, fn.uniqueName, params);
            bodyOut += "    {\n";
            if (!fn.isStatic)
            {
                bodyOut += "        if (__obj == nullptr) return";
                if (!retIsVoid) bodyOut += " {}";
                bodyOut += ";\n";
            }
            bodyOut += retIsVoid ? Utils::String::Format("        {0};\n", retConvert) : Utils::String::Format("        return {0};\n", retConvert);
            bodyOut += "    }\n";

            // Plain-C export
            std::string exportParams;
            if (!fn.isStatic)
                exportParams += "void* __obj";
            for (int i = 0; i < fn.params.size(); ++i)
            {
                if (exportParams.length() > 0) exportParams += ", ";
                exportParams += Utils::String::Format("{0} {1}", GetInteropParamType(fn.params[i]), fn.params[i].name);
            }

            endOut += Utils::String::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}({3})\n", retType, cls.name, fn.uniqueName, exportParams);
            endOut += "{\n";
            if (!fn.isStatic)
            {
                std::string nativeTypeName = GetCppNativeTypeName(cls);
                std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls);
                std::string castExpr = Utils::String::Format("return {0}::{1}((::{2}*)__obj{3}{4})",
                    internalName, fn.uniqueName, nativeTypeName,
                    fn.params.size() > 0 ? ", " : "", BuildForwardArgs(fn));
                endOut += Utils::String::Format("    {0};\n", retIsVoid ?
                    Utils::String::Format("{0}::{1}((::{2}*)__obj{3}{4})",
                        internalName, fn.uniqueName, nativeTypeName,
                        fn.params.size() > 0 ? ", " : "",
                        BuildForwardArgs(fn)) :
                    castExpr);
            }
            else
            {
                endOut += Utils::String::Format("    {0}{1}Internal::{2}({3});\n",
                    retIsVoid ? "" : "return ",
                    cls.name, fn.uniqueName, BuildForwardArgs(fn));
            }
            endOut += "}\n";
        }
    }

    void BindingsCppGenerator::GenerateCppPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                                              bool compilerIsMSVC, std::string& bodyOut, std::string& endOut)
    {
        std::string fullType = GetCppNativeTypeName(cls);
        std::string interopRetType = StripTypeQualifiers(prop.cppType);
        const TypeMapping* map = FindTypeMapping(interopRetType.c_str());
        if (map && map->isString)
            interopRetType = "void*";
        if (IsScriptingObjectPointer(prop.cppType))
            interopRetType = "void*";
        if (interopRetType != "void*")
            interopRetType = CodeGeneratorUtils::QualifyCppType(interopRetType);

        if (prop.hasGetter)
        {
            if (compilerIsMSVC)
            {
                bodyOut += Utils::String::Format("    DLLEXPORT static {0} {1}({2}* __obj)\n",
                    interopRetType, prop.getterUniqueName, fullType);
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return {};\n";
                std::string callExpr = Utils::String::Format("__obj->{0}()", prop.getterName);
                std::string converted = GetNativeToManagedConvert(prop.cppType, callExpr);
                bodyOut += Utils::String::Format("        return {0};\n", converted);
                bodyOut += "    }\n";
            }
            else
            {
                bodyOut += Utils::String::Format("    static {0} {1}(::{2}* __obj)\n",
                    interopRetType, prop.getterUniqueName, fullType);
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return {};\n";
                std::string callExpr = Utils::String::Format("__obj->{0}()", prop.getterName);
                std::string converted = GetNativeToManagedConvert(prop.cppType, callExpr);
                bodyOut += Utils::String::Format("        return {0};\n", converted);
                bodyOut += "    }\n";

                endOut += Utils::String::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}(void* __obj)\n",
                    interopRetType, cls.name, prop.getterUniqueName);
                endOut += "{\n";
                endOut += Utils::String::Format("    return {0}::{1}((::{2}*)__obj);\n",
                    CodeGeneratorUtils::GetInternalClassName(cls), prop.getterUniqueName, fullType);
                endOut += "}\n";
            }
        }

        if (prop.hasSetter)
        {
            std::string valueParam = GetManagedToNativeConvert(prop.cppType, "value");

            if (compilerIsMSVC)
            {
                bodyOut += Utils::String::Format("    DLLEXPORT static void {0}(::{1}* __obj, {2} value)\n",
                    prop.setterUniqueName, fullType, interopRetType);
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return;\n";
                bodyOut += Utils::String::Format("        __obj->{0}({1});\n", prop.setterName, valueParam);
                bodyOut += "    }\n";
            }
            else
            {
                bodyOut += Utils::String::Format("    static void {0}(::{1}* __obj, {2} value)\n",
                    prop.setterUniqueName, fullType, interopRetType);
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return;\n";
                bodyOut += Utils::String::Format("        __obj->{0}({1});\n", prop.setterName, valueParam);
                bodyOut += "    }\n";

                endOut += Utils::String::Format("DEFINE_INTERNAL_CALL(void) {0}_{1}(void* __obj, {2} value)\n",
                    cls.name, prop.setterUniqueName, interopRetType);
                endOut += "{\n";
                endOut += Utils::String::Format("    {0}::{1}((::{2}*)__obj, value);\n",
                    CodeGeneratorUtils::GetInternalClassName(cls), prop.setterUniqueName, fullType);
                endOut += "}\n";
            }
        }
    }

    void BindingsCppGenerator::GenerateCppEventWrappers(const ApiClass& cls, const ApiEvent& evt,
                                                        std::string& bodyOut)
    {
        std::string fullType = GetCppNativeTypeName(cls);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls);

        // Build parameter type list for the event callback
        std::string paramTypes;
        for (int i = 0; i < evt.params.size(); ++i)
        {
            if (i > 0) paramTypes += ", ";
            paramTypes += evt.params[i].cppType;
        }

        // Managed wrapper - C++ calls C# delegate
        bodyOut += Utils::String::Format("    void {0}_ManagedWrapper({1})\n", evt.name, paramTypes);
        bodyOut += "    {\n";
        bodyOut += "        static CLRMethod* method = nullptr;\n";
        bodyOut += Utils::String::Format("        if (!method) {{ method = ::{0}::TypeInitializer->GetType().ManagedClass->GetMethod(\"Internal_{1}_Invoke\", {2}); CHECK(method); }}\n",
            fullType, evt.name, evt.params.size());
        bodyOut += "        CLRObject* exception = nullptr;\n";
        if (evt.params.size() > 0)
        {
            bodyOut += Utils::String::Format("        void* params[{0}];\n", evt.params.size());
            for (int i = 0; i < evt.params.size(); ++i)
            {
                std::string convertExpr = GetNativeToManagedConvert(evt.params[i].cppType, evt.params[i].name);
                bodyOut += Utils::String::Format("        params[{0}] = (void*){1};\n", i, convertExpr);
            }
        }
        if (evt.isStatic)
            bodyOut += "        method->Invoke(nullptr, params, &exception);\n";
        else
            bodyOut += Utils::String::Format("        CLRObject* instance = ((::{0}*)this)->GetManagedInstance();\n", fullType);
            bodyOut += "        method->Invoke(instance, params, &exception);\n";
        bodyOut += "        if (exception) DebugLog::LogException(exception);\n";
        bodyOut += "    }\n\n";

        // Managed bind/unbind
        std::string bindTarget = Utils::String::Format("&{0}::{1}_ManagedWrapper", internalName, evt.name);
        bodyOut += Utils::String::Format("    DLLEXPORT static void {0}_ManagedBind({::1}* __obj, bool bind)\n", evt.name, fullType);
        bodyOut += "    {\n";
        bodyOut += "        if (__obj == nullptr) return;\n";
        bodyOut += Utils::String::Format("        Function<void({0})> f;\n", paramTypes);
        bodyOut += Utils::String::Format("        f.Bind<{0}, {1}>({2}({0}*)__obj);\n",
            internalName, bindTarget, evt.isStatic ? "" : "(");
        bodyOut += Utils::String::Format("        if (bind) __obj->{0}.Bind(f);\n", evt.name);
        bodyOut += Utils::String::Format("        else __obj->{0}.Unbind(f);\n", evt.name);
        bodyOut += "    }\n\n";

        // Generic scripting event wrapper (Variant-based)
        bodyOut += Utils::String::Format("    void {0}_Wrapper({1})\n", evt.name, paramTypes);
        bodyOut += "    {\n";
        if (evt.params.size() > 0)
        {
            bodyOut += Utils::String::Format("        Variant parameters[{0}];\n", evt.params.size());
            for (int i = 0; i < evt.params.size(); ++i)
            {
                std::string convertExpr = GetNativeToVariantConvert(evt.params[i].cppType, evt.params[i].name);
                bodyOut += Utils::String::Format("        parameters[{0}] = {1};\n", i, convertExpr);
            }
            bodyOut += Utils::String::Format("        ScriptingEvents::Event((ScriptingObject*)this, Span<Variant>(parameters, {0}), ::{1}::TypeInitializer, StringView(SE_TEXT(\"{2}\")));\n",
                evt.params.size(), fullType, evt.name);
        }
        else
        {
            bodyOut += Utils::String::Format("        ScriptingEvents::Event((ScriptingObject*)this, Span<Variant>(), ::{0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")));\n",
                fullType, evt.name);
        }
        bodyOut += "    }\n\n";

        // Generic scripting bind/unbind
        bodyOut += Utils::String::Format("    static void {0}_Bind(::{1}* __obj, void* instance, bool bind)\n", evt.name, fullType);
        bodyOut += "    {\n";
        bodyOut += Utils::String::Format("        Function<void({0})> f;\n", paramTypes);
        bodyOut += Utils::String::Format("        f.Bind<{0}, &{0}::{1}_Wrapper>(({0}*)instance);\n", internalName, evt.name);
        bodyOut += Utils::String::Format("        if (bind) __obj->{0}.Bind(f);\n", evt.name);
        bodyOut += Utils::String::Format("        else __obj->{0}.Unbind(f);\n", evt.name);
        bodyOut += "    }\n";
    }

    void BindingsCppGenerator::GenerateCppFieldAccessors(const ApiClass& cls, const ApiField& field,
                                                          bool compilerIsMSVC, std::string& bodyOut, std::string& endOut)
    {
        std::string fullType = GetCppNativeTypeName(cls);
        std::string staticAccessType = GetCppNativeInvokeTypeName(cls);
        std::string interopType = StripTypeQualifiers(field.cppType);
        const TypeMapping* map = FindTypeMapping(interopType.c_str());
        if (map && map->isString) interopType = "void*";
        if (IsScriptingObjectPointer(field.cppType)) interopType = "void*";
        if (interopType != "void*")
            interopType = CodeGeneratorUtils::QualifyCppType(interopType);

        // Getter
        if (compilerIsMSVC)
        {
            bodyOut += Utils::String::Format("    DLLEXPORT static {0} {1}_Get(::{2}* __obj)\n",
                interopType, field.name, fullType);
        }
        else
        {
            bodyOut += Utils::String::Format("    static {0} {1}_Get(::{2}* __obj)\n",
                interopType, field.name, fullType);
        }
        bodyOut += "    {\n";
        if (!field.isStatic)
        {
            bodyOut += "        if (__obj == nullptr) return {};\n";
            std::string access = Utils::String::Format("__obj->{0}", field.name);
            std::string converted = GetNativeToManagedConvert(field.cppType, access);
            bodyOut += Utils::String::Format("        return {0};\n", converted);
        }
        else
        {
            std::string access = Utils::String::Format("::{0}::{1}", staticAccessType, field.name);
            std::string converted = GetNativeToManagedConvert(field.cppType, access);
            bodyOut += Utils::String::Format("        return {0};\n", converted);
        }
        bodyOut += "    }\n";

        // Setter (if not readonly)
        if (!field.isReadOnly)
        {
            if (compilerIsMSVC)
            {
                bodyOut += Utils::String::Format("    DLLEXPORT static void {0}_Set(::{1}* __obj, {2} value)\n",
                    field.name, fullType, interopType);
            }
            else
            {
                bodyOut += Utils::String::Format("    static void {0}_Set(::{1}* __obj, {2} value)\n",
                    field.name, fullType, interopType);
            }
            bodyOut += "    {\n";
            if (!field.isStatic)
            {
                bodyOut += "        if (__obj == nullptr) return;\n";
                std::string nativeValue = GetManagedToNativeConvert(field.cppType, "value");
                bodyOut += Utils::String::Format("        __obj->{0} = {1};\n", field.name, nativeValue);
            }
            else
            {
                std::string nativeValue = GetManagedToNativeConvert(field.cppType, "value");
                bodyOut += Utils::String::Format("        ::{0}::{1} = {2};\n", staticAccessType, field.name, nativeValue);
            }
            bodyOut += "    }\n";
        }

        // Clang plain-C exports
        if (!compilerIsMSVC)
        {
            endOut += Utils::String::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}_Get(void* __obj)\n",
                interopType, cls.name, field.name);
            endOut += Utils::String::Format("{{ return {0}::{1}_Get((::{2}*)__obj); }}\n",
                CodeGeneratorUtils::GetInternalClassName(cls), field.name, fullType);

            if (!field.isReadOnly)
            {
                endOut += Utils::String::Format("DEFINE_INTERNAL_CALL(void) {0}_{1}_Set(void* __obj, {2} value)\n",
                    cls.name, field.name, interopType);
                endOut += Utils::String::Format("{{ {0}::{1}_Set((::{2}*)__obj, value); }}\n",
                    CodeGeneratorUtils::GetInternalClassName(cls), field.name, fullType);
            }
        }
    }

    void BindingsCppGenerator::GenerateCppInitRuntime(const ApiClass& cls, std::string& output)
    {
        output += "    static void InitRuntime()\n    {\n";

        // Register events in ScriptingEvents table
        for (auto& evt : cls.events)
        {
            std::string fullType = GetCppNativeTypeName(cls);
            output += Utils::String::Format(
                "        ScriptingEvents::EventsTable[Pair<ScriptingTypeHandle, StringView>({0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")))] = (void(*)(ScriptingObject*, void*, bool)){2}Internal::{1}_Bind;\n",
                CodeGeneratorUtils::RemovePreNameSpace(fullType), evt.name, cls.name);
        }

        output += "    }\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppClass(const ApiClass& cls, const std::string& assemblyType,
                                                 bool compilerIsMSVC, std::string& output)
    {
        std::string fullname = GetCppNativeTypeName(cls);
        std::string fullTypename = CodeGeneratorUtils::GetFullCSTypeName(cls.namespaceNameList, cls.name);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls);
        std::string bodyOut, endOut;
        bool useScripting = cls.isScriptingObject;

        // Internal class header
        if (!cls.namespaceNameList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", CodeGeneratorUtils::GetFullCNameSpaceName(cls.namespaceNameList));
        }

        output += Utils::String::Format("class {0}\n{{\npublic:\n", internalName);

        // Event wrappers require the ScriptingObject event table.
        if (useScripting)
        {
            for (auto& evt : cls.events)
            {
                GenerateCppEventWrappers(cls, evt, bodyOut);
            }
        }

        // Function, property and field exports also apply to native-handle API classes.
        for (auto& field : cls.fields)
        {
            GenerateCppFieldAccessors(cls, field, compilerIsMSVC, bodyOut, endOut);
        }

        for (auto& prop : cls.properties)
        {
            GenerateCppPropertyAccessors(cls, prop, compilerIsMSVC, bodyOut, endOut);
        }

        for (auto& fn : cls.functions)
        {
            GenerateCppWrapperFunction(cls, fn, compilerIsMSVC, bodyOut, endOut);
        }

        if (useScripting)
        {
            GenerateCppInitRuntime(cls, bodyOut);
        }

        output += bodyOut;
        output += "};\n\n";

        if (!useScripting)
        {
            if (!compilerIsMSVC && endOut.length() > 0)
            {
                output += Utils::String::Format("\n// Clang plain-C exports\n{0}", endOut);
            }

            if (!cls.namespaceNameList.empty())
            {
                output += "}\n";
            }

            output += "\n";
            return;
        }

        // Interface inheritance table
        if (!cls.interfaces.empty())
        {
            output += Utils::String::Format("static const ScriptingType::InterfaceImplementation {0}_Interfaces[] = {{\n", fullname);
            for (auto& iface : cls.interfaces)
            {
                std::string ifaceNativeName = iface.nativeName.empty() ? iface.name : iface.nativeName;
                std::string ifaceFull = CodeGeneratorUtils::GetFullCTypeName(iface.namespaceNameList, ifaceNativeName);
                output += Utils::String::Format("    {{ &{0}::TypeInitializer, (int16)VTABLE_OFFSET({1}, {0}), 0, true }},\n", ifaceFull, CodeGeneratorUtils::RemovePreNameSpace(fullname));
            }
            output += "    { nullptr, 0 },\n};\n\n";
        }

        // ScriptingTypeInitializer
        output += Utils::String::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n", CodeGeneratorUtils::RemovePreNameSpace(fullname));
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullTypename);
        output += Utils::String::Format("    sizeof(::{0}),\n", fullname);
        output += Utils::String::Format("    &{0}::InitRuntime,\n", internalName);

        if (useScripting)
        {
            // ScriptingObject path: spawn, baseType, vtable, vtable, interfaces
            if (cls.isStatic || cls.noSpawn)
            {
                output += "    &ScriptingType::DefaultSpawn, \n";
            }
            else
            {
                output += Utils::String::Format("    (ScriptingType::SpawnHandler)&::{0}::Spawn,\n", fullname);
            }

            if (!cls.baseClassName.empty())
            {
                output += Utils::String::Format("    &::{0}::TypeInitializer,\n", cls.baseClassName);
            }
            else
            {
                output += "    nullptr,\n";
            }

            output += "    nullptr,\n    nullptr";
            if (!cls.interfaces.empty())
            {
                output += Utils::String::Format(",\n    ::{0}_Interfaces", fullname);
            }
            output += "\n);\n";
        }
        else
        {
            // Non-scripting class path: ctor, dtor, baseType, interfaces
            if (!cls.isAbstract)
            {
                output += Utils::String::Format("    &{0}::Ctor, &{0}::Dtor,\n", internalName);
            }
            else
            {
                output += "    nullptr, nullptr,\n";
            }

            if (!cls.baseClassName.empty())
            {
                output += Utils::String::Format("    &::{0}::TypeInitializer", cls.baseClassName);
            }
            else
            {
                output += "    nullptr";
            }

            if (!cls.interfaces.empty())
            {
                output += Utils::String::Format(",\n    ::{0}_Interfaces", fullname);
            }
            output += "\n);\n";
        }

        // Clang plain-C exports
        if (!compilerIsMSVC && endOut.length() > 0)
        {
            output += Utils::String::Format("\n// Clang plain-C exports\n{0}", endOut);
        }

        if (!cls.namespaceNameList.empty())
        {
            output += "}\n";
        }

        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Struct generation (independent, not delegating to GenerateCppClass)
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppStruct(const ApiClass& cls, const std::string& /*assemblyType*/,
                                                  bool compilerIsMSVC, std::string& output)
    {
        std::string fullName = GetCppNativeTypeName(cls);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls);
        std::string bodyOut, endOut;

        // Internal class header
        if (!cls.namespaceNameList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceNameList));
        }

        output += Utils::String::Format("class {0}\n{{\npublic:\n", internalName);

        // Field accessors - for structs, non-static fields get direct getter/setter
        for (auto& field : cls.fields)
        {
            if (field.isHidden)
                continue;

            // Non-static fields: generate getter/setter
            if (!field.isStatic)
            {
                GenerateCppFieldAccessors(cls, field, compilerIsMSVC, bodyOut, endOut);
            }
        }

        output += bodyOut;
        output += "};\n\n";

        // Clang plain-C exports
        if (!compilerIsMSVC && endOut.length() > 0)
        {
            output += Utils::String::Format("\n// Clang plain-C exports\n{0}", endOut);
        }

        if (!cls.namespaceNameList.empty())
        {
            output += "}\n";
        }

        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Enum generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppEnum(const ApiEnum& en, const std::string& assemblyType, std::string& output)
    {
        std::string fullNameName = CodeGeneratorUtils::GetNativeName(en.namespaceScopeList, en.structScopeList , en.name);
        std::string namespaceName = CodeGeneratorUtils::GetFullCSNameSpaceName(en.namespaceScopeList);

        std::string InternalNativeName = en.name;
        if (!en.structScopeList.empty())
        {
            InternalNativeName = Utils::String::Format("{0}_{1}", Utils::CombineStringList(en.structScopeList, "_"), en.name);
        }


        if (!en.namespaceScopeList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", namespaceName);
        }

        output += Utils::String::Format("class {0}Internal\n{{\npublic:\n", InternalNativeName);
        output += "    static ScriptingType::EnumItem Items[];\n";
        output += "};\n\n";

        // Items array
        output += Utils::String::Format("ScriptingType::EnumItem {0}Internal::Items[] = {{\n", InternalNativeName);
        for (int i = 0; i < en.valueNames.size(); ++i)
        {
            output += Utils::String::Format("    {{ (uint64)::{0}::{1}, \"{1}\" }},\n", fullNameName, en.valueNames[i]);
        }
        output += "    { 0, nullptr }\n};\n\n";

        // ScriptingTypeInitializer
        output += Utils::String::Format("inline ScriptingTypeInitializer {0}_TypeInitializer(\n", en.name);
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"::{0}\") - 1),\n", fullNameName);
        output += Utils::String::Format("    sizeof(::{0}),\n", fullNameName);
        output += Utils::String::Format("    StableID::Generate<::{0}>(),\n", fullNameName);
        output += Utils::String::Format("    {0}Internal::Items\n", InternalNativeName);
        output += ");\n\n";

        if (!namespaceName.empty())
        {
            output += "}\n";
        }
    }

    // -------------------------------------------------------------------------
    // Interface generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppInterface(const ApiInterface& iface, const std::string& assemblyType,
                                                     std::string& output)
    {
        std::string nativeName = iface.nativeName.empty() ? iface.name : iface.nativeName;
        std::string fullType = CodeGeneratorUtils::GetFullCTypeName(iface.namespaceNameList, nativeName);
        std::string fullTypename = CodeGeneratorUtils::GetFullCSTypeName(iface.namespaceNameList, iface.name);

        if (!iface.namespaceNameList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", CodeGeneratorUtils::GetFullCSNameSpaceName(iface.namespaceNameList));
        }

        // Wrapper class
        output += Utils::String::Format("class {0}Wrapper : public {1}\n{{\npublic:\n", iface.name, nativeName);
        output += "    ScriptingObject* Object;\n";

        for (auto& fn : iface.functions)
        {
            std::string paramTypes;
            for (int i = 0; i < fn.params.size(); ++i)
            {
                if (i > 0) paramTypes += ", ";
                paramTypes += fn.params[i].cppType;
            }

            output += Utils::String::Format("    {0} {1}({2}) const override\n", fn.returnType, fn.name, paramTypes);
            output += "    {\n";
            if (fn.params.size() > 0)
            {
                output += Utils::String::Format("        Variant parameters[{0}];\n", fn.params.size());
                for (int i = 0; i < fn.params.size(); ++i)
                {
                    std::string convertExpr = GetNativeToVariantConvert(fn.params[i].cppType, fn.params[i].name);
                    output += Utils::String::Format("        parameters[{0}] = {1};\n", i, convertExpr);
                }
            }

            output +=                   "        auto typeHandle = Object->GetTypeHandle();\n";
            output +=                   "        while (typeHandle)\n        {\n";
            output += Utils::String::Format("            auto method = typeHandle.Module->FindMethod(typeHandle, StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1), {1});\n",
                fn.name, fn.params.size());
            output +=                   "            if (method)\n            {\n";
            output +=                   "                Variant __result;\n";
            if (fn.params.size() > 0)
            {
                output += Utils::String::Format("                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(parameters, {0}), __result);\n", fn.params.size());
            }
            else
            {
                output +=               "                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(), __result);\n";
            }
            if (fn.returnType != "void")
            {
                output += Utils::String::Format("                return ({0})__result;\n", fn.returnType);
            }
            else
            {
                output += "                return;\n";
            }
            output += "            }\n";
            output += "            typeHandle = typeHandle.GetType().GetBaseType();\n";
            output += "        }\n";
            if (fn.returnType != "void")
                output += "        return {};\n"; // default return
            output += "    }\n";
        }
        output += "};\n\n";

        // Internal class
        output += Utils::String::Format("class {0}Internal\n{{\npublic:\n", iface.name);
        output += Utils::String::Format("    static void InitRuntime() {{ }}\n");
        output += Utils::String::Format("    static void* GetInterfaceWrapper(ScriptingObject* __obj)\n    {{\n");
        output += Utils::String::Format("        auto wrapper = New<{0}Wrapper>();\n", iface.name);
        output += "        wrapper->Object = __obj;\n";
        output += "        return wrapper;\n";
        output += "    }\n";
        output += "};\n\n";

        if (!iface.namespaceNameList.empty())
        {
            output += "}\n";
        }

        // ScriptingTypeInitializer
        output += Utils::String::Format("ScriptingTypeInitializer ::{0}::TypeInitializer(\n", CodeGeneratorUtils::RemovePreNameSpace(fullType));
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullTypename);
        output += Utils::String::Format("    &{0}Internal::InitRuntime,\n", iface.name);
        output += "    nullptr,\n    nullptr,\n";
        output += Utils::String::Format("    &{0}Internal::GetInterfaceWrapper\n", iface.name);
        output += ");\n";
    }

    // -------------------------------------------------------------------------
    // Generate - entry point for a single header
    // -------------------------------------------------------------------------

    bool BindingsCppGenerator::GenerateSource(const BindingsHeaderInfo& headerInfo, bool compilerIsMSVC, std::string& output)
    {
        output.clear();
        bool hasCppInjectedCode = false;
        for (auto const& code : headerInfo.injectedCode)
        {
            if (IsInjectedCppCode(code))
            {
                hasCppInjectedCode = true;
                break;
            }
        }
        if (headerInfo.classes.empty() && headerInfo.enums.empty()
            && headerInfo.interfaces.empty() && headerInfo.events.empty()
            && !hasCppInjectedCode)
        {
            return true;
        }

        // Derive assemblyType
        std::string assemblyType = CodeGeneratorUtils::DeriveAssemblyCSharpType(headerInfo.assemblyName);

        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - do not edit manually.\n";
        output += Utils::String::Format("// Source: {0}\n", headerInfo.filePath);
        output += "//-------------------------------------------------------------------------\n";
        output += Utils::String::Format("#include \"{0}\"\n", headerInfo.filePath);
        output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRUtils.h\"\n";
        output += "#include \"Runtime/Core/Scripting/ScriptingObject.h\"\n";
        output += "#include \"Runtime/Core/Scripting/Internal/InternalCalls.h\"\n";
        output += "#include \"Runtime/Core/Scripting/ScriptingType.h\"\n";

        // Conditional includes for events
        bool hasEvents = false;
        for (auto& cls : headerInfo.classes)
        {
            if (!cls.events.empty())
            {
                hasEvents = true;
                break;
            }
        }
        if (hasEvents)
        {
            output += "#include \"Runtime/Core/Scripting/Events.h\"\n";
        }

        // Interfaces
        if (!headerInfo.interfaces.empty())
        {
            output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRClass.h\"\n";
        }
        for (auto const& code : headerInfo.injectedCode)
        {
            if (IsInjectedCppCode(code))
            {
                output += code.code;
                if (!Utils::String::EndsWith(output, '\n'))
                {
                    output += "\n";
                }
            }
        }

        output += "\n";

        // Generate enums first
        for (auto& en : headerInfo.enums)
        {
            GenerateCppEnum(en, assemblyType, output);
        }

        // Generate interfaces
        for (auto& iface : headerInfo.interfaces)
        {
            GenerateCppInterface(iface, assemblyType, output);
        }

        // Generate classes/structs
        for (auto& cls : headerInfo.classes)
        {
            if (cls.isStruct)
            {
                GenerateCppStruct(cls, assemblyType, compilerIsMSVC, output);
            }
            else
            {
                GenerateCppClass(cls, assemblyType, compilerIsMSVC, output);
            }
        }

        return true;
    }

} // namespace SE::BuildTool
