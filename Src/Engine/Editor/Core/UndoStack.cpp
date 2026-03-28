#include "UndoStack.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
    UndoStack::~UndoStack()
    {
        ENGINE_ASSERT(m_pCompoundStackAction == nullptr);
        ClearRedoStack();
        ClearUndoStack();
    }

    void UndoStack::Reset()
    {
        ENGINE_ASSERT(m_pCompoundStackAction == nullptr);
        ClearRedoStack();
        ClearUndoStack();
    }

    //-------------------------------------------------------------------------

    void UndoStack::ExecuteUndo(IUndoableAction *pAction) const
    {
        // Notify listeners
        m_preUndoRedoEvent.Execute(Operation::Redo, pAction);

        // Undo the action and place it on the undone stack
        pAction->Undo();

        // Notify listeners
        m_postUndoRedoEvent.Execute(Operation::Undo, pAction);
        m_actionPerformed.Execute();
    }

    IUndoableAction const *UndoStack::Undo()
    {
        ENGINE_ASSERT(CanUndo());
        ENGINE_ASSERT(m_pCompoundStackAction == nullptr);

        // Pop action off the recorded stack
        IUndoableAction *pAction = m_recordedActions.Last();
        m_recordedActions.Pop();

        if (auto pCompoundAction = TypeTryCast<CompoundStackAction>(pAction))
        {
            // Execute undo actions in reverse
            for (int32 i = pCompoundAction->m_actions.Count() - 1; i >= 0; i--)
            {
                ExecuteUndo(pCompoundAction->m_actions[i]);
            }
        }
        else
        {
            ExecuteUndo(pAction);
        }

        m_undoneActions.Add(pAction);
        return pAction;
    }

    void UndoStack::ExecuteRedo(IUndoableAction *pAction) const
    {
        // Notify listeners
        m_preUndoRedoEvent.Execute(Operation::Redo, pAction);

        // Undo the action and place it on the undone stack
        pAction->Redo();

        // Notify listeners
        m_postUndoRedoEvent.Execute(Operation::Redo, pAction);
        m_actionPerformed.Execute();
    }

    IUndoableAction const *UndoStack::Redo()
    {
        ENGINE_ASSERT(CanRedo());
        ENGINE_ASSERT(m_pCompoundStackAction == nullptr);

        // Pop action off the recorded stack
        IUndoableAction *pAction = m_undoneActions.Last();
        m_undoneActions.Pop();

        if (auto pCompoundAction = TypeTryCast<CompoundStackAction>(pAction))
        {
            for (auto pSubAction : pCompoundAction->m_actions)
            {
                ExecuteRedo(pSubAction);
            }
        }
        else
        {
            ExecuteRedo(pAction);
        }

        m_recordedActions.Add(pAction);
        return pAction;
    }

    //-------------------------------------------------------------------------

    void UndoStack::RegisterAction(IUndoableAction *pAction)
    {
        ENGINE_ASSERT(pAction != nullptr);

        if (m_pCompoundStackAction != nullptr)
        {
            m_pCompoundStackAction->AddToStack(pAction);
        }
        else
        {
            m_recordedActions.Add(pAction);
            ClearRedoStack();
            m_actionPerformed.Execute();
        }
    }

    void UndoStack::BeginCompoundAction()
    {
        ENGINE_ASSERT(m_pCompoundStackAction == nullptr);
        m_pCompoundStackAction = New<CompoundStackAction>();
    }

    void UndoStack::EndCompoundAction()
    {
        ENGINE_ASSERT(m_pCompoundStackAction != nullptr);
        m_recordedActions.Add(m_pCompoundStackAction);
        m_pCompoundStackAction = nullptr;

        ClearRedoStack();
        m_actionPerformed.Execute();
    }

    void UndoStack::ClearUndoStack()
    {
        for (auto &pAction : m_recordedActions)
        {
            Delete(pAction);
        }

        m_recordedActions.Clear();
    }

    void UndoStack::ClearRedoStack()
    {
        for (auto &pAction : m_undoneActions)
        {
            Delete(pAction);
        }

        m_undoneActions.Clear();
    }
}