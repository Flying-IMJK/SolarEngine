// BindingsCSharpGenerator.cpp
// Generates C# binding declarations using direct string building.

#include "CodeGenerator_BindingsCSharp.h"
#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Core/FileSystem.h"
#include "Core/Utils.h"

#include <string>

#include "CodeGenerator_Utils.h"

namespace SE::BuildTool
{
    // Keep binding flow below focused on declarations. Shared C# source
    // composition and ABI helpers live in CodeGeneratorUtils.
    using CodeGeneratorUtils::AppendCSharpComment;
    using CodeGeneratorUtils::AppendCSharpLibraryImport;
    using CodeGeneratorUtils::GetCSharpCollectionCountExpression;
    using CodeGeneratorUtils::GetCSharpStructAbiFieldType;
    using CodeGeneratorUtils::GetCSharpStructFieldFromAbi;
    using CodeGeneratorUtils::GetCSharpStructFieldToAbi;
    using CodeGeneratorUtils::IsCSharpCode;
    using CodeGeneratorUtils::IsValidCSharpAttributeList;
    using CodeGeneratorUtils::MakeCSharpIdentifier;
    using CodeGeneratorUtils::NormalizeCSharpDefaultValue;
    using CodeGeneratorUtils::UsesCSharpOutResult;

    namespace
    {
        std::string GetCppType(TypeInfoParam const& param)
        {
            return param.type.ToString();
        }

        std::string GetCppType(TypeInfoField const& field)
        {
            return field.type.ToString();
        }

        std::string GetCppType(TypeInfoFunc const& fn)
        {
            return fn.returnType.ToString();
        }

        bool IsVoid(TypeInfoFunc const& fn)
        {
            return fn.returnType == TypeID("void");
        }

        std::string GetPropertyName(TypeInfoFunc const& fn)
        {
            if ((Utils::String::StartsWith(fn.name, "Get") || Utils::String::StartsWith(fn.name, "Set"))
                && fn.name.length() > 3)
            {
                return fn.name.substr(3);
            }
            return fn.name;
        }
    }

    std::string BindingsCSharpGenerator::BuildCSharpParams(const TypeInfoFunc& fn, bool forPublic) const
    {
        std::string params;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0) params += ", ";

            std::string const cppType = GetCppType(fn.params[i]);
            std::string type = forPublic ? GetCSharpPublicType(cppType, fn.params[i].arraySize)
                                         : GetCSharpInteropType(cppType, fn.params[i].arraySize);

