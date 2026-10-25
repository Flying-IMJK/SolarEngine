#pragma once

#include <Runtime/Core/Types/UID.h>
#include <Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h>

namespace SE
{
    class ShaderProgramInstance;

    struct SE_API_RUNTIME SLC2MaterialParameterBinding
    {
        // Stable MaterialParameter identity.
        UID ParameterID = UID::Empty;
        // Exact, case-sensitive reflected NamePath.
        String NamePath;
        // Must exactly match the reflected binding kind at NamePath.
        ShaderProgramBindingKind BindingKind = ShaderProgramBindingKind::Uniform;
    };

    // Writes one non-authored reflected binding synchronously during PrepareDraw on the same GPU/render-resource
    // thread. binding and instance are borrowed and must not be retained. Return true only after writing this exact
    // binding; false aborts preparation and leaves its output empty.
    class SE_API_RUNTIME ISLC2MaterialRuntimeBindingProvider
    {
    public:
        virtual ~ISLC2MaterialRuntimeBindingProvider()                                                        = default;
        virtual bool WriteBinding(const ShaderProgramBinding& binding, ShaderProgramInstance& instance) const = 0;
    };

    struct SE_API_RUNTIME SLC2MaterialDrawContext
    {
        // Non-owning and required only when authored parameters do not cover every reflected binding. The provider
        // must live through PrepareDraw; GPU resources and external buffers it writes remain caller-owned through
        // backend descriptor submission.
        const ISLC2MaterialRuntimeBindingProvider* RuntimeBindings = nullptr;
    };
} // namespace SE
