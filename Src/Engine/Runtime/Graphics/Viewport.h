#pragma once

#include "Runtime/API.h"

#include "Core/Math/Vector2.h"

namespace SE
{
	struct Rectangle;

	struct SE_API_RUNTIME Viewport
	{
	public:
		union
		{
			struct
			{
				// Position of the pixel coordinate of the upper-left corner of the viewport.
				float x;

				// Position of the pixel coordinate of the upper-left corner of the viewport.
				float y;
			};

			// Upper left corner location.
			Float2 location;
		};

		union
		{
			struct
			{
				// Width dimension of the viewport.
				float width;

				// Height dimension of the viewport.
				float height;
			};

			// Size
			Float2 size;
		};

		// Minimum depth of the clip volume.
		float minDepth;

		// Maximum depth of the clip volume.
		float maxDepth;

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
			: x(x), y(y), width(width), height(height), minDepth(0.0f), maxDepth(1.0f)
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
			: x(x), y(y), width(width), height(height), minDepth(minDepth), maxDepth(maxDepth)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Viewport"/> struct.
		/// </summary>
		/// <param name="size">The viewport size.</param>
		explicit Viewport(const Float2& size)
			: x(0), y(0), width(size.x), height(size.y), minDepth(0.0f), maxDepth(1.0f)
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
			if (height != 0.0f)
			{
				return width / height;
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
			return x == other.x && y == other.y && width == other.width && height == other.height
				&& minDepth == other.minDepth && maxDepth == other.maxDepth;
		}

		bool operator!=(const Viewport& other) const
		{
			return x != other.x || y != other.y || width != other.width || height != other.height
				|| minDepth != other.minDepth || maxDepth != other.maxDepth;
		}
	};

}


