#pragma once

#include "Core/Types/Property.h"
#include "Core/Types/Variable.h"
#include "Core/TypeSystem/IType.h"
#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// Texture brush sampling modes.
	/// </summary>
	enum BrushFilter
	{
		/// <summary>
		/// The point sampling without blending.
		/// </summary>
		Point = 0,

		/// <summary>
		/// The linear color sampling.
		/// </summary>
		Linear = 1,
	};

	/// <summary>
	/// Interface that unifies input source textures, sprites, render targets, and any other brushes to be used in a more generic way.
	/// </summary>
	class SE_API_RUNTIME IBrush : public IType
	{
		SE_CLASS(IBrush, IType)
    protected:
		virtual Float2 __GetSize() = 0;

    public:
		IBrush() = default;
		~IBrush() override = default;

		/// <summary>
		/// Gets the size of the image brush in pixels (if relevant).
		/// </summary>
		PRO_GET(Size, IBrush, Float2, __GetSize);

		/// <summary>
		/// Draws the specified image using <see cref="Render2D"/> graphics backend.
		/// </summary>
		/// <param name="rect">The draw area rectangle.</param>
		/// <param name="color">The color.</param>
		virtual void Draw(Rectangle rect, Color color) = 0;
	};
}
