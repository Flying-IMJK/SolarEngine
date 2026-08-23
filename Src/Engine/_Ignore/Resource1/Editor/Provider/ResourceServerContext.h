#pragma once

#include "Core/Types/Strings/String.h"
#include "Editor/Resource/ResourceCompiler.h"

namespace SGE::Editor
{
	struct ResourceServerContext
	{
	public:
		String                        rawResourcePath;
		String                        compiledResourcePath;
		String						  packagedBuildCompiledResourcePath;
		String						  compiledResourceDatabasePath;
		CompilerRegistry const*       pCompiler = nullptr;

		// Set when we shutdown the server to skip processing of any scheduled tasks
		bool                          isExiting = false;
	};
}
