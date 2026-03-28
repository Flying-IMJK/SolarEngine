#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/graphics/base/gpupipelinestate.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: StencilOperation
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::StencilOperation> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::StencilOperation> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::StencilOperation>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::StencilOperation"));
            size = sizeof(::SE::StencilOperation);
            alignment = alignof(::SE::StencilOperation);
            name = SE_TEXT("StencilOperation");
            fullName = SE_TEXT("SE::StencilOperation");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Keep"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Keep the existing stencil data.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Zero"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Set the stencil data to 0.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Replace"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Set the stencil data to the reference value (set via GPUContext::SetStencilRef).");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("IncrementSaturated"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Increment the stencil value by 1, and clamp the result.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("DecrementSaturated"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Decrement the stencil value by 1, and clamp the result.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Invert"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Invert the stencil data.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Increment"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Increment the stencil value by 1, and wrap the result if necessary.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Decrement"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Decrement the stencil value by 1, and wrap the result if necessary.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Decrement the stencil value by 1, and wrap the result if necessary.");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::StencilOperation> * TTypeEnumInfo<::SE::StencilOperation>::s_pTypeInfo = nullptr;
}

