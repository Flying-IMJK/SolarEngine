#ifndef __QUAD_COMMON__
#define __QUAD_COMMON__

struct Quad_VS2PS
{
    float4 Position : SV_Position;
    noperspective float2 TexCoord : TEXCOORD0;
};

struct Quad_VS2GS
{
    Quad_VS2PS Vertex;
    uint LayerIndex : TEXCOORD1;
};

struct Quad_GS2PS
{
    Quad_VS2PS Vertex;
    uint LayerIndex : SV_RenderTargetArrayIndex;
};

#endif