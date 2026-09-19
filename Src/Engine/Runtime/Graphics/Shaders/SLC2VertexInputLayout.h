#pragma once

#include "Runtime/API.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"
#include "Runtime/Graphics/Base/VertexFactoryLayout.h"

namespace SE
{
    /// <summary>
    /// Shader 逻辑输入与 VertexFactory 物理元素匹配后的结果。
    /// </summary>
    struct SE_API_RUNTIME SLC2VertexInputMatchResult
    {
        List<const VertexFactoryInputElement*> Elements;
        uint32 RequiredVertexBufferSlotsMask = 0;
    };

    /// <summary>
    /// 集中执行 SLC2 Signature 与平台无关 VertexFactoryLayout 的匹配规则。
    /// </summary>
    class SE_API_RUNTIME SLC2VertexInputLayoutMatcher
    {
    public:
        static bool Match(
            const List<SLC2VertexInputSignatureElement>& signature,
            const VertexFactoryLayout& layout,
            SLC2VertexInputMatchResult& output,
            String& error);

        static bool IsCompatible(SLC2VertexInputType shaderType, PixelFormat format);
    };
}
