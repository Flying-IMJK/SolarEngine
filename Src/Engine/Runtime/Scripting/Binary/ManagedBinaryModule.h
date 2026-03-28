#pragma once

#include "BinaryModule.h"
#include "Runtime/Scripting/ManagedCLR/SEAssembly.h"



namespace SE
{
    /// <summary>
    /// The C#-only scripting assembly container that holds the native types information and supports interop with managed runtime.
    /// </summary>
    class SE_API_RUNTIME ManagedBinaryModule : public BinaryModule
    {
    public:
        /// <summary>
        /// Finds the module by C# assembly.
        /// </summary>
        /// <param name="assembly">The module C# assembly.</param>
        /// <returns>The found binary module or null if missing.</returns>
        static ManagedBinaryModule* GetModule(const SEAssembly* assembly);

    private:

        int32 _firstManagedTypeIndex = 0;
        List<void*> _managedMemoryBlocks;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagedBinaryModule" /> class.
        /// </summary>
        /// <param name="name">The module name.</param>
        ManagedBinaryModule(const StringAnsiView& name);

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagedBinaryModule" /> class.
        /// </summary>
        /// <param name="assembly">The managed assembly. Object will be deleted within the scripting assembly.</param>
        explicit ManagedBinaryModule(SEAssembly* assembly);

        /// <summary>
        /// Finalizes an instance of the <see cref="ManagedBinaryModule"/> class.
        /// </summary>
        ~ManagedBinaryModule();

    public:

        /// <summary>
        /// The managed assembly (C# DLL).
        /// </summary>
        SEAssembly* Assembly;

        /// <summary>
        /// The scripting types cache that maps the managed class to the scripting type index. Build after assembly is loaded and scripting types get the managed classes information.
        /// </summary>
        Dictionary<SEClass*, int32, HeapAllocation> ClassToTypeIndex;


        static ScriptingObject* ManagedObjectSpawn(const ScriptingObjectSpawnParams& params);
        static SEMethod* FindMethod(SEClass* mclass, const ScriptingTypeMethodSignature& signature);

        static ManagedBinaryModule* FindModule(const SEClass* klass);
        static ScriptingTypeHandle FindType(const SEClass* klass);

    private:

        void OnLoading(SEAssembly* assembly);
        void OnLoaded(SEAssembly* assembly);
        void InitType(SEClass* mclass);
        void OnUnloading(SEAssembly* assembly);
        void OnUnloaded(SEAssembly* assembly);

    public:

        // [BinaryModule]
        const StringAnsi& GetName() const override;
        bool IsLoaded() const override;
        void* FindMethod(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, int32 numParams = 0) override;
        void* FindMethod(const ScriptingTypeHandle& typeHandle, const ScriptingTypeMethodSignature& signature) override;
        bool InvokeMethod(void* method, const Variant& instance, Span<Variant> paramValues, Variant& result) override;
        void GetMethodSignature(void* method, ScriptingTypeMethodSignature& signature) override;
        void* FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name) override;
        void GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature) override;
        bool GetFieldValue(void* field, const Variant& instance, Variant& result) override;
        bool SetFieldValue(void* field, const Variant& instance, Variant& value) override;
        void Destroy(bool isReloading) override;
    };
} // namespace SE
