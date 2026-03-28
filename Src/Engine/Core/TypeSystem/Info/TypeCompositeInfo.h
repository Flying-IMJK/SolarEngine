#pragma once

#include "TypeInfo.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Memory/Allocation.h"

//-------------------------------------------------------------------------

namespace SE
{
    class TypeProperty;
    class IType;

    class ConstructArg
    {
    public:
        template<typename T>
        ConstructArg(T&& value) : m_holder(Holder<T>(::Forward<T>(value))) {}

        template<typename T>
        T Get() const
        {
            return static_cast<Holder<T>*>(m_holder)->m_value;
        }

        template<typename T>
        bool IsType() const
        {
            return typeid(T) == m_holder.type();
        }

    private:
        struct HolderBase
        {
            virtual ~HolderBase() = default;
            virtual const std::type_info& type() const;
        };

        template<typename T>
        struct Holder : HolderBase
        {
            T m_value;

            Holder(T&& value) : m_value(::Forward<T>>(value)) {}
            const std::type_info& type() const override { return typeid(T); }
        };

        HolderBase m_holder;
    };

    class SE_API_CORE TypeCompositeInfo : public TypeInfo
    {
    public:
        TypeCompositeInfo();
        virtual ~TypeCompositeInfo() = default;

        TypeCompositeInfo &operator=(TypeCompositeInfo const &rhs) = default;

        virtual IType const *GetDefaultInstance() const = 0;

        // Basic Type Info
        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        inline StringView GetFriendlyTypeName() const { return friendlyName; }
        inline StringView GetCategoryName() const { return category; }
        inline bool HasCategoryName() const { return !category.IsEmpty(); }
#endif

        bool IsAbstractType() const { return isAbstract; }

        bool IsDerivedFrom(TypeID const parentTypeID) const;

        template <typename T>
        inline bool IsDerivedFrom() const { return IsDerivedFrom(T::GetStaticTypeID()); }

        // Property Info
        //-------------------------------------------------------------------------

        TypeProperty const *GetPropertyInfo(StringID propertyID) const;

        // Function declaration for generated property registration functions
        template <typename T>
        void RegisterProperties(IType const *pDefaultTypeInstance)
        {
            ENGINE_UNREACHABLE_CODE(); // Default implementation should never be called
        }

        // Type Factories
        //-------------------------------------------------------------------------
        virtual IType *CreateType() const = 0;
        /*// 使用变参模板创建对象
        template<typename... Args>
        IType* CreateType(Args&&... args)
        {
            List<ConstructArg> argVec;
            (argVec.Add(::Forward<Args>(args)), ...);
            return CreateTypeWithArgs(argVec);
        }*/
        virtual void CreateTypeInPlace(IType *pAllocatedMemory) const = 0;

        // Array helpers
        //-------------------------------------------------------------------------

        virtual uint8 *GetArrayElementDataPtr(IType *pTypeInstance, uint32 arrayID, int64 arrayIdx) const = 0;
        virtual int64 GetArraySize(IType const *pTypeInstance, uint32 arrayID) const = 0;
        virtual int64 GetArrayElementSize(uint32 arrayID) const = 0;
        virtual void ClearArray(IType *pTypeInstance, uint32 arrayID) const = 0;
        virtual void AddArrayElement(IType *pTypeInstance, uint32 arrayID) const = 0;
        virtual void InsertArrayElement(IType *pTypeInstance, uint32 arrayID, int64 insertIdx) const = 0;
        virtual void MoveArrayElement(IType *pTypeInstance, uint32 arrayID, int64 originalElementIdx, int64 newElementIdx) const = 0;
        virtual void RemoveArrayElement(IType *pTypeInstance, uint32 arrayID, int64 elementIdx) const = 0;

        // Default value helpers
        //-------------------------------------------------------------------------

        virtual bool AreAllPropertyValuesEqual(IType const *pTypeInstance, IType const *pOtherTypeInstance) const = 0;
        virtual bool IsPropertyValueEqual(IType const *pTypeInstance, IType const *pOtherTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const = 0;
        virtual void ResetToDefault(IType *pTypeInstance, uint32 propertyID) const = 0;

        virtual bool AreAllPropertiesSetToDefault(IType const *pTypeInstance) const = 0;
        virtual bool IsPropertyValueSetToDefault(IType const *pTypeInstance, uint32 propertyID, int32_t arrayIdx = -1) const = 0;

    public:
        TypeCompositeInfo const *pParentTypeInfo = nullptr;
        List<TypeProperty*> properties;
        Dictionary<StringID, int32> propertyMap;
        bool isAbstract = false;

#ifdef SE_DEVELOPMENT
        bool isForDevelopmentUseOnly = false; // Whether this property only exists in development builds
        String friendlyName;
        String category;
#endif
    };

    //-------------------------------------------------------------------------

    template <typename T>
    class TTypeCompositeInfo : public TypeCompositeInfo
    {


    };


}
