#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Object.h"

namespace SE::Log
{
	/// <summary>
	/// Represents errors that occur during application execution.
	/// </summary>
	class SE_API_RUNTIME Exception : public Object
	{
	protected:
		String m_Category;
		String m_Message;
		String m_AdditionalInfo;
		Severity m_Severity;

	public:
		/// <summary>
		/// Creates default exception without additional data
		/// </summary>
		Exception(): Exception(String::Empty)
		{
		}

		/// <summary>
		/// Creates default exception with additional data
		/// </summary>
		/// <param name="additionalInfo">Additional information that help describe error</param>
		Exception(const String& additionalInfo)
			: Exception(String(SE_TEXT("An exception has occurred.")), additionalInfo)
		{
		}

		/// <summary>
		/// Creates new custom message with additional data
		/// </summary>
		/// <param name="message">The message that describes the error.</param>
		/// <param name="additionalInfo">Additional information that help describe error</param>
		Exception(const String& message, const String& additionalInfo)
			: m_Message(message), m_AdditionalInfo(additionalInfo), m_Severity(Log::Severity::Warning)
		{
		}

	public:
		/// <summary>
		/// Virtual destructor
		/// </summary>
		virtual ~Exception();

	public:
		/// <summary>
		/// Gets a message that describes the current exception.
		/// </summary>
		/// <returns>Message string.</returns>
		FORCE_INLINE const String& GetMessage() const
		{
			return m_Message;
		}

		/// <summary>
		/// Gets a message that describes the current exception.
		/// </summary>
		/// <returns>Message string.</returns>
		FORCE_INLINE String GetMessage()
		{
			return m_Message;
		}

		/// <summary>
		/// Gets a additional info that describes the current exception details.
		/// </summary>
		/// <returns>Message string.</returns>
		FORCE_INLINE const String& GetAdditionalInfo() const
		{
			return m_AdditionalInfo;
		}

		/// <summary>
		/// Gets a additional info that describes the current exception details.
		/// </summary>
		/// <returns>Message string.</returns>
		FORCE_INLINE String GetAdditionalInfo()
		{
			return m_AdditionalInfo;
		}

		/// <summary>
		/// Get exception level
		/// </summary>
		/// <returns>Message Type used for display log</returns>
		FORCE_INLINE const Severity& GetSeverity() const
		{
			return m_Severity;
		}

		/// <summary>
		/// Get exception level
		/// </summary>
		/// <returns>Message Type used for display log</returns>
		FORCE_INLINE Severity GetSeverity()
		{
			return m_Severity;
		}

		/// <summary>
		/// Override exception level
		/// </summary>
		/// <param name="level">Override default value of exception level</param>
		Exception& SetSeverity(Severity level)
		{
			m_Severity = level;
			return *this;
		}

		// TODO: implement StackTrace caching: https://www.codeproject.com/kb/threads/stackwalker.aspx

	public:
		// [Object]
		String ToString() const final override
		{
			if (!m_AdditionalInfo.IsEmpty())
			{
				return GetMessage() + SE_TEXT(" \n\n Additional info: ") + GetAdditionalInfo();
			}

			return GetMessage();
		}
	};

} // SE
