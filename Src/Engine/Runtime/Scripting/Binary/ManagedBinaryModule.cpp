
#include "ManagedBinaryModule.h"

#include "NativeBinaryModule.h"
#include "Runtime/Scripting/ManagedCLR/CLRCore.h"
#include "Runtime/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Scripting/ManagedCLR/CLRAssembly.h"
#include "Core/Logging/Logging.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Thread/Threading.h"
#include "Runtime/Scripting/Scripting.h"
#include "Runtime/Scripting/ScriptingObject.h"
#include "Runtime/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Scripting/ManagedCLR/CLRField.h"
#include "Runtime/Scripting/ManagedCLR/CLRMethod.h"
#include "Runtime/Scripting/ManagedCLR/CLRProperty.h"
#include "Runtime/Scripting/ManagedCLR/CLRUtils.h"

namespace SE
{
    ManagedBinaryModule::ManagedBinaryModule(const StringAnsiView& name)
    : ManagedBinaryModule(New<CLRAssembly>(nullptr, name))
    {
    }

    ManagedBinaryModule::ManagedBinaryModule(CLRAssembly* assembly)
    {
        Assembly = assembly;

        // Bind for C# assembly events
        assembly->Loading.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnLoading>(this);
        assembly->Loaded.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnLoaded>(this);
        assembly->Unloading.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnUnloading>(this);
        assembly->Unloaded.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnUnloaded>(this);

        if (Assembly->IsLoaded())
        {
            // Cache stuff if the input assembly has been already loaded
            OnLoaded(Assembly);
        }
    }

    ManagedBinaryModule::~ManagedBinaryModule()
    {
        // Unregister
        auto& modules = GetModules();
        modules.RemoveKeepOrder(this);

        // Cleanup
        Delete(Assembly);
    }

    ManagedBinaryModule* ManagedBinaryModule::GetModule(const CLRAssembly* assembly)
    {
        ManagedBinaryModule* result = nullptr;
        auto& modules = GetModules();
        for (int32 i = 0; i < modules.Count(); i++)
        {
            auto e = dynamic_cast<ManagedBinaryModule*>(modules[i]);
            if (e && e->Assembly == assembly)
            {
                result = e;
                break;
            }
        }
        return result;
    }

    ScriptingObject* ManagedBinaryModule::ManagedObjectSpawn(const ScriptingObjectSpawnParams& params)
    {
        // Create native object
        ScriptingTypeHandle managedTypeHandle = params.Type;
        const ScriptingType* managedTypePtr = &managedTypeHandle.GetType();
        if (managedTypePtr->ManagedClass && managedTypePtr->ManagedClass->IsAbstract())
        {
            LOG_ERROR("Scripting", "Failed to spawn abstract type '{}'", managedTypePtr->ToString());
            return nullptr;
        }
        while (managedTypePtr->Script.Spawn != &ManagedObjectSpawn)
        {
            managedTypeHandle = managedTypePtr->GetBaseType();
            managedTypePtr = &managedTypeHandle.GetType();
        }
        ScriptingType& managedType = (ScriptingType&)*managedTypePtr;
        ScriptingTypeHandle nativeTypeHandle = managedType.GetBaseType();
        const ScriptingType* nativeTypePtr = &nativeTypeHandle.GetType();
        while (nativeTypePtr->Script.Spawn == &ManagedObjectSpawn)
        {
            nativeTypeHandle = nativeTypePtr->GetBaseType();
            nativeTypePtr = &nativeTypeHandle.GetType();
        }
        ScriptingObject* object = nativeTypePtr->Script.Spawn(params);
        if (!object)
        {
            LOG_ERROR("Scripting", "Failed to spawn object of type {0} with native base type {1}.", managedTypePtr->ToString(), nativeTypePtr->ToString());
            return nullptr;
        }

        // Beware! Hacking vtables incoming! Undefined behaviors exploits! Low-level programming!
        managedType.HackObjectVTable(object, nativeTypeHandle, 0);

        // Mark as managed type
        // object->Flags |= ObjectFlags::IsManagedType;

        return object;
    }

