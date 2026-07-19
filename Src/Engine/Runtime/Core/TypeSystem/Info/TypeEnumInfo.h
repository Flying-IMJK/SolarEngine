#pragma once

#include "Runtime/Core/TypeSystem/CoreTypes.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "TypeInfo.h"


//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME TypeEnumInfo : public TypeInfo
    {
    public:
        struct ConstantInfo
        {
            StringID id;
            int32 alphabeticalOrder = 0;
            int64 value = 0;
#ifdef SE_DEVELOPMENT
            String description;
#endif
        };

        int32 GetNumConstants() const;

        bool IsValidValue(StringID label) const;

        int64 GetConstantValue(StringID label) const;

        bool TryGetConstantValue(StringID label, int64 &outValue) const;

        StringID GetConstantLabel(int64 value) const;

        bool TryGetConstantLabel(int64 value, StringID &outValue) const;

#ifdef SE_DEVELOPMENT
        String const *TryGetConstantDescription(StringID label) const;
        List<ConstantInfo> GetConstantsInAlphabeticalOrder() const;
#endif

    public:
        TypeIDCore underlyingType;
		List<ConstantInfo> constants;
    };

    template <typename T>
    class TTypeEnumInfo : public TypeEnumInfo
    {


    };

}
