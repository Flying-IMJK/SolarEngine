#pragma once

#include <Runtime/Render/Assets/Material/MaterialShader.h>

namespace SE
{
    class ForwardMaterialShader final : public MaterialShader
    {
    public:
        DrawPass GetDrawModes() const override;
        bool CanUseInstancing(InstancingHandler& handler) const override;
        void Bind(BindParameters& params) override;
    };
} // namespace SE
