#pragma once

#include <slang.h>
#include "SLC2/SLC2Artifact.h"

namespace SE
{
    // 编译端只负责把 Slang 的逻辑 Vertex Input Signature 固化到 SLC2，
    // 物理 VertexFactory/Buffer Layout 属于运行时绘制路径，不在此模块声明。
    class SLC2VertexInputCompiler
    {
    public:
        static bool AppendVertexInput(slang::VariableLayoutReflection* variable,
                                      List<SLC2VertexInputSignatureElement>& signature,
                                      String& error);

        static bool ReadVertexInputSignature(slang::ProgramLayout* layout,
                                             int32 entryPointIndex,
                                             List<SLC2VertexInputSignatureElement>& signature,
                                             String& error);

        static bool ValidateVertexInputSignature(const List<SLC2VertexInputSignatureElement>& signature,
                                                 String& error);

        // includeLocation=false 用于比较同一 Program 的跨 Target 变体；
        // includeLocation=true 还会检查同一 Target 内的后端 location 稳定性。
        static bool SameVertexInputSignature(const List<SLC2VertexInputSignatureElement>& a,
                                             const List<SLC2VertexInputSignatureElement>& b,
                                             bool includeLocation);
    };
}
