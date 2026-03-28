#pragma once

#include "Core/Types/Variable.h"
#include "Core/Types/Collections/List.h"
#include "ShaderAsset.h"

namespace SE
{

    class GraphicShaderAnalyzer
    {
    public:
        struct ShaderStageAnalyze
        {
            RHIShaderStage type;
            String entryPoint;
            int64 stringIndex;
            int64 stringLength;

            ShaderStageAnalyze() : type(RHIShaderStage::Count),
                            entryPoint("main"),
                            stringIndex(-1),
                            stringLength(0)
            {
            }
        };

    public:
        void Analyzer(String &shaderCode);
        String GetStageSourceCode(int stageIndex);

		List<ShaderStageAnalyze> &GetShaderStages() { return m_Stage; }
        String GetVersion() { return m_Version; }

        static String GetShaderStageTypeToString(RHIShaderStage type);

    private:
        void AnalyzeVersion();
        void AnalyzeMacro();
        void AnalyzeParame();
        void AnalyzeStage();

        RHIShaderStage GetShaderStageType(String &str);

    private:
        String m_Code{ ""};
        String m_Version{ "460"};

		List<ShaderStageAnalyze> m_Stage{};
    };

}