    CLRMethod* FindMethod(CLRClass* mclass, const CLRMethod* referenceMethod)
    {
        const List<CLRMethod*>& methods = mclass->GetMethods();
        for (int32 i = 0; i < methods.Count(); i++)
        {
            CLRMethod* method = methods[i];
            if (!method->IsStatic() &&
                method->GetName() == referenceMethod->GetName() &&
                method->GetParametersCount() == referenceMethod->GetParametersCount() &&
                method->GetReturnType() == referenceMethod->GetReturnType()
            )
            {
                return method;
            }
        }
        return nullptr;
    }

    bool VariantTypeEquals(const VariantTypeHandle& type, CLRType* mType, bool isOut = false)
    {
        CLRClass* mClass = CLRCore::Type::GetClass(mType);
        CLRClass* variantClass = CLRUtils::GetClass(type);
        if (variantClass != mClass)
        {
            // Hack for Vector2/3/4 which alias with Float2/3/4 or Double2/3/4 (depending on USE_LARGE_WORLDS)
            if (mClass == CLRCore::TypeCache::Vector2 && (type.Type == VariantTypes::Float2 || type.Type == VariantTypes::Double2))
                return true;
            if (mClass == CLRCore::TypeCache::Vector3 && (type.Type == VariantTypes::Float3 || type.Type == VariantTypes::Double3))
                return true;
            if (mClass == CLRCore::TypeCache::Vector4 && (type.Type == VariantTypes::Float4 || type.Type == VariantTypes::Double4))
                return true;

            return false;
        }
        return true;
    }

    CLRMethod* ManagedBinaryModule::FindMethod(CLRClass* mclass, const ScriptingTypeMethodSignature& signature)
    {
        if (!mclass)
            return nullptr;
        const auto& methods = mclass->GetMethods();
        for (CLRMethod* method : methods)
        {

            if (method->IsStatic() != signature.IsStatic)
                continue;
            if (method->GetName() != signature.Name)
                continue;
            if (method->GetParametersCount() != signature.Params.Count())
                continue;
            bool isValid = true;
            for (int32 paramIdx = 0; paramIdx < signature.Params.Count(); paramIdx++)
            {
                auto& param = signature.Params[paramIdx];
                CLRType* type = method->GetParameterType(paramIdx);
                if (param.IsOut != method->GetParameterIsOut(paramIdx) ||
                    !VariantTypeEquals(param.Type, type, param.IsOut))
                {
                    isValid = false;
                    break;
                }
            }
            if (isValid && VariantTypeEquals(signature.ReturnType, method->GetReturnType()))
                return method;
        }
        return nullptr;
    }



    ManagedBinaryModule* ManagedBinaryModule::FindModule(const CLRClass* klass)
    {
        ManagedBinaryModule* module = nullptr;
        if (klass && klass->GetAssembly())
        {
            auto& modules = BinaryModule::GetModules();
            for (auto e : modules)
            {
                auto managedModule = dynamic_cast<ManagedBinaryModule*>(e);
                if (managedModule && managedModule->Assembly == klass->GetAssembly())
                {
                    module = managedModule;
                    break;
                }
            }
        }
        return module;
    }

    ScriptingTypeHandle ManagedBinaryModule::FindType(const CLRClass* klass)
    {
        auto typeModule = FindModule(klass);
        if (typeModule)
        {
            int32 typeIndex;
            if (typeModule->ClassToTypeIndex.TryGet(klass, typeIndex))
                return ScriptingTypeHandle(typeModule, typeIndex);
        }
        return ScriptingTypeHandle();
    }



    void ManagedBinaryModule::OnLoading(CLRAssembly* assembly)
    {
        PROFILE_CPU();
        for (ScriptingType& type : Types)
        {
            type.InitRuntime();
        }
    }

