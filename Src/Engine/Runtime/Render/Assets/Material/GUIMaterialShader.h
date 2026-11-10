#pragma once

#include <Runtime/Render/Assets/Material/MaterialShader.h>

namespace SE
{
    class GUIMaterialShader final : public MaterialShader
    {
    public:
        explicit GUIMaterialShader(const StringView& name) : MaterialShader(name) {}
        DrawPass GetDrawModes() const override;
        void     Bind(BindParameters& params) override;
    };
} // namespace SE
