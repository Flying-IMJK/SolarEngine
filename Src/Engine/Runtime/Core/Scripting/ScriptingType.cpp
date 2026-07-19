
#include "ScriptingType.h"
#include "Runtime/Core/Scripting/Binary/BinaryModule.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"

namespace SE
{
    ScriptingTypeHandle::ScriptingTypeHandle(const ScriptingTypeInitializer& initializer)
        : Module(initializer.Module)
        , TypeIndex(initializer.TypeIndex)
    {
    }

    String ScriptingTypeHandle::ToString(bool withAssembly) const
    {
        String result = GetType().ToString();
        if (withAssembly)
        {
            result += SE_TEXT("(module ");
            result += String(Module->GetName());
            result += SE_TEXT(")");
        }
        return result;
    }

    const ScriptingType& ScriptingTypeHandle::GetType() const
    {
        ASSERT_LOW_LAYER(Module);
        return Module->Types[TypeIndex];
    }


    CLRClass* ScriptingTypeHandle::GetClass() const
    {
        ENGINE_ASSERT(Module && Module->Types[TypeIndex].ManagedClass);
        return Module->Types[TypeIndex].ManagedClass;
    }


    bool ScriptingTypeHandle::IsSubclassOf(ScriptingTypeHandle c) const
    {
        auto type = *this;
        if (type == c)
            return false;
        for (; type; type = type.GetType().GetBaseType())
        {
            if (type == c)
                return true;
        }
        return false;
    }

    bool ScriptingTypeHandle::IsAssignableFrom(ScriptingTypeHandle c) const
    {
        while (c)
        {
            if (c == *this)
                return true;
            c = c.GetType().GetBaseType();
        }
        return false;
    }

    bool ScriptingTypeHandle::operator==(const ScriptingTypeInitializer& other) const
    {
        return Module == other.Module && TypeIndex == other.TypeIndex;
    }

    bool ScriptingTypeHandle::operator!=(const ScriptingTypeInitializer& other) const
    {
        return Module != other.Module || TypeIndex != other.TypeIndex;
    }

