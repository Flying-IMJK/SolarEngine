
#include "CodeGenerator_CPP.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    std::string GetNativeTypeNameSpace(const std::vector<std::string>& nameSpaceName, const std::vector<std::string>& structScopes)
    {
        if (!nameSpaceName.empty() && !structScopes.empty())
        {
            return Utils::String::Format("{0}::{1}", Utils::CombineStringList(nameSpaceName, "::"), Utils::CombineStringList(structScopes, "::"));
        }
        else if (!nameSpaceName.empty())
        {
            return Utils::String::Format("{0}", Utils::CombineStringList(nameSpaceName, "::"));
        }
        else if (!structScopes.empty())
        {
            return Utils::String::Format("{0}", Utils::CombineStringList(structScopes, "::"));
        }

        return std::string();
    }

    //-------------------------------------------------------------------------
    // Factory/Serialization Methods
    //-------------------------------------------------------------------------
    
    static mustache::data GenerateCreationMethod(TypeInfoStruct const& type)
    {
        mustache::data generateData;

        if (!type.isAbstract)
        {
            mustache::data notAbstractData;
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);

            notAbstractData.set("namespace",  std::string(namespaceName.c_str()));
            notAbstractData.set("typeName",  type.name.c_str());
            
            generateData.set("IsNotAbstract", notAbstractData);
        }

        return generateData;
    }       

    //-------------------------------------------------------------------------
    // Array Methods
    //-------------------------------------------------------------------------

    static bool GenerateArrayAccessorMethod(TypeInfoStruct const& type, mustache::data& generateData)
    {
        auto const reflectedFields = type.GetReflectedFields();
        if (type.HasArrayProperties() && !reflectedFields.empty())
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;

            for (auto const* pField : reflectedFields)
            {
                if (!pField->IsStaticArray())
                {
                    continue;
                }

                mustache::data propertyDescData;
                propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");

                mustache::data propertyDescStaticArrayData;
                propertyDescStaticArrayData.set("propertyID", std::to_string(pField->GetPropertyID()));
                propertyDescStaticArrayData.set("propertyDescName", pField->name.c_str());
                propertyDescStaticArrayData.set("arraySize", pField->type.arraySize);
                propertyDescData.set("StaticArray", propertyDescStaticArrayData);

                propertyDescData.set("isDevOnlyEnd", "#endif");

                propertyDescDataList.push_back(propertyDescData);
            }

            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }
        return false;
    }

    static mustache::data GenerateArrayElementSizeMethod(TypeInfoStruct const& type)
    {
        mustache::data propertyDescDataList = mustache::data::type::list;
        for (auto const* pField : type.GetReflectedFields())
        {
            if (pField->IsStaticArray())
            {
                mustache::data propertyDescData;
                propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                propertyDescData.set("propertyID", std::to_string(pField->GetPropertyID()));
                propertyDescData.set("propertyTypeName", pField->type.ToString(false).c_str());
                propertyDescData.set("templateSpecializationString", "");
                propertyDescData.set("isDevOnlyEnd", "#endif");

                propertyDescDataList.push_back(propertyDescData);
            }
        }

        return propertyDescDataList;
    }

    //-------------------------------------------------------------------------
    // Default Value Methods
    //-------------------------------------------------------------------------

    static bool GenerateAreAllPropertiesEqualMethod(TypeInfoStruct const& type, mustache::data generateData)
    {
        auto const reflectedFields = type.GetReflectedFields();
        if (!reflectedFields.empty())
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for (auto const* pField : reflectedFields)
            {
                mustache::data propertyDescData;
                propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                propertyDescData.set("propertyID", std::to_string(pField->GetPropertyID()));
                propertyDescData.set("isDevOnlyEnd", "#endif");
                propertyDescDataList.push_back(propertyDescData);
            }
            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }

        return false;
    }

    static bool GenerateIsPropertyEqualMethod(TypeInfoStruct const& type, mustache::data& generateData)
    {
        auto const reflectedFields = type.GetReflectedFields();
        if (!reflectedFields.empty())
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for (auto const* pField : reflectedFields)
            {
                mustache::data propertyDescData;
                propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                propertyDescData.set("propertyID", std::to_string(pField->GetPropertyID()));
                propertyDescData.set("structureProperty", false);
                propertyDescData.set("propertyDescName", pField->name.c_str());
                propertyDescData.set("propertyDescTypeName", pField->type.ToString(false).c_str());

                // Arrays
                if (pField->IsStaticArray())
                {
                    mustache::data arrayPropertyData;
                    arrayPropertyData.set("propertyDescName", pField->name.c_str());
                    arrayPropertyData.set("propertyDescTypeName", pField->type.ToString(false).c_str());
                    arrayPropertyData.set("dynamicArrayProperty", false);
                    arrayPropertyData.set("propertyDescArraySize", std::to_string(pField->type.arraySize));

                    propertyDescData.set("arrayProperty", arrayPropertyData);
                }

                propertyDescData.set("isDevOnlyEnd", "#endif");

                propertyDescDataList.push_back(propertyDescData);
            }
        
            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }


        return false;
    }
    
    static bool GenerateSetToDefaultValueMethod(TypeInfoStruct const& type, mustache::data& generateData)
    {
        auto const reflectedFields = type.GetReflectedFields();
        if (!reflectedFields.empty())
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for (auto const* pField : reflectedFields)
            {
                mustache::data propertyDescData;
                propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                propertyDescData.set("propertyID", std::to_string(pField->GetPropertyID()));

                if (pField->IsStaticArray())
                {
                    mustache::data staticArrayDataList = mustache::data::type::list;;
                    for (int32 i = 0; i < pField->type.arraySize; i++)
                    {
                        mustache::data staticArrayData;
                        staticArrayData.set("propertyDescName", pField->name.c_str());
                        staticArrayData.set("staticArrayIndex", std::to_string(i));
                        staticArrayDataList.push_back(staticArrayData);
                    }
                    propertyDescData.set("staticArrayProperty", staticArrayDataList);
                }
                else
                {
                    propertyDescData.set("propertyDescName", pField->name.c_str());
                }

                propertyDescData.set("isDevOnlyEnd", "#endif");

                propertyDescDataList.push_back(propertyDescData);
            }

            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------
    // Type Registration Methods
    //-------------------------------------------------------------------------
    static mustache::data GenerateTypeInfoConstructor(TypeInfoStruct const& type, TypeInfoStruct const& parentType)
    {
        mustache::data generateData;

        auto generateFieldRegistrationCode = [&type](TypeInfoField const& field, mustache::data& propertiesData)
        {
            std::string const fieldTypeName = field.type.ToString(false);

            // Reflected fields are emitted only for development builds today.
            propertiesData.set("isDevOnlyBeginFlag", "#ifdef SGE_DEVELOPMENT");
            propertiesData.set("propertieName", field.name.c_str());
            propertiesData.set("propertieTypename", fieldTypeName.c_str());
            propertiesData.set("parentTypeID", std::to_string(type.typeID));
            propertiesData.set("propertieTemplateArgTypeName", "");
            propertiesData.set("propertieFriendlyName", field.GetFriendlyName().c_str());
            propertiesData.set("propertieCategory", "");

            std::string escapedDescription = field.comment;
            Utils::String::ReplaceAll(escapedDescription, "\"", "\\\"");
            propertiesData.set("propertieEscapedDescription", escapedDescription.c_str());
            propertiesData.set("propertieIsDevOnly", "true");
            propertiesData.set("propertieIsToolsReadOnly", "false");
            propertiesData.set("propertieShowInRestrictedMode", "false");

            // Abstract types cannot have default values since they cannot be instantiated
            if (!type.isAbstract)
            {
                mustache::data isNotAbstractData;

                std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

                isNotAbstractData.set("propertieName", field.name.c_str());
                isNotAbstractData.set("namespace", std::string(namespaceName.c_str()));
                isNotAbstractData.set("typeName", type.name.c_str());
                if (field.IsStaticArray())
                {
                    mustache::data staticArrayData;
                    staticArrayData.set("propertieName", field.name.c_str());
                    staticArrayData.set("staticArraySize", std::to_string(field.type.arraySize));
                    staticArrayData.set("propertieTypeName", fieldTypeName.c_str());
                    staticArrayData.set("templateSpecializationString", "");

                    isNotAbstractData.set("propertieStaticArray", staticArrayData);
                }
                else
                {
                    mustache::data notArrayData;
                    notArrayData.set("propertieTypeName", fieldTypeName.c_str());
                    notArrayData.set("templateSpecializationString", "");
                    
                    notArrayData.set("propertieNotArray", notArrayData);
                }

                // Emit the runtime enum directly so BuildTool does not duplicate flag values.
                isNotAbstractData.set("propertieFlags", field.IsStaticArray() ? "TypeProperty::IsArray" : "0");

                propertiesData.set("propertieIsNotAbstract", isNotAbstractData);
            }

            propertiesData.set("isDevOnlyEndFlag", "#endif");
        };

        //-------------------------------------------------------------------------

        std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);
        std::string parentTypeNamespace = GetNativeTypeNameSpace(parentType.namespaceScopeList, parentType.structScopeList);

        generateData.set("namespace", std::string(namespaceName.c_str()));
        generateData.set("typeName", type.name.c_str());
        generateData.set("isAbstract", type.isAbstract ? "true":"false");
        generateData.set("category", type.GetCategory().c_str());
        generateData.set("isDevOnly", type.isDevOnly ? "true":"false");
        generateData.set("parentTypeNamespace", std::string(parentTypeNamespace.c_str()));
        generateData.set("parentTypeName", parentType.name.c_str());
        auto const reflectedFields = type.GetReflectedFields();
        generateData.set("hasProperties", !reflectedFields.empty());
        if (!reflectedFields.empty())
        {
            if (!type.isAbstract)
            {
                mustache::data propertiesisAbstractData;
                propertiesisAbstractData.set("namespace", std::string(namespaceName.c_str()));
                propertiesisAbstractData.set("typeName", type.name.c_str());
                generateData.set("isNotAbstract", propertiesisAbstractData);
            }
            
            mustache::data propertieDataList = mustache::data::type::list;
            for (auto const* pField : reflectedFields)
            {
                mustache::data propertieData;
                generateFieldRegistrationCode(*pField, propertieData);

                propertieDataList.push_back(propertieData);

            }
            generateData.set("properties", propertieDataList);
        }
        return generateData;
    }


    //-------------------------------------------------------------------------
    // File generation
    //-------------------------------------------------------------------------

    static mustache::data GenerateTypeInfoFile(TypeDatabase const&   database,
                                               std::string const&        exportMacro,
                                               TypeInfoStruct const&     type,
                                               TypeInfoStruct const&     parentType)
    {
        mustache::data generateTypeData;
        // Dev Flag
        if ( type.isDevOnly )
        {
            generateTypeData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
        }

        std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);

        // Type Info
        //-------------------------------------------------------------------------
        generateTypeData.set("namespace", std::string(namespaceName.c_str()));
        generateTypeData.set("typeName", type.name.c_str());
        generateTypeData.set("exportMacro", exportMacro.c_str());
        generateTypeData.set("typeIDUint", std::to_string(type.typeID));
        if (!type.isAbstract)
        {
            generateTypeData.set("IsNotAbstract", true);
        }

        generateTypeData.set("ConstructorMethod", GenerateTypeInfoConstructor(type, parentType));
        generateTypeData.set("CreationMethod", GenerateCreationMethod(type));
        generateTypeData.set("InPlaceCreationMethod", GenerateCreationMethod(type));

        mustache::data generateArrayMethodData;
        if (GenerateArrayAccessorMethod(type, generateArrayMethodData))
        {
            generateTypeData.set("ArrayElementDataPtrMethod", generateArrayMethodData);
            generateTypeData.set("ArraySizeMethod", generateArrayMethodData);
        }
        

        generateTypeData.set("ArrayElementSizeMethod", GenerateArrayElementSizeMethod(type));

        mustache::data generateAreAllPropertyValuesEqual;
        if (GenerateAreAllPropertiesEqualMethod(type, generateAreAllPropertyValuesEqual))
        {
            generateTypeData.set("AreAllPropertyValuesEqual", generateAreAllPropertyValuesEqual);
        }
        
        mustache::data generateIsPropertyEqualMethod;
        if (GenerateIsPropertyEqualMethod(type, generateIsPropertyEqualMethod))
        {
            generateTypeData.set("IsPropertyEqualMethod", generateIsPropertyEqualMethod);
        }
        
        mustache::data generateSetToDefaultValueMethod;
        if (GenerateSetToDefaultValueMethod(type, generateSetToDefaultValueMethod))
        {
            generateTypeData.set("SetToDefaultValueMethod", generateSetToDefaultValueMethod);
        }

        // Dev Flag
        //-------------------------------------------------------------------------

        if (type.isDevOnly)
        {
            generateTypeData.set("isDevOnlyEnd", "#endif");;
        }

        return generateTypeData;
    }

    //-------------------------------------------------------------------------

    void CppGenerateType(Generator*            generator,
                         TypeDatabase const&   database,
                         std::stringstream&    codeFile,
                         std::string const&    exportMacro,
                         TypeInfoStruct const&     type,
                         TypeInfoStruct const&     parentType,
                         std::string               templateStr)
    {
        //GenerateTypeInfoFile( codeFile, database, exportMacro, type, parentType );
        mustache::data data = GenerateTypeInfoFile(database, exportMacro, type, parentType);
        mustache::mustache tmpl(templateStr);

        codeFile << tmpl.render(data);
    }
}
