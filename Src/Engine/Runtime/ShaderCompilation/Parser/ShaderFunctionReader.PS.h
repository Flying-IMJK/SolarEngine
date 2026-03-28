#pragma once

#include "ShaderFunctionReader.h"


namespace SE::ShaderParser
{
    /// <summary>
    /// Pixel Shaders reader
    /// </summary>
    class PixelShaderFunctionReader : public ShaderFunctionReader<PixelShaderMeta>
    {
    	DECLARE_SHADER_META_READER_HEADER("META_PS", PS);

        PixelShaderFunctionReader()
        {
        }

        ~PixelShaderFunctionReader()
        {
        }
    };
}

