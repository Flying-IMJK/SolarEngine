#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialDrawProgram.h>

#include <Runtime/Graphics/Base/RenderGeometry.h>
#include <Runtime/Graphics/GPUContext.h>

namespace SE
{
    SLC2MaterialDrawProgram::SLC2MaterialDrawProgram(UniquePtr<ShaderProgramInstance> selection,
                                                     SharedPtr<SLC2MaterialPipeline> pipeline) :
        m_Selection(MoveTemp(selection)), m_Pipeline(MoveTemp(pipeline))
    {}

    bool SLC2MaterialDrawProgram::Prepare(GPUContext& context, const RenderGeometry& geometry) const
    {
        if (m_Pipeline == nullptr || m_Pipeline->State == nullptr || !m_Pipeline->State->IsValid())
        {
            return false;
        }
        context.SetSLC2State(m_Pipeline->State);
        return context.BindRenderGeometry(geometry);
    }
} // namespace SE