    ScriptingType::ScriptingType()
        : ManagedClass(nullptr)
        , Module(nullptr)
        , InitRuntime(nullptr)
        , Fullname(nullptr, 0)
        , Type(ScriptingTypes::Script)
        , BaseTypePtr(nullptr)
        , Interfaces(nullptr)
    {
        Script.Spawn = nullptr;
        Script.VTable = nullptr;
        Script.InterfacesOffsets = nullptr;
        Script.ScriptVTable = nullptr;
        Script.ScriptVTableBase = nullptr;
        Script.SetupScriptVTable = nullptr;
        Script.SetupScriptObjectVTable = nullptr;
        Script.DefaultInstance = nullptr;
    }

    ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, SpawnHandler spawn, const ScriptingTypeHandle& baseType, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, const InterfaceImplementation* interfaces)
        : ManagedClass(nullptr)
        , Module(module)
        , InitRuntime(initRuntime)
        , Fullname(fullname)
        , Type(ScriptingTypes::Script)
        , BaseTypeHandle(baseType)
        , BaseTypePtr(nullptr)
        , Interfaces(interfaces)
        , Size(size)
    {
        Script.Spawn = spawn;
        Script.VTable = nullptr;
        Script.InterfacesOffsets = nullptr;
        Script.ScriptVTable = nullptr;
        Script.ScriptVTableBase = nullptr;
        Script.SetupScriptVTable = setupScriptVTable;
        Script.SetupScriptObjectVTable = setupScriptObjectVTable;
        Script.DefaultInstance = nullptr;
    }

    ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, SpawnHandler spawn, ScriptingTypeInitializer* baseType, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, const InterfaceImplementation* interfaces)
        : ManagedClass(nullptr)
        , Module(module)
        , InitRuntime(initRuntime)
        , Fullname(fullname)
        , Type(ScriptingTypes::Script)
        , BaseTypePtr(baseType)
        , Interfaces(interfaces)
        , Size(size)
    {
        Script.Spawn = spawn;
        Script.VTable = nullptr;
        Script.InterfacesOffsets = nullptr;
        Script.ScriptVTable = nullptr;
        Script.ScriptVTableBase = nullptr;
        Script.SetupScriptVTable = setupScriptVTable;
        Script.SetupScriptObjectVTable = setupScriptObjectVTable;
        Script.DefaultInstance = nullptr;
    }

    ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces)
        : ManagedClass(nullptr)
        , Module(module)
        , InitRuntime(initRuntime)
        , Fullname(fullname)
        , Type(ScriptingTypes::Class)
        , BaseTypePtr(baseType)
        , Interfaces(interfaces)
        , Size(size)
    {
        Class.Ctor = ctor;
        Class.Dtor = dtor;
    }

    ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, Copy copy, Box box, Unbox unbox, GetField getField, SetField setField, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces)
        : ManagedClass(nullptr)
        , Module(module)
        , InitRuntime(initRuntime)
        , Fullname(fullname)
        , Type(ScriptingTypes::Structure)
        , BaseTypePtr(baseType)
        , Interfaces(interfaces)
        , Size(size)
    {
        Struct.Ctor = ctor;
        Struct.Dtor = dtor;
        Struct.Copy = copy;
        Struct.Box = box;
        Struct.Unbox = unbox;
        Struct.GetField = getField;
        Struct.SetField = setField;
    }

    ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, EnumItem* items)
        : ManagedClass(nullptr)
        , Module(module)
        , InitRuntime(DefaultInitRuntime)
        , Fullname(fullname)
        , Type(ScriptingTypes::Enum)
        , BaseTypePtr(nullptr)
        , Interfaces(nullptr)
        , Size(size)
    {
        Enum.Items = items;
    }

    ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, InitRuntimeHandler initRuntime, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, GetInterfaceWrapper getInterfaceWrapper)
        : ManagedClass(nullptr)
        , Module(module)
        , InitRuntime(initRuntime)
        , Fullname(fullname)
        , Type(ScriptingTypes::Interface)
        , BaseTypePtr(nullptr)
        , Interfaces(nullptr)
        , Size(0)
    {
        Interface.SetupScriptVTable = setupScriptVTable;
        Interface.SetupScriptObjectVTable = setupScriptObjectVTable;
        Interface.GetInterfaceWrapper = getInterfaceWrapper;
    }

    ScriptingType::ScriptingType(const ScriptingType& other)
        : ManagedClass(other.ManagedClass)
        , Module(other.Module)
        , InitRuntime(other.InitRuntime)
        , Fullname(other.Fullname)
        , Type(other.Type)
        , BaseTypeHandle(other.BaseTypeHandle)
        , BaseTypePtr(other.BaseTypePtr)
        , Interfaces(other.Interfaces)
        , Size(other.Size)
    {
        switch (other.Type)
        {
        case ScriptingTypes::Script:
            Script.Spawn = other.Script.Spawn;
            Script.VTable = nullptr;
            Script.InterfacesOffsets = nullptr;
            Script.ScriptVTable = nullptr;
            Script.ScriptVTableBase = nullptr;
            Script.SetupScriptVTable = other.Script.SetupScriptVTable;
            Script.SetupScriptObjectVTable = other.Script.SetupScriptObjectVTable;
            Script.DefaultInstance = nullptr;
            break;
        case ScriptingTypes::Structure:
            Struct.Ctor = other.Struct.Ctor;
            Struct.Dtor = other.Struct.Dtor;
            Struct.Copy = other.Struct.Copy;
            Struct.Box = other.Struct.Box;
            Struct.Unbox = other.Struct.Unbox;
            Struct.GetField = other.Struct.GetField;
            Struct.SetField = other.Struct.SetField;
            break;
        case ScriptingTypes::Class:
            Class.Ctor = other.Class.Ctor;
            Class.Dtor = other.Class.Dtor;
            break;
        case ScriptingTypes::Enum:
            Enum.Items = other.Enum.Items;
            break;
        case ScriptingTypes::Interface:
            Interface.SetupScriptVTable = other.Interface.SetupScriptVTable;
            Interface.SetupScriptObjectVTable = other.Interface.SetupScriptObjectVTable;
            Interface.GetInterfaceWrapper = other.Interface.GetInterfaceWrapper;
            break;
        default: ;
        }
    }

    ScriptingType::ScriptingType(ScriptingType&& other)
        : ManagedClass(other.ManagedClass)
        , Module(other.Module)
        , InitRuntime(other.InitRuntime)
        , Fullname(other.Fullname)
        , Type(other.Type)
        , BaseTypeHandle(other.BaseTypeHandle)
        , BaseTypePtr(other.BaseTypePtr)
        , Interfaces(other.Interfaces)
        , Size(other.Size)
    {
        switch (other.Type)
        {
        case ScriptingTypes::Script:
            Script.Spawn = other.Script.Spawn;
            Script.VTable = other.Script.VTable;
            other.Script.VTable = nullptr;
            Script.InterfacesOffsets = other.Script.InterfacesOffsets;
            other.Script.InterfacesOffsets = nullptr;
            Script.ScriptVTable = other.Script.ScriptVTable;
            other.Script.ScriptVTable = nullptr;
            Script.ScriptVTableBase = other.Script.ScriptVTableBase;
            other.Script.ScriptVTableBase = nullptr;
            Script.SetupScriptVTable = other.Script.SetupScriptVTable;
            Script.SetupScriptObjectVTable = other.Script.SetupScriptObjectVTable;
            Script.DefaultInstance = other.Script.DefaultInstance;
            other.Script.DefaultInstance = nullptr;
            break;
        case ScriptingTypes::Structure:
            Struct.Ctor = other.Struct.Ctor;
            Struct.Dtor = other.Struct.Dtor;
            Struct.Copy = other.Struct.Copy;
            Struct.Box = other.Struct.Box;
            Struct.Unbox = other.Struct.Unbox;
            Struct.GetField = other.Struct.GetField;
            Struct.SetField = other.Struct.SetField;
            break;
        case ScriptingTypes::Class:
            Class.Ctor = other.Class.Ctor;
            Class.Dtor = other.Class.Dtor;
            break;
        case ScriptingTypes::Enum:
            Enum.Items = other.Enum.Items;
            break;
        case ScriptingTypes::Interface:
            Interface.SetupScriptVTable = other.Interface.SetupScriptVTable;
            Interface.SetupScriptObjectVTable = other.Interface.SetupScriptObjectVTable;
            Interface.GetInterfaceWrapper = other.Interface.GetInterfaceWrapper;
            break;
        default: ;
        }
    }

    ScriptingType::~ScriptingType()
    {
        switch (Type)
        {
        case ScriptingTypes::Script:
            if (Script.DefaultInstance)
                Delete(Script.DefaultInstance);
            if (Script.VTable)
                Platform::Free((byte*)Script.VTable - GetVTablePrefix());
            Platform::Free(Script.InterfacesOffsets);
            Platform::Free(Script.ScriptVTable);
            Platform::Free(Script.ScriptVTableBase);
            break;
        case ScriptingTypes::Structure:
            break;
        case ScriptingTypes::Enum:
            break;
        case ScriptingTypes::Interface:
            break;
        default: ;
        }
    }

    void ScriptingType::DefaultInitRuntime()
    {
    }

    ScriptingObject* ScriptingType::DefaultSpawn(const ScriptingObjectSpawnParams& params)
    {
        return nullptr;
    }

    ScriptingTypeHandle ScriptingType::GetHandle() const
    {
        int32 typeIndex;
        if (Module && Module->FindScriptingType(Fullname, typeIndex))
        {
            return ScriptingTypeHandle(Module, typeIndex);
        }
        return ScriptingTypeHandle();
    }

    ScriptingObject* ScriptingType::GetDefaultInstance() const
    {
        ENGINE_ASSERT(Type == ScriptingTypes::Script);
        if (!Script.DefaultInstance)
        {
            const ScriptingObjectSpawnParams params(UID::New(), GetHandle());
            Script.DefaultInstance = Script.Spawn(params);
            if (!Script.DefaultInstance)
            {
                LOG_ERROR("Scripting", "Failed to create default instance of type {0}", ToString());
            }
        }
        return Script.DefaultInstance;
    }

    const ScriptingType::InterfaceImplementation* ScriptingType::GetInterface(const ScriptingTypeHandle& interfaceType) const
    {
        const InterfaceImplementation* interfaces = Interfaces;
        if (interfaces)
        {
            while (interfaces->InterfaceType)
            {
                if (*interfaces->InterfaceType == interfaceType)
                    return interfaces;
                interfaces++;
            }
        }
        if (BaseTypeHandle)
        {
            return BaseTypeHandle.GetType().GetInterface(interfaceType);
        }
        if (BaseTypePtr)
        {
            return BaseTypePtr->GetType().GetInterface(interfaceType);
        }
        return nullptr;
    }

    void ScriptingType::SetupScriptVTable(ScriptingTypeHandle baseTypeHandle)
    {
        // Call setup for all class starting from the first native type (first that uses virtual calls will allocate table of a proper size, further base types will just add own methods)
        for (ScriptingTypeHandle e = baseTypeHandle; e;)
        {
            const ScriptingType& eType = e.GetType();

            if (eType.Script.SetupScriptVTable)
            {
                ASSERT(eType.ManagedClass);
                eType.Script.SetupScriptVTable(eType.ManagedClass, Script.ScriptVTable, Script.ScriptVTableBase);
            }

            auto interfaces = eType.Interfaces;
            if (interfaces && Script.ScriptVTable)
            {
                while (interfaces->InterfaceType)
                {
                    auto& interfaceType = interfaces->InterfaceType->GetType();
                    if (interfaceType.Interface.SetupScriptVTable)
                    {
                        ASSERT(eType.ManagedClass);
                        const auto scriptOffset = interfaces->ScriptVTableOffset; // Shift the script vtable for the interface implementation start
                        Script.ScriptVTable += scriptOffset;
                        Script.ScriptVTableBase += scriptOffset;
                        interfaceType.Interface.SetupScriptVTable(eType.ManagedClass, Script.ScriptVTable, Script.ScriptVTableBase);
                        Script.ScriptVTable -= scriptOffset;
                        Script.ScriptVTableBase -= scriptOffset;
                    }
                    interfaces++;
                }
            }
            e = eType.GetBaseType();
        }
    }

    NO_SANITIZE_ADDRESS
    void ScriptingType::SetupScriptObjectVTable(void* object, ScriptingTypeHandle baseTypeHandle, int32 wrapperIndex)
    {
        // Analyze vtable size
        void** vtable = *(void***)object;
        const int32 prefixSize = GetVTablePrefix();
        int32 entriesCount = 0;
        while (vtable[entriesCount] && entriesCount < 200)
            entriesCount++;

        // Calculate total vtable size by adding all implemented interfaces that use virtual methods
        const int32 size = entriesCount * sizeof(void*);
        int32 totalSize = prefixSize + size;
        int32 interfacesCount = 0;
        for (ScriptingTypeHandle e = baseTypeHandle; e;)
        {
            const ScriptingType& eType = e.GetType();
            auto interfaces = eType.Interfaces;
            if (interfaces)
            {
                while (interfaces->InterfaceType)
                {
                    auto& interfaceType = interfaces->InterfaceType->GetType();
                    if (interfaceType.Interface.SetupScriptObjectVTable)
                    {
                        void** vtableInterface = *(void***)((byte*)object + interfaces->VTableOffset);
                        int32 interfaceCount = 0;
                        while (vtableInterface[interfaceCount] && interfaceCount < 200)
                            interfaceCount++;
                        totalSize += prefixSize + interfaceCount * sizeof(void*);
                        interfacesCount++;
                    }
                    interfaces++;
                }
            }
            e = eType.GetBaseType();
        }

        // Duplicate vtable
        Script.VTable = (void**)((byte*)Platform::Allocate(totalSize, 16) + prefixSize);
        Platform::MemoryCopy((byte*)Script.VTable - prefixSize, (byte*)vtable - prefixSize, prefixSize + size);

        // Override vtable entries
        if (interfacesCount)
            Script.InterfacesOffsets = (uint16*)Platform::Allocate(interfacesCount * sizeof(uint16*), 16);
        int32 interfaceOffset = size;
        interfacesCount = 0;
        for (ScriptingTypeHandle e = baseTypeHandle; e;)
        {
            const ScriptingType& eType = e.GetType();

            if (eType.Script.SetupScriptObjectVTable)
            {
                // Override vtable entries for this class
                eType.Script.SetupScriptObjectVTable(Script.ScriptVTable, Script.ScriptVTableBase, Script.VTable, entriesCount, wrapperIndex);
            }

            auto interfaces = eType.Interfaces;
            if (interfaces)
            {
                while (interfaces->InterfaceType)
                {
                    auto& interfaceType = interfaces->InterfaceType->GetType();
                    if (interfaceType.Interface.SetupScriptObjectVTable)
                    {
                        // Analyze interface vtable size
                        void** vtableInterface = *(void***)((byte*)object + interfaces->VTableOffset);
                        int32 interfaceCount = 0;
                        while (vtableInterface[interfaceCount] && interfaceCount < 200)
                            interfaceCount++;
                        const int32 interfaceSize = interfaceCount * sizeof(void*);

                        // Duplicate interface vtable
                        Platform::MemoryCopy((byte*)Script.VTable + interfaceOffset, (byte*)vtableInterface - prefixSize, prefixSize + interfaceSize);

                        // Override interface vtable entries
                        const auto scriptOffset = interfaces->ScriptVTableOffset;
                        const auto nativeOffset = interfaceOffset + prefixSize;
                        void** interfaceVTable = (void**)((byte*)Script.VTable + nativeOffset);
                        interfaceType.Interface.SetupScriptObjectVTable(Script.ScriptVTable + scriptOffset, Script.ScriptVTableBase + scriptOffset, interfaceVTable, interfaceCount, wrapperIndex);

                        Script.InterfacesOffsets[interfacesCount++] = (uint16)nativeOffset;
                        interfaceOffset += prefixSize + interfaceSize;
                    }
                    interfaces++;
                }
            }
            e = eType.GetBaseType();
        }
    }

    void ScriptingType::HackObjectVTable(void* object, ScriptingTypeHandle baseTypeHandle, int32 wrapperIndex)
    {
        if (!Script.ScriptVTable)
            return;
        if (!Script.VTable)
        {
            // Ensure to have valid Script VTable hacked
            BinaryModule::Locker.Lock();
            if (!Script.VTable)
            {
                SetupScriptObjectVTable(object, baseTypeHandle, wrapperIndex);
            }
            BinaryModule::Locker.Unlock();
        }

        // Override object vtable with hacked one that has calls to overriden scripting functions
        *(void**)object = Script.VTable;

        if (Script.InterfacesOffsets)
        {
            // Override vtables for interfaces
            int32 interfacesCount = 0;
            for (ScriptingTypeHandle e = baseTypeHandle; e;)
            {
                const ScriptingType& eType = e.GetType();
                auto interfaces = eType.Interfaces;
                if (interfaces)
                {
                    while (interfaces->InterfaceType)
                    {
                        auto& interfaceType = interfaces->InterfaceType->GetType();
                        if (interfaceType.Interface.SetupScriptObjectVTable)
                        {
                            void** interfaceVTable = (void**)((byte*)Script.VTable + Script.InterfacesOffsets[interfacesCount++]);
                            *(void**)((byte*)object + interfaces->VTableOffset) = interfaceVTable;
                            interfacesCount++;
                        }
                        interfaces++;
                    }
                }
                e = eType.GetBaseType();
            }
        }
    }

    String ScriptingType::ToString() const
    {
        return String(Fullname.Get(), Fullname.Length());
    }

    StringAnsiView ScriptingType::GetName() const
    {
        int32 lastDotIndex = Fullname.FindLast('.');
        if (lastDotIndex != -1)
        {
            lastDotIndex++;
            return StringAnsiView(Fullname.Get() + lastDotIndex, Fullname.Length() - lastDotIndex);
        }
        return Fullname;
    }

    ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::SpawnHandler spawn, ScriptingTypeInitializer* baseType, ScriptingType::SetupScriptVTableHandler setupScriptVTable, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable, const ScriptingType::InterfaceImplementation* interfaces)
        : ScriptingTypeHandle(module, module->Types.Count())
    {
        // Script
        module->Types.AddUninitialized();
        new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, spawn, baseType, setupScriptVTable, setupScriptObjectVTable, interfaces);
