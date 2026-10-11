#include "LoggingSystem.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Types/DateTime.h"
#include "Runtime/Core/Math/Vector4.h"

#include <iostream>

//-------------------------------------------------------------------------

namespace SE::Log
{
	static Char const *const g_severityLabelChars[] = { SE_TEXT("Message"), SE_TEXT("Warning"), SE_TEXT("Error"), SE_TEXT("Fatal Error")};
	static char const *const g_severityLabelchars[] = { "Message", "Warning", "Error", "Fatal Error"};

	struct LogData
	{
		List<LogEntry> m_logEntries;
		List<LogEntry> m_unhandledWarningsAndErrors;
		String m_logPath;
		CriticalSection m_mutex;
		int32 m_fatalErrorIndex = -1;
		int32 m_numWarnings = 0;
		int32 m_numErrors = 0;
	};

	static LogData *g_pLog = nullptr;
	static List<Notification>* g_pNotifications = nullptr;

    //-------------------------------------------------------------------------

    Notification::Phase Notification::GetPhase() const
    {
    	float const elapsedTime = m_timer.GetElapsedTimeMilliseconds().ToFloat();

    	if (elapsedTime > s_defaultFadeTime + m_lifetime.ToFloat() + s_defaultFadeTime)
    	{
    		return Phase::Expired;
    	}
    	else if (elapsedTime > s_defaultFadeTime + m_lifetime.ToFloat())
    	{
    		return Phase::FadeOut;
    	}
    	else if (elapsedTime > s_defaultFadeTime)
    	{
    		return Phase::Wait;
    	}
    	else
    	{
    		return Phase::FadeIn;
    	}
    }

    float Notification::GetFadePercentage() const
    {
    	Phase const phase = GetPhase();
    	float const elapsedTime = m_timer.GetElapsedTimeMilliseconds().ToFloat();

    	if (phase == Phase::FadeIn)
    	{
    		return elapsedTime / s_defaultFadeTime;
    	}
    	else if (phase == Phase::FadeOut)
    	{
    		return (1.f - ((elapsedTime - s_defaultFadeTime - m_lifetime.ToFloat()) / s_defaultFadeTime));
    	}

    	return 1.f;
    }

    Float4 Notification::GetColor(float opacity) const
    {
    	switch (m_type)
    	{
    	case NotificationType::Success:
    		return Float4(0, 255, 0, opacity); // Green

    	case NotificationType::Warning:
    		return Float4(255, 255, 0, opacity); // Yellow

    	case NotificationType::Error:
    		return Float4(255, 0, 0, opacity); // Error

    	case NotificationType::Info:
    		return Float4(0, 157, 255, opacity); // Blue

    	default:
    		break;
    	}

    	//-------------------------------------------------------------------------

    	ENGINE_UNREACHABLE_CODE();
    	return Float4(255, 255, 255, 255);
    }

	StringView GetSeverityAsString(Severity severity)
    {
        return StringView(g_severityLabelChars[(int32)severity]);
    }

    //-------------------------------------------------------------------------

    void System::Initialize()
    {
        ENGINE_ASSERT(g_pLog == nullptr);
        g_pLog = New<LogData>();
        g_pNotifications = New<List<Notification>>();
    }

    void System::Shutdown()
    {
        ENGINE_ASSERT(g_pLog != nullptr);
        Delete(g_pNotifications);
        Delete(g_pLog);
    }

    bool System::IsInitialized()
    {
        return g_pLog != nullptr && g_pNotifications != nullptr;
    }

    //-------------------------------------------------------------------------

	List<LogEntry> const &System::GetLogEntries()
    {
        ENGINE_ASSERT(IsInitialized());
        return g_pLog->m_logEntries;
    }

	List<Notification> & System::GetNotifications()
    {
        ENGINE_ASSERT(IsInitialized());
        return *g_pNotifications;
    }

    //-------------------------------------------------------------------------

    void System::SetLogFilePath(String const &logFilePath)
    {
        ENGINE_ASSERT(IsInitialized());
        g_pLog->m_logPath = logFilePath;
    }

    void System::SaveToFile()
    {
        ENGINE_ASSERT(IsInitialized());

        String logData;

		Threading::ScopeLock lock(g_pLog->m_mutex);
        for (auto const &entry : g_pLog->m_logEntries)
        {
			logData = String::Format(SE_TEXT("[{0}] {1} >>> {2}: {3}, File: {4}, {5}\r\n"), entry.timestamp, entry.category,
				g_severityLabelChars[(int32)entry.severity], entry.message, entry.filename, entry.lineNumber);
        }

		File::WriteAllText(g_pLog->m_logPath, logData, Encoding::EncodingType::Unicode);
    }

