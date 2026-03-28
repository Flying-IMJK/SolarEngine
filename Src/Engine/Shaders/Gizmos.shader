
#include "./Shaders/Common.hlsl"
#include "./Shaders/Math.hlsl"

#define kAntialiasing 2.0

//-------------------------------------------------------------------------
 
#if WITH_PICKING
cbuffer EntityID : register( b0 )
{
    uint	m_entityID0;
    uint	m_entityID1;
    uint	m_padding0; // unused
    uint	m_padding1; // unused
    float4	m_padding2; // unused
};
#endif

struct VertexInput
{
    float4   position  : POSITION;
    float4   color     : COLOR;
};

struct VertexOutput
{
    linear        float4    position      : SV_POSITION;
    linear        float4    color         : COLOR;
    linear        float2    uv            : TEXCOORD0;
    linear        float3    viewPos       : TEXCOORD1;
    noperspective float     size          : SIZE;
    noperspective float     edgeDistance  : EDGE_DISTANCE;
};

//-------------------------------------------------------------------------

struct TextVertexInput
{
    float2 position : POSITION;
    float2 uv  : TEXCOORD0;
    float4 color : COLOR0;
};

struct TextVertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv  : TEXCOORD0;
};


struct PixelOutput
{
    float4                  color : SV_Target0;
    #if WITH_PICKING
    uint                    ID[2] : SV_Target1;
    #endif
};

META_CB_BEGIN(0, GizmosViewData)
float4x4  viewMatrix;
float4x4  ProjectionMatrix;
float4x4  viewProjectionMatrix;
float4    viewport;
float3    cameraPosition;
META_CB_END

//-------------------------------------------------------------------------
// Triangles


META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION,  0, R32G32B32A32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,     0, R8G8B8A8_UNorm,  0, ALIGN, PER_VERTEX, 0, true)
VertexOutput Triangles_VS(VertexInput input)
{
    VertexOutput output;
    output.position = mul( viewProjectionMatrix, float4( input.position.xyz, 1.0 ) );
    output.color = input.color;
    output.size = max( input.position.w, kAntialiasing );
    output.edgeDistance = 0;
    output.uv = float2( 0, 0 );
    return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
PixelOutput Triangles_PS( VertexOutput input )
{
    PixelOutput output;

    #if WITH_PICKING
    output.ID[0] = m_entityID0;
    output.ID[1] = m_entityID1;
    #endif

    output.color = input.color;
    return output;
}


//-------------------------------------------------------------------------
// Lines

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION,  0, R32G32B32A32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,     0, R8G8B8A8_UNorm,  0, ALIGN, PER_VERTEX, 0, true)
VertexOutput Lines_VS(VertexInput input)
{
    VertexOutput output;
    output.position = mul( viewProjectionMatrix, float4( input.position.xyz, 1.0 ) );
    output.viewPos = mul( viewMatrix, float4( input.position.xyz, 1.0 ) ).xyz;
    output.color = input.color;
    output.color.a *= smoothstep(0.0, 1.0, input.position.w / kAntialiasing );
    output.size = max( input.position.w, kAntialiasing );
    output.edgeDistance = 0;
    output.uv = float2( 0, 0 );
    return output;
}

float GetLineWorldWidth(float z, float pixelWidth)
{
    float fovY = Deg2Rad(60);
    float screenH = viewport.y;
    return 2.0f * z * tan(fovY * 0.5f) * (pixelWidth / screenH);
}

META_GS(true, FEATURE_LEVEL_SM4)
[maxvertexcount(4)]
void Lines_GS(line VertexOutput input[2], inout TriangleStream<VertexOutput> output)
{
    VertexOutput point0 = input[0];
    VertexOutput point1 = input[1];

    float3 p0 = point0.viewPos;
    float3 p1 = point1.viewPos;

    float2 dir = p0.xy - p1.xy;
    float2 tng = float2( -dir.y, dir.x );

    // 拓展线宽
    float2 offset0 = tng * GetLineWorldWidth(p0.z, point0.size) * 0.4f;
    float2 offset1 = tng * GetLineWorldWidth(p1.z, point1.size) * 0.4f;

    float4 v0L = mul(ProjectionMatrix, float4(float3((p0.xy + offset0), p0.z), 1));
    float4 v0R = mul(ProjectionMatrix, float4(float3((p0.xy - offset0), p0.z), 1));
    float4 v1L = mul(ProjectionMatrix, float4(float3((p1.xy + offset1), p1.z), 1));
    float4 v1R = mul(ProjectionMatrix, float4(float3((p1.xy - offset1), p1.z), 1));


/*     VertexOutput point0 = input[0];
    VertexOutput point1 = input[1];

    float2 pos0 = point0.position.xy / point0.position.w;
    float2 pos1 = point1.position.xy / point1.position.w;

    float2 dir = pos0 - pos1;
    dir = normalize( float2( dir.x, dir.y * viewport.x / viewport.y) ); // correct for aspect ratio
    float2 tng0 = float2( -dir.y, dir.x );
    float2 tng1 = tng0 * point1.size / viewport.xy;
    tng0 = tng0 * point0.size / viewport.xy; */

    VertexOutput vert;

    // line start
    vert.size = point0.size;
    vert.color = point0.color;
    vert.uv = float2( 0.0, 0.0 );

    vert.position = v0L; //float4( ( pos0 - tng0 ) * point0.position.w, point0.position.zw );
    vert.edgeDistance = -point0.size;
    output.Append( vert );

    vert.position = v0R; //float4( ( pos0 + tng0 ) * point0.position.w, point0.position.zw );
    vert.edgeDistance = point0.size;
    output.Append( vert );

    // line end
    vert.size = point1.size;
    vert.color = point1.color;
    vert.uv = float2( 1.0, 1.0 );

    vert.position = v1L; //float4( ( pos1 - tng1 ) * point1.position.w, point1.position.zw );
    vert.edgeDistance = -point1.size;
    output.Append( vert );

    vert.position = v1R; //float4( ( pos1 + tng1 ) * point1.position.w, point1.position.zw );
    vert.edgeDistance = point1.size;
    output.Append( vert );
}


