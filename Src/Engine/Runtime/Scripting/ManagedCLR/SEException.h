#pragma once

#include "Runtime/App.h"
#include "SETypes.h"
#include "Core/Types/Strings/String.h"

namespace SE
{
	/// <summary>
	/// Represents errors that occur during script execution.
	/// </summary>
	class SE_API_RUNTIME SEException
	{
	public:
		/// <summary>
		/// Gets a message that describes the current exception.
		/// </summary>
		String Message;

		/// <summary>
		/// Gets a string representation of the immediate frames on the call stack.
		/// </summary>
		String StackTrace;

		/// <summary>
		/// Gets an inner exception. Null if not used.
		/// </summary>
		SEException* InnerException;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="MException"/> class.
		/// </summary>
		/// <param name="exception">The exception object.</param>
		explicit SEException(SEObject* exception);

		/// <summary>
		/// Disposes a instance of the <see cref="MException"/> class.
		/// </summary>
		~SEException();

	public:
		/// <summary>
		/// Sends exception to the log.
		/// </summary>
		/// <param name="type">The log message type.</param>
		/// <param name="target">Execution target name.</param>
		void Log(const Log::Severity type, const Char* target);
	};

}
