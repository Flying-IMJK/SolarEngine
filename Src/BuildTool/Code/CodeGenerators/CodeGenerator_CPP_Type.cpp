
#include "CodeGenerator_CPP.h"
#include <regex>

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
    
    static mustache::data GenerateCreationMethod(TypeData const& type )
    {
        mustache::data generateData;

        if (!type.IsFlag(TypeData::Flags::IsAbstract))
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

    static bool GenerateArrayAccessorMethod(TypeData const& type, mustache::data &generateData)
    {
        if ( type.HasArrayProperties() && type.properties.size() > 0)
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;

            for ( auto& propertyDesc : type.properties )
            {
                mustache::data propertyDescData;

                if (propertyDesc.isDevOnly)
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }
                
                if (propertyDesc.IsDynamicArrayProperty())
                {
                    mustache::data propertyDescDynamicArrayData;
                    propertyDescDynamicArrayData.set("propertyID", std::to_string(propertyDesc.propertyID));
                    propertyDescDynamicArrayData.set("propertyDescName", propertyDesc.name.c_str());
                    propertyDescData.set("DynamicArray", propertyDescDynamicArrayData);
                }
                else if ( propertyDesc.IsStaticArrayProperty() )
                {
                    mustache::data propertyDescStaticArrayData;
                    propertyDescStaticArrayData.set("propertyID", std::to_string(propertyDesc.propertyID));
                    propertyDescStaticArrayData.set("propertyDescName", propertyDesc.name.c_str());
                    propertyDescStaticArrayData.set("arraySize", propertyDesc.GetArraySize());
                    propertyDescData.set("StaticArray", propertyDescStaticArrayData);
                }

                if (propertyDesc.isDevOnly)
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }

                propertyDescDataList.push_back(propertyDescData);
            }

            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }
        return false;
    }

    static mustache::data GenerateArrayElementSizeMethod(TypeData const& type )
    {
        mustache::data propertyDescDataList = mustache::data::type::list;
        for ( auto& propertyDesc : type.properties )
        {
			std::string const templateSpecializationString = propertyDesc.templateArgTypeName.empty() ? std::string() : "<" + propertyDesc.templateArgTypeName + ">";

            if ( propertyDesc.IsArrayProperty() )
            {
                mustache::data propertyDescData;
                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }
                propertyDescData.set("propertyID", std::to_string(propertyDesc.propertyID));
                propertyDescData.set("propertyTypeName", propertyDesc.typeName.c_str());
                propertyDescData.set("templateSpecializationString", templateSpecializationString.c_str());
                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }

                propertyDescDataList.push_back(propertyDescData);
            }
        }

        return propertyDescDataList;
    }

    static bool GenerateArrayElementOperateMethod(TypeData const& type, mustache::data &generateData)
    {
        if (type.HasDynamicArrayProperties() && type.properties.size() > 0)
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for ( auto& propertyDesc : type.properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    mustache::data propertyDescData;

                    if (propertyDesc.isDevOnly)
                    {
                        propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                    }

                    propertyDescData.set("propertyID", std::to_string(propertyDesc.propertyID));
                    propertyDescData.set("propertyDescName", propertyDesc.name.c_str());

                    if ( propertyDesc.isDevOnly )
                    {
                        propertyDescData.set("isDevOnlyEnd", "#endif");
                    }

                    propertyDescDataList.push_back(propertyDescData);
                }
            }

            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------
    // Default Value Methods
    //-------------------------------------------------------------------------

    static bool GenerateAreAllPropertiesEqualMethod(TypeData const& type, mustache::data generateData)
    {
        if (type.HasProperties() && type.properties.size() > 0)
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for ( auto& propertyDesc : type.properties )
            {
                mustache::data propertyDescData;
                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }

                propertyDescData.set("propertyID", std::to_string(propertyDesc.propertyID));

                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }
                propertyDescDataList.push_back(propertyDescData);
            }
            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }

        return false;
    }

    static bool GenerateIsPropertyEqualMethod(TypeData const& type, mustache::data &generateData)
    {

        if ( type.HasProperties() && type.properties.size() > 0)
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for ( auto& propertyDesc : type.properties )
            {
				std::string propertyTypeName = propertyDesc.typeName.c_str();
                if ( !propertyDesc.templateArgTypeName.empty() )
                {
                    propertyTypeName += "<";
                    propertyTypeName += propertyDesc.templateArgTypeName.c_str();
                    propertyTypeName += ">";
                }

                mustache::data propertyDescData;
                //-------------------------------------------------------------------------

                if (propertyDesc.isDevOnly)
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }

                propertyDescData.set("propertyID", std::to_string(propertyDesc.propertyID));
                propertyDescData.set("structureProperty", propertyDesc.IsStructureProperty());
                propertyDescData.set("propertyDescName", propertyDesc.name.c_str());
                propertyDescData.set("propertyDescTypeName", propertyDesc.typeName.c_str());

                // Arrays
                if (propertyDesc.IsArrayProperty())
                {
                    mustache::data arrayPropertyData;
                    // Handle individual element comparison
                    //-------------------------------------------------------------------------

                    arrayPropertyData.set("propertyDescName", propertyDesc.name.c_str());
                    arrayPropertyData.set("propertyDescTypeName", propertyDesc.typeName.c_str());
                    

                    // If it's a dynamic array check the sizes first
                    arrayPropertyData.set("dynamicArrayProperty", propertyDesc.IsDynamicArrayProperty());
                    // if ( propertyDesc.IsDynamicArrayProperty() )
                    // {
                    //     mustache::data dynamicArrayPropertyData;
                    //     arrayPropertyData.set("dynamicArrayProperty", dynamicArrayPropertyData);
                    // }


                    // Handle array comparison
                    //-------------------------------------------------------------------------
                    // If it's a dynamic array check the sizes first
                    if (propertyDesc.IsDynamicArrayProperty())
                    {

                    }
                    else
                    {
                        arrayPropertyData.set("propertyDescArraySize", propertyDesc.typeName.c_str());
                    }

                    propertyDescData.set("arrayProperty", arrayPropertyData);
                }

                if (propertyDesc.isDevOnly)
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }

                propertyDescDataList.push_back(propertyDescData);
            }
        
            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }


        return false;
    }
    
    static bool GenerateSetToDefaultValueMethod(TypeData const& type, mustache::data &generateData)
    {
        if ( type.HasProperties() && type.properties.size() > 0)
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for ( auto& propertyDesc : type.properties )
            {
                mustache::data propertyDescData;
                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }

                propertyDescData.set("propertyID", std::to_string(propertyDesc.propertyID));


                if ( propertyDesc.IsStaticArrayProperty() )
                {
                    mustache::data staticArrayDataList = mustache::data::type::list;;
                    for ( auto i = 0u; i < propertyDesc.GetArraySize(); i++ )
                    {
                        mustache::data staticArrayData;
                        staticArrayData.set("propertyDescName", propertyDesc.name.c_str());
                        staticArrayData.set("staticArrayIndex", std::to_string(i));
                        staticArrayDataList.push_back(staticArrayData);
                    }
                    propertyDescData.set("staticArrayProperty", staticArrayDataList);
                }
                else
                {
                    propertyDescData.set("propertyDescName", propertyDesc.name.c_str());
                }

                if (propertyDesc.isDevOnly)
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }

                propertyDescDataList.push_back(propertyDescData);
            }

            generateData.set("propertyDesc", propertyDescDataList);
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------
    // Resource Methods
    //-------------------------------------------------------------------------
    static bool GenerateResourcesMethod(TypeData const& type, mustache::data &generateData)
    {
        if (type.HasResourcePtrOrStructProperties() && type.properties.size() > 0)
        {
            std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

            generateData.set("namespace", std::string(namespaceName.c_str()));
            generateData.set("typeName", type.name.c_str());

            mustache::data propertyDescDataList = mustache::data::type::list;
/*            for ( auto& propertyDesc : type.m_properties)
            {
                mustache::data propertyDescData;

                if ( propertyDesc.m_isDevOnly )
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }

                if ( propertyDesc.m_typeID == TypeIDCore::TResourcePtr || propertyDesc.m_typeID == TypeIDCore::ResourcePtr )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            mustache::data resourceDynamicArrayData;
                            resourceDynamicArrayData.set("propertyDescName", propertyDesc.m_name.c_str());
                            propertyDescData.set("ResourceDynamicArray", resourceDynamicArrayData);
                        }
                        else // Static array
                        {
                            mustache::data resourceStaticArrayData = mustache::data::type::list;
                            
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                mustache::data temp;
                                temp.set("propertyDescName", propertyDesc.m_name.c_str());
                                temp.set("staticArrayIndex", std::to_string(i));
                                resourceStaticArrayData.push_back(temp);
                            }

                            propertyDescData.set("ResourceStaticArray", resourceStaticArrayData);
                        }
                    }
                    else
                    {
                        mustache::data resourceNotArrry;
                        resourceNotArrry.set("propertyDescName", propertyDesc.m_name.c_str());
                        propertyDescData.set("ResourceNotArrry", resourceNotArrry);
                    }
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            mustache::data othearDynamicArrayData;
                            othearDynamicArrayData.set("propertyDescName", propertyDesc.m_name.c_str());
                            othearDynamicArrayData.set("propertyDescTypeName", propertyDesc.m_typeName.c_str());
                            
                            propertyDescData.set("OtherDynamicArray", othearDynamicArrayData);
                        }
                        else // Static array
                        {
                            mustache::data otherStaticArrayData = mustache::data::type::list;
                            
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                mustache::data temp;
                                temp.set("propertyDescTypeName", propertyDesc.m_typeName.c_str());
                                temp.set("propertyDescName", propertyDesc.m_name.c_str());
                                temp.set("staticArrayIndex", std::to_string(i));
                                otherStaticArrayData.push_back(temp);
                            }

                            propertyDescData.set("OtherStaticArray", otherStaticArrayData);
                        }
                    }
                    else
                    {
                        mustache::data othearNotArrry;
                        othearNotArrry.set("propertyDescName", propertyDesc.m_name.c_str());
                        othearNotArrry.set("propertyDescTypeName", propertyDesc.m_typeName.c_str());
                        propertyDescData.set("OtherNotArrry", othearNotArrry);
                    }
                }

                if ( propertyDesc.m_isDevOnly )
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }

                propertyDescDataList.push_back(propertyDescData);
            }
            generateData.set("propertyDesc", propertyDescDataList);*/
            return true;
        }

        return false;
    }

    static mustache::data GenerateExpectedResourceTypeMethod(TypeData const& type )
    {
        mustache::data generateData;

        if (type.HasResourcePtrProperties())
        {
            mustache::data propertyDescDataList = mustache::data::type::list;
/*
            for ( auto& propertyDesc : type.m_properties )
            {
                bool const isResourceProp = ( propertyDesc.m_typeID == TypeIDCore::ResourcePtr ) || ( propertyDesc.m_typeID == TypeIDCore::TResourcePtr );
                if ( isResourceProp )
                {
                    mustache::data propertyDescData;
                    if ( propertyDesc.m_isDevOnly )
                    {
                        propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                    }

                    if ( propertyDesc.m_typeID == TypeIDCore::TResourcePtr )
                    {
                        mustache::data tResourcePtrData;
                        tResourcePtrData.set("propertyID", std::to_string(propertyDesc.m_propertyID.ToUint()));
                        tResourcePtrData.set("templateArgTypeName", propertyDesc.m_templateArgTypeName.c_str());
                        propertyDescData.set("TResourcePtr", tResourcePtrData);

                    }
                    else if ( propertyDesc.m_typeID == TypeIDCore::ResourcePtr )
                    {
                        mustache::data tResourcePtrData;
                        tResourcePtrData.set("propertyID", propertyDesc.m_propertyID.c_str());
                        propertyDescData.set("ResourcePtr", tResourcePtrData);
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        propertyDescData.set("isDevOnlyBegin", "#endif");
                    }

                    propertyDescDataList.push_back(propertyDescData);
                }
            }*/
            generateData.set("propertyDesc", propertyDescDataList); 
        }

        return generateData;
    }
    
    //-------------------------------------------------------------------------
    // Type Registration Methods
    //-------------------------------------------------------------------------
    static mustache::data GenerateTypeInfoConstructor(TypeData const& type, TypeData const& parentType )
    {
        mustache::data generateData;

        // The pass by value here is intentional!
        auto GeneratePropertyRegistrationCode = [type] (PropertyData prop, mustache::data &propertiesData)
        {
		  	std::string templateSpecializationString;
			if (prop.templateArgTypeName.empty())
			{
				templateSpecializationString = prop.templateArgTypeName;
			}
			else
			{
				if (Utils::String::StartsWith(prop.templateArgTypeName, "SE"))
				{
					templateSpecializationString = "<::";
				}
				else
				{
					templateSpecializationString = "<";
				}
				templateSpecializationString += prop.templateArgTypeName;
				templateSpecializationString += ">";
			}

            if ( prop.isDevOnly )
            {
                propertiesData.set("isDevOnlyBeginFlag", "#ifdef SGE_DEVELOPMENT");
            }


            propertiesData.set("propertieName", prop.name.c_str());
            propertiesData.set("propertieTypename", prop.typeName.c_str());
            propertiesData.set("parentTypeID", std::to_string(type.typeID));
            propertiesData.set("propertieTemplateArgTypeName", prop.templateArgTypeName.c_str());

            if (prop.HasMetaData())
            {
                mustache::data metaList = mustache::data::type::list;
                std::string metaContext = std::string(prop.metaData.c_str());

                // 正则表达式匹配 xxx(xxx) 格式，使用非贪婪匹配以支持多个元数据
                // 匹配模式：非空白字符组 + ( + 非贪婪任意字符 + )
                std::regex metaPattern(R"((\S+)\((.*?)\))");

                // 使用迭代器遍历所有匹配
                std::sregex_iterator begin(metaContext.begin(), metaContext.end(), metaPattern);
                std::sregex_iterator end;

                for (std::sregex_iterator it = begin; it != end; ++it)
                {
                    std::smatch match = *it;
                    mustache::data metaItem;
                    metaItem.set("metaType", match[1].str());
                    std::string content = match[2].str();
                    content.insert(0, "[");
                    content.push_back(']');
                    metaItem.set("metaContext", content);

                    metaList.push_back(metaItem);
                }

                propertiesData.set("metaContents", metaList);
            }

            propertiesData.set("propertieFriendlyName", prop.GetFriendlyName().c_str());
            propertiesData.set("propertieCategory", std::string(prop.GetCategory()));
		  	std::string escapedDescription = prop.description;
		  	Utils::String::ReplaceAll(escapedDescription, "\"", "\\\"");
            propertiesData.set("propertieEscapedDescription", escapedDescription.c_str());
            propertiesData.set("propertieIsDevOnly", prop.isDevOnly ? "true" : "false");
            propertiesData.set("propertieIsToolsReadOnly", prop.isToolsReadOnly ? "true" : "false");
            propertiesData.set("propertieShowInRestrictedMode", prop.showInRestrictedMode ? "true" : "false");


            // Abstract types cannot have default values since they cannot be instantiated
            if (!type.IsFlag(TypeData::Flags::IsAbstract))
            {
                mustache::data isNotAbstractData;

                std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);;

                isNotAbstractData.set("propertieName", prop.name.c_str());
                isNotAbstractData.set("namespace", std::string(namespaceName.c_str()));
                isNotAbstractData.set("typeName", type.name.c_str());
                if ( prop.IsDynamicArrayProperty() )
                {
                    mustache::data dynamicArrayData;
                    dynamicArrayData.set("propertieName", prop.name.c_str());
                    dynamicArrayData.set("propertieTypeName", prop.typeName.c_str());
                    dynamicArrayData.set("templateSpecializationString", templateSpecializationString.c_str());

                    isNotAbstractData.set("propertieDynamicArray", dynamicArrayData);
                }
                else if(prop.IsStaticArrayProperty())
                {
                    mustache::data staticArrayData;
                    staticArrayData.set("propertieName", prop.name.c_str());
                    staticArrayData.set("staticArraySize", std::to_string(prop.GetArraySize()));
                    staticArrayData.set("propertieTypeName", prop.typeName.c_str());
                    staticArrayData.set("templateSpecializationString", templateSpecializationString.c_str());

                    isNotAbstractData.set("propertieStaticArray", staticArrayData);
                }
                else
                {
                    mustache::data notArrayData;
                    notArrayData.set("propertieTypeName", prop.typeName.c_str());
                    notArrayData.set("templateSpecializationString", templateSpecializationString.c_str());
                    
                    notArrayData.set("propertieNotArray", notArrayData);
                }

                isNotAbstractData.set("propertieFlags", std::to_string((prop.flags.Get())));

                propertiesData.set("propertieIsNotAbstract", isNotAbstractData);
            }

            if (prop.isDevOnly)
            {
                propertiesData.set("isDevOnlyEndFlag", "#endif");
            }

        };

        //-------------------------------------------------------------------------

        std::string namespaceName = GetNativeTypeNameSpace(type.namespaceScopeList, type.structScopeList);
        std::string parentTypeNamespace = GetNativeTypeNameSpace(parentType.namespaceScopeList, parentType.structScopeList);

        generateData.set("namespace", std::string(namespaceName.c_str()));
        generateData.set("typeName", type.name.c_str());
        generateData.set("isAbstract", type.IsFlag(TypeData::Flags::IsAbstract) ? "true":"false");
        generateData.set("category", type.GetCategory().c_str());
        generateData.set("isDevOnly", type.isDevOnly ? "true":"false");
        generateData.set("parentTypeNamespace", std::string(parentTypeNamespace.c_str()));
        generateData.set("parentTypeName", parentType.name.c_str());
        generateData.set("hasProperties", type.HasProperties());
        if (type.HasProperties())
        {
            if (!type.IsFlag(TypeData::Flags::IsAbstract))
            {
                mustache::data propertiesisAbstractData;
                propertiesisAbstractData.set("namespace", std::string(namespaceName.c_str()));
                propertiesisAbstractData.set("typeName", type.name.c_str());
                generateData.set("isNotAbstract", propertiesisAbstractData);
            }
            
            mustache::data propertieDataList = mustache::data::type::list;
            for ( auto& prop : type.properties )
            {
                mustache::data propertieData;
                GeneratePropertyRegistrationCode(prop, propertieData);

                propertieDataList.push_back(propertieData);

            }
            generateData.set("properties", propertieDataList);
        }
        return generateData;
    }


    //-------------------------------------------------------------------------
    // File generation
    //-------------------------------------------------------------------------

    static mustache::data GenerateTypeInfoFile(ReflectionDatabase const& database, std::string const& exportMacro, TypeData const& type, TypeData const& parentType )
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
        if (!type.IsFlag(TypeData::Flags::IsAbstract))
        {
            generateTypeData.set("IsNotAbstract", true);
        }

        generateTypeData.set("ConstructorMethod", GenerateTypeInfoConstructor(type, parentType));
        generateTypeData.set("CreationMethod", GenerateCreationMethod(type));
        generateTypeData.set("InPlaceCreationMethod", GenerateCreationMethod(type));

        mustache::data generateResourcesMethodData;
        if (GenerateResourcesMethod(type, generateResourcesMethodData))
        {
            generateTypeData.set("LoadResourcesMethod", generateResourcesMethodData);
            generateTypeData.set("UnloadResourcesMethod", generateResourcesMethodData);
            generateTypeData.set("ResourceLoadingStatusMethod", generateResourcesMethodData);
            generateTypeData.set("ResourceUnloadingStatusMethod", generateResourcesMethodData);
            generateTypeData.set("ReferencedResourcesMethod", generateResourcesMethodData);
        }


        generateTypeData.set("ExpectedResourceTypeForPropertyMethod", GenerateExpectedResourceTypeMethod(type));

        mustache::data generateArrayMethodData;
        if (GenerateArrayAccessorMethod(type, generateArrayMethodData))
        {
            generateTypeData.set("ArrayElementDataPtrMethod", generateArrayMethodData);
            generateTypeData.set("ArraySizeMethod", generateArrayMethodData);
        }
        

        generateTypeData.set("ArrayElementSizeMethod", GenerateArrayElementSizeMethod(type));

        mustache::data generateArrayElementOperateMethodData;
        if (GenerateArrayElementOperateMethod(type, generateArrayElementOperateMethodData))
        {
            generateTypeData.set("ArrayClearMethod", generateArrayElementOperateMethodData);
            generateTypeData.set("AddArrayElementMethod", generateArrayElementOperateMethodData);
            generateTypeData.set("InsertArrayElementMethod", generateArrayElementOperateMethodData);
            generateTypeData.set("MoveArrayElementMethod", generateArrayElementOperateMethodData);
            generateTypeData.set("RemoveArrayElementMethod", generateArrayElementOperateMethodData);
        }
        

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

    void CppGenerateType(Generator* generator,  ReflectionDatabase const& database, std::stringstream& codeFile, std::string const& exportMacro,
            TypeData const& type, TypeData const& parentType, std::string templateStr)
    {
        //GenerateTypeInfoFile( codeFile, database, exportMacro, type, parentType );
        mustache::data data = GenerateTypeInfoFile(database, exportMacro, type, parentType);
        mustache::mustache tmpl(templateStr);

        codeFile << tmpl.render(data);
    }
}
