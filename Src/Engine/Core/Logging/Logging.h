#pragma once

#include "Core/API.h"
#include "Core/Utilities/Formatting.h"

//-------------------------------------------------------------------------
// This is the global logging API included throughout the entire engine
//-------------------------------------------------------------------------
// If you want access to the logging system and the engine log include "LoggingSystem.h"

namespace SE::Log
{
    enum class Severity
    {
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4,
    };

    enum class NotificationType
    {
        None,
        Success,
        Warning,
        Error,
        Info,
    };

    // Logging
    //-------------------------------------------------------------------------

    SE_API_CORE void AddEntry(Severity severity, Char const* pCategory, char const* pFilename, int pLineNumber, Char const* pMessage);

	template<typename... T>
	void AddEntry( Severity severity, Char const* pCategory, char const* pFilename, int pLineNumber, Char const* pMessageFormat, T&&... args)
	{
		fmtExtend::Allocator allocator;
		fmtExtend::memory_buffer buffer = fmtExtend::memory_buffer(allocator);
		fmtExtend::format(buffer, pMessageFormat, args...);
		buffer.push_back(0);
		AddEntry(severity, pCategory, pFilename, pLineNumber, buffer.data());
	}

    // Asserts
    //-------------------------------------------------------------------------

    SE_API_CORE void LogAssert( char const* pFile, int line, Char const* pAssertInfo);

    // Trace to Output Log
    //-------------------------------------------------------------------------

    SE_API_CORE void TraceMessage(Char const* pMessage);

    SE_API_CORE void Notify(NotificationType type, const Char *pFormat);

    template<typename... T>
	void Notify(NotificationType type, Char const* pMessageFormat, T&&... args)
    {
		fmtExtend::Allocator allocator;
		fmtExtend::memory_buffer buffer = fmtExtend::memory_buffer(allocator);
		fmtExtend::format(buffer, pMessageFormat, args...);
		buffer.push_back(0);

        Notify(type, buffer.data());
    }

}

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT

#define SE_DEVELOPMENT_LINE_IN_MACRO(x) x
#define SE_DEVELOPMENT_ONLY(x) x

#define STATIC_ASSERT(cond, error) static_assert(cond, error);

#if PLATFORM_CHAR16
#define ENGINE_ASSERT(cond) do { if( !(cond) ) { ::SE::Log::LogAssert( __FILE__, __LINE__, u""); PLATFORM_DEBUG_BREAK; } } while( 0 )
#define ENGINE_UNIMPLEMENTED_FUNCTION() { ::SE::Log::LogAssert( __FILE__, __LINE__, u"Function not implemented!"); PLATFORM_DEBUG_BREAK; }
#define ENGINE_UNREACHABLE_CODE() { ::SE::Log::LogAssert( __FILE__, __LINE__, u"Unreachable code encountered!"); PLATFORM_DEBUG_BREAK; }
#else
#define ENGINE_ASSERT(cond) { if( !(cond) ) { ::SE::Log::LogAssert( __FILE__, __LINE__, L""); PLATFORM_DEBUG_BREAK; } }
#define ENGINE_UNIMPLEMENTED_FUNCTION() { ::SE::Log::LogAssert( __FILE__, __LINE__, L"Function not implemented!"); PLATFORM_DEBUG_BREAK; }
#define ENGINE_UNREACHABLE_CODE() { ::SE::Log::LogAssert( __FILE__, __LINE__, L"Unreachable code encountered!"); PLATFORM_DEBUG_BREAK; }
#endif

#define ENGINE_ASSERT_M(cond, error) { if( !(cond) ){ ::SE::Log::LogAssert( __FILE__, __LINE__, error); PLATFORM_DEBUG_BREAK; } }

#else
#define STATIC_ASSERT(cond, error) static_assert(cond, error);
#define ENGINE_ASSERT(cond) do { (void)sizeof( cond );} while (0)
#define ENGINE_ASSERT_M(cond, error) do { (void)sizeof( cond );} while (0)
#define ENGINE_UNIMPLEMENTED_FUNCTION()
#define ENGINE_UNREACHABLE_CODE()

#define SE_DEVELOPMENT_LINE_IN_MACRO(x)
#define SE_DEVELOPMENT_ONLY(x)

#endif

//-------------------------------------------------------------------------

