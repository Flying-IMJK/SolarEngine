#pragma once
#include "Core/TypeSystem/IType.h"

namespace SE::Editor
{
	/// <summary>
	/// Drag event arguments data container.
	/// </summary>
	class DragEventArgs : public IType
	{
		SE_CLASS_DEFAULT(DragEventArgs, IType)
	};
}
