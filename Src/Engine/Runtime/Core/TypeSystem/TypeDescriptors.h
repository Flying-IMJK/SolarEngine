#pragma once

#include "IType.h"
#include "Property/TypePropertyPath.h"
#include "CoreTypes.h"
#include "CoreTypeConversions.h"
#include "Types.h"

//-------------------------------------------------------------------------
// Basic descriptor of a reflected property
//-------------------------------------------------------------------------

namespace SE
{
    class Types;
    class TypeCompositeInfo;

    struct SE_API_RUNTIME PropertyDescriptor
    {
        PropertyDescriptor() = default;

        PropertyDescriptor(TypePropertyPath const &path, TypeProperty const &propertyInfo, String const &stringValue)
            : path(path)
        {
            ENGINE_ASSERT(path.IsValid());
            ConvertStringToBinary(propertyInfo, stringValue, byteValue);

#ifdef SE_DEVELOPMENT
			this->stringValue = stringValue;
			typeID = propertyInfo.typeID;
			templatedArgumentTypeID = propertyInfo.templateArgumentTypeID;
#endif
        }

        PropertyDescriptor(Types const &typeRegistry, TypePropertyPath const &path, TypeID propertyTypeID, TypeID propertyTemplatedArgumentTypeID, String const &stringValue)
            : path(path)
        {
            ENGINE_ASSERT(path.IsValid() && !stringValue.IsEmpty());
            ConvertStringToBinary(propertyTypeID, propertyTemplatedArgumentTypeID, stringValue, byteValue);

#ifdef SE_DEVELOPMENT
			this->stringValue = stringValue;
			typeID = propertyTypeID;
			templatedArgumentTypeID = propertyTemplatedArgumentTypeID;
#endif
        }

        inline bool IsValid() const { return path.IsValid() && !byteValue.IsEmpty(); }

        // Value Access
        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        template <typename T>
        inline T GetValue() const
        {
            TypeID const coreTypeID = GetCoreTypeID<T>();

            T value;
            ConvertStringToNativeType(coreTypeID, TypeID(), stringValue, value);
            return value;
        }
#endif

        // Tools only constructors
        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        PropertyDescriptor(TypePropertyPath const &path, char const *pValue, TypeID typeID, TypeID templatedArgumentTypeID = TypeID())
            : path(path), stringValue(pValue), typeID(typeID), templatedArgumentTypeID(templatedArgumentTypeID)
        {
            ENGINE_ASSERT(path.IsValid());
        }
#endif

    public:
        TypePropertyPath path;
        List<uint8> byteValue;

// Not-serialized - used in tooling
#ifdef SE_DEVELOPMENT
        String stringValue;
        TypeID typeID;
        TypeID templatedArgumentTypeID;
#endif
    };

    //-------------------------------------------------------------------------
    // Type Descriptor
    //-------------------------------------------------------------------------
    // A serialized description of a SE type with all property overrides

    class SE_API_RUNTIME TypeDescriptor
    {
    public:
        TypeDescriptor() = default;
        TypeDescriptor(TypeID typeID) : typeID(typeID) { ENGINE_ASSERT(typeID != TypeID::Invalid); }
        TypeDescriptor(IType *pTypeInstance, bool shouldSetPropertyStringValues = false);

		virtual inline bool IsValid() const { return typeID != TypeID::Invalid; }

        // Descriptor Creation
        //-------------------------------------------------------------------------

        void DescribeTypeInstance(IType const *pTypeInstance, bool shouldSetPropertyStringValues);

        // Type Creation
        //-------------------------------------------------------------------------

        // Create a new instance of the described type!
        /*template <typename T, typename... Args>
        T *CreateTypeInstance(TypeInfo const *pTypeInfo, Args... args) const
        {
            ENGINE_ASSERT(pTypeInfo != nullptr && pTypeInfo->id == typeID);
            ENGINE_ASSERT(pTypeInfo->IsDerivedFrom<T>());

            // Create new instance
            void *pTypeInstance = pTypeInfo->CreateType();
            ENGINE_ASSERT(pTypeInstance != nullptr);

            new (pTypeInstance) T(Forward<Args...>(args));

            // Set properties
            SetPropertyValues(pTypeInfo, pTypeInstance);
            return reinterpret_cast<T *>(pTypeInstance);
        }*/

        template <typename T>
        [[nodiscard]] T *CreateTypeInstance(TypeCompositeInfo const *pTypeInfo) const
        {
            ENGINE_ASSERT(pTypeInfo != nullptr && pTypeInfo->id == typeID);
            ENGINE_ASSERT(pTypeInfo->IsDerivedFrom<T>());

            // Create new instance
            void *pTypeInstance = pTypeInfo->CreateType();
            ENGINE_ASSERT(pTypeInstance != nullptr);

            // Set properties
            SetPropertyValues(pTypeInfo, pTypeInstance);
            return reinterpret_cast<T *>(pTypeInstance);
        }

