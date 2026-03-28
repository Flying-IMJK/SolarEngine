#pragma once

#include "ManagedBinaryModule.h"

namespace SE
{
	/// <summary>
	/// The C# and C++ scripting assembly container that holds the native types information and supports interop with managed runtime.
	/// </summary>
    class SE_API_RUNTIME NativeBinaryModule : public ManagedBinaryModule
    {
    public:

    	/// <summary>
    	/// Initializes a new instance of the <see cref="NativeBinaryModule" /> class.
    	/// </summary>
    	/// <param name="name">The module name.</param>
    	NativeBinaryModule(const StringAnsiView& name);

    	/// <summary>
    	/// Initializes a new instance of the <see cref="NativeBinaryModule" /> class.
    	/// </summary>
    	/// <param name="assembly">The managed assembly. Object will be deleted within the scripting assembly.</param>
    	explicit NativeBinaryModule(SEAssembly* assembly);

    public:

    	/// <summary>
    	/// The native library (C++ DLL).
    	/// </summary>
    	void* Library;

    public:

    	// [ManagedBinaryModule]
    	void Destroy(bool isReloading) override;
    };

} // namespace SE
