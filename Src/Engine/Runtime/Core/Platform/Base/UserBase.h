#pragma once

#include "Runtime/Core/Types/Strings/String.h"
//#include "Engine/Scripting/ScriptingObject.h"

namespace SE
{
	/// <summary>
	/// Native platform user object.
	/// </summary>
	class SE_API_RUNTIME UserBase
	{
	protected:
		const String _name;

	public:

		UserBase(const String& name);

		/// <summary>
		/// Gets the username.
		/// </summary>
		String GetName() const;
	};
}
