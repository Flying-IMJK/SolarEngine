#pragma once

#include "Logging.h"
#include "Core/Types/Strings/String.h"
#include "Core/Utilities/Time.h"
#include "Core/Utilities/Timers.h"

//-------------------------------------------------------------------------

namespace SE::Log
{
    struct LogEntry
    {
        String      timestamp;
        String      category;
        String      message;
        String      filename;
        uint32      lineNumber;
        Severity    severity;
    };

    class SE_API_CORE Notification
    {
        constexpr static float const s_defaultLifetime = 3000;
        constexpr static float const s_defaultFadeTime = 150;

    public:
        enum class Phase
        {
            FadeIn,
            Wait,
            FadeOut,
            Expired,
        };

        enum class Position
        {
            TopLeft,
            TopCenter,
            TopRight,
            BottomLeft,
            BottomCenter,
            BottomRight,
            Center,
        };

    public:
		Notification() = default;

        Notification(NotificationType type, String &&message)
            : m_type(type), m_message(message)
        {
            ENGINE_ASSERT(type != NotificationType::None);
            m_timer.Start();
        }

        NotificationType GetType() const { return m_type; };

        Phase GetPhase() const;

        float GetFadePercentage() const;

        Float4 GetColor(float opacity = 1.0f) const;

		/*
		NotificationType GetType() const
        {
            switch (m_type)
            {
            case NotificationType::Success:
                return ICON_CHECK_CIRCLE;

            case NotificationType::Warning:
                return ICON_ALERT;

            case NotificationType::Error:
                return ICON_CLOSE_CIRCLE;

            case NotificationType::Info:
                return ICON_INFORMATION;

            default:
                break;
            }

            //-------------------------------------------------------------------------

            ENGINE_UNREACHABLE_CODE();
            return nullptr;
        }*/

        char const *GetTitle() const
        {
            switch (m_type)
            {
            case NotificationType::Success:
                return "Success";

            case NotificationType::Warning:
                return "Warning";

            case NotificationType::Error:
                return "Error";

            case NotificationType::Info:
                return "Info";

            default:
                break;
            }

            //-------------------------------------------------------------------------

            ENGINE_UNREACHABLE_CODE();
            return nullptr;
        };

        Char const *GetMessage() const { return m_message.Get(); };

    private:
        NotificationType m_type = NotificationType::None;
        String m_message;
        Milliseconds m_lifetime = s_defaultLifetime;
        Timer<EngineClock> m_timer;
    };


    SE_API_CORE StringView GetSeverityAsString( Severity severity );

    // Lifetime
    //-------------------------------------------------------------------------

    struct SE_API_CORE System
    {
        // Lifetime
        //-------------------------------------------------------------------------

        static void Initialize();
        static void Shutdown();
        static bool IsInitialized();

        // Accessors
        //-------------------------------------------------------------------------

        static List<LogEntry> const& GetLogEntries();
        static List<Notification> & GetNotifications();
        static int32 GetNumWarnings();
        static int32 GetNumErrors();

        static bool HasFatalErrorOccurred();
        static LogEntry const& GetFatalError();

        // Transfers a list of unhandled warnings and errors - useful for displaying all errors for a given frame.
        // Calling this function will clear the list of warnings and errors.
        static List<LogEntry> GetUnhandledWarningsAndErrors();

        // Output
        //-------------------------------------------------------------------------

        static void SetLogFilePath(String const& logFilePath );
        static void SaveToFile();
    };
}