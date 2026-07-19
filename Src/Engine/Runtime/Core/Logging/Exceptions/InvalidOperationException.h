#pragma once
#include "Runtime/Core/Logging/Exception.h"

namespace SE::Log
{
	/// <summary>
	/// The exception that is thrown when a method call is invalid in an object's current state.
	/// </summary>
	class InvalidOperationException : public Exception
	{
	public:

		/// <summary>
		/// Init
		/// </summary>
		InvalidOperationException()
			: InvalidOperationException(String::Empty)
		{
		}

		/// <summary>
		/// Creates default exception with additional data
		/// </summary>
		/// <param name="additionalInfo">Additional information that help describe error</param>
		InvalidOperationException(const String& additionalInfo)
			: Exception(SE_TEXT("Current object didn't exists or its state was invalid."), additionalInfo)
		{
		}
	};
}
