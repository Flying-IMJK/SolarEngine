#pragma once

#include "BinaryModule.h"
#include "Runtime/Scripting/ManagedCLR/CLRAssembly.h"



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
        static ManagedBinaryModule* GetModule(const CLRAssembly* assembly);

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
        explicit ManagedBinaryModule(CLRAssembly* assembly);

        /// <summary>
        /// Finalizes an instance of the <see cref="ManagedBinaryModule"/> class.
        /// </summary>
        ~ManagedBinaryModule() override;

    public:

        /// <summary>
        /// The managed assembly (C# DLL).
        /// </summary>
        CLRAssembly* Assembly;

        /// <summary>
        /// The scripting types cache that maps the managed class to the scripting type index. Build after assembly is loaded and scripting types get the managed classes information.
        /// </summary>
        Dictionary<CLRClass*, int32, HeapAllocation> ClassToTypeIndex;


        static ScriptingObject* ManagedObjectSpawn(const ScriptingObjectSpawnParams& params);
        static CLRMethod* FindMethod(CLRClass* mclass, const ScriptingTypeMethodSignature& signature);

        static ManagedBinaryModule* FindModule(const CLRClass* klass);
        static ScriptingTypeHandle FindType(const CLRClass* klass);

    private:

        void OnLoading(CLRAssembly* assembly);
        void OnLoaded(CLRAssembly* assembly);
        void InitType(CLRClass* mclass);
        void OnUnloading(CLRAssembly* assembly);
        void OnUnloaded(CLRAssembly* assembly);

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
