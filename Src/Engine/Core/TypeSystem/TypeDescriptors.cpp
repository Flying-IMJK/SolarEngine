#include "TypeDescriptors.h"
#include "Types.h"
#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"

//-------------------------------------------------------------------------

namespace SE
{
    namespace
    {
        struct ResolvedPathElement
        {
            StringID m_propertyID;
            int32_t m_arrayElementIdx;
            TypeProperty const *m_pPropertyInfo;
            byte* m_pAddress;
        };

        struct ResolvedPath
        {
            inline bool IsValid() const { return !m_pathElements.IsEmpty(); }
            inline void Reset() { m_pathElements.Clear(); }

            List<ResolvedPathElement> m_pathElements = List<ResolvedPathElement>(6);
        };

        // Resolves a given property path with a given type instance to calculate the final address of the property this path refers to
        static ResolvedPath ResolvePropertyPath(TypeCompositeInfo const *pTypeInfo, byte* const pTypeInstanceAddress, TypePropertyPath const &path)
        {
            ResolvedPath resolvedPath;

            byte* pResolvedTypeInstance = pTypeInstanceAddress;
            TypeCompositeInfo const *pResolvedTypeInfo = pTypeInfo;
            TypeProperty const *pFoundPropertyInfo = nullptr;

            // Resolve property path
            int64 const numPathElements = path.GetNumElements();
            for (int64 i = 0; i < numPathElements; i++)
            {
                ENGINE_ASSERT(pResolvedTypeInfo != nullptr);

                // Get the property info for the path element
                pFoundPropertyInfo = pResolvedTypeInfo->GetPropertyInfo(path[i].propertyID);
                if (pFoundPropertyInfo == nullptr)
                {
                    resolvedPath.Reset();
                    break;
                }

                ResolvedPathElement &resolvedPathElement = resolvedPath.m_pathElements.AddOne();
                resolvedPathElement.m_propertyID = path[i].propertyID;
                resolvedPathElement.m_arrayElementIdx = path[i].arrayElementIdx;
                resolvedPathElement.m_pPropertyInfo = pFoundPropertyInfo;

                // Calculate the address of the resolved property
                if (pFoundPropertyInfo->IsArrayProperty())
                {
                    resolvedPathElement.m_pAddress = pResolvedTypeInfo->GetArrayElementDataPtr(reinterpret_cast<IType *>(pResolvedTypeInstance), path[i].propertyID, path[i].arrayElementIdx);
                }
                else // Structure/Type
                {
                    resolvedPathElement.m_pAddress = pResolvedTypeInstance + pFoundPropertyInfo->offset;
                }

                // Update the resolved type instance and type info
                pResolvedTypeInstance = resolvedPathElement.m_pAddress;
                if (!IsCoreType(resolvedPathElement.m_pPropertyInfo->typeID))
                {
                    pResolvedTypeInfo = Types::GetTypeInfo(resolvedPathElement.m_pPropertyInfo->typeID);
                }
                else
                {
                    pResolvedTypeInfo = nullptr;
                }
            }

            return resolvedPath;
        }
    }

    //-------------------------------------------------------------------------

    namespace
    {
        struct TypeDescriber
        {
            static void DescribeType(TypeDescriptor &typeDesc, TypeID typeID, IType const *pTypeInstance, TypePropertyPath &path, bool shouldSetPropertyStringValues)
            {
                ENGINE_ASSERT(!IsCoreType(typeID));
                auto const pTypeInfo = Types::GetTypeInfo(typeID);
                ENGINE_ASSERT(pTypeInfo != nullptr);

                //-------------------------------------------------------------------------

                for (auto const &propInfo : pTypeInfo->properties)
                {
                    if (propInfo->IsArrayProperty())
                    {
                        size_t const elementByteSize = propInfo->arrayElementSize;
                        size_t const numArrayElements = propInfo->IsStaticArrayProperty() ? propInfo->arraySize : pTypeInfo->GetArraySize(pTypeInstance, propInfo->id);
                        if (numArrayElements > 0)
                        {
                            uint8_t const *pArrayElementAddress = pTypeInfo->GetArrayElementDataPtr(const_cast<IType *>(pTypeInstance), propInfo->id, 0);

                            // Write array elements
                            for (auto i = 0u; i < numArrayElements; i++)
                            {
                                path.Append(propInfo->id, i);
                                DescribeProperty(typeDesc, pTypeInfo, pTypeInstance, *propInfo, shouldSetPropertyStringValues, pArrayElementAddress, path, i);
                                pArrayElementAddress += elementByteSize;
                                path.RemoveLastElement();
                            }
                        }
                    }
                    else
                    {
                        path.Append(propInfo->id);
                        auto pPropertyInstance = propInfo->GetPropertyAddress(pTypeInstance);
                        DescribeProperty(typeDesc, pTypeInfo, pTypeInstance, *propInfo, shouldSetPropertyStringValues, pPropertyInstance, path);
                        path.RemoveLastElement();
                    }
                }
            }

