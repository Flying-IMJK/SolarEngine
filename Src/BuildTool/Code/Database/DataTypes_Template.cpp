#include "DataTypes_Template.h"

std::string SE::BuildTool::TypeInfoTemplate::ToCppString(bool includeArray) const 
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
                result += genericArgs[i].ToCppStringInternal();
            }
            result += ">";
        }

        return result;
    }
