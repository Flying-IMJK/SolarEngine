// BindingsCSharpGenerator.cpp
// Generates C# binding declarations using direct string building.

#include "CodeGenerator_BindingsCSharp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"

#include <fstream>
#include <string>

#include "../ReflectorSettingsAndUtils.h"

namespace SE::ReflectTool
{
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    StringAnsi BindingsCSharpGenerator::GetAccessString(AccessLevel access) const
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

    StringAnsi BindingsCSharpGenerator::GetCSharpNamespace(const StringAnsi& namespaceName) const
    {
        if (namespaceName.IsEmpty())
            return StringAnsi("SE");
        // Replace :: with . for C# namespace syntax
        StringAnsi result = namespaceName;
        int pos;
        while ((pos = result.Find("::")) != INVALID_INDEX)
        {
            result = result.Substring(0, pos) + "." + result.Substring(pos + 2);
        }
        return result;
    }

    StringAnsi BindingsCSharpGenerator::BuildCSharpParams(const ApiFunction& fn, bool forPublic) const
    {
        StringAnsi params;
        for (int i = 0; i < fn.params.Count(); ++i)
        {
            if (i > 0) params += ", ";

            StringAnsi type = forPublic ? GetCSharpPublicType(fn.params[i].cppType) : GetCSharpInteropType(fn.params[i].cppType);

            // Pass by ref for non-interop
            if (forPublic && UsePassByReference(fn.params[i].cppType) && !fn.params[i].isOut)
            {
                params += StringAnsi::Format("ref {0} {1}", type.Get(), fn.params[i].name.Get());
            }
            else if (forPublic && fn.params[i].isOut)
            {
                params += StringAnsi::Format("out {0} {1}", type.Get(), fn.params[i].name.Get());
            }
            else
            {
                // Add marshal attribute for interop params
                if (!forPublic)
                {
                    StringAnsi marshalAttr = GetCSharpParamMarshalAttribute(fn.params[i].cppType, fn.params[i].name);
                    if (!marshalAttr.IsEmpty())
                        params += StringAnsi::Format("{0} ", marshalAttr.Get());
                }
                params += StringAnsi::Format("{0} {1}", type.Get(), fn.params[i].name.Get());
            }
        }
        return params;
    }

    StringAnsi BindingsCSharpGenerator::BuildCSharpInteropParams(const ApiClass& cls, const ApiFunction& fn) const
    {
        StringAnsi params;
        if (!fn.isStatic)
            params += "IntPtr __obj";

        for (int i = 0; i < fn.params.Count(); ++i)
        {
            if (params.Length() > 0) params += ", ";
            StringAnsi interopType = GetCSharpInteropType(fn.params[i].cppType);
            StringAnsi marshalAttr = GetCSharpParamMarshalAttribute(fn.params[i].cppType, fn.params[i].name);

            if (!marshalAttr.IsEmpty())
                params += StringAnsi::Format("{0} ", marshalAttr.Get());
            params += StringAnsi::Format("{0} {1}", interopType.Get(), fn.params[i].name.Get());
        }
        return params;
    }

    StringAnsi BindingsCSharpGenerator::BuildCSharpCallArgs(const ApiClass& cls, const ApiFunction& fn, bool isInterop) const
    {
        StringAnsi args;
        if (!fn.isStatic && isInterop)
            args += "__unmanagedPtr";

        for (int i = 0; i < fn.params.Count(); ++i)
        {
            if (args.Length() > 0) args += ", ";

            if (isInterop)
            {
                // Convert to interop representation
                StringAnsi toInterop = GetCSharpToInterop(fn.params[i].cppType, fn.params[i].name);
                if (UsePassByReference(fn.params[i].cppType))
                    args += StringAnsi::Format("ref {0}", toInterop.Get());
                else if (fn.params[i].isOut)
                    args += StringAnsi::Format("out {0}", fn.params[i].name.Get());
                else
                    args += toInterop;
            }
            else
            {
                // Public call — forward as-is with ref/out keywords
                if (UsePassByReference(fn.params[i].cppType))
                    args += StringAnsi::Format("ref {0}", fn.params[i].name.Get());
                else if (fn.params[i].isOut)
                    args += StringAnsi::Format("out {0}", fn.params[i].name.Get());
                else
                    args += fn.params[i].name;
            }
        }
        return args;
    }

