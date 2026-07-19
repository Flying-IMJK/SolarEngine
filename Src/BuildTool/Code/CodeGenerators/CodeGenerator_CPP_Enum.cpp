#include "CodeGenerator_CPP_Enum.h"

#include "CodeGenerator_Utils.h"
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static mustache::data GenerateFile(std::string const &exportMacro, TypeData const &type)
    {
        mustache::data generatorData;

        std::vector<int32_t> sortingConstantIndices; // Sorted list of constant indices
        std::vector<int32_t> sortedOrder;            // Final order for each constant

        for (auto i = 0u; i < type.enumConstants.size(); i++)
        {
            sortingConstantIndices.push_back(i);
        }

        Function<bool(const int32_t &, const int32_t &)> Comparator = [&type](const int32_t &a, const int32_t &b)
        {
            auto const &elemA = type.enumConstants[a];
            auto const &elemB = type.enumConstants[b];
            return elemA.label < elemB.label;
        };

        Utils::Vector::QuickSort(sortingConstantIndices, Comparator);

        sortedOrder.resize(sortingConstantIndices.size());

        for (auto i = 0u; i < type.enumConstants.size(); i++)
        {
            sortedOrder[sortingConstantIndices[i]] = i;
        }

        if (type.isDevOnly)
        {
            generatorData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
            generatorData.set("isDevOnlyEnd", "#endif");
        }

        std::string namespaceName = CodeGeneratorUtils::GetFullCNameSpaceName(type.namespaceScopeList).c_str();
        if (type.structScopeList.size() > 0)
        {
            namespaceName.append("::");
            namespaceName.append(CodeGeneratorUtils::GetFullCNameSpaceName(type.structScopeList).c_str());
        }

        generatorData.set("namespace", namespaceName);
        generatorData.set("typeName", type.name.c_str());

        generatorData.set("friendlyName", type.GetFriendlyName().c_str());
        generatorData.set("Category", type.GetCategory().c_str());

        switch (type.underlyingType)
        {
        case Utils::TypeIDCore::Uint8:
                generatorData.set("underlyingType", "TypeIDCore::Uint8");
            break;

        case Utils::TypeIDCore::Int8:
                generatorData.set("underlyingType", "TypeIDCore::Int8");
            break;

        case Utils::TypeIDCore::Uint16:
                generatorData.set("underlyingType", "TypeIDCore::Uint16");
            break;

        case Utils::TypeIDCore::Int16:
                generatorData.set("underlyingType", "TypeIDCore::Int16");
            break;

        case Utils::TypeIDCore::Uint32:
                 generatorData.set("underlyingType", "TypeIDCore::Uint32");
            break;

        case Utils::TypeIDCore::Int32:
                 generatorData.set("underlyingType", "TypeIDCore::Int32");
            break;

        default:
            ENGINE_UNREACHABLE_CODE();
            break;
        }

        mustache::data enumConstantsData(mustache::data::type::list);
        for (auto i = 0u; i < type.enumConstants.size(); i++)
        {
            mustache::data enumConstantData;
			std::string escapedDescription = type.enumConstants[i].description;
			Utils::String::ReplaceAll(escapedDescription, "\"", "\\\"");

            enumConstantData.set("enumConstantLabel", type.enumConstants[i].label.c_str());
            enumConstantData.set("enumConstantValue", std::to_string(type.enumConstants[i].value));
            enumConstantData.set("enumConstantSortedOrder", std::to_string(sortedOrder[i]));
            enumConstantData.set("enumConstantEscapedDescription", escapedDescription.c_str());
            enumConstantsData.push_back(enumConstantData);
        }

        generatorData.set("enumConstants", enumConstantsData);

        return generatorData;
    }

    //-------------------------------------------------------------------------

    void CppGenerateEnum(Generator* generator, std::stringstream &codeFile, std::string const &exportMacro, TypeData const &type, std::string templateStr)
    {
        ENGINE_ASSERT(type.IsFlag(TypeData::Flags::IsEnum));
        // GenerateFile(codeFile, exportMacro, type);

        mustache::data data = GenerateFile(exportMacro, type);
        mustache::mustache tmpl(templateStr);

        codeFile << tmpl.render(data);
    }
}