            static void DescribeProperty(TypeDescriptor &typeDesc, TypeCompositeInfo const *pParentTypeInfo, IType const *pParentInstance, TypeProperty const &propertyInfo, bool shouldSetPropertyStringValues, void const *pPropertyInstance, TypePropertyPath &path, int32_t arrayElementIdx = -1)
            {
                if (IsCoreType(propertyInfo.typeID) || propertyInfo.IsEnumProperty())
                {
                    // Only describe non-default properties
                    if (!pParentTypeInfo->IsPropertyValueSetToDefault(pParentInstance, propertyInfo.id, arrayElementIdx))
                    {
                        PropertyDescriptor propertyDesc = PropertyDescriptor();
						typeDesc.properties.Add(propertyDesc);
                        propertyDesc.path = path;
                        ConvertNativeTypeToBinary(propertyInfo, pPropertyInstance, propertyDesc.byteValue);

#ifdef SE_DEVELOPMENT
                        if (shouldSetPropertyStringValues)
                        {
                            ConvertNativeTypeToString(propertyInfo, pPropertyInstance, propertyDesc.stringValue);
                        }
#endif
                    }
                }
                else
                {
                    DescribeType(typeDesc, propertyInfo.typeID, (IType *)pPropertyInstance, path, shouldSetPropertyStringValues);
                }
            }
        };
    }

    //-------------------------------------------------------------------------

    TypeDescriptor::TypeDescriptor(IType *pTypeInstance, bool shouldSetPropertyStringValues)
    {
        ENGINE_ASSERT(pTypeInstance != nullptr);
        DescribeTypeInstance(pTypeInstance, shouldSetPropertyStringValues);
    }

    void TypeDescriptor::DescribeTypeInstance(IType const *pTypeInstance, bool shouldSetPropertyStringValues)
    {
        // Reset descriptor
        typeID = pTypeInstance->GetType();
        properties.Clear();

        // Fill property values
        TypePropertyPath path;
        TypeDescriber::DescribeType(*this, typeID, pTypeInstance, path, shouldSetPropertyStringValues);
    }

    PropertyDescriptor *TypeDescriptor::GetProperty(TypePropertyPath const &path)
    {
        for (auto &prop : properties)
        {
            if (prop.path == path)
            {
                return &prop;
            }
        }

        return nullptr;
    }

    void TypeDescriptor::RemovePropertyValue(TypePropertyPath const &path)
    {
        for (int32_t i = (int32_t)properties.Count() - 1; i >= 0; i--)
        {
            if (properties[i].path == path)
            {
                properties.RemoveAt(i);
            }
        }
    }

    void *TypeDescriptor::SetPropertyValues(TypeCompositeInfo const *pTypeInfo, void *pTypeInstance) const
    {
        ENGINE_ASSERT(pTypeInfo != nullptr);
        ENGINE_ASSERT(IsValid() && pTypeInfo->id == typeID);

        for (auto const &propertyValue : properties)
        {
            ENGINE_ASSERT(propertyValue.IsValid());

            // Resolve a property path for a given instance
            auto resolvedPath = ResolvePropertyPath(pTypeInfo, (byte*)pTypeInstance, propertyValue.path);
            if (!resolvedPath.IsValid())
            {
                LOG_ERROR("TypeSystem", "Type Descriptor Tried to set the value for an invalid property ({0}) for type ({1})",
					propertyValue.path.ToString(), pTypeInfo->fullName);
                continue;
            }

            // Set actual property value
            auto const &resolvedProperty = resolvedPath.m_pathElements.Last();
            ConvertBinaryToNativeType(*resolvedProperty.m_pPropertyInfo, propertyValue.byteValue, resolvedProperty.m_pAddress);
        }

        return pTypeInstance;
    }

    //-------------------------------------------------------------------------

    void TypeDescriptorCollection::Reset()
    {
        descriptors.Clear();
		totalRequiredSize = -1;
		requiredAlignment = -1;
        typeInfos.Clear();
        typeSizes.Clear();
        typePaddings.Clear();
    }

    void TypeDescriptorCollection::CalculateCollectionRequirements(Types const &typeRegistry)
    {
		totalRequiredSize = 0;
		requiredAlignment = 0;

        typeInfos.Clear();
        typeSizes.Clear();
        typePaddings.Clear();

        int32_t const numDescs = (int32_t)descriptors.Count();
        if (numDescs == 0)
        {
            return;
        }

        //-------------------------------------------------------------------------

        typeInfos.Resize(numDescs);
        typeSizes.Resize(numDescs);
        typePaddings.Resize(numDescs);

        uintptr_t predictedMemoryOffset = 0;

        for (auto const &typeDesc : descriptors)
        {
            auto pTypeInfo = typeRegistry.GetTypeInfo(typeDesc.typeID);
            ENGINE_ASSERT(pTypeInfo != nullptr);
            ENGINE_ASSERT(pTypeInfo->size > 0 && pTypeInfo->alignment > 0);

            // Update overall alignment requirements and the required padding
            requiredAlignment = Math::Max(requiredAlignment, pTypeInfo->alignment);
            int64 const requiredPadding = Memory::CalculatePaddingForAlignment(predictedMemoryOffset, pTypeInfo->alignment);
            predictedMemoryOffset += (uintptr)pTypeInfo->size + requiredPadding;

            typeInfos.Add(pTypeInfo);
            typeSizes.Add(pTypeInfo->size);
            typePaddings.Add((uint32)requiredPadding);
        }

		totalRequiredSize = (uint32)predictedMemoryOffset;
    }
}