/**
 * 输出信息 不带文件行信息
 * @param category 消息种类
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_INFO(category, msg, ...) ::SE::Log::AddEntry(::SE::Log::Severity::Info, SE_TEXT(category), "", 0, SE_TEXT(msg), ##__VA_ARGS__)

/**
 * 输出信息 自定义文件行信息
 * @param category 消息种类
 * @param file 文件
 * @param line 行数
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_INFO_LINE(category, file, line, msg, ...) ::SE::Log::AddEntry(::SE::Log::Severity::Info, SE_TEXT(category), file, line, SE_TEXT(msg), ##__VA_ARGS__)

/**
 * 输出信息 带文件行信息
 * @param category 消息种类
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_INFO_LINE_AUTO(category, msg, ...) ::SE::Log::AddEntry(::SE::Log::Severity::Info, SE_TEXT(category), __FILE__, __LINE__, SE_TEXT(msg), ##__VA_ARGS__)


/**
 * 输出警告 不带文件行信息
 * @param category 消息种类
 * @param source
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_WARNING(category, msg, ...) ::SE::Log::AddEntry( ::SE::Log::Severity::Warning, SE_TEXT(category), "", 0, SE_TEXT(msg), ##__VA_ARGS__)
/**
 * 输出信息 自定义文件行信息
 * @param category 消息种类
 * @param file 文件
 * @param line 行数
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_WARNING_LINE(category, file, line, msg, ...) ::SE::Log::AddEntry( ::SE::Log::Severity::Warning, SE_TEXT(category), file, line, SE_TEXT(msg), ##__VA_ARGS__)

/**
 * 输出信息 带文件行信息
 * @param category 消息种类
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_WARNING_LINE_AUTO(category, msg, ...) ::SE::Log::AddEntry(::SE::Log::Severity::Warning, SE_TEXT(category), __FILE__, __LINE__, SE_TEXT(msg), ##__VA_ARGS__)

/**
 * 输出错误
 * @param category 消息种类
 * @param msg 消息
 */
#define LOG_ERROR(category, msg, ...) ::SE::Log::AddEntry( ::SE::Log::Severity::Error, SE_TEXT(category), __FILE__, __LINE__, SE_TEXT(msg), ##__VA_ARGS__)
/**
 * 输出信息
 * @param category 消息种类
 * @param file 文件
 * @param line 行数
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_ERROR_LINE(category, file, line, msg, ...) ::SE::Log::AddEntry(::SE::Log::Severity::Error, SE_TEXT(category), file, line, SE_TEXT(msg), ##__VA_ARGS__)

/**
 * 输出错误 并断点
 * @param msg 消息
 */
#define LOG_FATAL(category, msg, ...)		\
{											\
	if (Platform::IsDebuggerPresent())		\
	{										\
		PLATFORM_DEBUG_BREAK;				\
	}										\
	::SE::Log::AddEntry( ::SE::Log::Severity::Fatal, SE_TEXT(category), __FILE__, __LINE__, SE_TEXT(msg), ##__VA_ARGS__); \
}

/**
 * 输出信息
 * @param file 文件
 * @param line 行数
 * @param msg 消息
 * @param ... 格式化参数
 */
#define LOG_FATAL_LINE(file, line, category, msg, ...)	\
{												\
	if (Platform::IsDebuggerPresent())			\
	{											\
		PLATFORM_DEBUG_BREAK;					\
	}											\
	::SE::Log::AddEntry(::SE::Log::Severity::Fatal, SE_TEXT(category), file, line, SE_TEXT(msg), ##__VA_ARGS__); \
}


/**
 * 信息通知
 * @param msg 格消息
 */
#define NOTIFY_INFO(msg) ::SE::Log::Notify(::SE::Log::NotificationType::Info, msg)
/**
 * 信息通知
 * @param msgFormat 消息
 * @param ... 格式化参数
 */
#define NOTIFY_INFO_FORMAT(msgFormat, ...) ::SE::Log::Notify(::SE::Log::NotificationType::Info, Format(msgFormat, ##__VA_ARGS__))

/**
 * 错误通知
 * @param msg 消息
 */
#define NOTIFY_ERROR(msg) ::SE::Log::Notify(::SE::Log::NotificationType::Error, msg)
/**
 * 信息通知
 * @param msgFormat 消息
 * @param ... 格式化参数
 */
#define NOTIFY_ERROR_FORMAT(msgFormat, ...) ::SE::Log::Notify(::SE::Log::NotificationType::Error, msgFormat, ##__VA_ARGS__)

/**
 * 警告通知
 * @param msg 消息
 */
#define NOTIFY_WARNING(msg) ::SE::Log::Notify(::SE::Log::NotificationType::Warning, msg)
/**
 * 信息通知
 * @param msgFormat 消息
 * @param ... 格式化参数
 */
#define NOTIFY_WARNING_FORMAT(msgFormat, ...) ::SE::Log::Notify(::SE::Log::NotificationType::Warning, msgFormat, ##__VA_ARGS__)

/**
 * 成功通知
 * @param msg 消息
 */
#define NOTIFY_SUCCESS(msg) ::SE::Log::Notify(::SE::Log::NotificationType::Success, msg)
/**
 * 信息通知
 * @param msgFormat 消息
 * @param ... 格式化参数
 */
#define NOTIFY_SUCCESS_FORMAT(msgFormat, ...) ::SE::Log::Notify(::SE::Log::NotificationType::Success, msgFormat, ##__VA_ARGS__)