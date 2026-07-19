#include "TypeCompositeInfo.h"
#include "Runtime/Core/TypeSystem/TypeDescriptors.h"

//-------------------------------------------------------------------------

namespace SE
{
    TypeCompositeInfo::TypeCompositeInfo()
    {
    }

    bool TypeCompositeInfo::IsDerivedFrom(TypeID const potentialParentTypeID) const
    {
        if (potentialParentTypeID == id)
        {
            return true;
        }

        if (pParentTypeInfo != nullptr)
        {
            if (pParentTypeInfo->id == potentialParentTypeID)
            {
                return true;
            }

            // Check inheritance hierarchy
            if (pParentTypeInfo->IsDerivedFrom(potentialParentTypeID))
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    TypeProperty const *TypeCompositeInfo::GetPropertyInfo(StringID propertyID) const
    {
        TypeProperty const *pProperty = nullptr;

        auto propertyIter = propertyMap.Find(propertyID);
        if (propertyIter != propertyMap.end())
        {
            int32 const propertyIdx = propertyIter->Value;
            pProperty = properties[propertyIdx];
        }

        return pProperty;
    }
}