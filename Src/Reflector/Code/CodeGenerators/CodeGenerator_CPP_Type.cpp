#include "CodeGenerator_CPP_Type.h"
#include "Core/TypeSystem/TypeID.h"
#include "Core/TypeSystem/Types.h"
#include <regex>

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    //-------------------------------------------------------------------------
    // Factory/Serialization Methods
    //-------------------------------------------------------------------------
    
    static mustache::data GenerateCreationMethod(ReflectedType const& type )
    {
        mustache::data generateData;

        if (!type.IsAbstract())
        {
            mustache::data notAbstractData;
            notAbstractData.set("namespace",  type.namespaceName.Get());
            notAbstractData.set("typeName",  type.name.Get());
            
            generateData.set("IsNotAbstract", notAbstractData);
        }

        return generateData;
    }       

    //-------------------------------------------------------------------------
    // Array Methods
    //-------------------------------------------------------------------------

    static bool GenerateArrayAccessorMethod(ReflectedType const& type, mustache::data &generateData)
    {
        if ( type.HasArrayProperties() && type.properties.Count() > 0)
        {
            generateData.set("namespace", type.namespaceName.Get());
            generateData.set("typeName", type.name.Get());

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
                    propertyDescDynamicArrayData.set("propertyDescName", propertyDesc.name.Get());
                    propertyDescData.set("DynamicArray", propertyDescDynamicArrayData);
                }
                else if ( propertyDesc.IsStaticArrayProperty() )
                {
                    mustache::data propertyDescStaticArrayData;
                    propertyDescStaticArrayData.set("propertyID", std::to_string(propertyDesc.propertyID));
                    propertyDescStaticArrayData.set("propertyDescName", propertyDesc.name.Get());
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

    static mustache::data GenerateArrayElementSizeMethod(ReflectedType const& type )
    {
        mustache::data propertyDescDataList = mustache::data::type::list;
        for ( auto& propertyDesc : type.properties )
        {
			StringAnsi const templateSpecializationString = propertyDesc.templateArgTypeName.IsEmpty() ? StringAnsi() : "<" + propertyDesc.templateArgTypeName + ">";

            if ( propertyDesc.IsArrayProperty() )
            {
                mustache::data propertyDescData;
                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                }
                propertyDescData.set("propertyID", std::to_string(propertyDesc.propertyID));
                propertyDescData.set("propertyTypeName", propertyDesc.typeName.Get());
                propertyDescData.set("templateSpecializationString", templateSpecializationString.Get());
                if ( propertyDesc.isDevOnly )
                {
                    propertyDescData.set("isDevOnlyEnd", "#endif");
                }

                propertyDescDataList.push_back(propertyDescData);
            }
        }

        return propertyDescDataList;
    }

    static bool GenerateArrayElementOperateMethod(ReflectedType const& type, mustache::data &generateData)
    {
        if (type.HasDynamicArrayProperties() && type.properties.Count() > 0)
        {
            generateData.set("namespace", type.namespaceName.Get());
            generateData.set("typeName", type.name.Get());

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
                    propertyDescData.set("propertyDescName", propertyDesc.name.Get());

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

    static bool GenerateAreAllPropertiesEqualMethod(ReflectedType const& type, mustache::data generateData)
    {
        if (type.HasProperties() && type.properties.Count() > 0)
        {
            generateData.set("namespace", type.namespaceName.Get());
            generateData.set("typeName", type.name.Get());

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

    static bool GenerateIsPropertyEqualMethod(ReflectedType const& type, mustache::data &generateData)
    {

        if ( type.HasProperties() && type.properties.Count() > 0)
        {
            generateData.set("namespace", type.namespaceName.Get());
            generateData.set("typeName", type.name.Get());

            mustache::data propertyDescDataList = mustache::data::type::list;
            for ( auto& propertyDesc : type.properties )
            {
				StringAnsi propertyTypeName = propertyDesc.typeName.Get();
                if ( !propertyDesc.templateArgTypeName.IsEmpty() )
                {
                    propertyTypeName += "<";
                    propertyTypeName += propertyDesc.templateArgTypeName.Get();
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
                propertyDescData.set("propertyDescName", propertyDesc.name.Get());
                propertyDescData.set("propertyDescTypeName", propertyDesc.typeName.Get());

                // Arrays
                if (propertyDesc.IsArrayProperty())
                {
                    mustache::data arrayPropertyData;
                    // Handle individual element comparison
                    //-------------------------------------------------------------------------

                    arrayPropertyData.set("propertyDescName", propertyDesc.name.Get());
                    arrayPropertyData.set("propertyDescTypeName", propertyDesc.typeName.Get());
                    

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
                        arrayPropertyData.set("propertyDescArraySize", propertyDesc.typeName.Get());
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
    
    static bool GenerateSetToDefaultValueMethod(ReflectedType const& type, mustache::data &generateData)
    {
        if ( type.HasProperties() && type.properties.Count() > 0)
        {
            generateData.set("namespace", type.namespaceName.Get());
            generateData.set("typeName", type.name.Get());

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
                        staticArrayData.set("propertyDescName", propertyDesc.name.Get());
                        staticArrayData.set("staticArrayIndex", std::to_string(i));
                        staticArrayDataList.push_back(staticArrayData);
                    }
                    propertyDescData.set("staticArrayProperty", staticArrayDataList);
                }
                else
                {
                    propertyDescData.set("propertyDescName", propertyDesc.name.Get());
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
    static bool GenerateResourcesMethod(ReflectedType const& type, mustache::data &generateData)
    {
        if (type.HasResourcePtrOrStructProperties() && type.properties.Count() > 0)
        {
            generateData.set("namespace", type.namespaceName.Get());
            generateData.set("typeName", type.name.Get());

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
                            resourceDynamicArrayData.set("propertyDescName", propertyDesc.m_name.Get());
                            propertyDescData.set("ResourceDynamicArray", resourceDynamicArrayData);
                        }
                        else // Static array
                        {
                            mustache::data resourceStaticArrayData = mustache::data::type::list;
                            
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                mustache::data temp;
                                temp.set("propertyDescName", propertyDesc.m_name.Get());
                                temp.set("staticArrayIndex", std::to_string(i));
                                resourceStaticArrayData.push_back(temp);
                            }

                            propertyDescData.set("ResourceStaticArray", resourceStaticArrayData);
                        }
                    }
                    else
                    {
                        mustache::data resourceNotArrry;
                        resourceNotArrry.set("propertyDescName", propertyDesc.m_name.Get());
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
                            othearDynamicArrayData.set("propertyDescName", propertyDesc.m_name.Get());
                            othearDynamicArrayData.set("propertyDescTypeName", propertyDesc.m_typeName.Get());
                            
                            propertyDescData.set("OtherDynamicArray", othearDynamicArrayData);
                        }
                        else // Static array
                        {
                            mustache::data otherStaticArrayData = mustache::data::type::list;
                            
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                mustache::data temp;
                                temp.set("propertyDescTypeName", propertyDesc.m_typeName.Get());
                                temp.set("propertyDescName", propertyDesc.m_name.Get());
                                temp.set("staticArrayIndex", std::to_string(i));
                                otherStaticArrayData.push_back(temp);
                            }

                            propertyDescData.set("OtherStaticArray", otherStaticArrayData);
                        }
                    }
                    else
                    {
                        mustache::data othearNotArrry;
                        othearNotArrry.set("propertyDescName", propertyDesc.m_name.Get());
                        othearNotArrry.set("propertyDescTypeName", propertyDesc.m_typeName.Get());
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

    static mustache::data GenerateExpectedResourceTypeMethod(ReflectedType const& type )
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
                        tResourcePtrData.set("templateArgTypeName", propertyDesc.m_templateArgTypeName.Get());
                        propertyDescData.set("TResourcePtr", tResourcePtrData);

                    }
                    else if ( propertyDesc.m_typeID == TypeIDCore::ResourcePtr )
                    {
                        mustache::data tResourcePtrData;
                        tResourcePtrData.set("propertyID", propertyDesc.m_propertyID.ToString().Get());
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
    static mustache::data GenerateTypeInfoConstructor(ReflectedType const& type, ReflectedType const& parentType )
    {
        mustache::data generateData;

        // The pass by value here is intentional!
        auto GeneratePropertyRegistrationCode = [type] (ReflectedProperty prop, mustache::data &propertiesData)
        {
		  	StringAnsi templateSpecializationString;
			if (prop.templateArgTypeName.IsEmpty())
			{
				templateSpecializationString = prop.templateArgTypeName;
			}
			else
			{
				if (prop.templateArgTypeName.StartsWith("SE"))
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


            propertiesData.set("propertieName", prop.name.Get());
            propertiesData.set("propertieTypename", prop.typeName.Get());
            propertiesData.set("parentTypeID", std::to_string(type.typeID));
            propertiesData.set("propertieTemplateArgTypeName", prop.templateArgTypeName.Get());

            if (prop.HasMetaData())
            {
                mustache::data metaList = mustache::data::type::list;
                std::string metaContext = std::string(prop.metaData.Get());

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

            propertiesData.set("propertieFriendlyName", prop.GetFriendlyName().Get());
            propertiesData.set("propertieCategory", prop.GetCategory().Get());
		  	StringAnsi escapedDescription = prop.description;
		  	escapedDescription.Replace("\"", "\\\"");
            propertiesData.set("propertieEscapedDescription", escapedDescription.Get());
            propertiesData.set("propertieIsDevOnly", prop.isDevOnly ? "true" : "false");
            propertiesData.set("propertieIsToolsReadOnly", prop.isToolsReadOnly ? "true" : "false");
            propertiesData.set("propertieShowInRestrictedMode", prop.showInRestrictedMode ? "true" : "false");


            // Abstract types cannot have default values since they cannot be instantiated
            if (!type.IsAbstract())
            {
                mustache::data isNotAbstractData;

                isNotAbstractData.set("propertieName", prop.name.Get());
                isNotAbstractData.set("namespace", type.namespaceName.Get());
                isNotAbstractData.set("typeName", type.name.Get());
                if ( prop.IsDynamicArrayProperty() )
                {
                    mustache::data dynamicArrayData;
                    dynamicArrayData.set("propertieName", prop.name.Get());
                    dynamicArrayData.set("propertieTypeName", prop.typeName.Get());
                    dynamicArrayData.set("templateSpecializationString", templateSpecializationString.Get());

                    isNotAbstractData.set("propertieDynamicArray", dynamicArrayData);
                }
                else if(prop.IsStaticArrayProperty())
                {
                    mustache::data staticArrayData;
                    staticArrayData.set("propertieName", prop.name.Get());
                    staticArrayData.set("staticArraySize", std::to_string(prop.GetArraySize()));
                    staticArrayData.set("propertieTypeName", prop.typeName.Get());
                    staticArrayData.set("templateSpecializationString", templateSpecializationString.Get());

                    isNotAbstractData.set("propertieStaticArray", staticArrayData);
                }
                else
                {
                    mustache::data notArrayData;
                    notArrayData.set("propertieTypeName", prop.typeName.Get());
                    notArrayData.set("templateSpecializationString", templateSpecializationString.Get());
                    
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
        generateData.set("namespace", type.namespaceName.Get());
        generateData.set("typeName", type.name.Get());
        generateData.set("isAbstract", type.IsAbstract() ? "true":"false");
        generateData.set("category", type.GetCategory().Get());
        generateData.set("isDevOnly", type.m_isDevOnly ? "true":"false");
        generateData.set("parentTypeNamespace", parentType.namespaceName.Get());
        generateData.set("parentTypeName", parentType.name.Get());
        generateData.set("hasProperties", type.HasProperties());
        if (type.HasProperties())
        {
            if (!type.IsAbstract())
            {
                mustache::data propertiesisAbstractData;
                propertiesisAbstractData.set("namespace", type.namespaceName.Get());
                propertiesisAbstractData.set("typeName", type.name.Get());
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

    static mustache::data GenerateTypeInfoFile(ReflectionDatabase const& database, String const& exportMacro, ReflectedType const& type, ReflectedType const& parentType )
    {
        mustache::data generateTypeData;
        // Dev Flag
        if ( type.m_isDevOnly )
        {
            generateTypeData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
        }

        // Type Info
        //-------------------------------------------------------------------------
        generateTypeData.set("namespace", type.namespaceName.Get());
        generateTypeData.set("typeName", type.name.Get());
        generateTypeData.set("exportMacro", exportMacro.Get());
        generateTypeData.set("typeIDUint", std::to_string((type.typeID)));
        if (!type.IsAbstract())
        {
            generateTypeData.set("IsNotAbstract", true);
        }

        /*
        file << "\n";
        file << "//-------------------------------------------------------------------------\n";
        file << "// TypeInfo: " << type.m_namespace.Get().c_str() << type.m_name.Get().c_str() << "\n";
        file << "//-------------------------------------------------------------------------\n\n";

        file << "namespace SE\n";
        file << "{\n";

        // Define static type info for type
        file << "    Reflect::TypeInfo const* " << type.m_namespace.Get().c_str() << type.m_name.Get().c_str() << "::s_pTypeInfo = nullptr;\n\n";

        file << "    namespace TypeSystem\n";
        file << "    {\n";
        file << "        template<>\n";
        file << "        class " << exportMacro.c_str() << " TTypeInfo<" << type.m_namespace.Get().c_str() << type.m_name.Get().c_str() << "> final : public TypeInfo\n";
        file << "        {\n";

        file << "           static " << type.m_namespace.Get().c_str() << type.m_name.Get().c_str() <<" const* s_pDefaultInstance_" << type.m_ID.ToUint() << ";\n\n";

        file << "        public:\n\n";

        GenerateStaticTypeRegistrationMethod( file, type );
        GenerateStaticTypeUnregistrationMethod( file, type );
        
        file << "        public:\n\n";
        */

        generateTypeData.set("ConstructorMethod", GenerateTypeInfoConstructor(type, parentType));
        generateTypeData.set("CreationMethod", GenerateCreationMethod(type));
        generateTypeData.set("InPlaceCreationMethod", GenerateCreationMethod(type));
        
        // GenerateCreationMethod(file, type);
        // GenerateInPlaceCreationMethod( file, type );
        
        // GenerateLoadResourcesMethod( database, file, type );
        // GenerateUnloadResourcesMethod( database, file, type );
        // GenerateResourceLoadingStatusMethod( database, file, type );
        // GenerateResourceUnloadingStatusMethod( database, file, type );
        // GenerateGetReferencedResourceMethod( database, file, type );
        // GenerateExpectedResourceTypeMethod( file, type );
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
        


        // GenerateArraySizeMethod( file, type );
        // GenerateArrayElementSizeMethod( file, type );
        // GenerateArrayClearMethod(file, type);
        // GenerateAddArrayElementMethod( file, type );
        // GenerateInsertArrayElementMethod( file, type );
        // GenerateMoveArrayElementMethod( file, type );
        // GenerateRemoveArrayElementMethod( file, type );
        // GenerateAreAllPropertiesEqualMethod( file, type );
        // GenerateIsPropertyEqualMethod( file, type );
        // GenerateSetToDefaultValueMethod( file, type );

        // Generate entity component methods
        //-------------------------------------------------------------------------
        // mustache::data generateEntityMethodData = GenerateEntityMethod(type);
        // generateTypeData.set("EntityLoadMethod", generateEntityMethodData);
        // generateTypeData.set("EntityUnloadMethod", generateEntityMethodData);
        // generateTypeData.set("EntityUpdateLoadingMethod", generateEntityMethodData);

        // GenerateLoadMethod( file, type );
        // GenerateUnloadMethod( file, type );
        // GenerateUpdateLoadingMethod( file, type );

        // Dev Flag
        //-------------------------------------------------------------------------

        if (type.m_isDevOnly)
        {
            generateTypeData.set("isDevOnlyEnd", "#endif");;
        }

        return generateTypeData;
    }

    //-------------------------------------------------------------------------

    void CppGenerateType(Generator* generator,  ReflectionDatabase const& database, std::stringstream& codeFile, String const& exportMacro,
            ReflectedType const& type, ReflectedType const& parentType, std::string templateStr)
    {
        //GenerateTypeInfoFile( codeFile, database, exportMacro, type, parentType );
        mustache::data data = GenerateTypeInfoFile(database, exportMacro, type, parentType);
        mustache::mustache tmpl(templateStr);

        codeFile << tmpl.render(data);
    }
}