#if BUILD_DEBUG
        if (module->TypeNameToTypeIndex.ContainsKey(fullname))
            LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
        module->TypeNameToTypeIndex[fullname] = TypeIndex;
    }

    ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
        : ScriptingTypeHandle(module, module->Types.Count())
    {
        // Class
        module->Types.AddUninitialized();
        new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, ctor, dtor, baseType, interfaces);
#if BUILD_DEBUG
        if (module->TypeNameToTypeIndex.ContainsKey(fullname))
            LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
        module->TypeNameToTypeIndex[fullname] = TypeIndex;
    }

    ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingType::Copy copy, ScriptingType::Box box, ScriptingType::Unbox unbox, ScriptingType::GetField getField, ScriptingType::SetField setField, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
        : ScriptingTypeHandle(module, module->Types.Count())
    {
        // Structure
        module->Types.AddUninitialized();
        new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, ctor, dtor, copy, box, unbox, getField, setField, baseType, interfaces);
#if BUILD_DEBUG
        if (module->TypeNameToTypeIndex.ContainsKey(fullname))
            LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
        module->TypeNameToTypeIndex[fullname] = TypeIndex;
    }

    ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, StableID stableId, ScriptingType::EnumItem* items)
        : ScriptingTypeHandle(module, module->Types.Count())
    {
        // Enum
        module->Types.AddUninitialized();
        new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, items);
#if BUILD_DEBUG
        if (module->TypeNameToTypeIndex.ContainsKey(fullname))
            LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
        module->TypeNameToTypeIndex[fullname] = TypeIndex;

        ScriptingTypeCache::scriptTypeDict.Add(stableId, *this);
    }

    ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::SetupScriptVTableHandler setupScriptVTable, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable, ScriptingType::GetInterfaceWrapper getInterfaceWrapper)
        : ScriptingTypeHandle(module, module->Types.Count())
    {
        // Interface
        module->Types.AddUninitialized();
        new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, initRuntime, setupScriptVTable, setupScriptObjectVTable, getInterfaceWrapper);
#if BUILD_DEBUG
        if (module->TypeNameToTypeIndex.ContainsKey(fullname))
            LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
        module->TypeNameToTypeIndex[fullname] = TypeIndex;
    }


    Dictionary<StableID, ScriptingTypeInitializer> ScriptingTypeCache::scriptTypeDict;

} // namespace SE