    void ManagedBinaryModule::OnLoaded(CLRAssembly* assembly)
    {
        PROFILE_CPU();
        ASSERT(ClassToTypeIndex.IsEmpty());
        Threading::ScopeLock lock(Locker);

        const auto& classes = assembly->GetClasses();

        // Cache managed types information
        ClassToTypeIndex.EnsureCapacity(Types.Count() * 4);
        for (int32 typeIndex = 0; typeIndex < Types.Count(); typeIndex++)
        {
            ScriptingType& type = Types[typeIndex];
            ASSERT(type.ManagedClass == nullptr);

            // Cache class
            const StringAnsi typeName(type.Fullname.Get(), type.Fullname.Length());
            classes.TryGet(typeName, type.ManagedClass);
            if (type.ManagedClass == nullptr)
            {
                LOG_ERROR("Scripting", "Missing class {0} from assembly {1}.", type.ToString(), assembly->ToString());
                continue;
            }

            // Cache klass -> type index lookup
            CLRClass* klass = type.ManagedClass;
#if !BUILD_RELEASE
            if (ClassToTypeIndex.ContainsKey(klass))
            {
                LOG_ERROR("Scripting", "Duplicated native types for class {0} from assembly {1}.", type.ToString(), assembly->ToString());
                continue;
            }
#endif
            ClassToTypeIndex[klass] = typeIndex;
        }

        // Cache types for managed-only types that can be used in the engine
        _firstManagedTypeIndex = Types.Count();
        NativeBinaryModule* engine = (NativeBinaryModule*)GetBinaryModuleSERuntime();
        if (engine->Assembly->IsLoaded())
        {
            // TODO: check only assemblies that references SolarEngine.CSharp.dll
            CLRClass* scriptingObjectType = this == engine ? classes["SE.Object"] : ScriptingObject::GetScriptingClass();
            for (auto i = classes.begin(); i.IsNotEnd(); ++i)
            {
                CLRClass* mclass = i->Value;

                // Check if C# class inherits from C++ object class it has no C++ representation
                if (mclass->IsStatic() || mclass->IsInterface() || !mclass->IsSubClassOf(scriptingObjectType))
                {
                    continue;
                }

                InitType(mclass);
            }
        }

        // Invoke module initializers
        if (engine->Assembly->IsLoaded() && this != engine)
        {
            const CLRClass* attribute = engine->Assembly->GetClass("SE.ModuleInitializerAttribute");
            ASSERT_LOW_LAYER(attribute);
            for (auto i = classes.begin(); i.IsNotEnd(); ++i)
            {
                CLRClass* mclass = i->Value;
                if (mclass->IsStatic() && !mclass->IsInterface() && mclass->HasAttribute(attribute))
                {
                    const auto& methods = mclass->GetMethods();
                    for (const CLRMethod* method : methods)
                    {
                        if (method->GetParametersCount() == 0)
                        {
                            CLRObject* exception = nullptr;
                            method->Invoke(nullptr, nullptr, &exception);
                            if (exception)
                            {
                                CLRException ex(exception);
                                String methodName = String(method->GetName());
                                ex.Log(Log::Severity::Error, methodName.Get());
                                LOG_ERROR("Scripting", "Failed to call module initializer for class {0} from assembly {1}.", String(mclass->GetFullName()), assembly->ToString());
                            }
                        }
                    }
                }
            }
        }

    }

