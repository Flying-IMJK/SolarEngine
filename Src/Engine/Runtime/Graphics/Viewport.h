#pragma once

#include "Runtime/API.h"

#include "Runtime/Core/Math/Vector2.h"

namespace SE
{
	struct Rectangle;

	SE_STRUCT(API(InBuild("SE.Viewport")))
	struct SE_API_RUNTIME Viewport
	{
	public:
		union
		{
			struct
			{
				// Position of the pixel coordinate of the upper-left corner of the viewport.
				float X;

				// Position of the pixel coordinate of the upper-left corner of the viewport.
				float Y;
			};

			// Upper left corner location.
			Float2 Location;
		};

		union
		{
			struct
			{
				// Width dimension of the viewport.
				float Width;

				// Height dimension of the viewport.
				float Height;
			};

			// Size
			Float2 Size;
		};

		// Minimum depth of the clip volume.
		float MinDepth;

		// Maximum depth of the clip volume.
		float MaxDepth;

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		Viewport() = default;

		// Init
		// @param x The x coordinate of the upper-left corner of the viewport in pixels
		// @param y The y coordinate of the upper-left corner of the viewport in pixels
		// @param width The width of the viewport in pixels
		// @param height The height of the viewport in pixels
		Viewport(float x, float y, float width, float height)
			: X(x), Y(y), Width(width), Height(height), MinDepth(0.0f), MaxDepth(1.0f)
		{
		}

		// Init
		// @param x The x coordinate of the upper-left corner of the viewport in pixels
		// @param y The y coordinate of the upper-left corner of the viewport in pixels
		// @param width The width of the viewport in pixels
		// @param height The height of the viewport in pixels
		// @param minDepth The minimum depth of the clip volume
		// @param maxDepth The maximum depth of the clip volume
		Viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
			: X(x), Y(y), Width(width), Height(height), MinDepth(minDepth), MaxDepth(maxDepth)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Viewport"/> struct.
		/// </summary>
		/// <param name="size">The viewport size.</param>
		explicit Viewport(const Float2& size)
			: X(0), Y(0), Width(size.X), Height(size.Y), MinDepth(0.0f), MaxDepth(1.0f)
		{
		}

		// Init
		// @param bounds A bounding box that defines the location and size of the viewport in a render target
		Viewport(const Rectangle& bounds);

	public:
		String ToString() const;

	public:
		// Gets the aspect ratio used by the viewport
		// @returns The aspect ratio
		float GetAspectRatio() const
		{
			if (Height != 0.0f)
			{
				return Width / Height;
			}
			return 0.0f;
		}

		// Gets the size of the viewport
		// @eturns The bounds
		Rectangle GetBounds() const;

		// Sets the size of the viewport
		// @param bounds The bounds
		void SetBounds(const Rectangle& bounds);

		// Projects a 3D vector from object space into screen space
		// @param source The vector to project
		// @param vp A combined WorldViewProjection matrix
		// @param vector The projected vector
		void Project(const Float3& source, const Matrix& vp, Float3& result) const;

		// Converts a screen space point into a corresponding point in world space
		// @param source The vector to project
		// @param vp An inverted combined WorldViewProjection matrix
		// @param vector The unprojected vector
		void UnProject(const Float3& source, const Matrix& ivp, Float3& result) const;

	public:
		bool operator==(const Viewport& other) const
		{
			return X == other.X && Y == other.Y && Width == other.Width && Height == other.Height
				&& MinDepth == other.MinDepth && MaxDepth == other.MaxDepth;
		}

		bool operator!=(const Viewport& other) const
		{
			return X != other.X || Y != other.Y || Width != other.Width || Height != other.Height
				|| MinDepth != other.MinDepth || MaxDepth != other.MaxDepth;
		}
	};

}


