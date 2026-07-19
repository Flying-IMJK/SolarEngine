#pragma once

#include "Runtime/Core/Memory/STLAllocator.h"
#include "fmt/format.h"

namespace SE::fmtExtend
{
	typedef STLAllocator<Char> Allocator;
	typedef STLAllocator<char> Allocator_ansi;

	typedef ::fmt::basic_memory_buffer<Char, ::fmt::inline_buffer_size, Allocator> memory_buffer;
	typedef ::fmt::basic_memory_buffer<char, ::fmt::inline_buffer_size, Allocator_ansi> memory_buffer_ansi;

	template<typename T, typename... Args>
	inline static void format(::fmt::basic_memory_buffer<T, ::fmt::inline_buffer_size, STLAllocator<T>>& buffer, const T* format, const Args& ... args)
	{
		::fmt::detail::vformat_to(buffer, ::fmt::basic_string_view<T>(format), ::fmt::make_format_args<::fmt::buffer_context<T>>(args...), {});
	}
}

#define DEFINE_DEFAULT_FORMATTING(type, formatText, ...) 								\
	namespace fmt 																		\
	{ 																					\
		template<> 																		\
		struct formatter<type, SE::Char> 												\
		{ 																				\
			template<typename ParseContext> 											\
			auto parse(ParseContext& ctx) 												\
			{ 																			\
				return ctx.begin(); 													\
			}                                                     						\
                                                         								\
            template <typename... T>                                        			\
			auto GetArgs(T&... args)													\
			{																			\
				return fmt::make_format_args<buffer_context<SE::Char>>(args...);		\
			}																			\
																						\
			template<typename FormatContext> 											\
			auto format(const type& v, FormatContext& ctx) -> decltype(ctx.out())		\
			{ 																			\
				using detail::get_buffer;												\
				auto&& buf = get_buffer<SE::Char>(ctx.out());							\
				detail::vformat_to(buf, basic_string_view<SE::Char>(SE_TEXT(formatText)), GetArgs(__VA_ARGS__), {});	\
				return detail::get_iterator(buf);																			\
			} 																												\
		}; 																													\
	}

#define DEFINE_DEFAULT_FORMATTING_VIA_TO_STRING(type) \
        namespace fmt \
        { \
            template<> \
            struct formatter<type, char> \
            { \
                template<typename ParseContext> \
                auto parse(ParseContext& ctx) \
                { \
                    return ctx.begin(); \
                } \
                template<typename FormatContext> \
                auto format(const type& v, FormatContext& ctx) -> decltype(ctx.out()) \
                { \
                    const String str = v.ToString(); \
                    return ::fmt::detail::copy_str<char>(str.Get(), str.Get() + str.Length(), ctx.out()); \
                } \
            }; \
        }