    void ManagedBinaryModule::InitType(CLRClass* mclass)
    {
        // Skip if already initialized
        const StringAnsi& typeName = mclass->GetFullName();
        if (TypeNameToTypeIndex.ContainsKey(typeName))
            return;

        // Find first native base C++ class of this C# class
        CLRClass* baseClass = mclass->GetBaseClass();
        ScriptingTypeHandle baseType;
        baseType.Module = FindModule(baseClass);
        if (!baseClass)
        {
            LOG_ERROR("Scripting", "Missing base class for managed class {0} from assembly {1}.", String(typeName), Assembly->ToString());
            return;
        }
        if (baseType.Module == this)
            InitType(baseClass); // Ensure base is initialized before

        baseType.Module->TypeNameToTypeIndex.TryGet(baseClass->GetFullName(), *(int32*)&baseType.TypeIndex);

        // So we must special case this flow of a generic class of which its possible the generic base class is not in the same module
        if (baseType.TypeIndex == -1 && baseClass->IsGeneric())
        {
            auto genericNameIndex = baseClass->GetFullName().FindLast('`');
            // we add 2 because of the way generic names work its `N
            auto genericClassName = baseClass->GetFullName().Substring(0, genericNameIndex + 2);

            // We check for the generic class name instead of the baseclass fullname
            baseType.Module->TypeNameToTypeIndex.TryGet(genericClassName, *(int32*)&baseType.TypeIndex);
        }

        if (baseType.TypeIndex == -1 || baseType.Module == nullptr)
        {
            if (baseType.Module)
                LOG_ERROR("Scripting", "Missing base class for managed class {0} from assembly {1}.", String(baseClass->GetFullName()), baseType.Module->GetName().ToString());
            else
                LOG_ERROR("Scripting", "Missing base class for managed class {0} from unknown assembly.", String(baseClass->GetFullName()));
            return;
        }

        ScriptingTypeHandle nativeType = baseType;
        while (true)
        {
            auto& type = nativeType.GetType();
            if (type.Script.Spawn != &ManagedObjectSpawn)
                break;
            nativeType = type.GetBaseType();
            if (!nativeType)
            {
                LOG_ERROR("Scripting", "Missing base class for managed class {0} from assembly {1}.", String(typeName), Assembly->ToString());
                return;
            }
        }

        // Scripting Type has Fullname span pointing to the string in memory (usually static data) so store the name in assembly
        char* typeNameData = (char*)PlatformAllocator::Allocate(typeName.Length() + 1);
        Platform::MemoryCopy(typeNameData, typeName.Get(), typeName.Length());
        typeNameData[typeName.Length()] = 0;
        _managedMemoryBlocks.Add(typeNameData);

        // Initialize scripting interfaces implemented in C#
        int32 interfacesCount = 0;
        CLRClass* klass = mclass;
        const List<CLRClass*>& interfaceClasses = klass->GetInterfaces();
        for (const CLRClass* interfaceClass : interfaceClasses)
        {
            const ScriptingTypeHandle interfaceType = FindType(interfaceClass);
            if (interfaceType)
                interfacesCount++;
        }
        ScriptingType::InterfaceImplementation* interfaces = nullptr;
        if (interfacesCount != 0)
        {
            interfaces = (ScriptingType::InterfaceImplementation*)PlatformAllocator::Allocate((interfacesCount + 1) * sizeof(ScriptingType::InterfaceImplementation));
            interfacesCount = 0;
            for (const CLRClass* interfaceClass : interfaceClasses)
            {
                const ScriptingTypeHandle interfaceTypeHandle = FindType(interfaceClass);
                if (!interfaceTypeHandle)
                    continue;
                auto& interface = interfaces[interfacesCount++];
                auto ptr = (ScriptingTypeHandle*)PlatformAllocator::Allocate(sizeof(ScriptingTypeHandle));
                *ptr = interfaceTypeHandle;
                _managedMemoryBlocks.Add(ptr);
                interface.InterfaceType = ptr;
                interface.VTableOffset = 0;
                interface.ScriptVTableOffset = 0;
                interface.IsNative = false;
            }
            Platform::MemoryClear(interfaces + interfacesCount, sizeof(ScriptingType::InterfaceImplementation));
            _managedMemoryBlocks.Add(interfaces);
        }

        // Create scripting type descriptor for managed-only type based on the native base class
        const int32 typeIndex = Types.Count();
        Types.AddUninitialized();
        new(Types.Get() + Types.Count() - 1)ScriptingType(typeName, this, baseType.GetType().Size, ScriptingType::DefaultInitRuntime, ManagedObjectSpawn, baseType, nullptr, nullptr, interfaces);
        TypeNameToTypeIndex[typeName] = typeIndex;
        auto& type = Types[typeIndex];
        type.ManagedClass = mclass;

        // Register C# class
        ASSERT(!ClassToTypeIndex.ContainsKey(klass));
        ClassToTypeIndex[klass] = typeIndex;

        // Create managed vtable for this class (build out of the wrapper C++ methods that call C# methods)
        type.SetupScriptVTable(nativeType);
        CLRMethod** scriptVTable = (CLRMethod**)type.Script.ScriptVTable;
        while (scriptVTable && *scriptVTable)
        {
            const CLRMethod* referenceMethod = *scriptVTable;

            // Find that method overriden in C# class (the current or one of the base classes in C#)
            CLRMethod* method = ::SE::FindMethod(mclass, referenceMethod);
            if (method == nullptr)
            {
                // Check base classes (skip native class)
                baseClass = mclass->GetBaseClass();
                CLRClass* nativeBaseClass = nativeType.GetType().ManagedClass;
                while (baseClass && baseClass != nativeBaseClass && method == nullptr)
                {
                    method = ::SE::FindMethod(baseClass, referenceMethod);

                    // Special case if method was found but the base class uses generic arguments
                    if (method && baseClass->IsGeneric())
                    {
                        CLRClass* parentClass = mclass->GetBaseClass();
                        CLRMethod* parentMethod = parentClass->GetMethod(referenceMethod->GetName().Get(), 0);
                        method = parentMethod->InflateGeneric();
                    }

                    baseClass = baseClass->GetBaseClass();
                }
            }

            // Set the method to call (null entry marks unused entries that won't use C# wrapper calls)
            *scriptVTable = method;

            // Move to the next entry (table is null terminated)
            scriptVTable++;
        }

    }

