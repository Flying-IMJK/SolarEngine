#include "CodeGenerator_CPP_Enum.h"
#include "Core/TypeSystem/TypeID.h"
#include "Core/Types/Collections/Sorting.h"
//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    static mustache::data GenerateFile(String const &exportMacro, ReflectedType const &type)
    {
        mustache::data generatorData;

        List<int32_t> sortingConstantIndices; // Sorted list of constant indices
        List<int32_t> sortedOrder;            // Final order for each constant

        for (auto i = 0u; i < type.enumConstants.Count(); i++)
        {
            sortingConstantIndices.Add(i);
        }

        Function<bool(const int32_t &, const int32_t &)> Comparator = [&type](const int32_t &a, const int32_t &b)
        {
            auto const &elemA = type.enumConstants[a];
            auto const &elemB = type.enumConstants[b];
            return elemA.label < elemB.label;
        };

        Sorting::QuickSort(sortingConstantIndices, Comparator);

        sortedOrder.Resize(sortingConstantIndices.Count());

        for (auto i = 0u; i < type.enumConstants.Count(); i++)
        {
            sortedOrder[sortingConstantIndices[i]] = i;
        }

        if (type.m_isDevOnly)
        {
            generatorData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
            generatorData.set("isDevOnlyEnd", "#endif");
        }

        generatorData.set("namespace", type.namespaceName.Get());
        generatorData.set("typeName", type.name.Get());

        generatorData.set("friendlyName", type.GetFriendlyName().Get());
        generatorData.set("Category", type.GetCategory().Get());

        switch (type.underlyingType)
        {
        case TypeIDCore::Uint8:
                generatorData.set("underlyingType", "TypeIDCore::Uint8");
            break;

        case TypeIDCore::Int8:
                generatorData.set("underlyingType", "TypeIDCore::Int8");
            break;

        case TypeIDCore::Uint16:
                generatorData.set("underlyingType", "TypeIDCore::Uint16");
            break;

        case TypeIDCore::Int16:
                generatorData.set("underlyingType", "TypeIDCore::Int16");
            break;

        case TypeIDCore::Uint32:
                 generatorData.set("underlyingType", "TypeIDCore::Uint32");
            break;

        case TypeIDCore::Int32:
                 generatorData.set("underlyingType", "TypeIDCore::Int32");
            break;

        default:
            ENGINE_UNREACHABLE_CODE();
            break;
        }

        mustache::data enumConstantsData(mustache::data::type::list);
        for (auto i = 0u; i < type.enumConstants.Count(); i++)
        {
            mustache::data enumConstantData;
			StringAnsi escapedDescription = type.enumConstants[i].description;
			escapedDescription.Replace("\"", "\\\"");

            enumConstantData.set("enumConstantLabel", type.enumConstants[i].label.Get());
            enumConstantData.set("enumConstantValue", std::to_string(type.enumConstants[i].value));
            enumConstantData.set("enumConstantSortedOrder", std::to_string(sortedOrder[i]));
            enumConstantData.set("enumConstantEscapedDescription", escapedDescription.Get());
            enumConstantsData.push_back(enumConstantData);
        }

        generatorData.set("enumConstants", enumConstantsData);

        return generatorData;
    }

    //-------------------------------------------------------------------------

    void CppGenerateEnum(Generator* generator, std::stringstream &codeFile, String const &exportMacro, ReflectedType const &type, std::string templateStr)
    {
        ENGINE_ASSERT(type.IsEnum());
        // GenerateFile(codeFile, exportMacro, type);

        mustache::data data = GenerateFile(exportMacro, type);
        mustache::mustache tmpl(templateStr);

        codeFile << tmpl.render(data);
    }
}