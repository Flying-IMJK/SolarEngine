
#include "Exception.h"

namespace SE::Log
{
	Exception::~Exception()
	{
		// Always write exception to the log
		AddEntry(m_Severity, m_Category.Get(), "", 0, ToString().Get());
	}
} // SE