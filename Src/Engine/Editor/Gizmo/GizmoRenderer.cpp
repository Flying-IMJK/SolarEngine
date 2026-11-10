#include <Editor/Gizmo/GizmoRenderer.h>
// #include <Editor/Gizmo/TransformObjectsAction.h>

#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Level/Actor.h>
#include <Runtime/Render/RenderTask.h>

namespace SE::Editor
{
    GizmoRenderer::GizmoRenderer(const SpawnParams& params) : ScriptingObject(params)
    {
        m_Models[0] = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/GizmoTranslationAxis"));
        m_Models[1] = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/GizmoScaleAxis"));
        m_Models[2] = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/GizmoRotationRing"));
        m_Models[3] = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/GizmoPlane"));
        m_Models[4] = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/GizmoCenter"));
        m_Shader    = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Editor/TransformGizmo"));
#if COMPILE_WITH_DEV_ENV
        if (m_Shader.HasReference())
        {
            m_Shader->OnReloadingEvent.Bind<GizmoRenderer, &GizmoRenderer::OnShaderReloading>(this);
        }
#endif
    }

    GizmoRenderer::~GizmoRenderer()
    {
#if COMPILE_WITH_DEV_ENV
        if (m_Shader.HasReference())
        {
            m_Shader->OnReloadingEvent.Unbind<GizmoRenderer, &GizmoRenderer::OnShaderReloading>(this);
        }
#endif
        // ClearUndo();
        ReleaseResources();
    }

    void GizmoRenderer::ReleaseResources()
    {
        // Pipelines reference the selected Program; release them before its owner.
        Delete(m_Pipeline);
        Delete(m_Program);
        m_Pipeline = nullptr;
        m_Program  = nullptr;
    }

#if COMPILE_WITH_DEV_ENV
    void GizmoRenderer::OnShaderReloading(Asset* pAsset) { ReleaseResources(); }
#endif

    float GizmoRenderer::RayCast(Actor* pActor, const Ray& ray)
    {
        float  distance;
        Float3 normal;
        return pActor != nullptr && pActor->IsActiveInHierarchy() && pActor->IntersectsItself(ray, distance, normal) &&
                       distance >= 0.0f ?
                   distance :
                   -1.0f;
    }

    // void GizmoRenderer::BeginTransform()
    // {
    //     CancelTransform();
    //     m_TransformAction = New<TransformObjectsAction>(this);
    // }
    //
    // void GizmoRenderer::RecordTransform(Actor* pActor)
    // {
    //     if (m_TransformAction != nullptr)
    //     {
    //         m_TransformAction->AddActor(pActor);
    //     }
    // }
    //
    // void GizmoRenderer::EndTransform()
    // {
    //     if (m_TransformAction != nullptr && m_TransformAction->CaptureAfter())
    //     {
    //         // Transfer ownership once, on mouse release, rather than once per update.
    //         m_Undo.RegisterAction(m_TransformAction);
    //         m_TransformAction = nullptr;
    //     }
    //     else
    //     {
    //         CancelTransform();
    //     }
    // }
    //
    // void GizmoRenderer::CancelTransform()
    // {
    //     Delete(m_TransformAction);
    //     m_TransformAction = nullptr;
    // }
    //
    // void GizmoRenderer::Undo()
    // {
    //     if (m_TransformAction == nullptr && m_Undo.CanUndo())
    //     {
    //         m_Undo.Undo();
    //     }
    // }
    //
    // void GizmoRenderer::Redo()
    // {
    //     if (m_TransformAction == nullptr && m_Undo.CanRedo())
    //     {
    //         m_Undo.Redo();
    //     }
    // }
    //
    // void GizmoRenderer::ClearUndo()
    // {
    //     CancelTransform();
    //     m_Undo.Reset();
    // }

    void GizmoRenderer::Draw(SceneRenderTask* pTask,
                             GPUContext*      pContext,
                             int32            modelIndex,
                             const Matrix&    world,
                             const Color&     color)
    {
        if (pTask == nullptr || pContext == nullptr || pTask->Output == nullptr || modelIndex < 0 || modelIndex >= 5 ||
            !m_Shader.HasReference() || !m_Shader->IsLoaded() || !m_Models[modelIndex].HasReference() ||
            !m_Models[modelIndex]->IsLoaded() || !m_Models[modelIndex]->CanBeRendered())
        {
            return;
        }

        if (m_Program == nullptr)
        {
            m_Program  = New<ShaderProgramInstance>();
            m_Pipeline = GPUDevice::instance->CreateSLC2PipelineState();
        }
        if (m_Pipeline == nullptr)
        {
            return;
        }
        SLC2GPUShaderProgram* program = nullptr;
        if (!m_Shader->SelectProgram(SE_TEXT("TransformGizmo"), {}, program))
        {
            return;
        }
        if (m_Program->GetProgram() != program || !m_Pipeline->IsValid())
        {
            m_Pipeline->ReleaseGPU();
            auto desc      = SLC2GPUPipelineState::Description::DefaultNoDepth;
            desc.CullMode  = CullMode::TwoSided;
            desc.BlendMode = BlendingMode::AlphaBlend;
            if (!m_Program->Initialize(program) || !m_Pipeline->Init(desc, program))
            {
                return;
            }
        }

        struct PerGizmo
        {
            Matrix WVP;
            Float4 Color;
        } data;
        Matrix relativeWorld = world;
        pTask->View.GetWorldMatrix(relativeWorld);
        data.WVP = Matrix::Multiply(relativeWorld, pTask->View.ViewProjection());
        //Matrix::Transpose(, data.WVP);
        data.Color = Float4(color.R, color.G, color.B, color.A);
        if (!m_Program->SetConstantBuffer(SE_TEXT("perGizmo"), &data, sizeof(data)))
        {
            return;
        }

        // Gizmos are editor overlays, drawn after the scene and its post processing.
        pContext->SetViewportAndScissors(pTask->GetOutputViewport());
        pContext->SetRenderTarget(pTask->Output->View());
        pContext->SetSLC2State(m_Pipeline);
        for (const auto& mesh : m_Models[modelIndex]->LODs[0].Meshes)
        {
            if (mesh.IsInitialized() && pContext->BindRenderGeometry(mesh.GetRenderGeometry()))
            {
                pContext->DrawIndexed(m_Program, mesh.GetTriangleCount() * 3);
            }
        }
        pContext->ResetRenderTarget();
    }
} // namespace SE::Editor
