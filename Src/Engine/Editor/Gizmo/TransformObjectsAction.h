#pragma once

// Migrated transform undo/redo support. Intentionally commented out as requested.
// Enable together with the UndoStack members and call sites in GizmoRenderer and TransformGizmo.
//
// #include <Editor/Core/UndoStack.h>
// #include <Runtime/Core/Math/Transform.h>
// #include <Runtime/Core/Types/UID.h>
//
// namespace SE { class Actor; }
// namespace SE::Editor
// {
//     class GizmoRenderer;
//
//     SE_CLASS(Reflect)
//     class TransformObjectsAction final : public IUndoableAction
//     {
//         SE_DEFINE_CLASS_DEFAULT(TransformObjectsAction, IUndoableAction)
//
//     public:
//         explicit TransformObjectsAction(GizmoRenderer* pOwner) : m_Owner(pOwner) { }
//         void AddActor(Actor* pActor);
//         bool CaptureAfter();
//
//     protected:
//         void Undo() override;
//         void Redo() override;
//
//     private:
//         struct Entry
//         {
//             UID ActorID;
//             Transform Before;
//             Transform After;
//         };
//         List<Entry> m_Entries;
//         GizmoRenderer* m_Owner;
//     };
// }
