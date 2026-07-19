// BindingsCSharpGenerator.cpp
// Generates C# binding declarations using direct string building.

#include "CodeGenerator_BindingsCSharp.h"
#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Core/FileSystem.h"
#include "Core/Utils.h"

#include <fstream>
#include <string>

#include "CodeGenerator_Utils.h"

namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static bool IsInjectedCSharpCode(ApiInjectedCode const& code)
    {
        return Utils::String::ToLowerCopy(code.lang) == "csharp";
    }

    std::string BindingsCSharpGenerator::GetAccessString(AccessLevel access) const
    {
        switch (access)
        {
        case AccessLevel::Private:   return "private";
        case AccessLevel::Protected: return "protected";
        case AccessLevel::Public:    return "public";
        case AccessLevel::Internal:  return "internal";
        default:                     return "public";
        }
    }

    static void GenerateCSharpComment(std::string& output, const std::string& indent, const std::string& comment)
    {
        if (comment.empty())
            return;
        output += Utils::String::Format("{0}/// <summary>\n", indent);
        output += Utils::String::Format("{0}/// {1}\n", indent, EscapeCSharpXml(comment));
        output += Utils::String::Format("{0}/// </summary>\n", indent);
    }

    static bool IsValidCSharpAttributeList(const std::string& attributes)
    {
        if (attributes.empty() || !Utils::String::StartsWith(attributes, "["))
            return false;

        const char* raw = attributes.c_str();
        for (int i = 0; i < attributes.length(); ++i)
        {
            const unsigned char ch = (unsigned char)raw[i];
            if (ch < 32 && ch != '\r' && ch != '\n' && ch != '\t')
                return false;
        }
        return true;
    }

    static void AppendDllImport(std::string& output, const std::string& assemblyName, const std::string& entryPoint)
    {
        output += Utils::String::Format("        [DllImport(\"{0}\", EntryPoint = \"{1}\", CharSet = CharSet.Unicode)]\n",assemblyName, entryPoint);
    }

    static std::string NormalizeCSharpDefaultValue(const ApiParam& param)
    {
        std::string value = param.defaultValue;
        Utils::String::TrimStart(value);
        Utils::String::TrimEnd(value);
        if (value.empty())
            return value;

        Utils::String::ReplaceAll(value, " :: ", "::");
        Utils::String::ReplaceAll(value, ":: ", "::");
        Utils::String::ReplaceAll(value, " ::", "::");
        Utils::String::ReplaceAll(value, "nullptr", "null");
        Utils::String::ReplaceAll(value, "NULL", "null");

        int pos;
        while ((pos = Utils::String::Find(value, "::")) != INVALID_INDEX)
        {
            value = value.substr(0, pos) + "." + value.substr(pos + 2);
        }
        return value;
    }

    std::string BindingsCSharpGenerator::BuildCSharpParams(const ApiFunction& fn, bool forPublic) const
    {
        std::string params;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0) params += ", ";

            std::string type = forPublic ? GetCSharpPublicType(fn.params[i].cppType) : GetCSharpInteropType(fn.params[i].cppType);

            // Pass by ref for non-interop
            if (forPublic && UsePassByReference(fn.params[i].cppType) && !fn.params[i].isOut)
            {
                params += Utils::String::Format("ref {0} {1}", type, MakeCSharpIdentifier(fn.params[i].name));
            }
            else if (forPublic && fn.params[i].isOut)
            {
                params += Utils::String::Format("out {0} {1}", type, MakeCSharpIdentifier(fn.params[i].name));
            }
            else
            {
                // Add marshal attribute for interop params
                if (!forPublic)
                {
                    std::string marshalAttr = GetCSharpParamMarshalAttribute(fn.params[i].cppType, fn.params[i].name);
                    if (!marshalAttr.empty())
                        params += Utils::String::Format("{0} ", marshalAttr);
                }
                std::string defaultValue;
                if (forPublic && !fn.params[i].defaultValue.empty() && !fn.params[i].isOut && !UsePassByReference(fn.params[i].cppType))
                    defaultValue = Utils::String::Format(" = {0}", NormalizeCSharpDefaultValue(fn.params[i]));
                params += Utils::String::Format("{0} {1}{2}", type, MakeCSharpIdentifier(fn.params[i].name), defaultValue);
            }
        }
        return params;
    }

    std::string BindingsCSharpGenerator::BuildCSharpInteropParams(const ApiClass& cls, const ApiFunction& fn) const
    {
        std::string params;
        if (!fn.isStatic)
            params += "IntPtr __obj";

        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (params.length() > 0) params += ", ";
            std::string interopType = GetCSharpInteropType(fn.params[i].cppType);
            std::string marshalAttr = GetCSharpParamMarshalAttribute(fn.params[i].cppType, fn.params[i].name);

            if (!marshalAttr.empty())
                params += Utils::String::Format("{0} ", marshalAttr);
            if (fn.params[i].isOut)
                params += "out ";
            else if (UsePassByReference(fn.params[i].cppType))
                params += "ref ";
            params += Utils::String::Format("{0} {1}", interopType, MakeCSharpIdentifier(fn.params[i].name));
        }
        return params;
    }

    std::string BindingsCSharpGenerator::BuildCSharpCallArgs(const ApiClass& cls, const ApiFunction& fn, bool isInterop) const
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
                std::string toInterop = GetCSharpToInterop(fn.params[i].cppType, paramName);
                if (fn.params[i].isOut)
                    args += Utils::String::Format("out {0}", paramName);
                else if (UsePassByReference(fn.params[i].cppType))
                    args += Utils::String::Format("ref {0}", toInterop);
                else
                    args += toInterop;
            }
            else
            {
                // Public call - forward as-is with ref/out keywords
                if (fn.params[i].isOut)
                    args += Utils::String::Format("out {0}", MakeCSharpIdentifier(fn.params[i].name));
                else if (UsePassByReference(fn.params[i].cppType))
                    args += Utils::String::Format("ref {0}", MakeCSharpIdentifier(fn.params[i].name));
                else
                    args += MakeCSharpIdentifier(fn.params[i].name);
            }
        }
        return args;
    }

    // -------------------------------------------------------------------------
    // [DllImport] declaration for a single function
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                                                const std::string& assemblyName, std::string& output)
    {
        std::string interopRetType = GetCSharpInteropType(fn.returnType);
        std::string interopParams = BuildCSharpInteropParams(cls, fn);
        std::string returnMarshalAttr = GetCSharpReturnMarshalAttribute(fn.returnType);
        std::string internalName = Utils::String::Format("Internal_{0}", fn.uniqueName);

        AppendDllImport(output, assemblyName, fn.entryPoint);
        if (!returnMarshalAttr.empty())
        {
            output += Utils::String::Format("        {0}\n", returnMarshalAttr);
        }
        output += Utils::String::Format("        internal static extern {0} {1}({2});\n\n", interopRetType, internalName, interopParams);
    }

    // -------------------------------------------------------------------------
    // Public wrapper function call (calls the InternalCall)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunctionCall(const ApiClass& cls, const ApiFunction& fn,
                                                                     std::string& output)
    {
        std::string publicRetType = GetCSharpPublicType(fn.returnType);
        std::string publicParams = BuildCSharpParams(fn, true);
        bool retIsVoid = (fn.returnType == "void");
        std::string access = GetAccessString(fn.isHidden ? AccessLevel::Private : AccessLevel::Public);
        std::string staticKeyword = fn.isStatic ? "static " : "";

        GenerateCSharpComment(output, "        ", fn.comment);
        if (IsValidCSharpAttributeList(fn.attributes))
            output += Utils::String::Format("        {0}\n", fn.attributes);
        output += Utils::String::Format("        {0} {1}{2}{3} {4}({5})\n",
            access, staticKeyword,
            retIsVoid ? "void" : publicRetType,
            std::string(" "), MakeCSharpIdentifier(fn.name), publicParams);
        output += "        {\n";

        std::string interopCall = Utils::String::Format("Internal_{0}({1})",
            fn.uniqueName, BuildCSharpCallArgs(cls, fn, true));

        if (retIsVoid)
        {
            output += Utils::String::Format("            {0};\n", interopCall);
        }
        else
        {
            std::string fromInterop = GetCSharpFromInterop(fn.returnType, interopCall);
            output += Utils::String::Format("            return {0};\n", fromInterop);
        }
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Property accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                                                   const std::string& assemblyName, std::string& output)
    {
        std::string publicType = GetCSharpPublicType(prop.cppType);
        std::string interopType = GetCSharpInteropType(prop.cppType);

        // Getter [DllImport]
        if (prop.hasGetter)
        {
            std::string getterMarshal = GetCSharpReturnMarshalAttribute(prop.cppType);
            AppendDllImport(output, assemblyName, prop.getterEntryPoint);
            if (!getterMarshal.empty())
                output += Utils::String::Format("        {0}\n", getterMarshal);
            output += Utils::String::Format("        internal static extern {0} Internal_{1}(IntPtr __obj);\n\n",
                interopType, prop.getterUniqueName);
        }

        // Setter [DllImport]
        if (prop.hasSetter)
        {
            std::string setterParamMarshal = GetCSharpParamMarshalAttribute(prop.cppType, "value");
            std::string setterParamPrefix;
            if (!setterParamMarshal.empty())
                setterParamPrefix = Utils::String::Format("{0} ", setterParamMarshal);

            AppendDllImport(output, assemblyName, prop.setterEntryPoint);
            output += Utils::String::Format("        internal static extern void Internal_{0}(IntPtr __obj, {1}{2} value);\n\n",
                prop.setterUniqueName, setterParamPrefix, interopType);
        }

        // Public property declaration
        std::string propAccess = GetAccessString(prop.getterAccess);
        GenerateCSharpComment(output, "        ", prop.comment);
        if (IsValidCSharpAttributeList(prop.attributes))
            output += Utils::String::Format("        {0}\n", prop.attributes);
        output += Utils::String::Format("        {0} {1} {2}\n        {\n", propAccess, publicType, MakeCSharpIdentifier(prop.name));

        if (prop.hasGetter)
        {
            std::string getterCall = Utils::String::Format("Internal_{0}(__unmanagedPtr)", prop.getterUniqueName);
            std::string fromInterop = GetCSharpFromInterop(prop.cppType, getterCall);
            std::string getterAccess = GetAccessString(prop.getterAccess);
            output += Utils::String::Format("            {0}get { return {1}; }\n", getterAccess, fromInterop);
        }

        if (prop.hasSetter)
        {
            std::string toInterop = GetCSharpToInterop(prop.cppType, std::string("value"));
            std::string setterAccess = GetAccessString(prop.setterAccess);
            output += Utils::String::Format("            {0}set { Internal_{1}(__unmanagedPtr, {2}); }\n",
                setterAccess, prop.setterUniqueName, toInterop);
        }

        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Field accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpFieldAccessors(const ApiClass& cls, const ApiField& field,
                                                                const std::string& assemblyName, std::string& output)
    {
        std::string publicType = GetCSharpPublicType(field.cppType);
        std::string interopType = GetCSharpInteropType(field.cppType);
        std::string stripped = StripTypeQualifiers(field.cppType);
        const TypeMapping* map = FindTypeMapping(stripped.c_str());

        // Static fields become properties (get/set via InternalCall)
        // Instance fields with arraySize become fixed-size buffers
        // Regular instance fields become direct fields

        if (field.isStatic)
        {
            // Static field → [DllImport] getter/setter + public property

            // Getter InternalCall
            std::string getterMarshal = GetCSharpReturnMarshalAttribute(field.cppType);
            AppendDllImport(output, assemblyName, Utils::String::Format("{0}_{1}_Get", cls.name, field.name));
            if (!getterMarshal.empty())
                output += Utils::String::Format("        {0}\n", getterMarshal);
            output += Utils::String::Format("        internal static extern {0} Internal_{1}_Get(IntPtr __obj);\n\n",
                interopType, field.name);

            // Setter InternalCall (if not readonly)
            if (!field.isReadOnly)
            {
                std::string setterParamMarshal = GetCSharpParamMarshalAttribute(field.cppType, "value");
                std::string setterParamPrefix;
                if (!setterParamMarshal.empty())
                    setterParamPrefix = Utils::String::Format("{0} ", setterParamMarshal);

                AppendDllImport(output, assemblyName, Utils::String::Format("{0}_{1}_Set", cls.name, field.name));
                output += Utils::String::Format("        internal static extern void Internal_{0}_Set(IntPtr __obj, {1}{2} value);\n\n",
                    field.name, setterParamPrefix, interopType);
            }

            // Public static property
            std::string fieldAccess = GetAccessString(field.isHidden ? AccessLevel::Private : AccessLevel::Public);
            GenerateCSharpComment(output, "        ", field.comment);
            if (IsValidCSharpAttributeList(field.attributes))
                output += Utils::String::Format("        {0}\n", field.attributes);
            output += Utils::String::Format("        {0} static {1} {2}\n        {\n",
                fieldAccess, publicType, MakeCSharpIdentifier(field.name));

            std::string getterCall = Utils::String::Format("Internal_{0}_Get(IntPtr.Zero)", field.name);
            std::string fromInterop = GetCSharpFromInterop(field.cppType, getterCall);
            output += Utils::String::Format("            get { return {0}; }\n", fromInterop);

            if (!field.isReadOnly)
            {
                std::string toInterop = GetCSharpToInterop(field.cppType, std::string("value"));
                output += Utils::String::Format("            set { Internal_{0}_Set(IntPtr.Zero, {1}); }\n",
                    field.name, toInterop);
            }
            output += "        }\n\n";
        }
        else if (field.arraySize > 0)
        {
            // Fixed-size array → fixed buffer
            GenerateCSharpComment(output, "        ", field.comment);
            output += Utils::String::Format("        public fixed {0} {1}[{2}];\n\n",
                publicType, MakeCSharpIdentifier(field.name), field.arraySize);
        }
        else
        {
            // Regular instance field
            std::string fieldAccess = GetAccessString(field.isHidden ? AccessLevel::Private : AccessLevel::Public);
            GenerateCSharpComment(output, "        ", field.comment);
            if (IsValidCSharpAttributeList(field.attributes))
                output += Utils::String::Format("        {0}\n", field.attributes);
            std::string fieldDecl = Utils::String::Format("        {0} {1} {2}", fieldAccess, publicType, MakeCSharpIdentifier(field.name));

            // Add marshal attributes for special types
            if (stripped == "bool")
                fieldDecl = Utils::String::Format("        [MarshalAs(UnmanagedType.U1)]\n        {0} {1} {2}", fieldAccess, publicType, MakeCSharpIdentifier(field.name));
            else if (map && map->isString)
                fieldDecl = Utils::String::Format("        [MarshalAs(UnmanagedType.LPUTF8Str)]\n        {0} {1} {2}", fieldAccess, publicType, MakeCSharpIdentifier(field.name));

            fieldDecl += ";\n\n";
            output += fieldDecl;
        }
    }

    // -------------------------------------------------------------------------
    // Event accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpEventAccessors(const ApiClass& cls, const ApiEvent& evt,
                                                                const std::string& assemblyName, std::string& output)
    {
        // Build the C# delegate type for the event
        std::string delegateParams;
        for (int i = 0; i < evt.params.size(); ++i)
        {
            if (i > 0) delegateParams += ", ";
            std::string publicType = GetCSharpPublicType(evt.params[i].cppType);
            if (UsePassByReference(evt.params[i].cppType))
                delegateParams += Utils::String::Format("ref {0} {1}", publicType, evt.params[i].name);
            else
                delegateParams += Utils::String::Format("{0} {1}", publicType, evt.params[i].name);
        }

        // _Bind InternalCall
        AppendDllImport(output, assemblyName, Utils::String::Format("{0}_{1}_ManagedBind", cls.name, evt.name));
        output += Utils::String::Format("        internal static extern void Internal_{0}_Bind(IntPtr __obj, bool bind);\n\n", evt.name);

        // _Invoke InternalCall
        std::string invokeParams = "IntPtr __obj";
        for (int i = 0; i < evt.params.size(); ++i)
        {
            invokeParams += ", ";
            std::string interopType = GetCSharpInteropType(evt.params[i].cppType);
            std::string marshalAttr = GetCSharpParamMarshalAttribute(evt.params[i].cppType, evt.params[i].name);
            if (!marshalAttr.empty())
                invokeParams += Utils::String::Format("{0} ", marshalAttr);
            invokeParams += Utils::String::Format("{0} {1}", interopType, evt.params[i].name);
        }
        AppendDllImport(output, assemblyName, Utils::String::Format("{0}_{1}_ManagedWrapper", cls.name, evt.name));
        output += Utils::String::Format("        internal static extern void Internal_{0}_Invoke({1});\n\n", evt.name, invokeParams);

        // Binding count tracking
        std::string evtAccess = GetAccessString(evt.access);
        output += Utils::String::Format("        private int _{0}BindCount;\n", evt.name);

        // Event declaration with add/remove
        output += Utils::String::Format("        {0} event Action<{1}> {2}\n        {\n",
            evtAccess, delegateParams, evt.name);
        output += "            add\n            {\n";
        output += Utils::String::Format("                if (Interlocked.Exchange(ref _{0}BindCount, _{0}BindCount + 1) == 0)\n", evt.name);
        output += Utils::String::Format("                    Internal_{0}_Bind(__unmanagedPtr, true);\n", evt.name);
        output += "                _" + evt.name + " += value;\n";
        output += "            }\n";
        output += "            remove\n            {\n";
        output += Utils::String::Format("                if (Interlocked.Decrement(ref _{0}BindCount) == 0)\n", evt.name);
        output += Utils::String::Format("                    Internal_{0}_Bind(__unmanagedPtr, false);\n", evt.name);
        output += "                _" + evt.name + " -= value;\n";
        output += "            }\n";
        output += "        }\n\n";

        // Private backing delegate
        output += Utils::String::Format("        private event Action<{0}> _{1};\n\n", delegateParams, evt.name);
    }

    // -------------------------------------------------------------------------
    // Class marshaller (ManagedHandleMarshaller for ScriptingObject types)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpClassMarshaller(const ApiClass& cls, std::string& output)
    {
        output += Utils::String::Format("        internal struct {0}Marshaller\n        {{\n", cls.name);
        if (cls.isAbstract)
            output += Utils::String::Format("            public static {0} ConvertToManaged(IntPtr unmanaged) => null;\n", cls.name);
        else
            output += Utils::String::Format("            public static {0} ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? new {0}() {{ __unmanagedPtr = unmanaged }} : null;\n", cls.name);
        output += Utils::String::Format("            public static IntPtr ConvertToUnmanaged({0} managed) => managed != null ? managed.__unmanagedPtr : IntPtr.Zero;\n", cls.name);
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Struct marshaller (CustomMarshaller with blittable internal)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpStructMarshaller(const ApiClass& cls, std::string& output)
    {
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.Default, typeof({0}Marshaller))]\n", cls.name);
        output += Utils::String::Format("        internal static partial class {0}Marshaller\n        {{\n", cls.name);

        // Blittable internal representation
        output += Utils::String::Format("            internal struct {0}Internal\n            {{\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            std::string fieldType = GetCSharpInteropType(field.cppType);
            if (field.arraySize > 0)
                output += Utils::String::Format("                public fixed {0} {1}[{2}];\n", fieldType, field.name, field.arraySize);
            else
                output += Utils::String::Format("                public {0} {1};\n", fieldType, field.name);
        }
        output += "            }\n\n";

        // ToManaged
        output += Utils::String::Format("            public static {0} ToManaged({0}Internal unmanaged)\n            {{\n", cls.name);
        output += Utils::String::Format("                var result = new {0}();\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            if (field.arraySize > 0)
            {
                output += Utils::String::Format("                for (int i = 0; i < {0}; i++) result.{1}[i] = unmanaged.{1}[i];\n",
                    field.arraySize, field.name);
            }
            else
            {
                std::string fromInterop = GetCSharpFromInterop(field.cppType, Utils::String::Format("unmanaged.{0}", field.name));
                output += Utils::String::Format("                result.{0} = {1};\n", field.name, fromInterop);
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        // ToNative
        output += Utils::String::Format("            public static {0}Internal ToNative({0} managed)\n            {{\n", cls.name);
        output += Utils::String::Format("                var result = new {0}Internal();\n", cls.name);
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            if (field.arraySize > 0)
            {
                output += Utils::String::Format("                for (int i = 0; i < {0}; i++) result.{1}[i] = managed.{1}[i];\n",
                    field.arraySize, field.name);
            }
            else
            {
                std::string toInterop = GetCSharpToInterop(field.cppType, Utils::String::Format("managed.{0}", field.name));
                output += Utils::String::Format("                result.{0} = {1};\n", field.name, toInterop);
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        // Free
        output += Utils::String::Format("            public static void Free({0}Internal unmanaged) {{ }}\n", cls.name);

        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpClass(const ApiClass& cls, const std::string& assemblyName,
                                                        std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceNameList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        // Class declaration
        std::string classKeyword = cls.isStatic ? "static " : "";
        std::string sealedKeyword = cls.isSealed ? "sealed " : "";
        std::string abstractKeyword = cls.isAbstract ? "abstract " : "";

        GenerateCSharpComment(output, "    ", cls.comment);
        // User attributes
        if (IsValidCSharpAttributeList(cls.attributes))
            output += Utils::String::Format("    {0}\n", cls.attributes);

        output += Utils::String::Format("    public unsafe {0}{1}{2}{3}partial class {4}",
            abstractKeyword, classKeyword, sealedKeyword, cls.isStruct ? "" : "", MakeCSharpIdentifier(cls.name));

        // Base class
        if (!cls.baseClassName.empty())
        {
            output += Utils::String::Format(" : {0}", GetCSharpPublicType(cls.baseClassName));
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
                output += cls.interfaces[i].name;
            }
        }

        output += "\n    {\n";

        // Unmanaged pointer
        if (!cls.isStatic)
        {
            output += "        internal IntPtr __unmanagedPtr = IntPtr.Zero;\n\n";
        }

        // Constructor (if not abstract, not static, not noConstructor)
        if (!cls.isAbstract && !cls.isStatic && !cls.noConstructor && !cls.noSpawn)
        {
            output += Utils::String::Format("        public {0}() {{ }}\n\n", MakeCSharpIdentifier(cls.name));
            output += Utils::String::Format("        internal {0}(IntPtr nativePtr) {{ __unmanagedPtr = nativePtr; }}\n\n", MakeCSharpIdentifier(cls.name));
        }

        // Events
        for (auto& evt : cls.events)
        {
            GenerateCSharpEventAccessors(cls, evt, assemblyName, output);
        }

        // Properties
        for (auto& prop : cls.properties)
        {
            GenerateCSharpPropertyAccessors(cls, prop, assemblyName, output);
        }

        // Functions - first generate all [LibraryImport] declarations, then public wrappers
        for (auto& fn : cls.functions)
        {
            GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);
        }

        for (auto& fn : cls.functions)
        {
            GenerateCSharpWrapperFunctionCall(cls, fn, output);
        }

        // Fields
        for (auto& field : cls.fields)
        {
            GenerateCSharpFieldAccessors(cls, field, assemblyName, output);
        }

        // Marshaller
        if (!cls.isStruct && !cls.isStatic)
        {
            GenerateCSharpClassMarshaller(cls, output);
        }

        output += "    }\n";

        if (!nsName.empty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Structure generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpStructure(const ApiClass& cls, const std::string& assemblyName,
                                                            std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceNameList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        // Marshaller (must come before the struct for CustomMarshaller attribute)
        GenerateCSharpStructMarshaller(cls, output);

        // Struct declaration
        GenerateCSharpComment(output, "    ", cls.comment);
        output += Utils::String::Format("    [StructLayout(LayoutKind.Sequential)]\n");
        if (IsValidCSharpAttributeList(cls.attributes))
            output += Utils::String::Format("    {0}\n", cls.attributes);

        output += Utils::String::Format("    public unsafe partial struct {0}", MakeCSharpIdentifier(cls.name));

        if (!cls.baseClassName.empty())
            output += Utils::String::Format(" : {0}", GetCSharpPublicType(cls.baseClassName));

        output += "\n    {\n";

        // Fields
        for (auto& field : cls.fields)
            GenerateCSharpFieldAccessors(cls, field, assemblyName, output);

        // Properties (for struct)
        for (auto& prop : cls.properties)
            GenerateCSharpPropertyAccessors(cls, prop, assemblyName, output);

        // Functions - LibraryImport + public wrappers
        for (auto& fn : cls.functions)
            GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);

        for (auto& fn : cls.functions)
            GenerateCSharpWrapperFunctionCall(cls, fn, output);

        // Default property
        output += Utils::String::Format("        public static {0} Default => new {0}();\n\n", cls.name);

        output += "    }\n";

        if (!nsName.empty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Enum generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpEnum(const ApiEnum& en, std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(en.namespaceScopeList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        // Enum declaration
        GenerateCSharpComment(output, "    ", en.comment);
        if (IsValidCSharpAttributeList(en.attributes))
        {
            output += Utils::String::Format("    {0}\n", en.attributes);
        }

        // Map underlying type
        std::string csUnderlyingType;
        std::string stripped = StripTypeQualifiers(en.underlyingType);
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

        for (int i = 0; i < en.valueNames.size(); ++i)
        {
            if (i < en.valueComments.size())
                GenerateCSharpComment(output, "        ", en.valueComments[i]);
            output += Utils::String::Format("        {0} = {1}", MakeCSharpIdentifier(en.valueNames[i]), en.values[i]);
            if (i < en.valueNames.size() - 1)
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

    void BindingsCSharpGenerator::GenerateCSharpInterface(const ApiInterface& iface, std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(iface.namespaceNameList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        GenerateCSharpComment(output, "    ", iface.comment);
        if (IsValidCSharpAttributeList(iface.attributes))
        {
            output += Utils::String::Format("    {0}\n", iface.attributes);
        }

        output += Utils::String::Format("    public unsafe partial interface {0}\n    {{\n", MakeCSharpIdentifier(iface.name));

        // Function signatures
        for (auto& fn : iface.functions)
        {
            std::string publicRetType = GetCSharpPublicType(fn.returnType);
            std::string publicParams = BuildCSharpParams(fn, true);
            GenerateCSharpComment(output, "        ", fn.comment);
            output += Utils::String::Format("        {0} {1}({2});\n",
                fn.returnType == "void" ? "void" : publicRetType,
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
        bool hasCSharpInjectedCode = false;
        for (auto const& code : headerInfo.injectedCode)
        {
            if (IsInjectedCSharpCode(code))
            {
                hasCSharpInjectedCode = true;
                break;
            }
        }
        if (headerInfo.classes.empty() && headerInfo.enums.empty()
            && headerInfo.interfaces.empty() && headerInfo.events.empty()
            && !hasCSharpInjectedCode)
            return true;

        std::string output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - do not edit manually.\n";
        output += Utils::String::Format("// Source: {0}\n", headerInfo.filePath);
        output += "//-------------------------------------------------------------------------\n";
        output += "#pragma warning disable CS0108\n";
        output += "#pragma warning disable CS8603\n";
        output += "#pragma warning disable CS8625\n";
        output += "using System;\n";
        output += "using System.Runtime.InteropServices;\n";
        output += "using System.Runtime.InteropServices.Marshalling;\n";
        output += "using System.Threading;\n";
        for (auto const& code : headerInfo.injectedCode)
        {
            if (IsInjectedCSharpCode(code))
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
            GenerateCSharpEnum(en, output);
        }

        // Generate interfaces
        for (auto& iface : headerInfo.interfaces)
        {
            GenerateCSharpInterface(iface, output);
        }

        // Generate classes/structs
        for (auto& cls : headerInfo.classes)
        {
            if (cls.isStruct)
            {
                GenerateCSharpStructure(cls, headerInfo.assemblyName, output);
            }
            else
            {
                GenerateCSharpClass(cls, headerInfo.assemblyName, output);
            }
        }

        std::string baseName = FileSystem::GetFileNameWithoutExtension(headerInfo.filePath);
        std::string assemblyDir = headerInfo.assemblyDir;
        std::string outDir = Utils::String::Format("{0}/{1}", assemblyDir, Settings::g_autogeneratedDirectory);
        FileSystem::NormalizePath(outDir);
        if (!FileSystem::DirectoryExists(outDir))
        {
            FileSystem::CreateDirectory(outDir);
        }

        std::string outPath = outDir + "/" + baseName + ".CSharp.cs";
        return SaveFile(outPath, std::string(output.c_str()));
    }

    bool BindingsCSharpGenerator::GenerateAll(const std::vector<BindingsHeaderInfo>& headers,
                                               const std::string& solutionRoot)
    {
        for (auto& h : headers)
        {
            if (!Generate(h, solutionRoot))
                return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Binary module assembly info generation
    // -------------------------------------------------------------------------

    bool BindingsCSharpGenerator::GenerateBinaryModuleAssemblyInfo(const BinaryModuleInfo& module)
    {
        std::string output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - do not edit manually.\n";
        output += "//-------------------------------------------------------------------------\n";
        output += "using System.Reflection;\n";
        output += "using System.Runtime.InteropServices;\n\n";

        output += Utils::String::Format("[assembly: AssemblyTitle(\"{0}\")]\n", module.name);
        output += Utils::String::Format("[assembly: AssemblyVersion(\"1.0.0.0\")]\n");
        output += Utils::String::Format("[assembly: AssemblyFileVersion(\"1.0.0.0\")]\n");
        output += Utils::String::Format("[assembly: AssemblyProduct(\"{0}\")]\n", module.name);
        output += "[assembly: ComVisible(false)]\n";

        std::string moduleDir = module.assemblyDir;
        std::string outDir = Utils::String::Format("{0}/{1}", moduleDir, Settings::g_autogeneratedDirectory);
        FileSystem::NormalizePath(outDir);
        if (!FileSystem::DirectoryExists(outDir))
        {
            FileSystem::CreateDirectory(outDir);
        }

        std::string outPath = outDir + "/" + module.assemblyType + ".Gen.cs";
        return SaveFile(outPath, std::string(output.c_str()));
    }

    // -------------------------------------------------------------------------
    // SaveFile
    // -------------------------------------------------------------------------

    bool BindingsCSharpGenerator::SaveFile(const std::string& path, const std::string& content)
    {
        std::string parentDir = FileSystem::GetParentDirectory(path);
        if (!FileSystem::DirectoryExists(parentDir))
            FileSystem::CreateDirectory(parentDir);

        std::string existing;
        if (FileSystem::FileExists(path) && Utils::ReadAllText(path, existing) && existing == content.c_str())
        {
            return true;
        }

        std::string pathAnsi(path);
        std::ofstream f(pathAnsi.c_str(), std::ios::out | std::ios::trunc);
        if (!f.is_open())
            return false;
        f << content;
        return true;
    }

} // namespace SE::BuildTool
