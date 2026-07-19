#pragma once

#include "Runtime/Core/Types/BitFlags.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Strings/StringID.h"
#include "Runtime/Core/TypeSystem/TypeID.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaContainer.h"

namespace SE
{
    class TypeMetadataContainer;

    class SE_API_RUNTIME TypeProperty
    {
    public:
        enum Flags
        {
            IsArray = 1 << 0,
            IsDynamicArray = 1 << 1,
            IsEnum = 1 << 2,
            IsBitFlags = 1 << 3,
            IsStructure = 1 << 4,
            IsValue = 1 << 5,
        };
    private:
        bool IsFlag(Flags flag) const { return (flags & flag) > 0; }

    public:
        TypeProperty();
        ~TypeProperty();

        // General queries
        //-------------------------------------------------------------------------

        bool IsStructureProperty() const { return IsFlag(Flags::IsStructure); }
        bool IsEnumProperty() const { return IsFlag(Flags::IsEnum); }
        bool IsBitFlagsProperty() const { return IsFlag(Flags::IsBitFlags); }
        bool IsArrayProperty() const { return IsFlag(Flags::IsArray) || IsFlag(Flags::IsDynamicArray); }
        bool IsStaticArrayProperty() const { return IsFlag(Flags::IsArray) && !IsFlag(Flags::IsDynamicArray); }
        bool IsDynamicArrayProperty() const { return IsFlag(Flags::IsArray) && IsFlag(Flags::IsDynamicArray); }

        // Addressing functions
        //-------------------------------------------------------------------------
        // Warning! These are only valid if the type address is the immediate parent type!

        template <typename T>
        T *GetPropertyAddress(void *pTypeAddress) const
        {
            return reinterpret_cast<T *>(GetPropertyAddress(pTypeAddress));
        }

        template <typename T>
        T const *GetPropertyAddress(void const *pTypeAddress) const
        {
            return reinterpret_cast<T const *>(GetPropertyAddress(pTypeAddress));
        }

        void* GetPropertyAddress(void *pTypeAddress) const;

        void const *GetPropertyAddress(void const *pTypeAddress) const;

        // Default value
        //-------------------------------------------------------------------------
        // NB! no type-safety here, these functions are not for general use!

        template <typename T>
        T const &GetDefaultValue() const
        {
            ENGINE_ASSERT(!IsArrayProperty());
            T const &defaultValue = *reinterpret_cast<T const *>(pDefaultValue);
            return defaultValue;
        }

        void const *GetArrayDefaultElementPtr(int32 elementIdx) const;
    public:
        uint8 flags;                             // 属性类型标记
        StringID id;							 // 属性 ID
        TypeID typeID;	                         // 属性类型 ID
        TypeID parentTypeID;                     // The type ID for the parent type that this property belongs to
        TypeID templateArgumentTypeID;           // A property's contained TypeID for templatized types i.e. the specialization type for a TResourcePtr
        int32 offset = -1;                       // 属性字节偏移

        int32 arraySize = -1;                    // Number of elements the array (for static arrays this is the static dimensions, for dynamic arrays this is the default size)
        int32 arrayElementSize = -1;             // uint8 size of an individual array element
        int32 size = -1;                         // uint8 size of property / total array byte size for static array properties
        void const *pDefaultValue = nullptr;     // A ptr to the default value of the property
        void const *pDefaultArrayData = nullptr; // A ptr to the contained data within the default value array

        TypeMetadataContainer* metaContainer;    // 属性元数据容器

        StringView name;
#ifdef SE_DEVELOPMENT
        bool isForDevelopmentUseOnly = false; 	 // 此属性是否仅存在于开发版本中
        bool isToolsReadOnly = false;         	 // 此属性是否可以由任何工具修改，或者它是否只是一个可序列化的值
        bool showInRestrictedMode = false;    	 // 对于不同类型的具有不同的语义，例如，对于动态数组，不允许大小操作
        String category;
        String description;      				 // 从属性的注释生成
        StringID customEditorID; 				 // 是否使用自定义属性编辑器
#endif
    };
}