    //-------------------------------------------------------------------------

    bool System::HasFatalErrorOccurred()
    {
        ENGINE_ASSERT(IsInitialized());
        return g_pLog->m_fatalErrorIndex != -1;
    }

    LogEntry const &System::GetFatalError()
    {
        ENGINE_ASSERT(IsInitialized() && g_pLog->m_fatalErrorIndex != -1);
        return g_pLog->m_logEntries[g_pLog->m_fatalErrorIndex];
    }

    //-------------------------------------------------------------------------

    List<LogEntry> System::GetUnhandledWarningsAndErrors()
    {
        ENGINE_ASSERT(IsInitialized());
		Threading::ScopeLock lock(g_pLog->m_mutex);

		List<LogEntry> outEntries = g_pLog->m_unhandledWarningsAndErrors;
        g_pLog->m_unhandledWarningsAndErrors.Clear();
        return outEntries;
    }

    int32 System::GetNumWarnings()
    {
        ENGINE_ASSERT(IsInitialized());
        return g_pLog->m_numWarnings;
    }

    int32 System::GetNumErrors()
    {
        ENGINE_ASSERT(IsInitialized());
        return g_pLog->m_numErrors;
    }

    Delegate<Severity, Char const*> System::g_LogCallback;

	void AddEntry(Severity severity, Char const *pCategory, char const *pFilename, int pLineNumber, Char const *pMessage)
	{
		ENGINE_ASSERT(System::IsInitialized());
		ENGINE_ASSERT(pCategory != nullptr && pFilename != nullptr && pMessage != nullptr);

		{
			Threading::ScopeLock lock(g_pLog->m_mutex);

			if (severity == Severity::Fatal)
			{
				g_pLog->m_fatalErrorIndex = (int32)g_pLog->m_logEntries.Count();
			}

			LogEntry& entry = g_pLog->m_logEntries.AddOne();

			//-------------------------------------------------------------------------

			entry.category = pCategory;
			entry.filename = pFilename;
			entry.lineNumber = pLineNumber;
			entry.severity = severity;
			entry.timestamp.Resize(9);

			// Message
			entry.message = pMessage;

			// Timestamp
			entry.timestamp.Clear();
			
			entry.timestamp.Append( DateTime::NowUTC().ToString());

            // Track unhandled warnings and errors
			//-------------------------------------------------------------------------

			if (entry.severity > Severity::Info)
			{
				g_pLog->m_numWarnings += (entry.severity == Severity::Warning) ? 1 : 0;
				g_pLog->m_numErrors += (entry.severity == Severity::Error) ? 1 : 0;
				g_pLog->m_unhandledWarningsAndErrors.Add(entry);
			}
        }

        if (System::g_LogCallback.IsBinded())
        {
            String traceMessage;
			if (pFilename != nullptr)
			{
				traceMessage = String::Format(SE_TEXT("[{0}] {1}:{2}"), DateTime::NowUTC().ToString(), StringView(pCategory), StringView(pMessage));
			}
			else
			{
				traceMessage = String::Format(SE_TEXT("[{0}] {1}:{2} at {3}:{4}"), DateTime::NowUTC().ToString(), StringView(pCategory), StringView(pMessage), StringAnsiView(pFilename), pLineNumber);
			}

            System::g_LogCallback(severity, traceMessage.Get());
        }

		// Immediate display of log
		//-------------------------------------------------------------------------
		// This uses a less verbose format, if you want more info look at the saved log
		// Print to std out
        #if !BUILD_RELEASE
		StringAnsi traceMessage;
		if (pFilename != nullptr)
		{
			traceMessage = StringAnsi::Format("[{0}][{1}] {2}:{3}", DateTime::NowUTC().ToString(),
				g_severityLabelchars[(int32)severity], StringView(pCategory), StringView(pMessage));
		}
		else
		{
			traceMessage = StringAnsi::Format("[{0}][{1}] {2}:{3} at {4}:{5}", DateTime::NowUTC().ToString(),
				g_severityLabelchars[(int32)severity], StringView(pCategory), StringView(pMessage), StringAnsiView(pFilename), pLineNumber);
		}

		std::cout << traceMessage.Get() << std::endl;
        Platform::Log(traceMessage.ToString());
        #endif

	}

	//-------------------------------------------------------------------------

	void LogAssert(char const *pFile, int32 line, Char const *pAssertInfo)
	{
		if (g_pLog != nullptr)
		{
			AddEntry(Severity::Error, SE_TEXT("Assert"), pFile, line, pAssertInfo);
		}
	}

	void Notify(NotificationType type, const Char *pFormat)
	{
		g_pNotifications->Add(Notification(type, pFormat));
	}
}
