#pragma once

#include "Database/DataTypes.h"

#include <string>
#include <vector>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    struct TypeRefTemplate
    {
        std::string ToCppString(bool includeArray = true) const
        {
            std::string result;
            if (isConst)
            {
                result += "const ";
            }

            result += name;
            if (!genericArgs.empty())
            {
                result += "<";
                for (int i = 0; i < genericArgs.size(); i++)
                {
                    if (i > 0)
                    {
                        result += ", ";
                    }
                    result += genericArgs[i].ToCppString();
                }
                result += ">";
            }

            if (isPointer)
            {
                result += "*";
            }
            if (isMoveRef)
            {
                result += "&&";
            }
            else if (isRef)
            {
                result += "&";
            }
            if (includeArray && isArray)
            {
                result += "[";
                if (arraySize > 0)
                {
                    result += std::to_string(arraySize);
                }
                result += "]";
            }
            return result;
        }

        bool IsValid() const { return !name.empty(); }

        std::string name;
        std::vector<TypeRefTemplate> genericArgs;
        bool isTemplateParameter = false;
        bool isConst             = false;
        bool isPointer           = false;
        bool isRef               = false;
        bool isMoveRef           = false;
        bool isArray             = false;
        int  arraySize           = 0;
    };

    struct TypeInfoParamTemplate
    {
        TypeRefTemplate type;
        std::string     name;
        std::string     defaultValue;
        std::string     attributes;
        std::string     marshalAs;
        std::string     comment;
        bool            isOut = false;
    };

    struct TypeInfoFuncTemplate
    {
        std::string                        name;
        TypeRefTemplate                    returnType;
        std::vector<TypeInfoParamTemplate> params;

        bool isReflect = false;
        bool isAPI     = false;

        bool isStatic        = false;
        bool isVirtual       = false;
        bool isConst         = false;
        bool APINoProxy      = false;
        bool APIIsSealed     = false;
        bool APIIsStatic     = false;
        bool APIIsPropertie  = false;

        std::string uniqueName;
        std::string entryPoint;

        AccessLevel access = AccessLevel::Public;
        std::string attributes;
        std::string comment;
        std::string marshalAs;
        int         lineNumber = -1;
    };

    struct TypeInfoEventTemplate
    {
        bool isReflect = false;
        bool isAPI     = false;

        std::string                        name;
        TypeRefTemplate                    cppType;
        std::vector<TypeInfoParamTemplate> params;
        bool                               isStatic = false;
        AccessLevel                        access   = AccessLevel::Public;
        std::string                        attributes;
        std::string                        comment;
        int                                lineNumber = -1;
    };

    struct TypeInfoFieldTemplate
    {
        bool isAPI    = false;
        bool isStatic = false;

        TypeRefTemplate type;
        std::string     name;

        bool isReflect = false;

        bool APIIsReadOnly   = false;

        std::string attributes;
        std::string defaultValue;
        std::string comment;
        std::string marshalAs;
        int         lineNumber = -1;
    };

    struct TypeInfoStructTemplate
    {
        TypeID                   typeID;
        HeaderID                 headerID;
        std::string              name = "Invalid";
        std::vector<std::string> namespaceScopeList;
        std::vector<std::string> structScopeList;
        std::vector<std::string> templateParameters;

        TypeRefTemplate                     baseType;
        std::vector<TypeInfoFieldTemplate>  fields;
        std::vector<TypeInfoFuncTemplate>   functions;
        std::vector<TypeInfoEventTemplate>  events;

        bool isStruct          = false;
        bool isScriptingObject = false;
        bool isAbstract        = false;
        bool isReflect         = false;
        bool isAPI             = false;

        bool        APIIsAbstract    = false;
        bool        APIIsSealed      = false;
        bool        APIIsStatic      = false;
        bool        APINoSpawn       = false;
        bool        APINoConstructor = false;
        bool        APIIsInterface   = false;
        bool        APIIsNativeInvokeUseName = false;
        std::string APIName;
        std::string APIAttributes;
        std::string APIMarshalAs;

        std::string comment;
    };
} // namespace SE::BuildTool
