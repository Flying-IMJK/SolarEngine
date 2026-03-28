#pragma once

#include <cassert>

#include "Core/Compiler.h"
#include "Core/Templates.h"

namespace SE
{
	template<typename HostType, typename ValueType>
	class Property
	{
		using Getter = ValueType (HostType::*)();
		using Setter = void (HostType::*)(ValueType);

		HostType* host;
		Getter getter;
		Setter setter;

		template<typename G, typename S>
			static constexpr bool check_types() {
			static_assert(
				std::is_same_v<G, Getter> || std::is_same_v<G, std::nullptr_t>,
				"Getter type mismatch"
			);
			static_assert(
				std::is_same_v<S, Setter> || std::is_same_v<S, std::nullptr_t>,
				"Setter type mismatch"
			);
			return true;
		}

	public:
		template<typename G, typename S>
		Property(HostType* h, G g, S s)
			: host(h), getter(g), setter(s)
		{
			static_assert(check_types<G, S>(), "Type check failed");
		}

		template<typename G>
		Property(HostType* h, Getter g)
			: host(h), getter(g), setter()
		{
			static_assert(check_types<G>(), "Type check failed");
		}

		template<typename G>
		Property(HostType* h, Setter s)
			: host(h), getter(), setter(s)
		{
			static_assert(check_types<G>(), "Type check failed");
		}

		FORCE_INLINE operator ValueType()
		{
			// "Getter not defined"
			assert(getter != nullptr);

			return (host->*getter)();
		}

		FORCE_INLINE operator ValueType() const
		{
			// "Getter not defined"
			assert(getter != nullptr);

			return (host->*getter)();
		}

		FORCE_INLINE ValueType operator=(const ValueType& value)
		{
			// "Setter not defined"
			assert(setter != nullptr);

			(host->*setter)(const_cast<ValueType&>(value));
			return value;
		}

		FORCE_INLINE ValueType operator=(ValueType&& value)
		{
			// "Setter not defined"
			assert(setter != nullptr);

			(host->*setter)(value);
			return value;
		}

		FORCE_INLINE ValueType Get()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType Get() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType operator->()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType operator->() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}
	};

	template<typename HostType, typename ValueType>
	class PropertyRef
	{
		using Getter = ValueType& (HostType::*)();
		using Setter = void (HostType::*)(ValueType&);

		HostType* host;
		Getter getter;
		Setter setter;

		template<typename G, typename S>
			static constexpr bool check_types() {
			static_assert(
				std::is_same_v<G, Getter> || std::is_same_v<G, std::nullptr_t>,
				"Getter type mismatch"
			);
			static_assert(
				std::is_same_v<S, Setter> || std::is_same_v<S, std::nullptr_t>,
				"Setter type mismatch"
			);
			return true;
		}

	public:
		template<typename G, typename S>
		PropertyRef(HostType* h, G g, S s)
			: host(h), getter(g), setter(s)
		{
			static_assert(check_types<G, S>(), "Type check failed");
		}

		template<typename G>
		PropertyRef(HostType* h, Getter g)
			: host(h), getter(g), setter()
		{
			static_assert(check_types<G>(), "Type check failed");
		}

		template<typename G>
		PropertyRef(HostType* h, Setter s)
			: host(h), getter(), setter(s)
		{
			static_assert(check_types<G>(), "Type check failed");
		}

		FORCE_INLINE operator ValueType&()
		{
			// "Getter not defined"
			assert(getter != nullptr);

			return (host->*getter)();
		}

		FORCE_INLINE operator ValueType&() const
		{
			// "Getter not defined"
			assert(getter != nullptr);

			return (host->*getter)();
		}

		FORCE_INLINE ValueType& operator=(const ValueType& value)
		{
			// "Setter not defined"
			assert(setter != nullptr);

			(host->*setter)(const_cast<ValueType&>(value));
			return const_cast<ValueType&>(value);
		}

		FORCE_INLINE ValueType& operator=(ValueType&& value)
		{
			// "Setter not defined"
			assert(setter != nullptr);

			(host->*setter)(value);
			return value;
		}

		FORCE_INLINE ValueType& Get()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& Get() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& operator->()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& operator->() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}
	};

	template<typename HostType, typename ValueType>
	class PropertyGetOnly
	{
		using Getter = ValueType (HostType::*)();

		HostType* host;
		Getter getter;

		template<typename G>
			static constexpr bool check_types() {
			static_assert(
				std::is_same_v<G, Getter> || std::is_same_v<G, std::nullptr_t>,
				"Getter type mismatch"
			);
			return true;
		}

	public:
		template<typename G>
		PropertyGetOnly(HostType* h, G g)
			: host(h), getter(g)
		{
			static_assert(check_types<G>(), "Type check failed");
		}

		FORCE_INLINE operator ValueType()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE operator ValueType() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType Get()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType Get() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType operator->()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType operator->() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}
	};

	template<typename HostType, typename ValueType>
	class PropertyGetOnlyRef
	{
		using Getter = ValueType& (HostType::*)();

		HostType* host;
		Getter getter;

		template<typename G>
			static constexpr bool check_types() {
			static_assert(
				std::is_same_v<G, Getter> || std::is_same_v<G, std::nullptr_t>,
				"Getter type mismatch"
			);
			return true;
		}

	public:
		template<typename G>
		PropertyGetOnlyRef(HostType* h, G g)
			: host(h), getter(g)
		{
			static_assert(check_types<G>(), "Type check failed");
		}

		FORCE_INLINE operator ValueType&()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE operator ValueType&() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& Get()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& Get() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& operator->()
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}

		FORCE_INLINE ValueType& operator->() const
		{
			// "Getter not defined"
			assert(getter != nullptr);
			return (host->*getter)();
		}
	};

	template<typename HostType, typename ValueType>
	class PropertySetOnly
	{
		using Setter = void (HostType::*)(ValueType);

		HostType* host;
		Setter setter;

		template<typename S>
			static constexpr bool check_types() {
			static_assert(
				std::is_same_v<S, Setter> || std::is_same_v<S, std::nullptr_t>,
				"Setter type mismatch"
			);
			return true;
		}

	public:
		template<typename S>
		PropertySetOnly(HostType* h, S s)
			: host(h), setter(s)
		{
			static_assert(check_types<S>(), "Type check failed");
		}

		inline ValueType operator=(const ValueType& value)
		{
			// "Setter not defined"
			assert(setter != nullptr);

			(host->*setter)(const_cast<ValueType&>(value));
			return value;
		}

		inline ValueType operator=(ValueType& value)
		{
			// "Setter not defined"
			assert(setter != nullptr);

			(host->*setter)(value);
			return value;
		}
	};


	#define PRO(name, HostType, ValueType, getter, setter)	\
		Property<HostType, ValueType> name = {this, &HostType::getter, &HostType::setter}

	#define PRO_REF(name, HostType, ValueType, getter, setter)	\
		PropertyRef<HostType, ValueType> name = {this, &HostType::getter, &HostType::setter}

	#define PRO_GET(name, HostType, ValueType, getter)	\
		PropertyGetOnly<HostType, ValueType> name = {this, &HostType::getter}

	#define PRO_GET_REF(name, HostType, ValueType, getter)	\
		PropertyGetOnlyRef<HostType, ValueType> name = {this, &HostType::getter}

	#define PRO_SET(name, HostType, ValueType, setter)	\
		PropertySetOnly<HostType, ValueType> name = {this, &HostType::setter}

}
