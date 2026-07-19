#pragma once
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE
{
	class Control;
}

namespace SE::Editor
{
	/// <summary>
	/// Represents single element of the Custom Editor layout.
	/// </summary>
	SE_CLASS(Reflect)
	class LayoutElement : public IType
	{
		SE_DEFINE_CLASS_DEFAULT(LayoutElement, IType)
	public:
		/// <summary>
		/// Gets the control represented by this element.
		/// </summary>
		virtual Control* GetControl() = 0;
	};
}
