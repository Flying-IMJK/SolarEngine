#pragma once

namespace SE::Editor
{
	/// <summary>
	/// The floating point value editor element.
	/// </summary>
	class IFloatValueElement
	{
	public:
		/// <summary>
		/// Gets or sets the value.
		/// </summary>
		virtual float GetValue() = 0;
		virtual void SetValue(float value) = 0;

		/// <summary>
		/// Gets a value indicating whether user is using a slider.
		/// </summary>
		virtual bool IsSliding() = 0;

		/// <summary>
		/// Sets the editor limits from member <see cref="LimitAttribute"/>.
		/// </summary>
		/// <param name="limit">The limit.</param>
		// void SetLimits(LimitAttribute limit);
	};

}
