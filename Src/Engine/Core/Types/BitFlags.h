#pragma once

#include "Core/Types/Variable.h"
#include "Core/Logging/Logging.h"
#include "initializer_list"
#include <type_traits>

//-------------------------------------------------------------------------
//  Bit Flags
//-------------------------------------------------------------------------
// Generic flag flags type

namespace SE
{
    /*class BitFlags
    {
    public:
	    virtual ~BitFlags() = default;
	    constexpr static uint8 const MaxFlags = 64;
    	uint64 GetFlagMask(uint64 flag) const
		{
			if (m_BitMoveD)
			{
				return flag;
			}
			return (uint64)(1u << flag);
		}

    public:
    	BitFlags() = default;
    	explicit BitFlags(bool m_BitMove) : m_BitMoveD(m_BitMove) {}
    	explicit BitFlags(uint32 flags, bool m_BitMove) : m_flags(flags), m_BitMoveD(m_BitMove) {}
        //-------------------------------------------------------------------------

    	uint64 Get() const { return m_flags; }
    	void Set(uint64 flags) { m_flags = flags; }
    	operator uint64() const { return m_flags; }

        //-------------------------------------------------------------------------
        virtual bool IsBitSet(uint64 flag) const
        {
			if (!m_BitMoveD)
			{
				ENGINE_ASSERT(flag < MaxFlags);
			}
			else
			{
				ENGINE_ASSERT(flag <= Max_uint32);
			}

            return (m_flags & GetFlagMask(flag)) > 0;
        }

        virtual void SetBit(uint64 flag, bool value = true)
        {
			if (!m_BitMoveD)
			{
				ENGINE_ASSERT(flag < MaxFlags);
			}
			else
			{
				ENGINE_ASSERT(flag <= Max_uint32);
			}

    		if (value)
    		{
    			m_flags |
    		}
    		else
    		{
    			m_flags &= ~GetFlagMask(flag);
    		}

            value ? SetFlag(flag) : ClearFlag(flag);
        }

    	void SetAllBit()
        {
            m_flags = 0xFFFFFFFF;
        }

        //-------------------------------------------------------------------------

        virtual bool IsFlagCleared(uint64 flag) const
        {
			if (!m_BitMoveD)
			{
				ENGINE_ASSERT(flag < MaxFlags);
			}
			else
			{
				ENGINE_ASSERT(flag <= Max_uint32);
			}
            return (m_flags & GetFlagMask(flag)) == 0;
        }

        void ClearAllFlags()
        {
            m_flags = 0;
        }

        //-------------------------------------------------------------------------

        virtual void FlipBit(uint64 flag)
        {
			if (!m_BitMoveD)
			{
				ENGINE_ASSERT(flag >= 0 && flag < MaxFlags);
			}
			else
			{
				ENGINE_ASSERT(flag >= 0 && flag < Max_uint32);
			}
            m_flags ^= GetFlagMask(flag);
        }

    	void FlipAllBit()
        {
            m_flags = ~m_flags;
        }

    protected:
		uint64 m_flags = 0;
		bool m_BitMoveD = false;
    };*/
}

//-------------------------------------------------------------------------
//  Templatized Bit Flags
//-------------------------------------------------------------------------
// Helper to create flag flags variables from a specific enum type
namespace SE
{
	// template bit moved enum flags
	template <typename T>
	class EnumFlags
	{
		static_assert(TIsEnum<T>::Value, "EnumFlags only supports enum types");
	private:
		constexpr static uint8 const MaxFlags = 64;
		uint64 m_flags = 0;

	public:
		EnumFlags(std::initializer_list<T> list)
		{
			for (T arg : list)
			{
				m_flags |= (uint64)arg;
			}
		}

		template <typename... Args, class Enable = typename TEnableIf<(... && std::is_convertible_v<Args, T>)>::Type>
		EnumFlags(Args &&...args)
		{
			((m_flags |= (uint64)args), ...);
		}

		EnumFlags &operator=(EnumFlags const &rhs) = default;

		bool operator==(EnumFlags const &rhs) const
		{
			return rhs.m_flags == m_flags;
		}

		bool operator!=(EnumFlags const &rhs) const
		{
			return rhs.m_flags != m_flags;
		}

		uint64 Get() const { return m_flags; }
		void Set(uint64 value) { m_flags = value; }

		//-------------------------------------------------------------------------
		bool Is(T flag) const { return m_flags == (uint64)flag; }
		bool IsFlag(T flag) const { return (m_flags & (uint64)flag) != 0; }
		bool IsFlag(EnumFlags flag) const { return (m_flags & flag.m_flags) != 0; }
		bool IsNotFlag(T flag) const { return (m_flags & (uint64)flag) == 0; }
		bool IsNotFlag(EnumFlags& flag) const { return (m_flags & flag.m_flags) == 0; }

		void Flag(T flag, bool state)
		{
			if (state)
			{
				SetFlag(flag);
			}
			else
			{
				RemoveFlag(flag);
			}
		}
		void SetFlag(T flag) { m_flags |= (uint64)flag; }
		void RemoveFlag(T flag) { m_flags &= ~((uint64)flag); }
		void Marge(const EnumFlags& flag)
		{
			m_flags |= flag.m_flags;
		}

		template <typename... Args>
		void SetFlags(Args &&...args)
		{
			((m_flags |= (uint64)args), ...);
		}

		template <typename... Args>
		bool AllFlagsSet(Args &&...args) const
		{
			uint32 mask = 0;
			((mask |= (uint64)args), ...);

			return (m_flags & mask) != 0;
		}
	};

    //-------------------------------------------------------------------------

    // static_assert(sizeof(TBFlags<enum class Temp>) == sizeof(BitFlags), "TBitFlags is purely syntactic sugar for easy conversion of enums to flags. It must not contain any members!");
	// static_assert(sizeof(EnumFlags<enum class Temp>) == sizeof(BitFlags), "TBitMovedFlags is purely syntactic sugar for easy conversion of enums to flags. It must not contain any members!");
}
