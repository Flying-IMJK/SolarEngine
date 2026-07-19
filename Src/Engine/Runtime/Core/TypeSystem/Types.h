#pragma once

#include "Runtime/API.h"
#include "CoreTypes.h"

//-------------------------------------------------------------------------
namespace SE
{
	class TypeCompositeInfo;
	class TypeEnumInfo;
	class TypeMetaInfo;
	class TypePropertyPath;
	class TypeProperty;
	class ITyper;
	class TypeRegister;
	class IType;

	//--------------------------------------------------------------------
    class SE_API_RUNTIME Types
    {
		friend class TypeRegister;
    	friend class Typer;

		template <typename T> friend TypeID Typeof();
    public:

        Types();
        ~Types();

		static void InitTypeSystem();
		static void UnInitTypeSystem();

    	static void RegisterTyper(TypeRegister* pRegister);
    	static void UnregisterTyper(TypeRegister* pRegister);

    	static void RegisterTypeID(StableID id, TypeID typeId);
    	static void UnRegisterTypeID(StableID id, TypeID typeId);

        //-------------------------------------------------------------------------
        // Type
        //-------------------------------------------------------------------------
		template <typename T = IType>
		static typename TEnableIf<TIsBaseOf<IType, T>::Value, T*>::Type CreateInstance(const TypeID& typeID)
		{
			TypeCompositeInfo const* typeInfo = GetTypeInfo(typeID);
			if (typeInfo != nullptr)
			{
				return (T*)typeInfo->CreateType();
			}

			return nullptr;
		}

        /**
         * 注册类型
         * @param pType 类型信息
         */
        static void RegisterType(TypeCompositeInfo const *pType);
        /**
         * 移除注册的类型
         * @param pType 类型信息
         */
        static void UnregisterType(TypeCompositeInfo const *pType);

        /**
         * 返回给定类型ID 的类型信息
         * @param typeID 类型ID
         * @return 类型信息
         */
        static TypeCompositeInfo const *GetTypeInfo(TypeID typeID);

        /**
         * 返回给定类型的类型信息
         * @tparam T 泛型类型
         * @return 类型信息
         */
        template <typename T, typename = typename TEnableIf<TIsBaseOfT<IType, T>::Value>::Value>
		//  std::enable_if_t<std::is_base_of<IType, T>::Value
        static TypeCompositeInfo const *GetTypeInfo()
        {
            return T::s_pTypeInfo;
        }

        /**
         * 返回给定类型的大小
         * @param typeID 类型
         * @return 类型大小
         */
        static int64 GetTypeByteSize(TypeID typeID);

        /**
         * 返回给定路径的已解析属性信息
         * @param pTypeInfo 类型
         * @param pathID 属性路径
         * @return
         */
        static TypeProperty const *ResolvePropertyPath(TypeCompositeInfo const *pTypeInfo, TypePropertyPath const &pathID);

        /**
         * 给定类型是否派生自给定的父类型
         * @param typeID 类型
         * @param parentTypeID 父类
         * @return
         */
        static bool IsTypeDerivedFrom(TypeID typeID, TypeID parentTypeID);

        /**
         * 返回所有已知类型
         * @param includeAbstractTypes 包含抽象类型
         * @param sortAlphabetically 按字母顺序排序
         * @return
         */
        static List<TypeCompositeInfo const *> GetAllTypes(bool includeAbstractTypes = true, bool sortAlphabetically = false);

        /**
         * 返回派生自指定类型的所有类型
         * @param parentTypeID 父类
         * @param includeParentTypeInResults 包含父类
         * @param includeAbstractTypes 包含抽象类型
         * @param sortAlphabetically 按字母顺序排序
         * @return
         */
        static List<TypeCompositeInfo const *> GetAllDerivedTypes(TypeID parentTypeID,
                                                           bool includeParentTypeInResults = false,
                                                           bool includeAbstractTypes = true,
                                                           bool sortAlphabetically = false);

        /**
         * 获取允许此类型强制转换为的所有类型
         * @param pType
         * @return
         */
        static List<TypeID> GetAllCastableTypes(IType const *pType);

        /**
         * 这两种类型是否在同一派生链中（即其中一种派生自另一种）
         * @param typeA 类型A ID
         * @param typeB 类型B ID
         * @return
         */
        static bool AreTypesInTheSameHierarchy(TypeID typeA, TypeID typeB);

        /**
         * 这两种类型是否在同一派生链中（即其中一种派生自另一种）
         * @param pTypeInfoA 类型A 信息
         * @param pTypeInfoB 类型B 信息
         * @return
         */
        static bool AreTypesInTheSameHierarchy(TypeCompositeInfo const *pTypeInfoA, TypeCompositeInfo const *pTypeInfoB);

