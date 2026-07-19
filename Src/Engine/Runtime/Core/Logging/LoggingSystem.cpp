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

    /*    
    void NotificationSystem::Render()
    {
        constexpr static float const paddingX = 20.0f;             // Bottom-left X padding
        constexpr static float const paddingY = 20.0f;             // Bottom-left Y padding
        constexpr static float const paddingNotificationY = 10.0f; // Padding Y between each message

        //-------------------------------------------------------------------------

        ImGuiViewport const *pViewport = ImGui::GetMainViewport();
        ImVec2 const viewportSize = pViewport->Size;

        float notificationStartPosY = 0.f;
        for (auto i = 0; i < g_notifications.size(); i++)
        {
            Notification *pNotification = &g_notifications[i];

            // Remove notification if expired
            //-------------------------------------------------------------------------

            if (pNotification->GetPhase() == Notification::Phase::Expired)
            {
                g_notifications.erase(g_notifications.begin() + i);
                i--;
                continue;
            }

            // Preparation
            //-------------------------------------------------------------------------

            // Get icon, title and other data
            char const *pIcon = pNotification->GetIcon();
            char const *pTitle = pNotification->GetTitle();
            char const *pMessage = pNotification->GetMessage();
            float const opacity = pNotification->GetFadePercentage(); // Get opacity based of the current phase
            ImVec4 const textColor = pNotification->GetColor(opacity);

            // Generate new unique name for this notification
            String windowName = StringFormat("##Notification{0}", i);

            // Draw Notification
            //-------------------------------------------------------------------------

            ImGui::SetNextWindowBgAlpha(opacity);
            ImGui::SetNextWindowPos(pViewport->Pos + ImVec2(viewportSize.x - paddingX, viewportSize.y - paddingY - notificationStartPosY), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
            ImGui::PushStyleColor(ImGuiCol_Border, textColor);
            if (ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing))
            {
                ImGui::PushTextWrapPos(viewportSize.x / 3.f); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

                bool drawSeparator = false;

                // If an icon is set
                if (pIcon != nullptr)
                {
                    ImGui::TextColored(textColor, pIcon);
                    drawSeparator = true;
                }

                if (pTitle != nullptr)
                {
                    if (pIcon != nullptr)
                    {
                        ImGui::SameLine();
                    }

                    ImGui::Text(pTitle); // Render default title text (Success -> "Success", etc...)
                    drawSeparator = true;
                }

                // In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks centered vertically
                if (drawSeparator && pMessage != nullptr)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f); // Must be a better way to do this!!!!
                }

                // If a content is set
                if (pMessage != nullptr)
                {
                    if (drawSeparator)
                    {
                        ImGui::Separator();
                    }

                    ImGui::Text(pMessage); // Render content text
                }

                ImGui::PopTextWrapPos();
            }

            // Save height for next notifications
            notificationStartPosY += ImGui::GetWindowHeight() + paddingNotificationY;

            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
    }
    */


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
			DateTime dateTime = DateTime::NowUTC();
			entry.timestamp.Append(dateTime.ToString());

			// Immediate display of log
			//-------------------------------------------------------------------------
			// This uses a less verbose format, if you want more info look at the saved log

			StringAnsi traceMessage;
			if (entry.filename.IsEmpty())
			{
				traceMessage = StringAnsi::Format("[{0}][{1}][{2}] {3}", entry.timestamp,
					g_severityLabelchars[(int32)entry.severity], entry.category, entry.message);
			}
			else
			{
				traceMessage = StringAnsi::Format("[{0}][{1}][{2}] {3} at {4}:{5}", entry.timestamp,
					g_severityLabelchars[(int32)entry.severity], entry.category, entry.message, entry.filename, entry.lineNumber);
			}


			// Print to std out
			std::cout << traceMessage.Get() << std::endl;

			// Track unhandled warnings and errors
			//-------------------------------------------------------------------------

			if (entry.severity > Severity::Info)
			{
				g_pLog->m_numWarnings += (entry.severity == Severity::Warning) ? 1 : 0;
				g_pLog->m_numErrors += (entry.severity == Severity::Error) ? 1 : 0;
				g_pLog->m_unhandledWarningsAndErrors.Add(entry);
			}
		}
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