
#include "NativeBinaryModule.h"
#include "Runtime/Scripting/ManagedCLR/SEMethod.h"


namespace SE
{
	NativeBinaryModule::NativeBinaryModule(const StringAnsiView& name)
	: NativeBinaryModule(New<SEAssembly>(nullptr, name))
	{
	}

	NativeBinaryModule::NativeBinaryModule(SEAssembly* assembly)
		: ManagedBinaryModule(assembly)
		, Library(nullptr)
	{
	}

	void NativeBinaryModule::Destroy(bool isReloading)
	{
		ManagedBinaryModule::Destroy(isReloading);

		// Skip native code unloading from core libs
		if (this == GetBinaryModuleCorlib() || this == GetBinaryModuleFlaxEngine())
			return;

		// Release native library
		const auto library = Library;
		if (library)
		{
			Library = nullptr;
			Platform::FreeLibrary(library);
			// Don't do anything after FreeLibrary (this pointer might be invalid)
		}
	}


} // namespace SE