    void ManagedBinaryModule::OnUnloading(CLRAssembly* assembly)
    {
        PROFILE_CPU();

        // Clear managed types typenames
        for (int32 i = _firstManagedTypeIndex; i < Types.Count(); i++)
        {
            const ScriptingType& type = Types[i];
            const StringAnsi typeName(type.Fullname.Get(), type.Fullname.Length());
            TypeNameToTypeIndex.Remove(typeName);
        }
    }

    void ManagedBinaryModule::OnUnloaded(CLRAssembly* assembly)
    {
        PROFILE_CPU();

        // Clear managed-only types
        Types.Resize(_firstManagedTypeIndex);
        for (int32 i = 0; i < _managedMemoryBlocks.Count(); i++)
            PlatformAllocator::Free(_managedMemoryBlocks[i]);
        _managedMemoryBlocks.Clear();

        // Clear managed types information
        for (ScriptingType& type : Types)
        {
            type.ManagedClass = nullptr;
            if (type.Type == ScriptingTypes::Script && type.Script.ScriptVTable)
            {
                Platform::Free(type.Script.ScriptVTable);
                type.Script.ScriptVTable = nullptr;
            }
        }

        ClassToTypeIndex.Clear();
    }

    const StringAnsi& ManagedBinaryModule::GetName() const
    {
        return Assembly->GetName();
    }

    bool ManagedBinaryModule::IsLoaded() const
    {
        return Assembly->IsLoaded();
    }

