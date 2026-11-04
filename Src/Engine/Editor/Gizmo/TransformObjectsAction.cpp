// Migrated transform undo/redo support. Intentionally commented out as requested.
// #include <Editor/Gizmo/TransformObjectsAction.h>
// #include <Editor/Gizmo/GizmoRenderer.h>
// #include <Runtime/Level/Actor.h>
//
// namespace SE::Editor
// {
//     void TransformObjectsAction::AddActor(Actor* pActor)
//     {
//         if (pActor == nullptr)
//         {
//             return;
//         }
//         for (const auto& entry : m_Entries)
//         {
//             if (entry.ActorID == pActor->GetID())
//             {
//                 return;
//             }
//         }
//         Entry entry;
//         entry.ActorID = pActor->GetID();
//         entry.Before = entry.After = pActor->GetTransform();
//         m_Entries.Add(entry);
//     }
//
//     bool TransformObjectsAction::CaptureAfter()
//     {
//         bool changed = false;
//         for (auto& entry : m_Entries)
//         {
//             // Resolve IDs for every operation; scene unload must not leave stale native pointers.
//             Actor* actor = Cast<Actor>(FindObject(entry.ActorID, nullptr));
//             if (actor != nullptr)
//             {
//                 entry.After = actor->GetTransform();
//                 changed |= entry.After != entry.Before;
//             }
//         }
//         return changed;
//     }
//
//     void TransformObjectsAction::Undo()
//     {
//         for (const auto& entry : m_Entries)
//         {
//             Actor* actor = Cast<Actor>(FindObject(entry.ActorID, nullptr));
//             if (actor != nullptr)
//             {
//                 actor->SetTransform(entry.Before);
//                 m_Owner->TransformRestored.Execute(actor);
//             }
//         }
//     }
//
//     void TransformObjectsAction::Redo()
//     {
//         for (const auto& entry : m_Entries)
//         {
//             Actor* actor = Cast<Actor>(FindObject(entry.ActorID, nullptr));
//             if (actor != nullptr)
//             {
//                 actor->SetTransform(entry.After);
//                 m_Owner->TransformRestored.Execute(actor);
//             }
//         }
//     }
// }
