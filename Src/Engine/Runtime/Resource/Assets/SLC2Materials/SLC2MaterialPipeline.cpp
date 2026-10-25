#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialPipeline.h>

#include <Runtime/Core/Memory/Memory.h>

namespace SE
{
    SLC2MaterialPipeline::~SLC2MaterialPipeline()
    {
        // Lifetime is released after the Pipeline, which may still reference Program modules.
        Delete(State);
    }

    bool SLC2MaterialPipeline::ValidateState(const SLC2GPUPipelineState::Description& state)
    {
        const auto comparison = [](ComparisonFunc value) {
            return value >= ComparisonFunc::Never && value <= ComparisonFunc::Always;
        };
        const auto stencil = [](StencilOperation value) {
            return value >= StencilOperation::Keep && value < StencilOperation::MAX;
        };
        const auto factor = [](BlendingMode::Blend value) {
            const uint32 raw = static_cast<uint32>(value);
            return (raw >= 1 && raw <= 11) || (raw >= 14 && raw <= 19);
        };
        const auto operation = [](BlendingMode::Operation value) {
            return value >= BlendingMode::Operation::Add && value <= BlendingMode::Operation::Max;
        };
        const auto& blend = state.BlendMode;
        return comparison(state.DepthFunc) && comparison(state.StencilFunc) && stencil(state.StencilFailOp) &&
               stencil(state.StencilDepthFailOp) && stencil(state.StencilPassOp) &&
               state.PrimitiveTopology >= PrimitiveTopologyType::Point &&
               state.PrimitiveTopology <= PrimitiveTopologyType::Triangle && state.CullMode >= CullMode::Normal &&
               state.CullMode <= CullMode::TwoSided && factor(blend.SrcBlend) && factor(blend.DestBlend) &&
               factor(blend.SrcBlendAlpha) && factor(blend.DestBlendAlpha) && operation(blend.BlendOp) &&
               operation(blend.BlendOpAlpha) && static_cast<uint32>(blend.RenderTargetWriteMask) <= 15;
    }
} // namespace SE