    // -------------------------------------------------------------------------
    // [LibraryImport] declaration for a single function
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunction(const ApiClass& cls, const ApiFunction& fn,
                                                                const StringAnsi& assemblyName, StringAnsi& output)
    {
        StringAnsi interopRetType = GetCSharpInteropType(fn.returnType);
        StringAnsi interopParams = BuildCSharpInteropParams(cls, fn);
        StringAnsi returnMarshalAttr = GetCSharpReturnMarshalAttribute(fn.returnType);
        StringAnsi internalName = StringAnsi::Format("Internal_{0}", fn.uniqueName.Get());

        output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}\", StringMarshalling = StringMarshalling.Utf16)]\n",
            assemblyName.Get(), fn.entryPoint.Get());
        if (!returnMarshalAttr.IsEmpty())
            output += StringAnsi::Format("        {0}\n", returnMarshalAttr.Get());
        output += StringAnsi::Format("        internal static partial {0} {1}({2});\n\n",
            interopRetType.Get(), internalName.Get(), interopParams.Get());
    }

    // -------------------------------------------------------------------------
    // Public wrapper function call (calls the InternalCall)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunctionCall(const ApiClass& cls, const ApiFunction& fn,
                                                                     StringAnsi& output)
    {
        StringAnsi publicRetType = GetCSharpPublicType(fn.returnType);
        StringAnsi publicParams = BuildCSharpParams(fn, true);
        bool retIsVoid = (fn.returnType == "void");
        StringAnsi access = GetAccessString(fn.isHidden ? AccessLevel::Private : AccessLevel::Public);
        StringAnsi staticKeyword = fn.isStatic ? "static " : "";

        output += StringAnsi::Format("        {0} {1}{2}{3} {4}({5})\n",
            access.Get(), staticKeyword.Get(),
            retIsVoid ? "void" : publicRetType.Get(),
            StringAnsi(" "), fn.name.Get(), publicParams.Get());
        output += "        {\n";

        StringAnsi interopCall = StringAnsi::Format("Internal_{0}({1})",
            fn.uniqueName.Get(), BuildCSharpCallArgs(cls, fn, true).Get());

        if (retIsVoid)
        {
            output += StringAnsi::Format("            {0};\n", interopCall.Get());
        }
        else
        {
            StringAnsi fromInterop = GetCSharpFromInterop(fn.returnType, interopCall);
            output += StringAnsi::Format("            return {0};\n", fromInterop.Get());
        }
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Property accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpPropertyAccessors(const ApiClass& cls, const ApiProperty& prop,
                                                                   const StringAnsi& assemblyName, StringAnsi& output)
    {
        StringAnsi publicType = GetCSharpPublicType(prop.cppType);
        StringAnsi interopType = GetCSharpInteropType(prop.cppType);

        // Getter [LibraryImport]
        if (prop.hasGetter)
        {
            StringAnsi getterMarshal = GetCSharpReturnMarshalAttribute(prop.cppType);
            output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}\", StringMarshalling = StringMarshalling.Utf16)]\n",
                assemblyName.Get(), prop.getterEntryPoint.Get());
            if (!getterMarshal.IsEmpty())
                output += StringAnsi::Format("        {0}\n", getterMarshal.Get());
            output += StringAnsi::Format("        internal static partial {0} Internal_{1}(IntPtr __obj);\n\n",
                interopType.Get(), prop.getterUniqueName.Get());
        }

        // Setter [LibraryImport]
        if (prop.hasSetter)
        {
            StringAnsi setterParamMarshal = GetCSharpParamMarshalAttribute(prop.cppType, "value");
            StringAnsi setterParamPrefix;
            if (!setterParamMarshal.IsEmpty())
                setterParamPrefix = StringAnsi::Format("{0} ", setterParamMarshal.Get());

            output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}\", StringMarshalling = StringMarshalling.Utf16)]\n",
                assemblyName.Get(), prop.setterEntryPoint.Get());
            output += StringAnsi::Format("        internal static partial void Internal_{0}(IntPtr __obj, {1}{2} value);\n\n",
                prop.setterUniqueName.Get(), setterParamPrefix.Get(), interopType.Get());
        }

        // Public property declaration
        StringAnsi propAccess = GetAccessString(prop.getterAccess);
        output += StringAnsi::Format("        {0} {1} {2}\n        {{\n", propAccess.Get(), publicType.Get(), prop.name.Get());

        if (prop.hasGetter)
        {
            StringAnsi getterCall = StringAnsi::Format("Internal_{0}(__unmanagedPtr)", prop.getterUniqueName.Get());
            StringAnsi fromInterop = GetCSharpFromInterop(prop.cppType, getterCall);
            StringAnsi getterAccess = GetAccessString(prop.getterAccess);
            output += StringAnsi::Format("            {0}get {{ return {1}; }}\n", getterAccess.Get(), fromInterop.Get());
        }

        if (prop.hasSetter)
        {
            StringAnsi toInterop = GetCSharpToInterop(prop.cppType, StringAnsi("value"));
            StringAnsi setterAccess = GetAccessString(prop.setterAccess);
            output += StringAnsi::Format("            {0}set {{ Internal_{1}(__unmanagedPtr, {2}); }}\n",
                setterAccess.Get(), prop.setterUniqueName.Get(), toInterop.Get());
        }

        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Field accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpFieldAccessors(const ApiClass& cls, const ApiField& field,
                                                                const StringAnsi& assemblyName, StringAnsi& output)
    {
        StringAnsi publicType = GetCSharpPublicType(field.cppType);
        StringAnsi interopType = GetCSharpInteropType(field.cppType);
        StringAnsi stripped = StripTypeQualifiers(field.cppType);
        const TypeMapping* map = FindTypeMapping(stripped.Get());

        // Static fields become properties (get/set via InternalCall)
        // Instance fields with arraySize become fixed-size buffers
        // Regular instance fields become direct fields

        if (field.isStatic)
        {
            // Static field → [LibraryImport] getter/setter + public property

            // Getter InternalCall
            StringAnsi getterMarshal = GetCSharpReturnMarshalAttribute(field.cppType);
            output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}_{2}_Get\", StringMarshalling = StringMarshalling.Utf16)]\n",
                assemblyName.Get(), cls.name.Get(), field.name.Get());
            if (!getterMarshal.IsEmpty())
                output += StringAnsi::Format("        {0}\n", getterMarshal.Get());
            output += StringAnsi::Format("        internal static partial {0} Internal_{1}_Get(IntPtr __obj);\n\n",
                interopType.Get(), field.name.Get());

            // Setter InternalCall (if not readonly)
            if (!field.isReadOnly)
            {
                StringAnsi setterParamMarshal = GetCSharpParamMarshalAttribute(field.cppType, "value");
                StringAnsi setterParamPrefix;
                if (!setterParamMarshal.IsEmpty())
                    setterParamPrefix = StringAnsi::Format("{0} ", setterParamMarshal.Get());

                output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}_{2}_Set\", StringMarshalling = StringMarshalling.Utf16)]\n",
                    assemblyName.Get(), cls.name.Get(), field.name.Get());
                output += StringAnsi::Format("        internal static partial void Internal_{0}_Set(IntPtr __obj, {1}{2} value);\n\n",
                    field.name.Get(), setterParamPrefix.Get(), interopType.Get());
            }

            // Public static property
            StringAnsi fieldAccess = GetAccessString(field.isHidden ? AccessLevel::Private : AccessLevel::Public);
            output += StringAnsi::Format("        {0} static {1} {2}\n        {{\n",
                fieldAccess.Get(), publicType.Get(), field.name.Get());

            StringAnsi getterCall = StringAnsi::Format("Internal_{0}_Get(IntPtr.Zero)", field.name.Get());
            StringAnsi fromInterop = GetCSharpFromInterop(field.cppType, getterCall);
            output += StringAnsi::Format("            get {{ return {0}; }}\n", fromInterop.Get());

            if (!field.isReadOnly)
            {
                StringAnsi toInterop = GetCSharpToInterop(field.cppType, StringAnsi("value"));
                output += StringAnsi::Format("            set {{ Internal_{0}_Set(IntPtr.Zero, {1}); }}\n",
                    field.name.Get(), toInterop.Get());
            }
            output += "        }\n\n";
        }
        else if (field.arraySize > 0)
        {
            // Fixed-size array → fixed buffer
            output += StringAnsi::Format("        public fixed {0} {1}[{2}];\n\n",
                publicType.Get(), field.name.Get(), field.arraySize);
        }
        else
        {
            // Regular instance field
            StringAnsi fieldAccess = GetAccessString(field.isHidden ? AccessLevel::Private : AccessLevel::Public);
            StringAnsi fieldDecl = StringAnsi::Format("        {0} {1} {2}", fieldAccess.Get(), publicType.Get(), field.name.Get());

            // Add marshal attributes for special types
            if (stripped == "bool")
                fieldDecl = StringAnsi::Format("        {0} [MarshalAs(UnmanagedType.U1)] {1} {2}", fieldAccess.Get(), publicType.Get(), field.name.Get());
            else if (map && map->isString)
                fieldDecl = StringAnsi::Format("        {0} [MarshalAs(UnmanagedType.LPUTF8Str)] {1} {2}", fieldAccess.Get(), publicType.Get(), field.name.Get());

            fieldDecl += ";\n\n";
            output += fieldDecl;
        }
    }

    // -------------------------------------------------------------------------
    // Event accessor generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpEventAccessors(const ApiClass& cls, const ApiEvent& evt,
                                                                const StringAnsi& assemblyName, StringAnsi& output)
    {
        // Build the C# delegate type for the event
        StringAnsi delegateParams;
        for (int i = 0; i < evt.params.Count(); ++i)
        {
            if (i > 0) delegateParams += ", ";
            StringAnsi publicType = GetCSharpPublicType(evt.params[i].cppType);
            if (UsePassByReference(evt.params[i].cppType))
                delegateParams += StringAnsi::Format("ref {0} {1}", publicType.Get(), evt.params[i].name.Get());
            else
                delegateParams += StringAnsi::Format("{0} {1}", publicType.Get(), evt.params[i].name.Get());
        }

        // _Bind InternalCall
        output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}_{2}_ManagedBind\", StringMarshalling = StringMarshalling.Utf16)]\n",
            assemblyName.Get(), cls.name.Get(), evt.name.Get());
        output += StringAnsi::Format("        internal static partial void Internal_{0}_Bind(IntPtr __obj, bool bind);\n\n", evt.name.Get());

        // _Invoke InternalCall
        StringAnsi invokeParams = "IntPtr __obj";
        for (int i = 0; i < evt.params.Count(); ++i)
        {
            invokeParams += ", ";
            StringAnsi interopType = GetCSharpInteropType(evt.params[i].cppType);
            StringAnsi marshalAttr = GetCSharpParamMarshalAttribute(evt.params[i].cppType, evt.params[i].name);
            if (!marshalAttr.IsEmpty())
                invokeParams += StringAnsi::Format("{0} ", marshalAttr.Get());
            invokeParams += StringAnsi::Format("{0} {1}", interopType.Get(), evt.params[i].name.Get());
        }
        output += StringAnsi::Format("        [LibraryImport(\"{0}\", EntryPoint = \"{1}_{2}_ManagedWrapper\", StringMarshalling = StringMarshalling.Utf16)]\n",
            assemblyName.Get(), cls.name.Get(), evt.name.Get());
        output += StringAnsi::Format("        internal static partial void Internal_{0}_Invoke({1});\n\n", evt.name.Get(), invokeParams.Get());

        // Binding count tracking
        StringAnsi evtAccess = GetAccessString(evt.access);
        output += StringAnsi::Format("        private int _{0}BindCount;\n", evt.name.Get());

        // Event declaration with add/remove
        output += StringAnsi::Format("        {0} event Action<{1}> {2}\n        {{\n",
            evtAccess.Get(), delegateParams.Get(), evt.name.Get());
        output += "            add\n            {\n";
        output += StringAnsi::Format("                if (Interlocked.Exchange(ref _{0}BindCount, _{0}BindCount + 1) == 0)\n", evt.name.Get());
        output += StringAnsi::Format("                    Internal_{0}_Bind(__unmanagedPtr, true);\n", evt.name.Get());
        output += "                _" + evt.name + " += value;\n";
        output += "            }\n";
        output += "            remove\n            {\n";
        output += StringAnsi::Format("                if (Interlocked.Decrement(ref _{0}BindCount) == 0)\n", evt.name.Get());
        output += StringAnsi::Format("                    Internal_{0}_Bind(__unmanagedPtr, false);\n", evt.name.Get());
        output += "                _" + evt.name + " -= value;\n";
        output += "            }\n";
        output += "        }\n\n";

        // Private backing delegate
        output += StringAnsi::Format("        private event Action<{0}> _{1};\n\n", delegateParams.Get(), evt.name.Get());
    }

    // -------------------------------------------------------------------------
    // Class marshaller (ManagedHandleMarshaller for ScriptingObject types)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpClassMarshaller(const ApiClass& cls, StringAnsi& output)
    {
        output += StringAnsi::Format("        internal struct {0}Marshaller\n        {{\n", cls.name.Get());
        output += StringAnsi::Format("            public static {0} ConvertToManaged(IntPtr unmanaged) => unmanaged != IntPtr.Zero ? new {0}() {{ __unmanagedPtr = unmanaged }} : null;\n", cls.name.Get());
        output += StringAnsi::Format("            public static IntPtr ConvertToUnmanaged({0} managed) => managed != null ? managed.__unmanagedPtr : IntPtr.Zero;\n", cls.name.Get());
        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Struct marshaller (CustomMarshaller with blittable internal)
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpStructMarshaller(const ApiClass& cls, StringAnsi& output)
    {
        output += StringAnsi::Format("        [CustomMarshaller(typeof({0}), MarshalMode.Default, typeof({0}.{0}Marshaller))]\n", cls.name.Get());
        output += StringAnsi::Format("        internal static partial class {0}Marshaller\n        {{\n", cls.name.Get());

        // Blittable internal representation
        output += StringAnsi::Format("            internal struct {0}Internal\n            {{\n", cls.name.Get());
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            StringAnsi fieldType = GetCSharpInteropType(field.cppType);
            if (field.arraySize > 0)
                output += StringAnsi::Format("                public fixed {0} {1}[{2}];\n", fieldType.Get(), field.name.Get(), field.arraySize);
            else
                output += StringAnsi::Format("                public {0} {1};\n", fieldType.Get(), field.name.Get());
        }
        output += "            }\n\n";

        // ToManaged
        output += StringAnsi::Format("            public static {0} ToManaged({0}Internal unmanaged)\n            {{\n", cls.name.Get());
        output += StringAnsi::Format("                var result = new {0}();\n", cls.name.Get());
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            if (field.arraySize > 0)
            {
                output += StringAnsi::Format("                for (int i = 0; i < {0}; i++) result.{1}[i] = unmanaged.{1}[i];\n",
                    field.arraySize, field.name.Get());
            }
            else
            {
                StringAnsi fromInterop = GetCSharpFromInterop(field.cppType, StringAnsi::Format("unmanaged.{0}", field.name.Get()));
                output += StringAnsi::Format("                result.{0} = {1};\n", field.name.Get(), fromInterop.Get());
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        // ToNative
        output += StringAnsi::Format("            public static {0}Internal ToNative({0} managed)\n            {{\n", cls.name.Get());
        output += StringAnsi::Format("                var result = new {0}Internal();\n", cls.name.Get());
        for (auto& field : cls.fields)
        {
            if (field.isStatic) continue;
            if (field.arraySize > 0)
            {
                output += StringAnsi::Format("                for (int i = 0; i < {0}; i++) result.{1}[i] = managed.{1}[i];\n",
                    field.arraySize, field.name.Get());
            }
            else
            {
                StringAnsi toInterop = GetCSharpToInterop(field.cppType, StringAnsi::Format("managed.{0}", field.name.Get()));
                output += StringAnsi::Format("                result.{0} = {1};\n", field.name.Get(), toInterop.Get());
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        // Free
        output += StringAnsi::Format("            public static void Free({0}Internal unmanaged) {{ }}\n", cls.name.Get());

        output += "        }\n\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpClass(const ApiClass& cls, const StringAnsi& assemblyName,
                                                        StringAnsi& output)
    {
        StringAnsi nsName = GetCSharpNamespace(cls.namespaceName);
        if (!nsName.IsEmpty())
            output += StringAnsi::Format("namespace {0}\n{{\n", nsName.Get());

        // Class declaration
        StringAnsi classKeyword = cls.isStatic ? "static " : (cls.isSealed ? "sealed " : "");
        StringAnsi abstractKeyword = cls.isAbstract ? "abstract " : "";

        output += StringAnsi::Format("    [Unmanaged]\n");
        if (!cls.isStruct)
            output += StringAnsi::Format("    [NativeMarshalling(typeof({0}.{0}Marshaller))]\n", cls.name.Get());

        // User attributes
        if (!cls.attributes.IsEmpty())
            output += StringAnsi::Format("    {0}\n", cls.attributes.Get());

        output += StringAnsi::Format("    public unsafe {0}{1}{2}partial class {3}",
            abstractKeyword.Get(), classKeyword.Get(), cls.isStruct ? "" : "", cls.name.Get());

        // Base class
        if (!cls.baseClassName.IsEmpty())
            output += StringAnsi::Format(" : {0}", GetCSharpPublicType(cls.baseClassName).Get());

        // Interface implementations
        if (!cls.interfaces.IsEmpty())
        {
            if (cls.baseClassName.IsEmpty())
                output += " : ";
            else
                output += ", ";
            for (int i = 0; i < cls.interfaces.Count(); ++i)
            {
                if (i > 0) output += ", ";
                output += cls.interfaces[i].name;
            }
        }

        output += "\n    {\n";

        // Unmanaged pointer
        output += "        internal IntPtr __unmanagedPtr;\n\n";

        // Constructor (if not abstract, not static, not noConstructor)
        if (!cls.isAbstract && !cls.isStatic && !cls.noConstructor && !cls.noSpawn)
        {
            output += StringAnsi::Format("        public {0}() {{ }}\n\n", cls.name.Get());
            output += StringAnsi::Format("        internal {0}(IntPtr nativePtr) {{ __unmanagedPtr = nativePtr; }}\n\n", cls.name.Get());
        }

        // Events
        for (auto& evt : cls.events)
            GenerateCSharpEventAccessors(cls, evt, assemblyName, output);

        // Properties
        for (auto& prop : cls.properties)
            GenerateCSharpPropertyAccessors(cls, prop, assemblyName, output);

        // Functions — first generate all [LibraryImport] declarations, then public wrappers
        for (auto& fn : cls.functions)
            GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);

        for (auto& fn : cls.functions)
            GenerateCSharpWrapperFunctionCall(cls, fn, output);

        // Fields
        for (auto& field : cls.fields)
            GenerateCSharpFieldAccessors(cls, field, assemblyName, output);

        // Marshaller
        if (!cls.isStruct)
            GenerateCSharpClassMarshaller(cls, output);

        output += "    }\n";

        if (!nsName.IsEmpty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Structure generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpStructure(const ApiClass& cls, const StringAnsi& assemblyName,
                                                            StringAnsi& output)
    {
        StringAnsi nsName = GetCSharpNamespace(cls.namespaceName);
        if (!nsName.IsEmpty())
            output += StringAnsi::Format("namespace {0}\n{{\n", nsName.Get());

        // Marshaller (must come before the struct for CustomMarshaller attribute)
        GenerateCSharpStructMarshaller(cls, output);

        // Struct declaration
        output += StringAnsi::Format("    [StructLayout(LayoutKind.Sequential)\n");
        if (cls.fields.Count() > 0)
            output += StringAnsi::Format("    [NativeMarshalling(typeof({0}.{0}Marshaller))]\n", cls.name.Get());

        if (!cls.attributes.IsEmpty())
            output += StringAnsi::Format("    {0}\n", cls.attributes.Get());

        output += StringAnsi::Format("    public unsafe partial struct {0}", cls.name.Get());

        if (!cls.baseClassName.IsEmpty())
            output += StringAnsi::Format(" : {0}", GetCSharpPublicType(cls.baseClassName).Get());

        output += "\n    {\n";

        // Fields
        for (auto& field : cls.fields)
            GenerateCSharpFieldAccessors(cls, field, assemblyName, output);

        // Properties (for struct)
        for (auto& prop : cls.properties)
            GenerateCSharpPropertyAccessors(cls, prop, assemblyName, output);

        // Functions — LibraryImport + public wrappers
        for (auto& fn : cls.functions)
            GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);

        for (auto& fn : cls.functions)
            GenerateCSharpWrapperFunctionCall(cls, fn, output);

        // Default property
        output += StringAnsi::Format("        public static {0} Default => new {0}();\n\n", cls.name.Get());

        output += "    }\n";

        if (!nsName.IsEmpty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Enum generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpEnum(const ApiEnum& en, StringAnsi& output)
    {
        StringAnsi nsName = GetCSharpNamespace(en.namespaceName);
        if (!nsName.IsEmpty())
            output += StringAnsi::Format("namespace {0}\n{{\n", nsName.Get());

        // Enum declaration
        output += StringAnsi::Format("    [Unmanaged]\n");
        if (!en.attributes.IsEmpty())
            output += StringAnsi::Format("    {0}\n", en.attributes.Get());

        // Map underlying type
        StringAnsi csUnderlyingType;
        StringAnsi stripped = StripTypeQualifiers(en.underlyingType);
        if (stripped == "uint8")   csUnderlyingType = "byte";
        else if (stripped == "int8")    csUnderlyingType = "sbyte";
        else if (stripped == "uint16")  csUnderlyingType = "ushort";
        else if (stripped == "int16")   csUnderlyingType = "short";
        else if (stripped == "uint32")  csUnderlyingType = "uint";
        else if (stripped == "int32")   csUnderlyingType = "int";
        else if (stripped == "uint64")  csUnderlyingType = "ulong";
        else if (stripped == "int64")   csUnderlyingType = "long";
        else                            csUnderlyingType = "int"; // default

        output += StringAnsi::Format("    public enum {0} : {1}\n    {{\n", en.name.Get(), csUnderlyingType.Get());

        for (int i = 0; i < en.valueNames.Count(); ++i)
        {
            output += StringAnsi::Format("        {0} = {1}", en.valueNames[i].Get(), en.values[i]);
            if (i < en.valueNames.Count() - 1)
                output += ",";
            output += "\n";
        }

        output += "    }\n";

        if (!nsName.IsEmpty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Interface generation
    // -------------------------------------------------------------------------

    void BindingsCSharpGenerator::GenerateCSharpInterface(const ApiInterface& iface, StringAnsi& output)
    {
        StringAnsi nsName = GetCSharpNamespace(iface.namespaceName);
        if (!nsName.IsEmpty())
            output += StringAnsi::Format("namespace {0}\n{{\n", nsName.Get());

        output += StringAnsi::Format("    [Unmanaged]\n");
        if (!iface.attributes.IsEmpty())
            output += StringAnsi::Format("    {0}\n", iface.attributes.Get());

        output += StringAnsi::Format("    public unsafe partial interface {0}\n    {{\n", iface.name.Get());

        // Function signatures
        for (auto& fn : iface.functions)
        {
            StringAnsi publicRetType = GetCSharpPublicType(fn.returnType);
            StringAnsi publicParams = BuildCSharpParams(fn, true);
            output += StringAnsi::Format("        {0} {1}({2});\n",
                fn.returnType == "void" ? "void" : publicRetType.Get(),
                fn.name.Get(), publicParams.Get());
        }

        output += "    }\n\n";

        // Interface Marshaller
        output += StringAnsi::Format("    internal struct {0}Marshaller\n    {{\n", iface.name.Get());
        output += StringAnsi::Format("        public static {0} ConvertToManaged(IntPtr unmanaged) => default;\n", iface.name.Get());
        output += StringAnsi::Format("        public static IntPtr ConvertToUnmanaged({0} managed) => IntPtr.Zero;\n", iface.name.Get());
        output += "    }\n";

        if (!nsName.IsEmpty())
            output += "}\n";
        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Generate — entry point for a single header
    // -------------------------------------------------------------------------

    bool BindingsCSharpGenerator::Generate(const BindingsHeaderInfo& headerInfo,
                                            const String& solutionRoot)
    {
        if (headerInfo.classes.IsEmpty() && headerInfo.enums.IsEmpty()
            && headerInfo.interfaces.IsEmpty() && headerInfo.events.IsEmpty())
            return true;

        StringAnsi output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator — do not edit manually.\n";
        output += StringAnsi::Format("// Source: {0}\n", headerInfo.filePath.Get());
        output += "//-------------------------------------------------------------------------\n";
        output += "using System;\n";
        output += "using System.Runtime.InteropServices;\n";
        output += "using System.Threading;\n";
        output += "\n";

        // Generate enums first
        for (auto& en : headerInfo.enums)
            GenerateCSharpEnum(en, output);

        // Generate interfaces
        for (auto& iface : headerInfo.interfaces)
            GenerateCSharpInterface(iface, output);

        // Generate classes/structs
        for (auto& cls : headerInfo.classes)
        {
            if (cls.isStruct)
                GenerateCSharpStructure(cls, headerInfo.assemblyName, output);
            else
                GenerateCSharpClass(cls, headerInfo.assemblyName, output);
        }

        StringAnsi baseName = FileSystem::GetFileNameWithoutExtension(headerInfo.filePath.ToString()).ToStringAnsi();
        String assemblyDir = headerInfo.assemblyDir.ToString();
        String outDir = assemblyDir + Settings::g_autogeneratedDirectory;
        if (!FileSystem::DirectoryExists(outDir))
            FileSystem::CreateDirectory(outDir);

        String outPath = outDir + SE_TEXT("/") + baseName.ToString() + SE_TEXT(".CSharp.cs");
        return SaveFile(outPath, std::string(output.Get()));
    }

    bool BindingsCSharpGenerator::GenerateAll(const List<BindingsHeaderInfo>& headers,
                                               const String& solutionRoot)
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
        StringAnsi output;
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator — do not edit manually.\n";
        output += "//-------------------------------------------------------------------------\n";
        output += "using System.Reflection;\n";
        output += "using System.Runtime.InteropServices;\n\n";

        output += StringAnsi::Format("[assembly: AssemblyTitle(\"{0}\")]\n", module.name.Get());
        output += StringAnsi::Format("[assembly: AssemblyVersion(\"1.0.0.0\")]\n");
        output += StringAnsi::Format("[assembly: AssemblyFileVersion(\"1.0.0.0\")]\n");
        output += StringAnsi::Format("[assembly: AssemblyProduct(\"{0}\")]\n", module.name.Get());
        output += "[assembly: ComVisible(false)]\n";

        String outDir = module.assemblyDir.ToString() + Settings::g_autogeneratedDirectory;
        if (!FileSystem::DirectoryExists(outDir))
            FileSystem::CreateDirectory(outDir);

        String outPath = outDir + SE_TEXT("/") + module.assemblyType.ToString() + SE_TEXT(".Gen.cs");
        return SaveFile(outPath, std::string(output.Get()));
    }

    // -------------------------------------------------------------------------
    // SaveFile
    // -------------------------------------------------------------------------

    bool BindingsCSharpGenerator::SaveFile(const String& path, const std::string& content)
    {
        String parentDir = FileSystem::GetParentDirectory(path);
        if (!FileSystem::DirectoryExists(parentDir))
            FileSystem::CreateDirectory(parentDir);

        StringAnsi pathAnsi(path);
        std::ofstream f(pathAnsi.Get(), std::ios::out | std::ios::trunc);
        if (!f.is_open())
            return false;
        f << content;
        return true;
    }

} // namespace SE::ReflectTool