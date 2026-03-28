#pragma once

#include "Core/Containers/List.h"
#include "Runtime/System/RHI/SystemRHI.h"
#include "Runtime/System/Renderer/RenderSystem.h"


namespace SE
{
    
    class DebugDrawFont
    {
    public:
        void getCharacterTextureRect(const unsigned char character, float &x1, float &y1, float &x2, float &y2);
        RHIImageView* getImageView() const;
        void inialize();
        void destroy();
        
    private:
        const unsigned char m_range_l = 32, m_range_r = 126;
        const int m_singleCharacterWidth = 32;
        const int m_singleCharacterHeight = 64;
        const int m_numOfCharacterInOneLine = 16;
        const int m_numOfCharacter = (m_range_r - m_range_l + 1);
        const int m_bitmap_w = m_singleCharacterWidth * m_numOfCharacterInOneLine;
        const int m_bitmap_h = m_singleCharacterHeight * ((m_numOfCharacter + m_numOfCharacterInOneLine - 1) / m_numOfCharacterInOneLine);

        RHIImage*        m_font_image = nullptr;
        RHIImageView*    m_font_imageView = nullptr;
        RHIDeviceMemory* m_font_imageMemory = nullptr;
        VmaAllocation    m_allocation;

        void loadFont();

    };
}