    void* ManagedBinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, int32 numParams)
    {
        const ScriptingType& type = typeHandle.GetType();
        return type.ManagedClass ? type.ManagedClass->GetMethod(name.Get(), numParams) : nullptr;
    }

    void* ManagedBinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const ScriptingTypeMethodSignature& signature)
    {
        const ScriptingType& type = typeHandle.GetType();
        return FindMethod(type.ManagedClass, signature);
    }

    bool ManagedBinaryModule::InvokeMethod(void* method, const Variant& instance, Span<Variant> paramValues, Variant& result)
    {
        const auto mMethod = (CLRMethod*)method;
        const int32 parametersCount = mMethod->GetParametersCount();;
        if (paramValues.Length() != parametersCount)
        {
            LOG_ERROR("Scripting", "Failed to call method '{0}.{1}' (args count: {2}) with invalid parameters amount ({3})", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, paramValues.Length());
            return true;
        }

        // Get instance object
        void* mInstance = nullptr;
        const bool withInterfaces = !mMethod->IsStatic() && mMethod->GetParentClass()->IsInterface();
        if (!mMethod->IsStatic())
        {
            // Box instance into C# object (and validate the type)
            CLRObject* instanceObject = CLRUtils::BoxVariant(instance);
            if (!instanceObject)
            {
                LOG_ERROR("Scripting", "Failed to call method '{0}.{1}' (args count: {2}) without object instance", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount);
                return true;
            }
            const CLRClass* instanceObjectClass = CLRCore::Object::GetClass(instanceObject);
            if (!instanceObjectClass->IsSubClassOf(mMethod->GetParentClass(), withInterfaces))
            {
                LOG_ERROR("Scripting", "Failed to call method '{0}.{1}' (args count: {2}) with invalid object instance of type '{3}'", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, String(CLRUtils::GetClassFullname(instanceObject)));
                return true;
            }

#if USE_NETCORE
            mInstance = instanceObject;
#else
            // For value-types instance is the actual boxed object data, not te object itself
            mInstance = instanceObjectClass->IsValueType() ? CLRCore::Object::Unbox(instanceObject) : instanceObject;
#endif
        }

        // Marshal parameters
        void** params = (void**)alloca(parametersCount * sizeof(void*));
        bool failed = false;
        bool hasOutParams = false;
        for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
        {
            Variant& paramValue = paramValues[paramIdx];
            const bool isOut = mMethod->GetParameterIsOut(paramIdx);
            hasOutParams |= isOut;

            // Marshal parameter for managed method
            CLRType* paramType = mMethod->GetParameterType(paramIdx);
            params[paramIdx] = CLRUtils::VariantToManagedArgPtr(paramValue, paramType, failed);
            if (failed)
            {
                LOG_ERROR("Scripting", "Failed to marshal parameter {5}:{4} of method '{0}.{1}' (args count: {2}), value type: {6}, value: {3}", mMethod->GetParentClass()->GetFullName(),
                    mMethod->GetName(), parametersCount, paramValue.ToString(), CLRCore::Type::ToString(paramType), paramIdx, paramValue.Type.ToString());
                return true;
            }
        }

        // Invoke the method
        CLRObject* exception = nullptr;
#if USE_NETCORE // NetCore uses the same path for both virtual and non-virtual calls
        SEObject* resultObject = mMethod->Invoke(mInstance, params, &exception);
#else
        CLRObject* resultObject = withInterfaces ? mMethod->InvokeVirtual((CLRObject*)mInstance, params, &exception) : mMethod->Invoke(mInstance, params, &exception);
#endif
        if (exception)
        {
            CLRException ex(exception);
            ex.Log(Log::Severity::Error, SE_TEXT("InvokeMethod"));
            return true;
        }

        // Unbox result
        result = CLRUtils::UnboxVariant(resultObject);

#if 0
        // Helper method invocations logging
        String log;
        log += result.ToString();
        log += TEXT(" ");
        log += String(mMethod->GetName());
        log += TEXT("(");
        for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
        {
            if (paramIdx != 0)
                log += TEXT(", ");
            log += paramValues[paramIdx].ToString();
        }
        log += TEXT(")");
        LOG_STR(Warning, log);
#endif

        // Unbox output parameters values
        if (hasOutParams)
        {
            for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
            {
                if (mMethod->GetParameterIsOut(paramIdx))
                {
                    auto& paramValue = paramValues[paramIdx];
                    auto param = params[paramIdx];
                    switch (paramValue.Type.Type)
                    {
                    case VariantTypes::String:
                        paramValue.SetString(CLRUtils::ToString((CLRString*)param));
                        break;
                    case VariantTypes::Object:
                        paramValue = CLRUtils::UnboxVariant((CLRObject*)param);
                        break;
                    case VariantTypes::Structure:
                    {
                        const ScriptingTypeHandle paramTypeHandle = Scripting::FindScriptingType(StringAnsiView(paramValue.Type.TypeName));
                        if (paramTypeHandle)
                        {
                            auto& valueType = paramTypeHandle.GetType();
                            CLRObject* boxed = CLRCore::Object::Box(param, valueType.ManagedClass);
                            valueType.Struct.Unbox(paramValue.AsBlob.Data, boxed);
                        }
                        break;
                    }
                    }
                }
            }
        }

        return false;

    }

    void ManagedBinaryModule::GetMethodSignature(void* method, ScriptingTypeMethodSignature& signature)
    {
        const auto mMethod = (CLRMethod*)method;
        signature.Name = mMethod->GetName();
        signature.IsStatic = mMethod->IsStatic();
        signature.ReturnType = MoveTemp(CLRUtils::UnboxVariantType(mMethod->GetReturnType()));
        const int32 paramsCount = mMethod->GetParametersCount();
        signature.Params.Resize(paramsCount);
        for (int32 paramIdx = 0; paramIdx < paramsCount; paramIdx++)
        {
            auto& param = signature.Params[paramIdx];
            param.Type = MoveTemp(CLRUtils::UnboxVariantType(mMethod->GetParameterType(paramIdx)));
            param.IsOut = mMethod->GetParameterIsOut(paramIdx);
        }
    }

    // Pointers with the highest bit turned on are properties
