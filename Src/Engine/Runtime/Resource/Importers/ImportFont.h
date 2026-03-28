#pragma once
#include "Types.h"

namespace SE
{

	class ImportFont
	{
	public:
		/// <summary>
		/// Imports the font file.
		/// </summary>
		/// <param name="context">The importing context.</param>
		/// <returns>Result.</returns>
		static CreateAssetResult Import(CreateAssetContext& context);
	};

} // SE

