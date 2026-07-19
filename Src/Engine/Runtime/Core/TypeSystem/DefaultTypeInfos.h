#pragma once

#include "IType.h"
#include "Types.h"
#include "Runtime/Core/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace SE
{
    template <>
    class TTypeCompositeInfo<IType> final : public TypeCompositeInfo
    {
    public:
        static void RegisterType(Types &typeRegistry)
        {
            IType::s_pTypeInfo = New<TTypeCompositeInfo<IType>>();
            typeRegistry.RegisterType(IType::s_pTypeInfo);
        }

        static void UnregisterType(Types &typeRegistry)
        {
            typeRegistry.UnregisterType(IType::s_pTypeInfo);
            Delete(IType::s_pTypeInfo);
        }

    public:
        TTypeCompositeInfo()
        {
            id = TypeID(SE_TEXT("SE::IType"));
            size = sizeof(IType);
            alignment = alignof(IType);
        }

        virtual IType *CreateType() const override
        {
            ENGINE_UNREACHABLE_CODE(); // Error! Trying to instantiate an abstract entity system!
            return nullptr;
        }

        virtual void CreateTypeInPlace(IType *pAllocatedMemory) const override
        {
            ENGINE_UNREACHABLE_CODE();
        }

        IType const *GetDefaultInstance() const override
        {
            return nullptr;
        }

        virtual uint8 *GetArrayElementDataPtr(IType *pType, uint32 arrayID, int64 arrayIdx) const override
        {
            ENGINE_UNREACHABLE_CODE();
            return nullptr;
        }

        virtual int64 GetArraySize(IType const *pTypeInstance, uint32 arrayID) const override
        {
            ENGINE_UNREACHABLE_CODE();
            return 0;
        }

        virtual int64 GetArrayElementSize(uint32 arrayID) const override
        {
            ENGINE_UNREACHABLE_CODE();
            return 0;
        }

        virtual void ClearArray(IType *pTypeInstance, uint32 arrayID) const override { ENGINE_UNREACHABLE_CODE(); }
        virtual void AddArrayElement(IType *pTypeInstance, uint32 arrayID) const override { ENGINE_UNREACHABLE_CODE(); }
        virtual void InsertArrayElement(IType *pTypeInstance, uint32 arrayID, int64 insertIdx) const override { ENGINE_UNREACHABLE_CODE(); }
        virtual void MoveArrayElement(IType *pTypeInstance, uint32 arrayID, int64 originalElementIdx, int64 newElementIdx) const override { ENGINE_UNREACHABLE_CODE(); }
        virtual void RemoveArrayElement(IType *pTypeInstance, uint32 arrayID, int64 arrayIdx) const override { ENGINE_UNREACHABLE_CODE(); }

        virtual bool AreAllPropertyValuesEqual(IType const *pTypeInstance, IType const *pOtherTypeInstance) const override { return false; }
        virtual bool IsPropertyValueEqual(IType const *pTypeInstance, IType const *pOtherTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override { return false; }
        virtual void ResetToDefault(IType *pTypeInstance, uint32 propertyID) const override {}
        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const override { return false; }
        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const override { return false; }
    };
}