#pragma once
#include "Runtime/Resource/ResourceID.h"
#include "Core/Types/SGUID.h"
#include "Core/Types/TimeSpan.h"

//-------------------------------------------------------------------------

namespace SGE::Editor
{
    class ResCompilationRequest final
    {
    public:

        enum class Status
        {
            Pending,
            Compiling,
            Succeeded,
            SucceededWithWarnings,
            SucceededUpToDate,
            Failed
        };

        enum class Origin
        {
            External,
			// 手动编译
            ManualCompile,
			// 手动编译强制
            ManualCompileForced,
            FileWatcher,
            Package
        };

    public:

        // Get the resource ID for this request
        inline ResID const& GetResourceID() const { return m_resourceID; }

        // 请求是外部请求还是内部请求(即由于文件更改)
        inline bool IsInternalRequest() const { return m_origin != Origin::External; }

        // 是否需要强制重新编译这个资源，即使它是最新的
        inline bool RequiresForcedRecompiliation() const { return m_origin == Origin::ManualCompileForced || m_origin == Origin::Package; }

        // Status
        inline Status GetStatus() const { return m_status; }
        inline bool IsPending() const { return m_status == Status::Pending; }
        inline bool IsCompiling() const { return m_status == Status::Compiling; }
        inline bool HasSucceeded() const { return m_status == Status::Succeeded || m_status == Status::SucceededWithWarnings || m_status == Status::SucceededUpToDate; }
        inline bool HasFailed() const { return m_status == Status::Failed; }
        inline bool IsComplete() const { return HasSucceeded() || HasFailed(); }

        // Request Info
        inline StringView GetLog() const { return m_log.Get(); }
        inline StringView GetCompilerArgs() const { return m_compilerArgs; }
        inline String const& GetSourceFilePath() const { return m_sourceFile; }
        inline String const& GetDestinationFilePath() const { return m_destinationFile; }

        inline TimeSpan const& GetTimeRequested() const { return m_timeRequested; }

        inline TimeSpan GetCompilationTimeSpan() const
        {
            if ( m_status == Status::Pending )
            {
                return 0;
            }

            if ( !IsComplete() )
            {
                return TimeSpan( Platform::GetTimeSeconds() - m_compilationTimeStarted );
            }

            return TimeSpan( m_compilationTimeFinished - m_compilationTimeStarted );
        }

    public:

        ResID           	              m_resourceID;
        int32                             m_compilerVersion = -1;
        uint64                            m_fileTimestamp = 0;
        uint64                            m_sourceTimestampHash = 0;
        String			                  m_sourceFile;
        String			                  m_destinationFile;
        String                            m_compilerArgs;

        TimeSpan                          m_timeRequested;
        double                       	  m_compilationTimeStarted = 0;
		double                       	  m_compilationTimeFinished = 0;

        String                            m_log;
        Status                            m_status = Status::Pending;
        Origin                            m_origin = Origin::External;
    };
}
