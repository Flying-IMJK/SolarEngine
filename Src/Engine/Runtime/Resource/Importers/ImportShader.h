#pragma once

#include "Types.h"

namespace SE
{
	/// <summary>
	/// Importing shaders utility
	/// </summary>
	class ImportShader
	{
	public:
		/// <summary>
		/// Imports the shader file.
		/// </summary>
		/// <param name="context">The importing context.</param>
		/// <returns>Result.</returns>
		static CreateAssetResult Import(CreateAssetContext& context);
	};

} // SE
