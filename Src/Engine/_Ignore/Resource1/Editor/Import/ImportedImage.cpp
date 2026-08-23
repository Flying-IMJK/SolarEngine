#include "ImportedImage.h"
#include "Core/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace SGE::Editor::Import
{
    ImportedImage::~ImportedImage()
    {
        PlatformAllocator::Free(m_pImageData);
    }
}