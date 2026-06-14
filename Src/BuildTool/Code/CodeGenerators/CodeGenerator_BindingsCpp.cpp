
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"

#include "../ReflectorSettingsAndUtils.h"

namespace SE::ReflectTool
{
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    StringAnsi BindingsCppGenerator::GetNativeName(const StringAnsi& nameSpaceName, const StringAnsi& name) const
    {
        if (!nameSpaceName.IsEmpty())
        {
            if (nameSpaceName == "SE")
            {
                return name;
            }
            if (nameSpaceName.StartsWith("SE::"))
            {
                return StringAnsi::Format("{0}::{1}", nameSpaceName.Substring(4), name);
            }

            return StringAnsi::Format("{0}::{1}", nameSpaceName, name);
        }
        return name;
    }

    StringAnsi BindingsCppGenerator::GetFullCSTypeName(const StringAnsi& nameSpaceName, const StringAnsi& name) const
    {
        if (!nameSpaceName.IsEmpty())
        {
            if (nameSpaceName.StartsWith("SE::"))
            {
                return StringAnsi::Format("{0}.{1}", nameSpaceName.Substring(4), name);
            }

            return StringAnsi::Format("{0}.{1}", nameSpaceName, name);
        }
        return name;
    }

    StringAnsi BindingsCppGenerator::GetInternalClassName(const ApiClass& cls) const
    {
        return StringAnsi::Format("{0}Internal", cls.name.Get());
    }

    StringAnsi BindingsCppGenerator::GetInteropReturnType(const ApiFunction& fn) const
    {
        StringAnsi stripped = StripTypeQualifiers(fn.returnType);
        const TypeMapping* map = FindTypeMapping(stripped.Get());
        if (fn.returnType == "void")
            return "void";
        if (map && map->isString)
            return "void*";
        if (IsScriptingObjectPointer(fn.returnType))
            return "void*";
        return fn.returnType;
    }

    StringAnsi BindingsCppGenerator::GetInteropParamType(const ApiParam& param) const
    {
        StringAnsi stripped = StripTypeQualifiers(param.cppType);
        const TypeMapping* map = FindTypeMapping(stripped.Get());
        if (map && map->isString)
            return "void*";
        if (IsScriptingObjectPointer(param.cppType))
            return "void*";
        return param.cppType;
    }

