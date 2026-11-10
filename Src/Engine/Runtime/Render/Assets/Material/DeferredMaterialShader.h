#pragma once

#include <Runtime/Render/Assets/Material/MaterialShader.h>

namespace SE
{
    class DeferredMaterialShader final : public MaterialShader
    {
    public:
        explicit DeferredMaterialShader(const StringView& name) : MaterialShader(name) {}
        DrawPass GetDrawModes() const override;
        bool     CanUseInstancing(InstancingHandler& handler) const override;
        void Bind(BindParameters& params) override;
    };
} // namespace SE
