#pragma once

#include "Runtime/Scripting/ScriptingObject.h"

namespace SE
{
	SE_CLASS(API, NoSpawn)
	class ManagedEditor : private ScriptingObject
	{
		SCRIPTING_TYPE_NO_SPAWN(ManagedEditor);

		static UID ObjectID;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="ManagedEditor"/> class.
		/// </summary>
		ManagedEditor();

		/// <summary>
		/// Finalizes an instance of the <see cref="ManagedEditor"/> class.
		/// </summary>
		~ManagedEditor() override;


		void Init();


	private:

		void OnEditorAssemblyLoaded(CLRAssembly* assembly);
	};

} // SE
