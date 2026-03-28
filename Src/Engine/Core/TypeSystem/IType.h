#pragma once

#include "Types.h"
#include "Core/API.h"
#include "Core/TypeSystem/Info/TypeCompositeInfo.h"

//-------------------------------------------------------------------------
// Type Reflection
//-------------------------------------------------------------------------

namespace SE
{
    class TypeProperty;
    typedef decltype(__nullptr) nullVoid;

	class SE_API_CORE IType
    {
    public:
        virtual ~IType() = default;
        static TypeCompositeInfo * s_pTypeInfo;

	    IType() = default;
        explicit IType(nullVoid v) {}
        IType(IType const&) = default;

        IType& operator=(IType const& rhs) = default;

        virtual TypeCompositeInfo const* GetTypeInfo() const = 0;
        virtual TypeID GetType() const = 0;
	};

    //-------------------------------------------------------------------------
    // 是否为指定类型
    template<typename T>
    bool TypeIs( IType const* pType )
    {
        if ( pType == nullptr )
        {
            return false;
        }

        return pType->GetType() == Typeof<T>();
    }

    // 是否为指定类型
    template<typename T>
    bool TypeIs( IType * pType )
    {
        if ( pType == nullptr )
        {
            return false;
        }
        return pType->GetType() == Typeof<T>();
    }

    // 是否可以转换为指定类型
    template<typename T>
    bool TypeAs( IType const* pType )
    {
        if ( pType == nullptr )
        {
            return false;
        }

        return pType->GetTypeInfo()->IsDerivedFrom(Typeof<T>());
    }

    // 是否可以转换为指定类型
    template<typename T>
    bool TypeAs( IType * pType )
    {
        if ( pType == nullptr )
        {
            return false;
        }

        return pType->GetTypeInfo()->IsDerivedFrom(Typeof<T>());
    }

    // This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
    template<typename T>
    T* TypeCast( IType* pType )
    {
        ENGINE_ASSERT( pType != nullptr );
        if (!TypeAs<T>(pType))
        {
            return nullptr;
        }

        return reinterpret_cast<T*>( pType );
    }

    // This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
    template<typename T>
    T const* TypeCast( IType const* pType )
    {
        ENGINE_ASSERT( pType != nullptr );
        if (!TypeAs<T>(pType))
        {
            return nullptr;
        }

        return reinterpret_cast<T const*>( pType );
    }

    // This will try to cast to the specified type but can fail. Also accepts null arguments
    template<typename T>
    T* TypeTryCast( IType* pType )
    {
        if ( pType != nullptr && TypeAs<T>(pType) )
        {
            return reinterpret_cast<T*>( pType );
        }

        return nullptr;
    }

    // This will try to cast to the specified type but can fail. Also accepts null arguments
    template<typename T>
    T const* TypeTryCast( IType const* pType )
    {
        if ( pType != nullptr && TypeAs<T>(pType) )
        {
            return reinterpret_cast<T const*>( pType );
        }

        return nullptr;
    }

    template<typename T>
    bool TypeTryCast( const IType* pType, const T*& target)
    {
        if ( pType != nullptr && TypeAs<T>(pType) )
        {
            target = reinterpret_cast<T*>( pType );
            return true;
        }

        target = nullptr;
        return false;
    }

    template<typename T>
    bool TypeTryCast( IType* pType, T*& target)
    {
        if ( pType != nullptr && TypeAs<T>(pType) )
        {
            target = reinterpret_cast<T*>( pType );
            return true;
        }

        target = nullptr;
        return false;
    }

}

//-------------------------------------------------------------------------
// Reflection Macros
//-------------------------------------------------------------------------
// SE_META
// class nameAttribute : public TypeMetaAttribute
// {
//
// }
#define SE_META()

// Flag this enum for reflection and expose it to the tools/serializers
// SE_ENUM
// class enum name
// {
//
// }
#define SE_ENUM(TypeName)


// Flag this type for reflection and expose it to the tools/serializers
// class name : BaseClass
// {
//    SE_CLASS(name, BaseCass);
// }
//
#define SE_CLASS(TypeName, BaseTypeName)                                                \
        friend TypeCompositeInfo;                                                       \
        template<typename TType> friend class TTypeCompositeInfo;                       \
    public:                                                                             \
        explicit TypeName(::SE::nullVoid v) : BaseTypeName(v) {};                       \
        inline static TypeCompositeInfo const* s_pTypeInfo = nullptr;                   \
        virtual TypeCompositeInfo const* GetTypeInfo() const override { ENGINE_ASSERT( s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo; }  \
        virtual TypeID GetType() const override { ENGINE_ASSERT(s_pTypeInfo != nullptr); return s_pTypeInfo->id; }

#define SE_CLASS_DEFAULT(TypeName, BaseTypeName)                                        \
        friend TypeCompositeInfo;                                                       \
        template<typename TType> friend class TTypeCompositeInfo;                       \
    public:                                                                             \
        explicit TypeName(::SE::nullVoid v) : BaseTypeName(v) {};                       \
        TypeName() = default;                                                           \
        inline static TypeCompositeInfo const* s_pTypeInfo = nullptr;                   \
        virtual TypeCompositeInfo const* GetTypeInfo() const override { ENGINE_ASSERT( s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo; }  \
        virtual TypeID GetType() const override { ENGINE_ASSERT(s_pTypeInfo != nullptr); return s_pTypeInfo->id; }


// Flag this member variable (i.e. class property) for reflection and expose it to the tools/serializers
// Users can fill the contents with a JSON string to define per variable meta data
//
// Currently Supported Meta Data:
// * "Category" : "Lorem Ipsum" - Create a category for this property
// * "Description" : "Lorem Ipsum" - Provides tooltip help text for this property
// * "IsToolsReadOnly" : true/false - Allows the tools to edit this property, by default all properties are writable
// * "ShowAsStaticArray" : true/false - Removes the resizing controls from a dynamic array
// * "CustomEditor" : "MyEditorID" - Allows for a custom editor to be used in the property grid without creating a new type
#define SE_PROPERTY(...)

// Flag a class as the module class for that project
#define ENGINE_REFLECT_MODULE \
        public: \
        static void RegisterTypes(); \
        static void UnregisterTypes();