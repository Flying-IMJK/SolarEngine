

#include "ManagedEditor.h"

#include "Runtime/Scripting/Binary/NativeBinaryModule.h"
#include "Runtime/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Scripting/ManagedCLR/CLRMethod.h"

namespace SE
{
	UID ManagedEditor::ObjectID(0x91970b4e, 0x99634f61, 0x84723632, 0x54c776af);


	ManagedEditor::ManagedEditor(): ScriptingObject(SpawnParams(ObjectID, ManagedEditor::TypeInitializer))
	{
		auto editor = ((NativeBinaryModule*)GetBinaryModuleSEEditor())->Assembly;
		editor->Loaded.Bind<ManagedEditor, &ManagedEditor::OnEditorAssemblyLoaded>(this);

	}

	ManagedEditor::~ManagedEditor()
	{
	}

	void ManagedEditor::Init()
	{
		void* args[4];
		CLRClass* clrClass = GetClass();
		if (clrClass == nullptr)
		{
			LOG_FATAL("Scripts", "Invalid Editor assembly! Missing class");
			return;
		}

		const auto initMethod = clrClass->GetMethod("Init");

		if (initMethod == nullptr)
		{
			LOG_FATAL("Scripts", "Invalid Editor assembly! Missing initialization method");
		}

		CLRObject* instance = GetOrCreateManagedInstance();
		if (initMethod == nullptr)
		{
			LOG_FATAL("Scripts", "Failed to create editor instance");
		}

		CLRObject* exception;

		initMethod->Invoke(instance, nullptr, &exception);
	}

	void ManagedEditor::OnEditorAssemblyLoaded(CLRAssembly* assembly)
	{
		ENGINE_ASSERT(!HasManagedInstance());

		// Editor.CSharp.dll has been loaded, let's create managed object for C# editor
		CreateManaged();
	}
} // SE