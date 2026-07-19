#pragma once
#include "Editor/API.h"
#include "Runtime/Core/TypeSystem/IType.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Event.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
    SE_CLASS(Reflect)
    class SE_API_EDITOR IUndoableAction : public IType
    {
        friend class UndoStack;

        SE_DEFINE_CLASS_DEFAULT(IUndoableAction, IType)

    public:
        ~IUndoableAction() override = default;

    protected:
        virtual void Undo() = 0;
        virtual void Redo() = 0;
    };

    //-------------------------------------------------------------------------

    SE_CLASS(Reflect)
    class SE_API_EDITOR CompoundStackAction final : public IUndoableAction
    {
        friend class UndoStack;

        SE_DEFINE_CLASS_DEFAULT(CompoundStackAction, IUndoableAction)

    public:
        ~CompoundStackAction() override
        {
            for (auto &pAction : m_actions)
            {
                Delete(pAction);
            }
        }

    private:
        inline void AddToStack(IUndoableAction *pAction)
        {
            ENGINE_ASSERT(pAction != nullptr);
            m_actions.Add(pAction);
        }

        virtual void Undo() override { ENGINE_UNREACHABLE_CODE(); }
        virtual void Redo() override { ENGINE_UNREACHABLE_CODE(); }

    private:
        List<IUndoableAction *> m_actions;
    };

    //-------------------------------------------------------------------------

    class SE_API_EDITOR UndoStack
    {
    public:
        enum class Operation : uint8
        {
            Undo,
            Redo
        };

    public:
        ~UndoStack();

        // Clear all registered actions
        void Reset();

        // Do we have an action to undo
        inline bool CanUndo() { return !m_recordedActions.IsEmpty(); }

        // Undo the last action - returns the action that we undid
        IUndoableAction const *Undo();

        // Do we have an action we can redo
        inline bool CanRedo() { return !m_undoneActions.IsEmpty(); }

        // Redoes the last action - returns the action that we redid
        IUndoableAction const *Redo();

        //-------------------------------------------------------------------------

        // Register a new action, this transfers ownership of the action memory to the stack
        void RegisterAction(IUndoableAction *pAction);

        // Begin a compound action stack, all subsequent registered undoable actions will be grouped together until you end the compound action
        void BeginCompoundAction();

        // Ends a compound action stack
        void EndCompoundAction();

        //-------------------------------------------------------------------------

        // Fired before we execute an undo/redo action
        TEventHandle<UndoStack::Operation, IUndoableAction const *> OnPreUndoRedo() { return m_preUndoRedoEvent; }

        // Fired after we execute an undo/redo action
        TEventHandle<UndoStack::Operation, IUndoableAction const *> OnPostUndoRedo() { return m_postUndoRedoEvent; }

        // Fired whenever we perform an action (register, undo, redo)
        TEventHandle<> OnActionPerformed() { return m_actionPerformed; }

    private:
        void ExecuteRedo(IUndoableAction *pAction) const;
        void ExecuteUndo(IUndoableAction *pAction) const;

        void ClearUndoStack();
        void ClearRedoStack();

    private:
        List<IUndoableAction *> m_recordedActions;
		List<IUndoableAction *> m_undoneActions;
        TEvent<UndoStack::Operation, IUndoableAction const *> m_preUndoRedoEvent;
        TEvent<UndoStack::Operation, IUndoableAction const *> m_postUndoRedoEvent;
        TEvent<> m_actionPerformed;
        CompoundStackAction *m_pCompoundStackAction = nullptr;
    };
}
