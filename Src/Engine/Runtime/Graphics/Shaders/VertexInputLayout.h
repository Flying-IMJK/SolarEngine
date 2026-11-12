#pragma once

#include "Runtime/API.h"
#include "Runtime/ShaderCompilation/Slang/Artifact/ShaderArtifact.h"
#include "Runtime/Graphics/Base/VertexFactoryLayout.h"

namespace SE
{
    /// <summary>
    /// Shader 逻辑输入与 VertexFactory 物理元素匹配后的结果。
    /// </summary>
    struct SE_API_RUNTIME VertexInputMatchResult
    {
        List<const VertexFactoryInputElement*> Elements;
        uint32 RequiredVertexBufferSlotsMask = 0;
    };

    /// <summary>
    /// 集中执行 ShaderArtifact Signature 与平台无关 VertexFactoryLayout 的匹配规则。
    /// </summary>
    class SE_API_RUNTIME VertexInputLayoutMatcher
    {
    public:
        static bool Match(
            const List<VertexInputSignatureElement>& signature,
            const VertexFactoryLayout& layout,
            VertexInputMatchResult& output,
            String& error);

        static bool IsCompatible(VertexInputType shaderType, PixelFormat format);
    };
}
