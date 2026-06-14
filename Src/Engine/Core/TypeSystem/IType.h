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

//-------------------------------------------------------------------------
// Code-Injecting Macros (expand to C++ code at compile time)
//-------------------------------------------------------------------------

// Flag this type for reflection and expose it to the tools/serializers
// class name : BaseClass
// {
//    DEFINE_CLASS(name, BaseClass);
// }
//
#define SE_DEFINE_CLASS(TypeName, BaseTypeName)                                                \
        friend TypeCompositeInfo;                                                       \
        template<typename TType> friend class TTypeCompositeInfo;                       \
    public:                                                                             \
        explicit TypeName(::SE::nullVoid v) : BaseTypeName(v) {};                       \
        inline static TypeCompositeInfo const* s_pTypeInfo = nullptr;                   \
        virtual TypeCompositeInfo const* GetTypeInfo() const override { ENGINE_ASSERT( s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo; }  \
        virtual TypeID GetType() const override { ENGINE_ASSERT(s_pTypeInfo != nullptr); return s_pTypeInfo->id; }

#define SE_DEFINE_CLASS_DEFAULT(TypeName, BaseTypeName)                                        \
        friend TypeCompositeInfo;                                                       \
        template<typename TType> friend class TTypeCompositeInfo;                       \
    public:                                                                             \
        explicit TypeName(::SE::nullVoid v) : BaseTypeName(v) {};                       \
        TypeName() = default;                                                           \
        inline static TypeCompositeInfo const* s_pTypeInfo = nullptr;                   \
        virtual TypeCompositeInfo const* GetTypeInfo() const override { ENGINE_ASSERT( s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo; }  \
        virtual TypeID GetType() const override { ENGINE_ASSERT(s_pTypeInfo != nullptr); return s_pTypeInfo->id; }

// Flag a class as the module class for that project
#define ENGINE_REFLECT_MODULE           \
        public:                         \
        static void RegisterTypes();    \
        static void UnregisterTypes();

//-------------------------------------------------------------------------
// Annotation-Only Macros (empty at compile time, parsed by Reflector)
//-------------------------------------------------------------------------

// SE_META
// class nameAttribute : public TypeMetaAttribute
// {
//
// }
#define SE_META()

// Flag this enum for reflection and/or C# binding
// Parameters: Reflect, API
// SE_ENUM(Reflect)             - reflection only
// SE_ENUM(API)                 - binding only
// SE_ENUM(Reflect, API)        - reflection + binding
#define SE_ENUM(...)

// Flag this class/struct/interface for reflection and/or C# binding
// Parameters: Reflect, API, NoSpawn, Abstract, Attributes=...
// [API] binding [NoSpawn] [Abstract]
// [Reflect] reflection
#define SE_CLASS(...)
#define SE_STRUCT(...)
#define SE_INTERFACE(...)

// Flag this member variable for reflection and/or C# binding
// Parameters: Reflect, API, plus JSON metadata
// SE_PROPERTY(Reflect, Category="MyCategory")     - reflection only
// SE_PROPERTY(API, ReadOnly)                       - binding only
// SE_PROPERTY(Reflect, API, Category="MyCategory") - reflection + binding
#define SE_PROPERTY(...)

// Flag this method for reflection and/or C# binding
// Parameters: Reflect, API, Static
// SE_FUNCTION(API)             - binding only
// SE_FUNCTION(Reflect, API)    - reflection + binding
#define SE_FUNCTION(...)

// Flag this event for C# binding
// Parameters: API
#define SE_EVENT(...)