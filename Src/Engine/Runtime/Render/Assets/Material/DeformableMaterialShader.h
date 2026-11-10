#pragma once

#include <Runtime/Render/Assets/Material/MaterialShader.h>

namespace SE
{
    class DeformableMaterialShader final : public MaterialShader
    {
    public:
        explicit DeformableMaterialShader(const StringView& name) : MaterialShader(name) {}
        DrawPass GetDrawModes() const override;
        void     Bind(BindParameters& params) override;
    };
}
