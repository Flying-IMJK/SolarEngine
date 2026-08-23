#pragma once

#include "ImportedData.h"

//-------------------------------------------------------------------------

namespace SGE::Editor::Import
{
    class SE_API_EDITOR ImportedImage : public ImportedData
    {

    public:

        virtual ~ImportedImage();

        virtual bool IsValid() const override final { return m_pImageData != nullptr; }

        uint8 const* GetImageData() const { return m_pImageData; }

        int32 GetNumChannels() const { return m_channels; }

        int32 GetWidth() const { return m_width; }

        int32 GetHeight() const { return m_height; }

        Int2 GetDimensions() const { return Int2( m_width, m_height ); }

        int32 GetStride() const { return m_stride; }

    protected:

        uint8*    m_pImageData = nullptr;
        int32     m_channels = 0;
        int32     m_width = 0;
        int32     m_height = 0;
        int32     m_stride = 0;
    };
}