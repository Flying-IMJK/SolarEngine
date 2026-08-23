#include "ResourceRecord.h"

//-------------------------------------------------------------------------

namespace SGE
{
    ResRecord::~ResRecord()
    {
        ENGINE_ASSERT( m_pRes == nullptr && !HasReferences() );
    }
}