#if PLATFORM_64BITS
#define ManagedBinaryModuleFieldIsPropertyBit (uintptr)(1ull << 63)
#else
#define ManagedBinaryModuleFieldIsPropertyBit (uintptr)(1ul << 31)
#endif

    void* ManagedBinaryModule::FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name)
    {
        const ScriptingType& type = typeHandle.GetType();
        void* result = type.ManagedClass ? type.ManagedClass->GetField(name.Get()) : nullptr;
        if (!result && type.ManagedClass)
        {
            result = type.ManagedClass->GetProperty(name.Get());
            if (result)
                result = (void*)((uintptr)result | ManagedBinaryModuleFieldIsPropertyBit);
        }
        return result;
    }

    void ManagedBinaryModule::GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature)
    {
        if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
        {
            const auto mProperty = (CLRProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
            fieldSignature.Name = mProperty->GetName();
            fieldSignature.ValueType = MoveTemp(CLRUtils::UnboxVariantType(mProperty->GetType()));
            fieldSignature.IsStatic = mProperty->IsStatic();
        }
        else
        {
            const auto mField = (CLRField*)field;
            fieldSignature.Name = mField->GetName();
            fieldSignature.ValueType = MoveTemp(CLRUtils::UnboxVariantType(mField->GetType()));
            fieldSignature.IsStatic = mField->IsStatic();
        }
    }

    bool ManagedBinaryModule::GetFieldValue(void* field, const Variant& instance, Variant& result)
    {
        bool isStatic;
        CLRClass* parentClass;
        StringAnsiView name;
        if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
        {
            const auto mProperty = (CLRProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
            isStatic = mProperty->IsStatic();
            parentClass = mProperty->GetParentClass();
            name = mProperty->GetName();
        }
        else
        {
            const auto mField = (CLRField*)field;
            isStatic = mField->IsStatic();
            parentClass = mField->GetParentClass();
            name = mField->GetName();
        }

        // Get instance object
        CLRObject* instanceObject = nullptr;
        if (!isStatic)
        {
            // Box instance into C# object
            instanceObject = CLRUtils::BoxVariant(instance);

            // Validate instance
            if (!instanceObject || !CLRCore::Object::GetClass(instanceObject)->IsSubClassOf(parentClass))
            {
                if (!instanceObject)
                    LOG_ERROR("Scripting", "Failed to get '{0}.{1}' without object instance", String(parentClass->GetFullName()), String(name));
                else
                    LOG_ERROR("Scripting", "Failed to get '{0}.{1}' with invalid object instance of type '{2}'", String(parentClass->GetFullName()), String(name), String(CLRUtils::GetClassFullname(instanceObject)));
                return true;
            }
        }

        // Get the value
        CLRObject* resultObject;
        if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
        {
            const auto mProperty = (CLRProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
            resultObject = mProperty->GetValue(instanceObject, nullptr);
        }
        else
        {
            const auto mField = (CLRField*)field;
            resultObject = mField->GetValueBoxed(instanceObject);
        }
        result = CLRUtils::UnboxVariant(resultObject);
        return false;
    }

    bool ManagedBinaryModule::SetFieldValue(void* field, const Variant& instance, Variant& value)
    {
        bool isStatic;
        CLRClass* parentClass;
        StringAnsiView name;
        if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
        {
            const auto mProperty = (CLRProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
            isStatic = mProperty->IsStatic();
            parentClass = mProperty->GetParentClass();
            name = mProperty->GetName();
        }
        else
        {
            const auto mField = (CLRField*)field;
            isStatic = mField->IsStatic();
            parentClass = mField->GetParentClass();
            name = mField->GetName();
        }

        // Get instance object
        CLRObject* instanceObject = nullptr;
        if (!isStatic)
        {
            // Box instance into C# object
            instanceObject = CLRUtils::BoxVariant(instance);

            // Validate instance
            if (!instanceObject || !CLRCore::Object::GetClass(instanceObject)->IsSubClassOf(parentClass))
            {
                if (!instanceObject)
                    LOG_ERROR("Scripting", "Failed to set '{0}.{1}' without object instance", String(parentClass->GetFullName()), String(name));
                else
                    LOG_ERROR("Scripting", "Failed to set '{0}.{1}' with invalid object instance of type '{2}'", String(parentClass->GetFullName()), String(name), String(CLRUtils::GetClassFullname(instanceObject)));
                return true;
            }
        }

        // Set the value
        bool failed = false;
        if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
        {
            const auto mProperty = (CLRProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
            mProperty->SetValue(instanceObject, CLRUtils::BoxVariant(value), nullptr);
        }
        else
        {
            const auto mField = (CLRField*)field;
            mField->SetValue(instanceObject, CLRUtils::VariantToManagedArgPtr(value, mField->GetType(), failed));
        }
        return failed;
    }

    void ManagedBinaryModule::Destroy(bool isReloading)
    {
        BinaryModule::Destroy(isReloading);

        // Release managed assembly
        Assembly->Unload(isReloading);
    }
}// namespace SE
