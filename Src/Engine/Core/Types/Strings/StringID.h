#pragma once

#include "Core/Types/Variable.h"
#include "Core/Types/Strings/String.h"


//-------------------------------------------------------------------------
// String ID
//-------------------------------------------------------------------------
// Deterministic numeric ID generated from a string
// StringIDs are CASE-SENSITIVE!
// Uses the 32bit default hash

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_CORE StringID
    {
    public:
        StringID() = default;
        explicit StringID(Char const *pStr);
        explicit StringID(uint32 ID) : m_ID(ID) {}
        explicit StringID(String const &str);

    	operator uint32() const { return m_ID; }

		StringView ToString() const;

    	bool operator==(StringID const &rhs) const { return m_ID == rhs.m_ID; }
    	bool operator!=(StringID const &rhs) const { return m_ID != rhs.m_ID; }

    	static StringID Invalid;

    protected:
        uint32 m_ID = 0;
    };

	SE_API_CORE __declspec(noinline) const Char* GetStringIDToString(StringID id);

	inline int32 GetHash(StringID id)
	{
		return (uint32)id;
	}
}

//-------------------------------------------------------------------------

namespace std
{
    /*template <>
    struct hash<::SE::StringID>
    {
        uint64 operator()(::SE::StringID const &ID) const { return (uint32)ID; }
    };*/
}

namespace fmt
{
	template<>
	struct SE_API_CORE formatter<::SE::StringID, SE::Char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const ::SE::StringID& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			auto string = v.ToString();
			return fmt::detail::copy_str<::SE::Char>(string.Get(), string.Get() + string.Length(), ctx.out());
		}
	};

	template<>
	struct formatter<::SE::StringID, char>
	{
		template<typename ParseContext>
		auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const ::SE::StringID& v, FormatContext& ctx) -> decltype(ctx.out())
		{
			auto string = v.ToString();
			return fmt::detail::copy_str<char>(string.Get(), string.Get() + string.Length(), ctx.out());
		}
	};
}