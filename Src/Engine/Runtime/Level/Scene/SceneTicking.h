#pragma once
#include "Core/Types/Collections/List.h"
#include "Runtime/API.h"

namespace SE
{
    class SceneTicking
    {
        /// <summary>
        /// Tick function type.
        /// </summary>
        struct SE_API_RUNTIME Tick
        {
            typedef void (*Signature)();
            typedef void (*SignatureObj)(void*);

            template<class T, void(T::*Method)()>
            static void MethodCaller(void* callee)
            {
                (static_cast<T*>(callee)->*Method)();
            }

            void* Callee;
            SignatureObj FunctionObj;

            template<class T, void(T::*Method)()>
            void Bind(T* callee)
            {
                Callee = callee;
                FunctionObj = &MethodCaller<T, Method>;
            }

            FORCE_INLINE void Call() const
            {
                (*FunctionObj)(Callee);
            }
        };

        /// <summary>
        /// Ticking data container.
        /// </summary>
        class SE_API_RUNTIME TickData
        {
        public:
            // List<Script*> Scripts;
            List<Tick> Ticks;
#if SE_EDITOR
            // List<Script*> ScriptsExecuteInEditor;
            List<Tick> TicksExecuteInEditor;
#endif

            explicit TickData(int32 capacity);

            // virtual void TickScripts(const List<Script*>& scripts) = 0;

            // void AddScript(Script* script);
            // void RemoveScript(Script* script);

            template<class T, void(T::*Method)()>
            void AddTick(T* callee)
            {
                SceneTicking::Tick tick;
                tick.Bind<T, Method>(callee);
                Ticks.Add(tick);
            }

            void RemoveTick(void* callee);
            void Tick();

#if SE_EDITOR
            template<class T, void(T::*Method)()>
            void AddTickExecuteInEditor(T* callee)
            {
                SceneTicking::Tick tick;
                tick.Bind<T, Method>(callee);
                TicksExecuteInEditor.Add(tick);
            }

            void RemoveTickExecuteInEditor(void* callee);
            void TickExecuteInEditor();
#endif

            void Clear();
        };

        class SE_API_RUNTIME FixedUpdateTickData : public TickData
        {
        public:
            FixedUpdateTickData();
            // void TickScripts(const List<Script*>& scripts) override;
        };

        class SE_API_RUNTIME UpdateTickData : public TickData
        {
        public:
            UpdateTickData();
            // void TickScripts(const List<Script*>& scripts) override;
        };

        class SE_API_RUNTIME LateUpdateTickData : public TickData
        {
        public:
            LateUpdateTickData();
            // void TickScripts(const List<Script*>& scripts) override;
        };

        class SE_API_RUNTIME LateFixedUpdateTickData : public TickData
        {
        public:
            LateFixedUpdateTickData();
            // void TickScripts(const List<Script*>& scripts) override;
        };

    public:
        /// <summary>
        /// The fixed update tick function.
        /// </summary>
        FixedUpdateTickData FixedUpdate;

        /// <summary>
        /// The update tick function.
        /// </summary>
        UpdateTickData Update;

        /// <summary>
        /// The late update tick function.
        /// </summary>
        LateUpdateTickData LateUpdate;

        /// <summary>
        /// The late fixed update tick function.
        /// </summary>
        LateFixedUpdateTickData LateFixedUpdate;
    };
} // SE

