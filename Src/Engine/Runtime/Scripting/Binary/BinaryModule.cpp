
#include "BinaryModule.h"

#include "ManagedBinaryModule.h"
#include "Runtime/Scripting/Events.h"
#include "Runtime/Scripting/ScriptingObject.h"

namespace SE
{
    ManagedBinaryModule* GetBinaryModuleCorlib()
    {
        static ManagedBinaryModule assembly("corlib");
        return &assembly;
    }


    CriticalSection BinaryModule::Locker;

    BinaryModule::BinaryModulesList& BinaryModule::GetModules()
    {
        static BinaryModulesList modules;
        return modules;
    }

    BinaryModule* BinaryModule::GetModule(const StringAnsiView& name)
    {
        BinaryModule* result = nullptr;
        auto& modules = GetModules();
        for (int32 i = 0; i < modules.Count(); i++)
        {
            if (modules[i]->GetName() == name)
            {
                result = modules[i];
                break;
            }
        }
        return result;
    }

    BinaryModule::BinaryModule()
    {
        // Register
        GetModules().Add(this);
    }

    void* BinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const ScriptingTypeMethodSignature& signature)
    {
        return FindMethod(typeHandle, signature.Name, signature.Params.Count());
    }

    void BinaryModule::Destroy(bool isReloading)
    {
        // Destroy any default script instances
        for (const auto& type : Types)
        {
            if (type.Type == ScriptingTypes::Script && type.Script.DefaultInstance)
            {
                Delete(type.Script.DefaultInstance);
                type.Script.DefaultInstance = nullptr;
            }
        }

        // Remove any scripting events
        for (auto i = ScriptingEvents::EventsTable.begin(); i.IsNotEnd(); ++i)
        {
            const ScriptingTypeHandle type = i->Key.First;
            if (type.Module == this)
            {
                ScriptingEvents::EventsTable.Remove(i);
            }
        }

        // Unregister
        GetModules().RemoveKeepOrder(this);
    }


    List<GetBinaryModuleFunc, InlinedAllocation<64>>& StaticallyLinkedBinaryModuleInitializer::GetStaticallyLinkedBinaryModules()
    {
        static List<GetBinaryModuleFunc, InlinedAllocation<64>> modules;
        return modules;
    }

    StaticallyLinkedBinaryModuleInitializer::StaticallyLinkedBinaryModuleInitializer(GetBinaryModuleFunc getter)
        : _getter(getter)
    {
        GetStaticallyLinkedBinaryModules().Add(getter);
    }

    StaticallyLinkedBinaryModuleInitializer::~StaticallyLinkedBinaryModuleInitializer()
    {
        GetStaticallyLinkedBinaryModules().Remove(_getter);
    }

} // namespace SE
