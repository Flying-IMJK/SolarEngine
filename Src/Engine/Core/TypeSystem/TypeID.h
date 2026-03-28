#pragma once
#include "Core/Types/Strings/StringID.h"


namespace SE
{
	class SE_API_CORE TypeID : public StringID
	{
	public:
		TypeID() : TypeID(Invalid) {}
		explicit TypeID(Char const *pStr) : StringID(pStr) {}
		explicit TypeID(uint32 ID) : StringID(ID) {}
		explicit TypeID(String const &str) : StringID(str) {}

		bool IsCoreType() const;

		operator uint32() const { return m_ID; }

		bool operator==(TypeID const &rhs) const { return m_ID == rhs.m_ID; }
		bool operator!=(TypeID const &rhs) const { return m_ID != rhs.m_ID; }

		static TypeID Invalid;
	};

	FORCE_INLINE int32 GetHash(TypeID id)
	{
		return id;
	}

    class SE_API_CORE StableID
	{
    private:
    	uint32 data1;
		uint16 data2;
		uint16 data3;
		uint64 data4;

    public:
    	static StableID Invalid;

		StableID() : StableID(Invalid)
    	{

    	}

    	constexpr StableID(uint32 data1, uint16 data2, uint16 data3, uint64 data4);

    	StableID(StableID& id) : data1(id.data1), data2(id.data2), data3(id.data3), data4(id.data4)
    	{

    	}

    	StableID(const StableID& id)  : data1(id.data1), data2(id.data2), data3(id.data3), data4(id.data4)
    	{

    	}

	    constexpr bool operator==(const StableID& other) const {
    		return data1 == other.data1 && data2 == other.data2 &&
				   data3 == other.data3 && data4 == other.data4;
    	}

    	constexpr bool operator!=(const StableID& other) const {
    		return data1 != other.data1 || data2 != other.data2 ||
				   data3 != other.data3 || data4 != other.data4;
    	}

    	int32 HashCode() const
	    {
    		uint32 hash = GetHash(data1);
    		HashCombine(hash, data2);
    		HashCombine(hash, data3);
    		HashCombine(hash, data4);
    		return hash;
    	}

		template <typename T>
    	static constexpr StableID Generate()
    	{
#ifdef _MSC_VER
    		return GenerateStableTypeID(__FUNCSIG__);
#else
    		return GenerateStableTypeID(__PRETTY_FUNCTION__);
#endif
    	}

    	constexpr StableID static GenerateStableTypeID(const char* typeName)
    	{
    		constexpr auto hash128 = [](const char* str)
    		{
			    // FNV-1a 64-bit 优化的常量
			    constexpr uint64 fnv_offset = 0xCBF29CE484222325ULL;

			    // xxHash3风格的质数常量
			    constexpr uint64 prime1 = 0x9E3779B185EBCA87ULL;
			    constexpr uint64 prime2 = 0xC2B2AE3D27D4EB4FULL;
			    constexpr uint64 prime3 = 0x165667B19E3779F9ULL;
			    constexpr uint64 prime4 = 0x85EBCA77C2B2AE63ULL;
			    constexpr uint64 prime5 = 0x27D4EB2F165667C5ULL;

			    // 初始化两个64位状态
			    uint64 h1 = fnv_offset ^ prime1;
			    uint64 h2 = fnv_offset ^ prime2;

			    // 计算字符串长度
			    int len = 0;
			    while (str[len] != 0) len++;

			    // 处理8字节块
			    int i = 0;
			    for (; i + 8 <= len; i += 8)
			    {
			        // 逐字节组合成64位（避免reinterpret_cast）
			        uint64 chunk = 0;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i]));
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 1])) << 8;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 2])) << 16;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 3])) << 24;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 4])) << 32;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 5])) << 40;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 6])) << 48;
			        chunk |= static_cast<uint64>(static_cast<unsigned char>(str[i + 7])) << 56;

			        // 混合到两个状态
			        h1 ^= chunk;
			        h1 = h1 * prime2;
			        h1 = (h1 << 31) | (h1 >> 33);

			        h2 += chunk;
			        h2 = h2 * prime3;
			        h2 ^= h1;
			    }

			    // 处理剩余的字节
			    uint64 remaining = 0;
			    uint64 remaining_len = 0;
			    for (; i < len; i++)
			    {
			        remaining |= static_cast<uint64>(static_cast<unsigned char>(str[i])) << (remaining_len * 8);
			        remaining_len++;
			    }

			    // 混合剩余字节
			    if (remaining_len > 0)
			    {
			        h1 ^= remaining;
			        h1 = h1 * prime4;
			        h1 = (h1 << 37) | (h1 >> 27);

			        h2 ^= remaining;
			        h2 = h2 * prime5;
			        h2 = (h2 << 23) | (h2 >> 41);
			    }

			    // 长度混合
			    h1 ^= len;
			    h2 ^= static_cast<uint64>(len) << 1;

			    // 最终avalanche混合 (类似mmh3 finalize)
			    h1 ^= h1 >> 33;
			    h1 = h1 * prime1;
			    h1 ^= h1 >> 29;

			    h2 ^= h2 >> 33;
			    h2 = h2 * prime3;
			    h2 ^= h2 >> 29;

			    // 交叉混合
			    h1 ^= h2 >> 18;
			    h2 ^= h1 >> 21;

			    return std::make_pair(h1, h2);
    		};

    		auto [h1, h2] = hash128(typeName);
    		return {
    			static_cast<uint32>(h1),
				static_cast<uint16>(h1 >> 32),
				static_cast<uint16>(h1 >> 48),
				h2
			};
    	}
    };

	constexpr StableID::StableID(uint32 data1, uint16 data2, uint16 data3, uint64 data4): data1(data1), data2(data2), data3(data3), data4(data4)
	{

	}

    FORCE_INLINE int32 GetHash(StableID id)
	{
		return id.HashCode();
	}

}

//-------------------------------------------------------------------------

namespace std
{
    template <>
    struct SE_API_CORE hash<SE::TypeID>
    {
        uint64 operator()(SE::TypeID const &ID) const noexcept
        {
            return ID;
        }
    };
}

namespace fmt
{
	template<>
	struct SE_API_CORE formatter<::SE::TypeID, ::SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const ::SE::TypeID& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			auto string = v.ToString();
			return fmt::detail::copy_str<::SE::Char>(string.Get(), string.Get() + string.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<::SE::TypeID, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const ::SE::TypeID& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			auto string = v.ToString();
			return fmt::detail::copy_str<char>(string.Get(), string.Get() + string.Length(), ctx.out());
		}
	};
}