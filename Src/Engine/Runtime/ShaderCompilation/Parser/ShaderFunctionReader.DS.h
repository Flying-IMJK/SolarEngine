#pragma once

#include "ShaderFunctionReader.h"

namespace SE::ShaderParser
{
    /// <summary>
    /// Domain Shaders reader
    /// </summary>
    class DomainShaderFunctionReader : public ShaderFunctionReader<DomainShaderMeta>
    {
    	DECLARE_SHADER_META_READER_HEADER("META_DS", DS);

        DomainShaderFunctionReader()
        {
            _childReaders.Add(New<StripLineReader>("domain"));
        }

        ~DomainShaderFunctionReader()
        {
        }
    };
}
