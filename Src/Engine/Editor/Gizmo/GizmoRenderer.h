#pragma once

#include <Runtime/Core/Math/Color.h>
#include <Runtime/Core/Math/Line.h>
#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Scripting/ScriptingObject.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Render/Assets/Geometry/Model.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>
#include <Editor/API.h>
// #include <Editor/Core/UndoStack.h>

namespace SE
{
    class Actor;
    class SceneRenderTask;
} // namespace SE

namespace SE::Editor
{
    // class TransformObjectsAction;
    /// Owns the native resources used to draw a viewport's FBX gizmo meshes.
    SE_CLASS(API())
    class SE_API_EDITOR GizmoRenderer : public ScriptingObject
    {
        SCRIPTING_TYPE(GizmoRenderer);

    public:
        ~GizmoRenderer() override;

        /// Called synchronously from RenderTask.End on the render thread. The task and context are borrowed.
        /// Model indices are translation axis, scale axis, rotation ring, plane, and center, in that order.
        SE_FUNCTION(API())
        void Draw(SceneRenderTask* pTask, GPUContext* pContext, int32 modelIndex, const Matrix& world, const Color& color);

        /// Returns the existing native actor intersection distance, or -1 for a miss/inactive actor.
        SE_FUNCTION(API())
        static float RayCast(Actor* pActor, const Ray& ray);

        // UndoStack support is intentionally disabled until the editor enables undo/redo.
        // SE_FUNCTION(API()) void BeginTransform();
        // SE_FUNCTION(API()) void RecordTransform(Actor* pActor);
        // SE_FUNCTION(API()) void EndTransform();
        // SE_FUNCTION(API()) void CancelTransform();
        // SE_FUNCTION(API()) void Undo();
        // SE_FUNCTION(API()) void Redo();
        // SE_FUNCTION(API()) void ClearUndo();
        // SE_EVENT(API()) Delegate<Actor*> TransformRestored;

    private:
        void ReleaseResources();
#if COMPILE_WITH_DEV_ENV
        void OnShaderReloading(Asset* pAsset);
#endif

        AssetRef<Model>         m_Models[5];
        AssetRef<Shader>        m_Shader;
        SLC2GPUPipelineState*   m_Pipeline = nullptr;
        ShaderProgramInstance*  m_Program  = nullptr;

        // UndoStack m_Undo;
        // TransformObjectsAction* m_TransformAction = nullptr;
    };
} // namespace SE::Editor
