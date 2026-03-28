#pragma once
#include "IBrush.h"
#include "Runtime/Render/2D/SpriteAtlas.h"

namespace SE
{
	/// <summary>
	/// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.Sprite"/>.
	/// </summary>
	/// <seealso cref="IBrush" />
	class SE_API_RUNTIME SpriteBrush : public IBrush
	{
	public:
		/// <summary>
		/// The sprite.
		/// </summary>
		// [ExpandGroups, EditorOrder(0), Tooltip("The sprite.")]
		SpriteHandle Sprite;

		/// <summary>
		/// The texture sampling filter mode.
		/// </summary>
		// [ExpandGroups, EditorOrder(1), Tooltip("The texture sampling filter mode.")]
		BrushFilter Filter = BrushFilter::Linear;

		/// <summary>
		/// Initializes a new instance of the <see cref="SpriteBrush"/> class.
		/// </summary>
		SpriteBrush()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="SpriteBrush"/> struct.
		/// </summary>
		/// <param name="sprite">The sprite.</param>
		SpriteBrush(SpriteHandle sprite)
		{
			Sprite = sprite;
		}

		/// <inheritdoc />
		PRO_GET(Size, SpriteBrush, Float2, __GetSize);

		/// <inheritdoc />
		void Draw(Rectangle rect, Color color);

	private:
		Float2 __GetSize();
	};

} // SE