        // Create a new instance of the described type! This function is slower since it has to look up the type info first, if you can prefer using the version above!
        template <typename T>
        [[nodiscard]] T *CreateTypeInstance() const
        {
            TypeCompositeInfo const *pTypeInfo = Types::GetTypeInfo(typeID);
            return CreateTypeInstance<T>(pTypeInfo);
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template <typename T>
        [[nodiscard]] T *CreateTypeInstanceInPlace(TypeCompositeInfo const *pTypeInfo, IType *pAllocatedMemoryForInstance) const
        {
            ENGINE_ASSERT(pTypeInfo != nullptr && pTypeInfo->id == typeID);
            ENGINE_ASSERT(pTypeInfo->IsDerivedFrom<T>());

            // Create new instance
            ENGINE_ASSERT(pAllocatedMemoryForInstance != nullptr);
            pTypeInfo->CreateTypeInPlace(pAllocatedMemoryForInstance);

            // Set properties
            SetPropertyValues(pTypeInfo, pAllocatedMemoryForInstance);
            return reinterpret_cast<T *>(pAllocatedMemoryForInstance);
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template <typename T>
        [[nodiscard]] T *CreateTypeInstanceInPlace(void *pAllocatedMemoryForInstance) const
        {
            TypeCompositeInfo const *pTypeInfo = Types::GetTypeInfo(typeID);
            return CreateTypeInstanceInPlace<T>(pTypeInfo, pAllocatedMemoryForInstance);
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template <typename T>
        inline void CreateTypeInstanceInPlace(T *pTypeInstance) const
        {
            pTypeInstance->~T();
            TypeCompositeInfo const *pTypeInfo = Types::GetTypeInfo(typeID);
            T *pCreatedType = CreateTypeInstanceInPlace<T>(pTypeInfo, pTypeInstance);
        }

        // Properties
        //-------------------------------------------------------------------------

        PropertyDescriptor *GetProperty(TypePropertyPath const &path);
        inline PropertyDescriptor const *GetProperty(TypePropertyPath const &path) const { return const_cast<TypeDescriptor *>(this)->GetProperty(path); }
        void RemovePropertyValue(TypePropertyPath const &path);

    private:
        void *SetPropertyValues(TypeCompositeInfo const *pTypeInfo, void *pTypeInstance) const;

    public:
        TypeID typeID;
        List<PropertyDescriptor> properties;
    };

    //-------------------------------------------------------------------------
    // Type Descriptor Collection
    //-------------------------------------------------------------------------
    // Generally only useful for when serializing a set of types all derived from the same base type
    // This collection can be instantiate in one of two ways
    // * Statically - all types are created in a single contiguous array of memory, this is immutable
    // * Dynamically - each type is individually allocated, these types can be destroyed individually at runtime

    struct SE_API_RUNTIME TypeDescriptorCollection
    {
    public:
        template <typename T>
        static void *InstantiateStaticCollection(Types const &typeRegistry, TypeDescriptorCollection const &collection, List<T *> &outTypes)
        {
            ENGINE_ASSERT(collection.typeSizes.Count() == collection.descriptors.Count());  // Did you forget to run the calculate requirements function?
            ENGINE_ASSERT(collection.typeSizes.Count() == collection.typePaddings.Count()); // Did you forget to run the calculate requirements function?

            void *pRawMemory = PlatformAllocator::Allocate(collection.totalRequiredSize, collection.requiredAlignment);
            uint8_t *pTypeMemory = (uint8_t *)pRawMemory;
            int32 const numDescs = (int32)collection.descriptors.Count();
            for (int32 i = 0; i < numDescs; i++)
            {
                pTypeMemory += collection.typePaddings[i];
                outTypes.emplace_back(collection.descriptors[i].CreateTypeInstanceInPlace<T>(typeRegistry, collection.typeInfos[i], (IType *)pTypeMemory));
                pTypeMemory += collection.typeSizes[i];
            }

            return pRawMemory;
        }

        template <typename T>
        static void DestroyStaticCollection(List<T *> &types)
        {
            ENGINE_ASSERT(!types.empty());
            void *pMemoryBlock = types[0];
            for (auto pType : types)
            {
                pType->~T();
            }
            PlatformAllocator::Free(pMemoryBlock);
        }

        template <typename T>
        static void InstantiateDynamicCollection(Types const &typeRegistry, TypeDescriptorCollection const &collection, List<T *> &outTypes)
        {
            int32 const numDescs = (int32)collection.descriptors.Count();
            for (int32 i = 0; i < numDescs; i++)
            {
                outTypes.emplace_back(collection.descriptors[i].CreateTypeInstance<T>(typeRegistry, collection.typeInfos[i]));
            }
        }

        template <typename T>
        static void DestroyDynamicCollection(List<T *> &types)
        {
            for (auto &pType : types)
            {
                Delete(pType);
            }
            types.clear();
        }

    public:
        void Reset();

        // Calculates all the necessary information needed to instantiate this collection statically (aka in a single immutable block)
        void CalculateCollectionRequirements(Types const &typeRegistry);

    public:
		List<TypeDescriptor> descriptors;
        int32 totalRequiredSize = -1;
        int32 requiredAlignment = -1;
		List<TypeCompositeInfo const *> typeInfos;
		List<uint32> typeSizes;
		List<uint32> typePaddings;
    };
}