        //-------------------------------------------------------------------------
        // Enum Info
        //-------------------------------------------------------------------------
        /**
         * 注册枚举类型
         * @param type
         * @return
         */
        static void RegisterEnum(TypeEnumInfo const *type);
        /**
         * 移除注册的枚举类型
         * @param type
         */
        static void UnregisterEnum(TypeEnumInfo const* type);

        /**
         * 返回给定类型ID 的枚举类型信息
         * @param enumID 枚举类型ID
         * @return 枚举类型信息
         */
        static TypeEnumInfo const *GetEnumInfo(TypeID enumID);

		/**
		 * 返回给定类型的枚举类型信息
		 * @tparam T 泛型类型
		 * @return
		 */
		template <typename T, typename = typename TEnableIf<TIsEnum<T>::Value>::Type>
		static String GetEnumString(T value)
		{
			TypeID const enumTypeID = Typeof<T>();
			TypeEnumInfo const * info = GetEnumInfo(enumTypeID);
			return GetEnumToString(info, (int64)value);
		}

        /**
         * 返回给定类型的枚举类型信息
         * @tparam T 泛型类型
         * @return
         */
        template <typename T, typename = typename TEnableIf<TIsEnum<T>::Value>::Type>
        static TypeEnumInfo const *GetEnumInfo()
        {
            TypeID const enumTypeID = Typeof<T>();
            return GetEnumInfo(enumTypeID);
        }

    	//-------------------------------------------------------------------------
    	// Meta Info
    	//-------------------------------------------------------------------------
    	/**
		 * 注册元数据类型
		 * @param pType 类型信息
		 */
    	static void RegisterMeta(TypeMetaInfo const *pType);
    	/**
		 * 移除注册的元数据类型
		 * @param pType 类型信息
		 */
    	static void UnregisterMeta(TypeMetaInfo const *pType);

    	/**
		 * 返回给定类型ID 的元数据类型
		 * @param typeID 类型ID
		 * @return 元数据类型信息
		 */
    	static TypeMetaInfo const *GetMetaTypeInfo(TypeID typeID);

    private:
        static Dictionary<TypeID, TypeCompositeInfo const *> m_RegisteredTypes;
        static Dictionary<TypeID, TypeEnumInfo const*> m_RegisteredEnums;
    	static Dictionary<TypeID, TypeMetaInfo const*> m_RegisteredMetas;
		static Dictionary<StableID, TypeID> m_StableIDToTypeID;
		static String GetEnumToString(TypeEnumInfo const * info, int64 value);
    };

    template <typename T>
	TypeID Typeof()
	{
		TypeID typeID;
		constexpr StableID id = StableID::Generate<T>();
		ENGINE_ASSERT(Types::m_StableIDToTypeID.TryGet(id, typeID))
		return typeID;
	}

	//--------------------------------------------------------------------

	inline bool TypeIsValid(TypeID id)
	{
		return Types::GetTypeInfo(id) != nullptr;
	}

    //---类型注册器---------------------------------------------------------

	class SE_API_RUNTIME ITyper
	{
		friend class Types;
	public:
		virtual ~ITyper() = default;
	protected:
		virtual void RegisterTypes() {}
		virtual void UnregisterTypes() {}
	};

	class SE_API_RUNTIME TypeRegister
	{
	public:
		explicit TypeRegister(TypeRegister* pRegister): getNameFunc(nullptr)
		{
			Types::RegisterTyper(pRegister);
		}

		virtual ITyper* Create() = 0;
		virtual ~TypeRegister() = default;

		StringView GetName() const
		{
			return StringView(getNameFunc());
		}

	protected:
		const Char* (*getNameFunc)();
	};

	template <typename T, const Char* (*GetNameFunc)()>
	class TTypeRegister final : TypeRegister
	{
		friend class Types;
		static_assert(TIsBaseOf<ITyper, T>::Value, "T is not derived from ITyper");
	private:
	public:
		TTypeRegister() : TypeRegister(this)
		{
			getNameFunc = GetNameFunc;
		}

		ITyper* Create() override
		{
			return New<T>();
		}
	};

	// 注册TypeRegister
	#define SE_TYPER_REGISTER(TypeName) 										\
	const Char* Get##TypeName##Name() { return SE_TEXT(#TypeName);	}			\
	::SE::TTypeRegister<TypeName, Get##TypeName##Name> TypeName##Register;		\

}
