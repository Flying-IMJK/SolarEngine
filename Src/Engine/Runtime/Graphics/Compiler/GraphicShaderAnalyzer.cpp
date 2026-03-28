#include "GraphicShaderAnalyzer.h"
#include <regex>

namespace SE
{
    void GraphicShaderAnalyzer::Analyzer(String &shaderCode)
    {
        m_Code = std::move(shaderCode);

        AnalyzeVersion();
        AnalyzeParame();
        AnalyzeMacro();
        AnalyzeStage();
    }

    String GraphicShaderAnalyzer::GetStageSourceCode(int stageIndex)
    {
        if (stageIndex < 0 || stageIndex > m_Stage.Count())
        {
            return String::Empty;
        }

        ShaderStageAnalyze stage = m_Stage[stageIndex];
        return m_Code.Substring(stage.stringIndex, stage.stringLength);
    }

    void GraphicShaderAnalyzer::AnalyzeVersion()
    {
/*        std::smatch match;
        static std::regex regex = std::regex("#version (.*);");

        if (std::regex_search(std::string( m_Code.Get()), match, regex))
        {
            m_Version = match[1];
        }
        else
        {
        }*/
    }

    void GraphicShaderAnalyzer::AnalyzeMacro()
    {
    }

    void GraphicShaderAnalyzer::AnalyzeParame()
    {
    }

    void GraphicShaderAnalyzer::AnalyzeStage()
    {
/*        static std::regex regex = std::regex(R"(#SHADER_BEGIN\((.*)\)([\w\W]*?)#SHADER_END)");

        std::regex_iterator<String::iterator> iter(m_Code.begin(), m_Code.end(), regex);
        std::regex_iterator<String::iterator> rend;
        Ref<List<String>> stageParamList = CreateRef<List<String>>();

        while (iter != rend)
        {
            auto match = *iter;

            String stageParam = match[1];
            StringTool::Trim(stageParam);
            stageParamList->clear();
            StringTool::Split(stageParam, *stageParamList, ",");

            ShaderStageAnalyze shaderStage;

            shaderStage.type = GetShaderStageType(stageParamList->at(0));
            shaderStage.entryPoint = stageParamList->at(1);
            shaderStage.stringIndex = match.position(2);
            shaderStage.stringLength = match.length(2);

            m_Stage.emplace_back(shaderStage);
            ++iter;
        }*/
    }

    RHIShaderStage GraphicShaderAnalyzer::GetShaderStageType(String &str)
    {
        RHIShaderStage type = RHIShaderStage::Count;

        if (str == "vert")
        {
            type = RHIShaderStage::VS;
        }
        else if (str == "frag")
        {
            type = RHIShaderStage::PS;
        }

        return type;
    }

    String GraphicShaderAnalyzer::GetShaderStageTypeToString(RHIShaderStage type)
    {
        switch (type)
        {
        case RHIShaderStage::VS:
            return "Vertex";
            break;
        case RHIShaderStage::PS:
            return "Fragme";
            break;
        }
        return "";
    }
}