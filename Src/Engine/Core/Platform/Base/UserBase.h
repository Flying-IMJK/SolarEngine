#pragma once

#include "Core/Types/Strings/String.h"
//#include "Engine/Scripting/ScriptingObject.h"

namespace SE
{
	/// <summary>
	/// Native platform user object.
	/// </summary>
	class SE_API_CORE UserBase
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