    StringAnsi BindingsCppGenerator::GetNativeToManagedConvert(const StringAnsi& cppType, const StringAnsi& expr) const
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        const TypeMapping* map = FindTypeMapping(stripped.Get());
        if (map && map->isString)
            return StringAnsi::Format("SEUtils::ToManagedString({0})", expr.Get());
        if (IsScriptingObjectPointer(cppType))
            return StringAnsi::Format("ScriptingObject::ToManaged({0})", expr.Get());
        return expr;
    }

    StringAnsi BindingsCppGenerator::GetManagedToNativeConvert(const StringAnsi& cppType, const StringAnsi& expr) const
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        const TypeMapping* map = FindTypeMapping(stripped.Get());
        if (map && map->isString)
            return StringAnsi::Format("SEUtils::ToString({0})", expr.Get());
        if (IsScriptingObjectPointer(cppType))
            return StringAnsi::Format("({0}*)ScriptingObject::ToNative({1})", stripped, expr.Get());
        return expr;
    }

    StringAnsi BindingsCppGenerator::GetNativeToVariantConvert(const StringAnsi& cppType, const StringAnsi& expr) const
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        if (stripped == "bool" || stripped == "int32" || stripped == "uint32"
            || stripped == "int64" || stripped == "uint64" || stripped == "float"
            || stripped == "double")
            return StringAnsi::Format("Variant({0})", expr.Get());
        if (IsScriptingObjectPointer(cppType))
            return StringAnsi::Format("Variant((ScriptingObject*){0})", expr.Get());
        return StringAnsi::Format("Variant({0})", expr.Get());
    }

    StringAnsi BindingsCppGenerator::GetVariantToNativeConvert(const StringAnsi& cppType, const StringAnsi& expr) const
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        if (stripped == "Variant" || stripped == "VariantType")
            return expr;
        if (stripped == "String")
            return StringAnsi::Format("(StringView){0}", expr.Get());
        if (stripped == "StringAnsi")
            return StringAnsi::Format("(StringAnsiView){0}", expr.Get());
        const TypeMapping* map = FindTypeMapping(stripped.Get());
        if (map && map->isString)
            return StringAnsi::Format("(StringView){0}", expr.Get());
        if (IsScriptingObjectPointer(cppType))
            return StringAnsi::Format("({0}*)ScriptingObject::Cast((ScriptingObject*){1})", stripped, expr.Get());
        // Enum types
        if (FindTypeMapping(stripped.Get()) == nullptr && !IsPodType(stripped))
            return StringAnsi::Format("({0})(uint64){1}", stripped, expr.Get());
        return StringAnsi::Format("({0}){1}", stripped, expr.Get());
    }

    StringAnsi BindingsCppGenerator::BuildWrapperParams(const ApiClass& cls, const ApiFunction& fn, bool forExport) const
    {
        StringAnsi params;
        if (!fn.isStatic)
        {
            if (forExport)
                params += StringAnsi::Format("{0}* __obj", GetNativeName(cls.namespaceName, cls.name).Get());
            else
                params += StringAnsi::Format("{0}* __obj", GetNativeName(cls.namespaceName, cls.name).Get());
        }
        for (int i = 0; i < fn.params.Count(); ++i)
        {
            if (params.Length() > 0)
                params += ", ";
            StringAnsi interopType = GetInteropParamType(fn.params[i]);
            params += StringAnsi::Format("{0} {1}", interopType, fn.params[i].name.Get());
        }
        return params;
    }

    StringAnsi BindingsCppGenerator::BuildForwardArgs(const ApiFunction& fn) const
    {
        StringAnsi args;
        for (int i = 0; i < fn.params.Count(); ++i)
        {
            if (i > 0)
                args += ", ";
            args += fn.params[i].name;
        }
        return args;
    }

    StringAnsi BindingsCppGenerator::BuildCallArgs(const ApiClass& cls, const ApiFunction& fn) const
    {
        StringAnsi args;
        for (int i = 0; i < fn.params.Count(); ++i)
        {
            if (i > 0)
                args += ", ";
            StringAnsi converted = GetManagedToNativeConvert(fn.params[i].cppType, fn.params[i].name);
            args += converted;
        }
        return args;
    }

    // -------------------------------------------------------------------------
    // Wrapper function generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                                           bool compilerIsMSVC, StringAnsi& bodyOut, StringAnsi& endOut)
    {
        StringAnsi retType = GetInteropReturnType(fn);
        StringAnsi params = BuildWrapperParams(cls, fn, true);
        StringAnsi callArgs = BuildCallArgs(cls, fn);
        StringAnsi callExpr;

        if (fn.isStatic)
            callExpr = StringAnsi::Format("{0}::{1}({2})", GetNativeName(cls.namespaceName, cls.name), fn.name, callArgs.Get());
        else
            callExpr = StringAnsi::Format("__obj->{0}({1})", fn.name, callArgs.Get());

        StringAnsi retConvert = GetNativeToManagedConvert(fn.returnType, callExpr);
        bool retIsVoid = (fn.returnType == "void");

        if (compilerIsMSVC)
        {
            // MSVC: DLLEXPORT + MSVC_FUNC_EXPORT
            bodyOut += StringAnsi::Format("    DLLEXPORT static {0} {1}({2})\n", retType, fn.uniqueName, params.Get());
            bodyOut += "    {\n";
            if (!fn.isStatic)
            {
                bodyOut += "        if (__obj == nullptr) return";
                if (!retIsVoid) bodyOut += " {}";
                bodyOut += ";\n";
            }
            bodyOut += StringAnsi::Format("        {0}\n", retIsVoid ? retConvert : StringAnsi::Format("return {0};", retConvert.Get()).Get());
            bodyOut += "    }\n";
        }
        else
        {
            // Clang: Internal class method + plain-C export at file end
            bodyOut += StringAnsi::Format("    static {0} {1}({2})\n", retType, fn.uniqueName, params.Get());
            bodyOut += "    {\n";
            if (!fn.isStatic)
            {
                bodyOut += "        if (__obj == nullptr) return";
                if (!retIsVoid) bodyOut += " {}";
                bodyOut += ";\n";
            }
            bodyOut += StringAnsi::Format("        {0}\n", retIsVoid ? retConvert : StringAnsi::Format("return {0};", retConvert.Get()).Get());
            bodyOut += "    }\n";

            // Plain-C export
            StringAnsi exportParams;
            if (!fn.isStatic)
                exportParams += "void* __obj";
            for (int i = 0; i < fn.params.Count(); ++i)
            {
                if (exportParams.Length() > 0) exportParams += ", ";
                exportParams += StringAnsi::Format("{0} {1}", GetInteropParamType(fn.params[i]), fn.params[i].name.Get());
            }

            endOut += StringAnsi::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}({3})\n",
                retType, cls.name, fn.uniqueName, exportParams.Get());
            endOut += "{\n";
            if (!fn.isStatic)
            {
                StringAnsi castExpr = StringAnsi::Format("return {0}Internal::{1}(({0}*)__obj{2}{3})",
                    cls.name, fn.uniqueName,
                    fn.params.Count() > 0 ? ", " : "",
                    BuildForwardArgs(fn).Get());
                endOut += StringAnsi::Format("    {0};\n", retIsVoid ?
                    StringAnsi::Format("{0}Internal::{1}(({0}*)__obj{2}{3})",
                        cls.name, fn.uniqueName,
                        fn.params.Count() > 0 ? ", " : "",
                        BuildForwardArgs(fn).Get()) :
                    castExpr.Get());
            }
            else
            {
                endOut += StringAnsi::Format("    {0}{1}Internal::{2}({3});\n",
                    retIsVoid ? "" : "return ",
                    cls.name, fn.uniqueName, BuildForwardArgs(fn).Get());
            }
            endOut += "}\n";
        }
    }

    void BindingsCppGenerator::GenerateCppPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                                              bool compilerIsMSVC, StringAnsi& bodyOut, StringAnsi& endOut)
    {
        StringAnsi fullType = GetNativeName(cls.namespaceName, cls.name);
        StringAnsi interopRetType = StripTypeQualifiers(prop.cppType);
        const TypeMapping* map = FindTypeMapping(interopRetType.Get());
        if (map && map->isString)
            interopRetType = "void*";
        if (IsScriptingObjectPointer(prop.cppType))
            interopRetType = "void*";

        if (prop.hasGetter)
        {
            if (compilerIsMSVC)
            {
                bodyOut += StringAnsi::Format("    DLLEXPORT static {0} {1}({2}* __obj)\n",
                    interopRetType, prop.getterUniqueName, fullType.Get());
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return {};\n";
                StringAnsi callExpr = StringAnsi::Format("__obj->{0}()", prop.getterName.Get());
                StringAnsi converted = GetNativeToManagedConvert(prop.cppType, callExpr);
                bodyOut += StringAnsi::Format("        return {0};\n", converted.Get());
                bodyOut += "    }\n";
            }
            else
            {
                bodyOut += StringAnsi::Format("    static {0} {1}({2}* __obj)\n",
                    interopRetType, prop.getterUniqueName, fullType.Get());
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return {};\n";
                StringAnsi callExpr = StringAnsi::Format("__obj->{0}()", prop.getterName.Get());
                StringAnsi converted = GetNativeToManagedConvert(prop.cppType, callExpr);
                bodyOut += StringAnsi::Format("        return {0};\n", converted.Get());
                bodyOut += "    }\n";

                endOut += StringAnsi::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}(void* __obj)\n",
                    interopRetType, cls.name, prop.getterUniqueName.Get());
                endOut += "{\n";
                endOut += StringAnsi::Format("    return {0}Internal::{1}(({0}*)__obj);\n",
                    cls.name, prop.getterUniqueName.Get());
                endOut += "}\n";
            }
        }

        if (prop.hasSetter)
        {
            StringAnsi valueParam = GetManagedToNativeConvert(prop.cppType, "value");

            if (compilerIsMSVC)
            {
                bodyOut += StringAnsi::Format("    DLLEXPORT static void {0}({1}* __obj, {2} value)\n",
                    prop.setterUniqueName, fullType, interopRetType.Get());
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return;\n";
                bodyOut += StringAnsi::Format("        __obj->{0}({1});\n", prop.setterName, valueParam.Get());
                bodyOut += "    }\n";
            }
            else
            {
                bodyOut += StringAnsi::Format("    static void {0}({1}* __obj, {2} value)\n",
                    prop.setterUniqueName, fullType, interopRetType.Get());
                bodyOut += "    {\n";
                bodyOut += "        if (__obj == nullptr) return;\n";
                bodyOut += StringAnsi::Format("        __obj->{0}({1});\n", prop.setterName, valueParam.Get());
                bodyOut += "    }\n";

                endOut += StringAnsi::Format("DEFINE_INTERNAL_CALL(void) {0}_{1}(void* __obj, {2} value)\n",
                    cls.name, prop.setterUniqueName, interopRetType.Get());
                endOut += "{\n";
                endOut += StringAnsi::Format("    {0}Internal::{1}(({0}*)__obj, value);\n",
                    cls.name, prop.setterUniqueName.Get());
                endOut += "}\n";
            }
        }
    }

    void BindingsCppGenerator::GenerateCppEventWrappers(const ApiClass& cls, const ApiEvent& evt,
                                                        StringAnsi& bodyOut)
    {
        StringAnsi fullType = GetNativeName(cls.namespaceName, cls.name);
        StringAnsi internalName = GetInternalClassName(cls);

        // Build parameter type list for the event callback
        StringAnsi paramTypes;
        for (int i = 0; i < evt.params.Count(); ++i)
        {
            if (i > 0) paramTypes += ", ";
            paramTypes += evt.params[i].cppType;
        }

        // Managed wrapper — C++ calls C# delegate
        bodyOut += StringAnsi::Format("    void {0}_ManagedWrapper({1})\n", evt.name, paramTypes.Get());
        bodyOut += "    {\n";
        bodyOut += "        static CLRMethod* method = nullptr;\n";
        bodyOut += StringAnsi::Format("        if (!method) {{ method = {0}::TypeInitializer->GetType().ManagedClass->GetMethod(\"Internal_{1}_Invoke\", {2}); CHECK(method); }}\n",
            fullType, evt.name, evt.params.Count());
        bodyOut += "        CLRObject* exception = nullptr;\n";
        if (evt.params.Count() > 0)
        {
            bodyOut += StringAnsi::Format("        void* params[{0}];\n", evt.params.Count());
            for (int i = 0; i < evt.params.Count(); ++i)
            {
                StringAnsi convertExpr = GetNativeToManagedConvert(evt.params[i].cppType, evt.params[i].name);
                bodyOut += StringAnsi::Format("        params[{0}] = (void*){1};\n", i, convertExpr.Get());
            }
        }
        if (evt.isStatic)
            bodyOut += "        method->Invoke(nullptr, params, &exception);\n";
        else
            bodyOut += StringAnsi::Format("        CLRObject* instance = (({0}*)this)->GetManagedInstance();\n", fullType.Get());
            bodyOut += "        method->Invoke(instance, params, &exception);\n";
        bodyOut += "        if (exception) DebugLog::LogException(exception);\n";
        bodyOut += "    }\n\n";

        // Managed bind/unbind
        StringAnsi bindTarget = StringAnsi::Format("&{0}::{1}_ManagedWrapper", internalName, evt.name.Get());
        bodyOut += StringAnsi::Format("    DLLEXPORT static void {0}_ManagedBind({1}* __obj, bool bind)\n",
            evt.name, fullType.Get());
        bodyOut += "    {\n";
        bodyOut += "        if (__obj == nullptr) return;\n";
        bodyOut += StringAnsi::Format("        Function<void({0})> f;\n", paramTypes.Get());
        bodyOut += StringAnsi::Format("        f.Bind<{0}, {1}>({2}({0}*)__obj);\n",
            internalName, bindTarget, evt.isStatic ? "" : "(");
        bodyOut += StringAnsi::Format("        if (bind) __obj->{0}.Bind(f);\n", evt.name.Get());
        bodyOut += StringAnsi::Format("        else __obj->{0}.Unbind(f);\n", evt.name.Get());
        bodyOut += "    }\n\n";

        // Generic scripting event wrapper (Variant-based)
        bodyOut += StringAnsi::Format("    void {0}_Wrapper({1})\n", evt.name, paramTypes.Get());
        bodyOut += "    {\n";
        if (evt.params.Count() > 0)
        {
            bodyOut += StringAnsi::Format("        Variant parameters[{0}];\n", evt.params.Count());
            for (int i = 0; i < evt.params.Count(); ++i)
            {
                StringAnsi convertExpr = GetNativeToVariantConvert(evt.params[i].cppType, evt.params[i].name);
                bodyOut += StringAnsi::Format("        parameters[{0}] = {1};\n", i, convertExpr.Get());
            }
            bodyOut += StringAnsi::Format("        ScriptingEvents::Event((ScriptingObject*)this, Span<Variant>(parameters, {0}), {1}::TypeInitializer, StringView(SE_TEXT(\"{2}\")));\n",
                evt.params.Count(), fullType, evt.name.Get());
        }
        else
        {
            bodyOut += StringAnsi::Format("        ScriptingEvents::Event((ScriptingObject*)this, Span<Variant>(), {0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")));\n",
                fullType, evt.name.Get());
        }
        bodyOut += "    }\n\n";

        // Generic scripting bind/unbind
        bodyOut += StringAnsi::Format("    static void {0}_Bind({1}* __obj, void* instance, bool bind)\n",
            evt.name, fullType.Get());
        bodyOut += "    {\n";
        bodyOut += StringAnsi::Format("        Function<void({0})> f;\n", paramTypes.Get());
        bodyOut += StringAnsi::Format("        f.Bind<{0}, &{0}::{1}_Wrapper>(({0}*)instance);\n", internalName, evt.name.Get());
        bodyOut += StringAnsi::Format("        if (bind) __obj->{0}.Bind(f);\n", evt.name.Get());
        bodyOut += StringAnsi::Format("        else __obj->{0}.Unbind(f);\n", evt.name.Get());
        bodyOut += "    }\n";
    }

    void BindingsCppGenerator::GenerateCppFieldAccessors(const ApiClass& cls, const ApiField& field,
                                                          bool compilerIsMSVC, StringAnsi& bodyOut, StringAnsi& endOut)
    {
        StringAnsi fullType = GetNativeName(cls.namespaceName, cls.name);
        StringAnsi interopType = StripTypeQualifiers(field.cppType);
        const TypeMapping* map = FindTypeMapping(interopType.Get());
        if (map && map->isString) interopType = "void*";
        if (IsScriptingObjectPointer(field.cppType)) interopType = "void*";

        // Getter
        if (compilerIsMSVC)
        {
            bodyOut += StringAnsi::Format("    DLLEXPORT static {0} {1}_Get({2}* __obj)\n",
                interopType, field.name, fullType.Get());
        }
        else
        {
            bodyOut += StringAnsi::Format("    static {0} {1}_Get({2}* __obj)\n",
                interopType, field.name, fullType.Get());
        }
        bodyOut += "    {\n";
        if (!field.isStatic)
        {
            bodyOut += "        if (__obj == nullptr) return {};\n";
            StringAnsi access = StringAnsi::Format("__obj->{0}", field.name.Get());
            StringAnsi converted = GetNativeToManagedConvert(field.cppType, access);
            bodyOut += StringAnsi::Format("        return {0};\n", converted.Get());
        }
        else
        {
            StringAnsi access = StringAnsi::Format("{0}::{1}", fullType, field.name.Get());
            StringAnsi converted = GetNativeToManagedConvert(field.cppType, access);
            bodyOut += StringAnsi::Format("        return {0};\n", converted.Get());
        }
        bodyOut += "    }\n";

        // Setter (if not readonly)
        if (!field.isReadOnly)
        {
            if (compilerIsMSVC)
            {
                bodyOut += StringAnsi::Format("    DLLEXPORT static void {0}_Set({1}* __obj, {2} value)\n",
                    field.name, fullType, interopType.Get());
            }
            else
            {
                bodyOut += StringAnsi::Format("    static void {0}_Set({1}* __obj, {2} value)\n",
                    field.name, fullType, interopType.Get());
            }
            bodyOut += "    {\n";
            if (!field.isStatic)
            {
                bodyOut += "        if (__obj == nullptr) return;\n";
                StringAnsi nativeValue = GetManagedToNativeConvert(field.cppType, "value");
                bodyOut += StringAnsi::Format("        __obj->{0} = {1};\n", field.name, nativeValue.Get());
            }
            else
            {
                StringAnsi nativeValue = GetManagedToNativeConvert(field.cppType, "value");
                bodyOut += StringAnsi::Format("        {0}::{1} = {2};\n", fullType, field.name, nativeValue.Get());
            }
            bodyOut += "    }\n";
        }

        // Clang plain-C exports
        if (!compilerIsMSVC)
        {
            endOut += StringAnsi::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}_Get(void* __obj)\n",
                interopType, cls.name, field.name);
            endOut += StringAnsi::Format("{{ return {0}Internal::{1}_Get(({0}*)__obj); }}\n",
                cls.name, field.name);

            if (!field.isReadOnly)
            {
                endOut += StringAnsi::Format("DEFINE_INTERNAL_CALL(void) {0}_{1}_Set(void* __obj, {2} value)\n",
                    cls.name, field.name, interopType);
                endOut += StringAnsi::Format("{{ {0}Internal::{1}_Set(({0}*)__obj, value); }}\n",
                    cls.name, field.name);
            }
        }
    }

    void BindingsCppGenerator::GenerateCppInitRuntime(const ApiClass& cls, StringAnsi& output)
    {
        output += "    static void InitRuntime()\n    {\n";

        // Register events in ScriptingEvents table
        for (auto& evt : cls.events)
        {
            StringAnsi fullType = GetNativeName(cls.namespaceName, cls.name);
            output += StringAnsi::Format(
                "        ScriptingEvents::EventsTable[Pair<ScriptingTypeHandle, StringView>({0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")))] = (void(*)(ScriptingObject*, void*, bool)){2}Internal::{1}_Bind;\n",
                fullType, evt.name, cls.name);
        }

        output += "    }\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppClass(const ApiClass& cls, const StringAnsi& assemblyType,
                                                 bool compilerIsMSVC, StringAnsi& output)
    {
        StringAnsi fullname = GetNativeName(cls.namespaceName, cls.name);
        StringAnsi fullTypename = GetFullCSTypeName(cls.namespaceName, cls.name);
        StringAnsi internalName = GetInternalClassName(cls);
        StringAnsi bodyOut, endOut;
        bool useScripting = cls.isStatic || cls.isScriptingObject;

        // Internal class header
        if (!cls.namespaceName.IsEmpty())
        {
            output += StringAnsi::Format("namespace {0}\n{{\n", cls.namespaceName);
        }

        output += StringAnsi::Format("class {0}\n{{\npublic:\n", internalName);

        // Event wrappers (scripting objects only)
        if (useScripting)
        {
            for (auto& evt : cls.events)
            {
                GenerateCppEventWrappers(cls, evt, bodyOut);
            }
        }

        // Field accessors (scripting objects only)
        if (useScripting)
        {
            for (auto& field : cls.fields)
            {
                GenerateCppFieldAccessors(cls, field, compilerIsMSVC, bodyOut, endOut);
            }
        }

        // Property accessors (scripting objects only)
        if (useScripting)
        {
            for (auto& prop : cls.properties)
            {
                GenerateCppPropertyAccessors(cls, prop, compilerIsMSVC, bodyOut, endOut);
            }
        }

        // Function wrappers (scripting objects only)
        if (useScripting)
        {
            for (auto& fn : cls.functions)
            {
                GenerateCppWrapperFunction(cls, fn, compilerIsMSVC, bodyOut, endOut);
            }
        }

        // InitRuntime
        GenerateCppInitRuntime(cls, bodyOut);

        if (!useScripting && !cls.isAbstract)
        {
            bodyOut += StringAnsi::Format(
                "    static void Ctor(void* ptr) {{ new(ptr){0}(); }}\n"
                "    static void Dtor(void* ptr) {{ (({0}*)ptr)->~{1}(); }}\n", fullname, cls.name);
        }

        output += bodyOut;
        output += "};\n\n";

        // Interface inheritance table
        if (!cls.interfaces.IsEmpty())
        {
            output += StringAnsi::Format("static const ScriptingType::InterfaceImplementation {0}_Interfaces[] = {{\n", fullname);
            for (auto& iface : cls.interfaces)
            {
                StringAnsi ifaceFull = !iface.namespaceName.IsEmpty()
                    ? StringAnsi::Format("{0}::{1}", iface.namespaceName, iface.name) : iface.name;
                output += StringAnsi::Format("    {{ &{0}::TypeInitializer, (int16)VTABLE_OFFSET({1}, {0}), 0, true }},\n", ifaceFull, fullname);
            }
            output += "    { nullptr, 0 },\n};\n\n";
        }

        // ScriptingTypeInitializer
        output += StringAnsi::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n", fullname);
        output += StringAnsi::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += StringAnsi::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullTypename);
        output += StringAnsi::Format("    sizeof({0}),\n", fullname);
        output += StringAnsi::Format("    &{0}::InitRuntime,\n", internalName);

        if (useScripting)
        {
            // ScriptingObject path: spawn, baseType, vtable, vtable, interfaces
            if (cls.isStatic || cls.noSpawn)
            {
                output += "    &ScriptingType::DefaultSpawn, \n";
            }
            else
            {
                output += StringAnsi::Format("    (ScriptingType::SpawnHandler)&{0}::Spawn,\n", fullname);
            }

            if (!cls.baseClassName.IsEmpty())
            {
                output += StringAnsi::Format("    &::{0}::TypeInitializer,\n", cls.baseClassName);
            }
            else
            {
                output += "    nullptr,\n";
            }

            output += "    nullptr,\n    nullptr";
            if (!cls.interfaces.IsEmpty())
            {
                output += StringAnsi::Format(",\n    {0}_Interfaces", fullname);
            }
            output += "\n);\n";
        }
        else
        {
            // Non-scripting class path: ctor, dtor, baseType, interfaces
            if (!cls.isAbstract)
            {
                output += StringAnsi::Format("    &{0}::Ctor, &{0}::Dtor,\n", internalName);
            }
            else
            {
                output += "    nullptr, nullptr,\n";
            }

            if (!cls.baseClassName.IsEmpty())
            {
                output += StringAnsi::Format("    &::{0}::TypeInitializer", cls.baseClassName);
            }
            else
            {
                output += "    nullptr";
            }

            if (!cls.interfaces.IsEmpty())
            {
                output += StringAnsi::Format(",\n    {0}_Interfaces", fullname);
            }
            output += "\n);\n";
        }

        // Clang plain-C exports
        if (!compilerIsMSVC && endOut.Length() > 0)
        {
            output += StringAnsi::Format("\n// Clang plain-C exports\n{0}", endOut);
        }

        if (!cls.namespaceName.IsEmpty())
        {
            output += "}\n";
        }

        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Struct generation (independent, not delegating to GenerateCppClass)
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppStruct(const ApiClass& cls, const StringAnsi& assemblyType,
                                                  bool compilerIsMSVC, StringAnsi& output)
    {
        StringAnsi fullName = GetNativeName(cls.namespaceName, cls.name);
        StringAnsi fullTypename = GetFullCSTypeName(cls.namespaceName, cls.name);
        StringAnsi internalName = GetInternalClassName(cls);
        StringAnsi bodyOut, endOut;

        // Internal class header
        if (!cls.namespaceName.IsEmpty())
        {
            output += StringAnsi::Format("namespace {0}\n{{\n", cls.namespaceName);
        }

        output += StringAnsi::Format("class {0}\n{{\npublic:\n", internalName);

        // Field accessors — for structs, non-static fields get direct getter/setter
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

        // InitRuntime
        GenerateCppInitRuntime(cls, bodyOut);

        // Ctor
        bodyOut += "    static void Ctor(void* ptr)\n    {\n";
        bodyOut += StringAnsi::Format("        new(ptr){0}();\n", fullName);
        bodyOut += "    }\n";

        // Dtor
        bodyOut += "    static void Dtor(void* ptr)\n    {\n";
        bodyOut += StringAnsi::Format("        (({0}*)ptr)->~{1}();\n", fullName, cls.name);
        bodyOut += "    }\n";

        // Copy
        bodyOut += "    static void Copy(void* dst, void* src)\n    {\n";
        bodyOut += StringAnsi::Format("        *({0}*)dst = *({0}*)src;\n", fullName);
        bodyOut += "    }\n";

        // Box — POD vs Non-POD paths
        bool isPod = IsPodType(fullName);
        bodyOut += "    static CLRObject* Box(void* ptr)\n    {\n";
        if (isPod)
        {
            bodyOut += StringAnsi::Format("        return CLRCore::Object::Box(ptr, {0}::TypeInitializer->GetClass());\n", fullName);
        }
        else
        {
            bodyOut += StringAnsi::Format("        return CLRUtils::Box(*({0}*)ptr, {0}::TypeInitializer->GetClass());\n", fullName);
        }
        bodyOut += "    }\n";

        // Unbox — POD vs Non-POD paths
        bodyOut += "    static void Unbox(void* ptr, CLRObject* managed)\n    {\n";
        if (isPod)
        {
            bodyOut += StringAnsi::Format("        Platform::MemoryCopy(ptr, CLRCore::Object::Unbox(managed), sizeof({0}));\n", fullName);
        }
        else
        {
            bodyOut += StringAnsi::Format("        *({0}*)ptr = {0}::ToNative(*({0}Managed*)CLRCore::Object::Unbox(managed));\n", fullName);
        }
        bodyOut += "    }\n";

        // GetField
        bodyOut += "    static void GetField(void* ptr, const String& name, Variant& value)\n    {\n";
        int count = 0;
        for (auto& field : cls.fields)
        {
            if (field.isReadOnly || field.isStatic || field.isHidden)
                continue;
            if (count == 0)
                bodyOut += StringAnsi::Format("        if (name == SE_TEXT(\"{0}\"))\n", field.name);
            else
                bodyOut += StringAnsi::Format("        else if (name == SE_TEXT(\"{0}\"))\n", field.name);
            StringAnsi fieldAccess = StringAnsi::Format("(({0}*)ptr)->{1}", fullName, field.name);
            StringAnsi converted = GetNativeToVariantConvert(field.cppType, fieldAccess);
            bodyOut += StringAnsi::Format("            value = {0};\n", converted);
            count++;
        }
        bodyOut += "    }\n";

        // SetField
        bodyOut += "    static void SetField(void* ptr, const String& name, const Variant& value)\n    {\n";
        count = 0;
        for (auto& field : cls.fields)
        {
            if (field.isReadOnly || field.isStatic || field.isHidden)
                continue;
            if (count == 0)
                bodyOut += StringAnsi::Format("        if (name == SE_TEXT(\"{0}\"))\n", field.name);
            else
                bodyOut += StringAnsi::Format("        else if (name == SE_TEXT(\"{0}\"))\n", field.name);
            if (field.arraySize > 0)
            {
                bodyOut += "        {\n";
                bodyOut += "            CHECK(value.Type.Type == VariantType::Array);\n";
                bodyOut += "            auto* array = reinterpret_cast<const Array<Variant, HeapAllocation>*>(value.AsData);\n";
                bodyOut += StringAnsi::Format("            for (int32 i = 0; i < {0} && i < array->Count(); i++)\n", field.arraySize);
                StringAnsi elemType = StripTypeQualifiers(field.cppType);
                bodyOut += StringAnsi::Format("                (({0}*)ptr)->{1}[i] = *({2}*)array->At(i).AsBlob.Data;\n", fullName, field.name, elemType);
                bodyOut += "        }\n";
            }
            else
            {
                StringAnsi converted = GetVariantToNativeConvert(field.cppType, "value");
                bodyOut += StringAnsi::Format("            (({0}*)ptr)->{1} = {2};\n", fullName, field.name, converted);
            }
            count++;
        }
        bodyOut += "    }\n";

        output += bodyOut;
        output += "};\n\n";

        // ScriptingTypeInitializer — struct-specific signature (Ctor, Dtor, Copy, Box, Unbox, GetField, SetField)
        output += StringAnsi::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n", fullName);
        output += StringAnsi::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += StringAnsi::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullTypename);
        output += StringAnsi::Format("    sizeof({0}),\n", fullName);
        output += StringAnsi::Format("    &{0}::InitRuntime,\n", internalName);
        output += StringAnsi::Format("    &{0}::Ctor, &{0}::Dtor,\n", internalName);
        output += StringAnsi::Format("    &{0}::Copy, &{0}::Box, &{0}::Unbox,\n", internalName);
        output += StringAnsi::Format("    &{0}::GetField, &{0}::SetField,\n", internalName);
        if (!cls.baseClassName.IsEmpty())
        {
            output += StringAnsi::Format("    {0}::TypeInitializer\n", cls.baseClassName);
        }
        else
        {
            output += StringAnsi("    nullptr\n");
        }
        output += ");\n";

        // Clang plain-C exports
        if (!compilerIsMSVC && endOut.Length() > 0)
        {
            output += StringAnsi::Format("\n// Clang plain-C exports\n{0}", endOut);
        }

        if (!cls.namespaceName.IsEmpty())
        {
            output += "}\n";
        }

        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Enum generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppEnum(const ApiEnum& en, const StringAnsi& assemblyType,
                                                StringAnsi& output)
    {
        StringAnsi fullType = GetNativeName(en.namespaceName, en.name);

        if (!en.namespaceName.IsEmpty())
            output += StringAnsi::Format("namespace {0}\n{{\n", en.namespaceName);

        output += StringAnsi::Format("class {0}Internal\n{{\npublic:\n", en.name);
        output += "    static ScriptingType::EnumItem Items[];\n";
        output += "};\n\n";

        // Items array
        output += StringAnsi::Format("ScriptingType::EnumItem {0}Internal::Items[] = {{\n", en.name);
        for (int i = 0; i < en.valueNames.Count(); ++i)
        {
            output += StringAnsi::Format("    {{ (uint64){0}::{1}, \"{1}\" }},\n", fullType, en.valueNames[i]);
        }
        output += "    { 0, nullptr }\n};\n\n";

        // ScriptingTypeInitializer
        output += StringAnsi::Format("ScriptingTypeInitializer {0}_TypeInitializer(\n", en.name);
        output += StringAnsi::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += StringAnsi::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullType);
        output += StringAnsi::Format("    sizeof({0}),\n", fullType);
        output += StringAnsi::Format("    {0}Internal::Items\n", en.name);
        output += ");\n\n";

        // StaticType specialization
        output += StringAnsi::Format("template<> SE_API_{0} ScriptingTypeHandle StaticType<{1}>() {{ return {2}_TypeInitializer; }}\n",
            assemblyType, fullType, en.name);

        if (!en.namespaceName.IsEmpty())
        {
            output += "}\n";
        }
    }

    // -------------------------------------------------------------------------
    // Interface generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppInterface(const ApiInterface& iface, const StringAnsi& assemblyType,
                                                     StringAnsi& output)
    {
        StringAnsi fullType;
        if (!iface.namespaceName.IsEmpty())
            fullType = StringAnsi::Format("{0}::{1}", iface.namespaceName, iface.name);
        else
            fullType = iface.name;

        if (!iface.namespaceName.IsEmpty())
            output += StringAnsi::Format("namespace {0}\n{{\n", iface.namespaceName);

        // Wrapper class
        output += StringAnsi::Format("class {0}Wrapper : public {0}\n{{\npublic:\n", iface.name);
        output += "    ScriptingObject* Object;\n";

        for (auto& fn : iface.functions)
        {
            StringAnsi paramTypes;
            for (int i = 0; i < fn.params.Count(); ++i)
            {
                if (i > 0) paramTypes += ", ";
                paramTypes += fn.params[i].cppType;
            }

            output += StringAnsi::Format("    {0} {1}({2}) const override\n",
                fn.returnType, fn.name, paramTypes);
            output += "    {\n";
            if (fn.params.Count() > 0)
            {
                output += StringAnsi::Format("        Variant parameters[{0}];\n", fn.params.Count());
                for (int i = 0; i < fn.params.Count(); ++i)
                {
                    StringAnsi convertExpr = GetNativeToVariantConvert(fn.params[i].cppType, fn.params[i].name);
                    output += StringAnsi::Format("        parameters[{0}] = {1};\n", i, convertExpr);
                }
            }

            output +=                   "        auto typeHandle = Object->GetTypeHandle();\n";
            output +=                   "        while (typeHandle)\n        {\n";
            output += StringAnsi::Format("            auto method = typeHandle.Module->FindMethod(typeHandle, StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1), {1});\n",
                fn.name, fn.params.Count());
            output +=                   "            if (method)\n            {\n";
            output +=                   "                Variant __result;\n";
            if (fn.params.Count() > 0)
            {
                output += StringAnsi::Format("                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(parameters, {0}), __result);\n", fn.params.Count());
            }
            else
            {
                output +=               "                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(), __result);\n";
            }
            if (fn.returnType != "void")
            {
                output += StringAnsi::Format("                return ({0})__result;\n", fn.returnType);
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
        output += StringAnsi::Format("class {0}Internal\n{{\npublic:\n", iface.name);
        output += StringAnsi::Format("    static void InitRuntime() {{ }}\n");
        output += StringAnsi::Format("    static void* GetInterfaceWrapper(ScriptingObject* __obj)\n    {{\n");
        output += StringAnsi::Format("        auto wrapper = New<{0}Wrapper>();\n", iface.name);
        output += "        wrapper->Object = __obj;\n";
        output += "        return wrapper;\n";
        output += "    }\n";
        output += "};\n\n";

        if (!iface.namespaceName.IsEmpty())
        {
            output += "}\n";
        }

        // ScriptingTypeInitializer
        output += StringAnsi::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n", fullType);
        output += StringAnsi::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += StringAnsi::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullType);
        output += StringAnsi::Format("    &{0}Internal::InitRuntime,\n", iface.name);
        output += "    nullptr,\n    nullptr,\n";
        output += StringAnsi::Format("    &{0}Internal::GetInterfaceWrapper\n", iface.name);
        output += ");\n";
    }

    // -------------------------------------------------------------------------
    // Generate — entry point for a single header
    // -------------------------------------------------------------------------

    bool BindingsCppGenerator::Generate(const BindingsHeaderInfo& headerInfo,
                                         const String& solutionRoot,
                                         bool compilerIsMSVC)
    {
        if (headerInfo.classes.IsEmpty() && headerInfo.enums.IsEmpty()
            && headerInfo.interfaces.IsEmpty() && headerInfo.events.IsEmpty())
            return true;

        // Derive assemblyType
        StringAnsi assemblyType = headerInfo.assemblyName;
        if (assemblyType.StartsWith("SE."))
        {
            assemblyType = assemblyType.Substring(3);
        }

        StringAnsi output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator — do not edit manually.\n";
        output += StringAnsi::Format("// Source: {0}\n", headerInfo.filePath);
        output += "//-------------------------------------------------------------------------\n";
        output += StringAnsi::Format("#include \"{0}\"\n", headerInfo.filePath);
        output += "#include \"Runtime/Scripting/ManagedCLR/CLRUtils.h\"\n";
        output += "#include \"Runtime/Scripting/ScriptingObject.h\"\n";
        output += "#include \"Runtime/Scripting/Internal/InternalCalls.h\"\n";
        output += "#include \"Runtime/Scripting/ScriptingType.h\"\n";

        // Conditional includes for events
        bool hasEvents = false;
        for (auto& cls : headerInfo.classes)
        {
            if (!cls.events.IsEmpty())
            {
                hasEvents = true;
                break;
            }
        }
        if (hasEvents)
        {
            output += "#include \"Runtime/Scripting/Events.h\"\n";
        }

        // Interfaces
        if (!headerInfo.interfaces.IsEmpty())
        {
            output += "#include \"Runtime/Scripting/ManagedCLR/CLRClass.h\"\n";
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

        StringAnsi baseName = FileSystem::GetFileNameWithoutExtension(headerInfo.filePath.ToString()).ToStringAnsi();
        String assemblyDir = headerInfo.assemblyDir.ToString();
        String outDir = assemblyDir + Settings::g_autogeneratedDirectory;
        if (!FileSystem::DirectoryExists(outDir))
            FileSystem::CreateDirectory(outDir);

        String outPath = String::Format(SE_TEXT("{0}/{1}.Gen.cpp"), outDir, baseName);
        return SaveFile(outPath, output);
    }

    bool BindingsCppGenerator::GenerateAll(const List<BindingsHeaderInfo>& headers,
                                            const String& solutionRoot,
                                            bool compilerIsMSVC)
    {
        for (auto& h : headers)
        {
            if (!Generate(h, solutionRoot, compilerIsMSVC))
            {
                return false;
            }
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Binary module generation
    // -------------------------------------------------------------------------

    bool BindingsCppGenerator::GenerateBinaryModule(const BinaryModuleInfo& module, const String& solutionRoot)
    {
        StringAnsi headerOut;
        headerOut += "//-------------------------------------------------------------------------\n";
        headerOut += "// Auto-generated by BindingsGenerator — do not edit manually.\n";
        headerOut += "//-------------------------------------------------------------------------\n";
        headerOut += "#pragma once\n\n";
        headerOut += StringAnsi::Format("#define {0}_NAME \"{1}\"\n", module.assemblyType, module.name);
        headerOut += StringAnsi::Format("#define {0}_VERSION Version(1, 0)\n", module.assemblyType);
        headerOut += StringAnsi::Format("#define {0}_VERSION_TEXT \"1.0\"\n", module.assemblyType);
        headerOut += "\nclass BinaryModule;\n";
        headerOut += StringAnsi::Format("extern \"C\" {0}_API BinaryModule* GetBinaryModule{0}();\n", module.assemblyType);

        StringAnsi sourceOut;
        sourceOut += "//-------------------------------------------------------------------------\n";
        sourceOut += "// Auto-generated by BindingsGenerator — do not edit manually.\n";
        sourceOut += "//-------------------------------------------------------------------------\n";
        sourceOut += "#include \"Engine/Scripting/BinaryModule.h\"\n";
        sourceOut += StringAnsi::Format("#include \"{0}.Gen.h\"\n\n", module.assemblyType);
        sourceOut += StringAnsi::Format("StaticallyLinkedBinaryModuleInitializer StaticallyLinkedBinaryModule{0}(GetBinaryModule{0}());\n\n", module.assemblyType);
        sourceOut += StringAnsi::Format("extern \"C\" BinaryModule* GetBinaryModule{0}()\n{{\n", module.assemblyType);
        sourceOut += StringAnsi::Format("    static NativeBinaryModule module(\"{0}\");\n", module.name);
        sourceOut += "    return &module;\n}\n";

        String outDir = String::Format(SE_TEXT("{0}{1}"), module.assemblyDir, Settings::g_autogeneratedDirectory);
        if (!FileSystem::DirectoryExists(outDir))
        {
            FileSystem::CreateDirectory(outDir);
        }

        String headerPath = String::Format(SE_TEXT("{0}{1}.Gen.h"), outDir, module.assemblyType);
        String sourcePath = String::Format(SE_TEXT("{0}{1}.Gen.cpp"), outDir, module.assemblyType);

        if (!SaveFile(headerPath, headerOut))
        {
            return false;
        }
        if (!SaveFile(sourcePath, sourceOut))
        {
            return false;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // SaveFile
    // -------------------------------------------------------------------------

    bool BindingsCppGenerator::SaveFile(const String& path, const StringAnsi& content)
    {
        String parentDir = FileSystem::GetParentDirectory(path);
        if (!FileSystem::DirectoryExists(parentDir))
        {
            FileSystem::CreateDirectory(parentDir);
        }

        return File::WriteAllText(path, content.ToString(), Encoding::EncodingType::UTF8);;
    }

} // namespace SE::ReflectTool