#pragma once
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE::Editor
{
	/// <summary>
	/// Drag event arguments data container.
	/// </summary>
	SE_CLASS(Reflect)
	class DragEventArgs : public IType
	{
		SE_DEFINE_CLASS_DEFAULT(DragEventArgs, IType)
	};
}