            // Pass by ref for non-interop
            if (forPublic && UsePassByReference(cppType) && !fn.params[i].isOut)
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
                    std::string marshalAttr = GetCSharpParamMarshalAttribute(cppType, fn.params[i].name,
                        fn.params[i].arraySize);
                    if (!marshalAttr.empty())
                        params += Utils::String::Format("{0} ", marshalAttr);
                }
                std::string defaultValue;
                if (forPublic && !fn.params[i].defaultValue.empty() && !fn.params[i].isOut && !UsePassByReference(cppType))
                    defaultValue = Utils::String::Format(" = {0}", NormalizeCSharpDefaultValue(fn.params[i]));
                params += Utils::String::Format("{0} {1}{2}", type, MakeCSharpIdentifier(fn.params[i].name), defaultValue);
            }
        }
        return params;
    }

    std::string BindingsCSharpGenerator::BuildCSharpInteropParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn) const
    {
        std::string params;
        if (!fn.isStatic)
        {
            params += "IntPtr __obj";
        }

        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (params.length() > 0) params += ", ";
            std::string const cppType = GetCppType(fn.params[i]);
            std::string interopType = GetCSharpInteropType(cppType, fn.params[i].arraySize);
            std::string marshalAttr = GetCSharpParamMarshalAttribute(cppType, fn.params[i].name,fn.params[i].arraySize);

            if (!marshalAttr.empty())
            {
                params += Utils::String::Format("{0} ", marshalAttr);
            }
            if (fn.params[i].isOut)
            {
                params += "out ";
            }
            else if (UsePassByReference(cppType))
            {
                params += "ref ";
            }
            params += Utils::String::Format("{0} {1}", interopType, MakeCSharpIdentifier(fn.params[i].name));
            const CollectionAbiInfo collection = GetCollectionAbiInfo(cppType, fn.params[i].arraySize);
            if (collection.HasRuntimeCount())
            {
                params += Utils::String::Format(", int __{0}Count", MakeCSharpIdentifier(fn.params[i].name));
            }
        }
        return params;
    }

    std::string BindingsCSharpGenerator::BuildCSharpCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool isInterop) const
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
                std::string const cppType = GetCppType(fn.params[i]);
                const CollectionAbiInfo collection = GetCollectionAbiInfo(cppType, fn.params[i].arraySize);
                std::string toInterop = collection.IsCollection() ? paramName
                    : GetCSharpToInterop(cppType, paramName);
                if (fn.params[i].isOut)
                    args += Utils::String::Format("out {0}", paramName);
                else if (UsePassByReference(cppType))
                    args += Utils::String::Format("ref {0}", toInterop);
                else
                    args += toInterop;
                if (collection.HasRuntimeCount())
                {
                    args += Utils::String::Format(", {0}", GetCSharpCollectionCountExpression(cppType, paramName));
                }
            }
            else
            {
                // Public call - forward as-is with ref/out keywords
                if (fn.params[i].isOut)
                    args += Utils::String::Format("out {0}", MakeCSharpIdentifier(fn.params[i].name));
                else if (UsePassByReference(GetCppType(fn.params[i])))
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

    void BindingsCSharpGenerator::GenerateCSharpWrapperFunction(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                                                const std::string& assemblyName, std::string& output)
    {
        std::string const returnType = GetCppType(fn);
        const CollectionAbiInfo returnCollection = GetCollectionAbiInfo(returnType, fn.returnArraySize);
        std::string interopRetType = GetCSharpInteropType(returnType, fn.returnArraySize);
        std::string interopParams = BuildCSharpInteropParams(cls, fn);
        std::string returnMarshalAttr = GetCSharpReturnMarshalAttribute(returnType, fn.returnArraySize);
        std::string internalName = Utils::String::Format("Internal_{0}", fn.uniqueName);
        const bool useOutResult = UsesCSharpOutResult(returnType);
        if (useOutResult)
        {
            if (!interopParams.empty())
                interopParams += ", ";
            std::string marshal = GetCSharpParamMarshalAttribute(returnType, "__resultAsRef");
            if (!marshal.empty())
                interopParams += marshal + " ";
            interopParams += Utils::String::Format("out {0} __resultAsRef", interopRetType);
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!interopParams.empty())
                interopParams += ", ";
            interopParams += "out int __returnCount";
        }

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
        std::string const returnType = GetCppType(fn);
        const CollectionAbiInfo returnCollection = GetCollectionAbiInfo(returnType, fn.returnArraySize);
        std::string publicRetType = GetCSharpPublicType(returnType, fn.returnArraySize);
        std::string publicParams = BuildCSharpParams(fn, true);
        bool retIsVoid = IsVoid(fn);
        std::string access = CodeGeneratorUtils::GetAccessString(AccessLevel::Public);
        std::string staticKeyword = fn.isStatic ? "static " : "";

        AppendCSharpComment(output, "        ", fn.comment);
        if (IsValidCSharpAttributeList(fn.attributes))
            output += Utils::String::Format("        {0}\n", fn.attributes);
        output += Utils::String::Format("        {0} {1}{2}{3} {4}({5})\n",
            access, staticKeyword,
            retIsVoid ? "void" : publicRetType,
            std::string(" "), MakeCSharpIdentifier(fn.name), publicParams);
        output += "        {\n";

        std::string interopCallArgs = BuildCSharpCallArgs(cls, fn, true);
        if (returnCollection.HasRuntimeCount())
        {
            if (!interopCallArgs.empty())
                interopCallArgs += ", ";
            interopCallArgs += "out _";
        }
        std::string interopCall = Utils::String::Format("Internal_{0}({1})", fn.uniqueName, interopCallArgs);

        const bool useOutResult = UsesCSharpOutResult(returnType);
        if (useOutResult)
        {
            std::string callArgs = BuildCSharpCallArgs(cls, fn, true);
            if (!callArgs.empty()) callArgs += ", ";
            callArgs += "out __resultAsRef";
            output += Utils::String::Format("            {0} __resultAsRef;\n", GetCSharpInteropType(returnType));
            output += Utils::String::Format("            Internal_{0}({1});\n", fn.uniqueName, callArgs);
            std::string fromInterop = GetCSharpFromInterop(returnType, "__resultAsRef");
            output += Utils::String::Format("            return {0};\n", fromInterop);
            output += "        }\n\n";
            return;
        }

        if (retIsVoid)
        {
            output += Utils::String::Format("            {0};\n", interopCall);
        }
        else
        {
            std::string fromInterop = GetCSharpFromInterop(returnType, interopCall);
            output += Utils::String::Format("            return {0};\n", fromInterop);
        }
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
            GenerateCSharpWrapperFunction(cls, getter->function, assemblyName, output);
        if (setter)
            GenerateCSharpWrapperFunction(cls, setter->function, assemblyName, output);

        const std::string publicType = getter
            ? GetCSharpPublicType(GetCppType(getter->function), getter->function.returnArraySize)
            : GetCSharpPublicType(GetCppType(setter->function.params[0]), setter->function.params[0].arraySize);
        const AccessLevel propertyAccess = getter ? getterAccess : setterAccess;
        const std::string propertyAccessText = CodeGeneratorUtils::GetAccessString(propertyAccess);
        AppendCSharpComment(output, "        ", comment);
        if (IsValidCSharpAttributeList(attributes))
            output += Utils::String::Format("        {0}\n", attributes);
        output += Utils::String::Format("        {0} {1}{2} {3}\n        {{\n", propertyAccessText,
            isStatic ? "static " : "", publicType, MakeCSharpIdentifier(publicName));

        if (getter)
        {
            const TypeInfoFunc& getterFunction = getter->function;
            const std::string getterCppType = GetCppType(getterFunction);
            const CollectionAbiInfo getterCollection = GetCollectionAbiInfo(getterCppType, getterFunction.returnArraySize);
            const bool usesOutResult = UsesCSharpOutResult(getterCppType);
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
                    GetCSharpInteropType(getterCppType), getterFunction.uniqueName, callArgs,
                    GetCSharpFromInterop(getterCppType, "__resultAsRef"));
            }
            else
            {
                const std::string getterCall = Utils::String::Format("Internal_{0}({1})", getterFunction.uniqueName, callArgs);
                output += Utils::String::Format("            get {{ return {0}; }}\n", GetCSharpFromInterop(getterCppType, getterCall));
            }
        }

        if (setter)
        {
            const TypeInfoFunc& setterFunction = setter->function;
            const std::string setterCppType = setterFunction.params.empty()
                ? publicCppType : GetCppType(setterFunction.params[0]);
            const bool usesPointer = UsePassByReference(setterCppType);
            const std::string setterModifier = getter && setterAccess != propertyAccess
                ? CodeGeneratorUtils::GetAccessString(setterAccess) + " " : "";
            const std::string instanceArg = setterFunction.isStatic ? "" : "__unmanagedPtr";
            const CollectionAbiInfo setterCollection = GetCollectionAbiInfo(setterCppType,
                setterFunction.params.empty() ? 0 : setterFunction.params[0].arraySize);
            const std::string toInterop = setterCollection.IsCollection() ? "value"
                : GetCSharpToInterop(setterCppType, "value");
            if (usesPointer)
            {
                output += Utils::String::Format("            {0}set {{ var __valueAsRef = {1}; Internal_{2}({3}{4}ref __valueAsRef); }}\n",
                    setterModifier, toInterop, setterFunction.uniqueName, instanceArg,
                    instanceArg.empty() ? "" : ", ");
            }
            else
            {
                std::string setterArgs = instanceArg;
                if (!setterArgs.empty())
                    setterArgs += ", ";
                setterArgs += toInterop;
                if (setterCollection.HasRuntimeCount())
                    setterArgs += Utils::String::Format(", {0}", GetCSharpCollectionCountExpression(setterCppType, "value"));
                if (setterCollection.kind == CollectionAbiKind::Fixed)
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
            if (IsVoid(fn) && fn.params.size() == 1)
            {
                setter = MakeBindingPropertySetter(cls, fn);
                setterPtr = &setter;
            }
            else
            {
                getter = MakeBindingPropertyGetter(cls, fn);
                getterPtr = &getter;
            }
        }

        if (getterPtr == nullptr && setterPtr == nullptr)
        {
            return;
        }

        TypeInfoFunc const& accessSource = getterPtr != nullptr ? getterPtr->function : setterPtr->function;
        std::string const publicCppType = getterPtr != nullptr
            ? GetCppType(getterPtr->function)
            : GetCppType(setterPtr->function.params[0]);
        GenerateCSharpAccessorProperty(cls, getterPtr, setterPtr, GetPropertyName(prop), publicCppType, accessSource.access,
            accessSource.access, accessSource.isStatic, accessSource.attributes, accessSource.comment, assemblyName, output);
    }

    void BindingsCSharpGenerator::GenerateCSharpFieldAccessors(const TypeInfoStruct& cls,
                                                               const TypeInfoField&  field,
                                                               const std::string&    assemblyName,
                                                               std::string&          output)
    {
        std::string const fieldCppType = GetCppType(field);
        const std::string publicType   = GetCSharpPublicType(fieldCppType, field.arraySize);
        const std::string stripped     = StripTypeQualifiers(fieldCppType);

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

        if (field.arraySize > 0)
        {
            AppendCSharpComment(output, "        ", field.comment);
            output += Utils::String::Format(
                "        public {0}[] {1};\n\n", publicType, MakeCSharpIdentifier(field.name), field.arraySize);
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
        if (stripped == "bool")
        {
            fieldDecl = Utils::String::Format("        [MarshalAs(UnmanagedType.U1)]\n        {0} {1} {2}",
                                              fieldAccess,
                                              publicType,
                                              MakeCSharpIdentifier(field.name));
        }
        else if (IsStringType(fieldCppType))
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
            std::string const cppType = GetCppType(evt.params[i]);
            std::string publicType = GetCSharpPublicType(cppType);
            std::string parameterName = MakeCSharpIdentifier(evt.params[i].name);
            delegateTypes += publicType;
            if (evt.params[i].isRef && !evt.params[i].isConst)
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
            if (param.isRef && !param.isConst)
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
            if (evt.params[i].isRef && !evt.params[i].isConst)
                invokeParams += "ref ";
            std::string const cppType = GetCppType(evt.params[i]);
            std::string interopType = GetCSharpInteropType(cppType);
            std::string publicType = GetCSharpPublicType(cppType);
            invokeParams += Utils::String::Format("{0} {1}", interopType, parameterName);
            if (evt.params[i].isRef && !evt.params[i].isConst)
            {
                if (publicType == interopType)
                {
                    managedArguments += Utils::String::Format("ref {0}", parameterName);
                }
                else
                {
                    std::string managedParameterName = Utils::String::Format("__managed_{0}", parameterName);
                    invokePreparation += Utils::String::Format("                {0} {1} = {2};\n", publicType, managedParameterName,
                        GetCSharpFromInterop(cppType, parameterName));
                    invokeWriteBack += Utils::String::Format("                {0} = {1};\n", parameterName,
                        GetCSharpToInterop(cppType, managedParameterName));
                    managedArguments += Utils::String::Format("ref {0}", managedParameterName);
                }
            }
            else
            {
                managedArguments += GetCSharpFromInterop(cppType, parameterName);
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
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedIn, typeof({1}.ManagedToNative))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementIn, typeof({1}.ManagedToNative))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ManagedToUnmanagedOut, typeof({1}.ManagedToNative))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.UnmanagedToManagedIn, typeof({1}.ManagedToNative))]\n", name, marshallerName);
        output += Utils::String::Format("        [CustomMarshaller(typeof({0}), MarshalMode.ElementOut, typeof({1}.ManagedToNative))]\n", name, marshallerName);
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

        output += Utils::String::Format("            internal static {0} ConvertToManaged(IntPtr unmanaged) => Unsafe.As<{0}>(ManagedHandleMarshaller.ConvertToManaged(unmanaged));\n", name);
        output += "            internal static void Free(IntPtr unmanaged) => ManagedHandleMarshaller.Free(unmanaged);\n";

        output += Utils::String::Format("            internal static {0} ToManaged(IntPtr managed) => Unsafe.As<{0}>(ManagedHandleMarshaller.ToManaged(managed));\n", name);
        output += Utils::String::Format("            internal static IntPtr ToNative({0} managed) => ManagedHandleMarshaller.ToNative(managed);\n", name);
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
            std::string const fieldCppType = GetCppType(field);
            std::string fieldType = GetCSharpStructAbiFieldType(fieldCppType);
            if (field.arraySize > 0)
                output += Utils::String::Format("                public fixed {0} {1}[{2}];\n", fieldType, field.name, field.arraySize);
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
            if (field.arraySize > 0)
            {
                output += Utils::String::Format("                result.{0} = new {1}[{2}];\n", field.name,
                    GetCSharpPublicType(GetCppType(field)), field.arraySize);
                output += Utils::String::Format("                for (int i = 0; i < {0}; i++) result.{1}[i] = {2};\n",
                    field.arraySize, field.name, GetCSharpStructFieldFromAbi(GetCppType(field),
                        Utils::String::Format("unmanaged.{0}[i]", field.name)));
            }
            else
            {
                std::string fromInterop = GetCSharpStructFieldFromAbi(GetCppType(field), Utils::String::Format("unmanaged.{0}", field.name));
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
            if (field.arraySize > 0)
            {
                output += Utils::String::Format("                if (managed.{0} != null)\n                {{\n", field.name);
                output += Utils::String::Format("                    int __{0}Count = Math.Min(managed.{0}.Length, {1});\n", field.name, field.arraySize);
                output += Utils::String::Format("                    for (int i = 0; i < __{0}Count; i++) result.{0}[i] = {1};\n",
                    field.name, GetCSharpStructFieldToAbi(GetCppType(field),
                        Utils::String::Format("managed.{0}[i]", field.name)));
                output += "                }\n";
            }
            else
            {
                std::string toInterop = GetCSharpStructFieldToAbi(GetCppType(field), Utils::String::Format("managed.{0}", field.name));
                output += Utils::String::Format("                result.{0} = {1};\n", field.name, toInterop);
            }
        }
        output += "                return result;\n";
        output += "            }\n\n";

        output += Utils::String::Format("            public static void Free({0}Internal unmanaged) {{ }}\n\n", cls.name);

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
        output += "                public void Free() { }\n            }\n";

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

        output += Utils::String::Format("    public unsafe {0}{1}{2}{3}partial class {4}",
                                        abstractKeyword,
                                        classKeyword,
                                        sealedKeyword,
                                        cls.isStruct ? "" : "",
                                        MakeCSharpIdentifier(className));

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
            if (fn.APINoProxy || fn.APIIsPropertie)
                continue;
            GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);
        }

        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (fn.APINoProxy || fn.APIIsPropertie)
                continue;
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

    void BindingsCSharpGenerator::GenerateCSharpStructure(const TypeInfoStruct& cls, const std::string& assemblyName,
                                                            std::string& output)
    {
        std::string nsName = CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceScopeList);
        if (!nsName.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", nsName);
        }

        // Marshaller (must come before the struct for CustomMarshaller attribute)
        GenerateCSharpStructMarshaller(cls, output);

        // Struct declaration
        AppendCSharpComment(output, "    ", cls.comment);
        output += Utils::String::Format("    [StructLayout(LayoutKind.Sequential)]\n");
        output += Utils::String::Format("    [NativeMarshalling(typeof({0}Marshaller))]\n", MakeCSharpIdentifier(cls.name));
        if (IsValidCSharpAttributeList(cls.APIAttributes))
            output += Utils::String::Format("    {0}\n", cls.APIAttributes);

        output += Utils::String::Format("    public unsafe partial struct {0}", MakeCSharpIdentifier(cls.name));

        if (!cls.baseClassName.empty())
        {
            output += Utils::String::Format(" : {0}", GetCSharpPublicType(cls.baseClassName));
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
            if (!fn.APINoProxy && !fn.APIIsPropertie)
                GenerateCSharpWrapperFunction(cls, fn, assemblyName, output);
        }

        for (auto& fn : cls.functions)
        {
            if (!fn.APINoProxy && !fn.APIIsPropertie)
                GenerateCSharpWrapperFunctionCall(cls, fn, output);
        }

        // Default property
        output += Utils::String::Format("        public static {0} Default => new {0}();\n\n", cls.name);

        output += "    }\n";

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
            std::string const returnType = GetCppType(fn);
            std::string publicRetType = GetCSharpPublicType(returnType, fn.returnArraySize);
            std::string publicParams = BuildCSharpParams(fn, true);
            AppendCSharpComment(output, "        ", fn.comment);
            output += Utils::String::Format("        {0} {1}({2});\n",
                IsVoid(fn) ? "void" : publicRetType,
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
        m_errorMessage.clear();

        std::vector<TypeInfoInjectedCode*> injectCodes;
        for (auto const& code : headerInfo.injectedCode)
        {
            if (IsCSharpCode(code))
            {
                injectCodes.emplace_back(code);
            }
        }

        if (headerInfo.classes.empty() && headerInfo.enums.empty() && headerInfo.interfaces.empty() &&
            headerInfo.events.empty())
        {
            return true;
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

        if (!m_errorMessage.empty())
        {
            return false;
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
        return CodeGeneratorUtils::SaveFile(outPath, std::string(output.c_str()));
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
        output += "using System.Runtime.CompilerServices;\n";
        output += "using System.Runtime.InteropServices;\n\n";

        output += Utils::String::Format("[assembly: AssemblyTitle(\"{0}\")]\n", module.name);
        output += Utils::String::Format("[assembly: AssemblyVersion(\"1.0.0.0\")]\n");
        output += Utils::String::Format("[assembly: AssemblyFileVersion(\"1.0.0.0\")]\n");
        output += Utils::String::Format("[assembly: AssemblyProduct(\"{0}\")]\n", module.name);
        output += "[assembly: ComVisible(false)]\n";
        output += "[assembly: DisableRuntimeMarshalling]\n";

        std::string moduleDir = module.assemblyDir;
        std::string outDir = Utils::String::Format("{0}/{1}", moduleDir, Settings::g_autogeneratedDirectory);
        FileSystem::NormalizePath(outDir);
        if (!FileSystem::DirectoryExists(outDir))
        {
            FileSystem::CreateDirectory(outDir);
        }

        std::string outPath = outDir + "/" + module.assemblyType + ".Gen.cs";
        return CodeGeneratorUtils::SaveFile(outPath, std::string(output.c_str()));
    }

} // namespace SE::BuildTool