META_PS(true, FEATURE_LEVEL_ES2)
PixelOutput Lines_PS( VertexOutput input )
{
    float d = abs( input.edgeDistance ) / input.size;
    d = smoothstep( 1.0, 1.0 - ( kAntialiasing / input.size ), d );

    PixelOutput output;

    #if WITH_PICKING
    output.ID[0] = m_entityID0;
    output.ID[1] = m_entityID1;
    #endif

    output.color = input.color;
    output.color.a *= d;
    return output;
}


//-------------------------------------------------------------------------
// Point

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION,  0, R32G32B32A32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,     0, R8G8B8A8_UNorm,  0, ALIGN, PER_VERTEX, 0, true)
VertexOutput Point_VS(VertexInput input)
{
    VertexOutput output;
    output.position = mul( viewProjectionMatrix, float4( input.position.xyz, 1.0 ) );
    output.color = input.color;
    output.color.a *= smoothstep(0.0, 1.0, input.position.w / kAntialiasing );
    output.size = max( input.position.w, kAntialiasing );
    output.edgeDistance = 0;
    output.uv = float2( 0, 0 );
    return output;
}


META_GS(true, FEATURE_LEVEL_SM4)
[maxvertexcount(4)]
void Point_GS( point VertexOutput input[1], inout TriangleStream<VertexOutput> output )
{
    VertexOutput vert;

    float2 scale = 1.0 / viewport.xy * input[0].size;
    vert.size = input[0].size;
    vert.color = input[0].color;
    vert.edgeDistance = input[0].edgeDistance;

    vert.position = float4( input[0].position.xy + float2( -1.0, -1.0 ) * scale * input[0].position.w, input[0].position.zw );
    vert.uv = float2( 0.0, 0.0 );
    output.Append( vert );

    vert.position = float4( input[0].position.xy + float2( 1.0, -1.0 ) * scale * input[0].position.w, input[0].position.zw );
    vert.uv = float2( 1.0, 0.0 );
    output.Append( vert );

    vert.position = float4( input[0].position.xy + float2( -1.0, 1.0 ) * scale * input[0].position.w, input[0].position.zw );
    vert.uv = float2( 0.0, 1.0 );
    output.Append( vert );

    vert.position = float4( input[0].position.xy + float2( 1.0, 1.0 ) * scale * input[0].position.w, input[0].position.zw );
    vert.uv = float2( 1.0, 1.0 );
    output.Append( vert );
}

META_PS(true, FEATURE_LEVEL_ES2)
PixelOutput Point_PS( VertexOutput input )
{
    float d = abs( input.edgeDistance ) / input.size;
    d = smoothstep( 1.0, 1.0 - ( kAntialiasing / input.size ), d );

    PixelOutput output;

    #if WITH_PICKING
    output.ID[0] = m_entityID0;
    output.ID[1] = m_entityID1;
    #endif

    output.color = input.color;
    output.color.a *= d;
    return output;
}


//-------------------------------------------------------------------------
// Text

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION,  0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(UV,        0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,     0, R8G8B8A8_UNorm,  0, ALIGN, PER_VERTEX, 0, true)
TextVertexOutput Text_VS( TextVertexInput input )
{
    float4 pointCS = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Convert to clipspace
    //-------------------------------------------------------------------------

    // To Normalized pixel space
    pointCS.x = input.position.x / viewport.x;
    pointCS.y = input.position.y / viewport.y;

    // Invert Y
    pointCS.y = 1.0f - pointCS.y;

    // Convert from [0,1] to [-1,1]
    pointCS.x = ( pointCS.x * 2 ) - 1.0f;
    pointCS.y = ( pointCS.y * 2 ) - 1.0f;

    // Set PS input
    //-------------------------------------------------------------------------

    TextVertexOutput output;
    output.position = pointCS;
    output.color = input.color;
    output.uv = input.uv;
    return output;
}


Texture2D texture0 : register(t0);

META_PS(true, FEATURE_LEVEL_ES2)
PixelOutput Text_PS( TextVertexOutput input )
{
    PixelOutput output;

    #if WITH_PICKING
    output.m_ID[0] = m_entityID0;
    output.m_ID[1] = m_entityID1;
    #endif

    output.color = input.color * SAMPLE_RT_LINEAR(texture0, input.uv);
    return output;
}