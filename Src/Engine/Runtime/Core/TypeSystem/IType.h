#pragma once

#include "Runtime/API.h"
#include "Types.h"
#include "Info/TypeCompositeInfo.h"

//-------------------------------------------------------------------------
// Type Reflection
//-------------------------------------------------------------------------

namespace SE
{
    class TypeProperty;
    typedef decltype(__nullptr) nullVoid;

	class SE_API_RUNTIME IType
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


    // Flag this type for reflection and expose it to the tools/serializers
    // class name : BaseClass
    // {
    //    DEFINE_CLASS(name, BaseClass);
    // }
    //
    #define SE_DEFINE_CLASS(TypeName, BaseTypeName)                                         \
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

}
