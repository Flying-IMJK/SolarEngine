#pragma once

#include <Runtime/Render/Assets/Material/MaterialShader.h>

namespace SE
{
    class GUIMaterialShader final : public MaterialShader
    {
    public:
        DrawPass GetDrawModes() const override;
        void     Bind(BindParameters& params) override;
    };
} // namespace SE
