
#pragma once

#include "Core/Math/Color.h"
#include "Runtime/API.h"
#include "Core/Math/Matrix.h"
#include "Core/Thread/Threading.h"

namespace SE::Gizmos
{
    constexpr static float s_defaultLineThickness = 1.0f;
    constexpr static float s_defaultPointThickness = 5.0f;

    enum class DepthTest : uint8
    {
        Enable,
        Disable
    };

    enum TextAlignment : uint8
    {
        AlignTopLeft = 0,
        AlignTopCenter,
        AlignTopRight,
        AlignMiddleLeft,
        AlignMiddleCenter,
        AlignMiddleRight,
        AlignBottomLeft,
        AlignBottomCenter,
        AlignBottomRight,
    };

    enum FontSize : uint8
    {
        FontNormal,
        FontSmall
    };

    //-------------------------------------------------------------------------

    struct PointCommand
    {
        PointCommand()
            : position()
              , thickness()
              , color()
        {}

        PointCommand( Float3 const& position, Color const& color, float pointThickness)
            : position( position )
              , thickness( pointThickness )
              , color( color )
        {}

        FORCE_INLINE bool IsTransparent() const { return color.a != 1.0f; }

        Float3      position;
        float       thickness;
        Color32     color;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME LineCommand
    {
        LineCommand()
            : startPosition()
              , startThickness()
              , startColor()
              , endPosition()
              , endThickness()
              , endColor()
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Color const& color, float lineThickness)
            : startPosition( startPosition )
              , startThickness( lineThickness )
              , startColor( color )
              , endPosition( endPosition )
              , endThickness( lineThickness )
              , endColor( color )
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Color const& startColor, Color const& endColor, float lineThickness)
            : startPosition( startPosition )
              , startThickness( lineThickness )
              , startColor( startColor )
              , endPosition( endPosition )
              , endThickness( lineThickness )
              , endColor( endColor )
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Color const& color, float startThickness, float endThickness)
            : startPosition( startPosition )
              , startThickness( startThickness )
              , startColor( color )
              , endPosition( endPosition )
              , endThickness( endThickness )
              , endColor( color )
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Color const& startColor, Color const& endColor, float startThickness, float endThickness)
            : startPosition( startPosition )
              , startThickness( startThickness )
              , startColor( startColor )
              , endPosition( endPosition )
              , endThickness( endThickness )
              , endColor( endColor )
        {}

        FORCE_INLINE bool IsTransparent() const { return startColor.a != 1.0f || endColor.a != 1.0f; }

        Float3      startPosition;
        float       startThickness;
        Color32     startColor;
        Float3      endPosition;
        float       endThickness;
        Color32     endColor;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME TriangleCommand
    {
        TriangleCommand()
            : vertex0()
              , color0()
              , vertex1()
              , color1()
              , vertex2()
              , color2()
        {}

        TriangleCommand( Float3 const& V0, Float3 const& V1, Float3 const& V2, Color const& color)
            : vertex0( V0 )
              , color0( color )
              , vertex1( V1 )
              , color1( color )
              , vertex2( V2 )
              , color2( color )
        {}

        TriangleCommand( Float3 const& V0, Float3 const& V1, Float3 const& V2, Color const& color0, Color const& color1, Color const& color2)
            : vertex0( V0 )
              , color0( color0 )
              , vertex1( V1 )
              , color1( color1 )
              , vertex2( V2 )
              , color2( color2 )
        {}

        FORCE_INLINE bool IsTransparent() const { return color0.a != 1.0f || color1.a != 1.0f || color2.a != 1.0f; }

        Float4      vertex0;
        Color32       color0;
        Float4      vertex1;
        Color32       color1;
        Float4      vertex2;
        Color32       color2;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME TextCommand
    {
        TextCommand() : color()
                        , position()
                        , fontSize()
                        , alignment()
                        , isScreenText(true)
                        , hasBackground()
                        , text()
        {}

        TextCommand( Float2 const& position, Char const* pText, Color const& color, FontSize size, TextAlignment alignment, bool background)
            : color( color )
              , position( position.x, position.y, 0 )
              , fontSize( size )
              , alignment( alignment )
              , isScreenText( true )
              , hasBackground( background )
              , text( pText )
        {}

        TextCommand( Float3 const& position, Char const* pText, Color const& color, FontSize size, TextAlignment alignment, bool background)
            : color( color )
              , position( position )
              , fontSize( size )
              , alignment( alignment )
              , isScreenText( false )
              , hasBackground( background )
              , text( pText )
        {}

        FORCE_INLINE bool IsTransparent() const { return color.a != 1.0f; }

        Color                       color;
        Float3                      position;
        FontSize                    fontSize;
        TextAlignment               alignment;
        bool                        isScreenText;
        bool                        hasBackground;
        String                      text;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME CommandBuffer
    {
        void Append( CommandBuffer const& buffer )
        {
            pointCommands.SetCapacity(pointCommands.Count() + buffer.pointCommands.Count());
            pointCommands.Add(buffer.pointCommands);

            lineCommands.SetCapacity(lineCommands.Count() + buffer.lineCommands.Count());
            lineCommands.Add( buffer.lineCommands);

            triangleCommands.SetCapacity(triangleCommands.Count() + buffer.triangleCommands.Count());
            triangleCommands.Add( buffer.triangleCommands);

            textCommands.SetCapacity(textCommands.Count() + buffer.textCommands.Count());
            textCommands.Add(buffer.textCommands);
        }

        void Clear()
        {
            pointCommands.Clear();
            lineCommands.Clear();
            triangleCommands.Clear();
            textCommands.Clear();
        }

        void Reset();

    public:

        List<PointCommand>       pointCommands;
        List<LineCommand>        lineCommands;
        List<TriangleCommand>    triangleCommands;
        List<TextCommand>        textCommands;
    };

    //-------------------------------------------------------------------------
    // Per-Thread command buffer
    //-------------------------------------------------------------------------
    // These are fully cleared each frame

    class SE_API_RUNTIME ThreadCommandBuffer
    {

    public:

        explicit ThreadCommandBuffer( int64 threadID )
            : m_ID( threadID )
        {}

        FORCE_INLINE int64 GetThreadID() const { return m_ID; }

        FORCE_INLINE void AddCommand( PointCommand&& cmd, DepthTest depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->pointCommands.Add(cmd);
        }

        FORCE_INLINE void AddCommand( LineCommand&& cmd, DepthTest depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->lineCommands.Add(cmd);
        }

        FORCE_INLINE void AddCommand( TriangleCommand&& cmd, DepthTest depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->triangleCommands.Add(cmd);
        }

        FORCE_INLINE void AddCommand( TextCommand&& cmd, DepthTest depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->textCommands.Add(cmd);
        }

        inline void Clear()
        {
            m_opaqueDepthOn.Clear();
            m_opaqueDepthOff.Clear();
            m_transparentDepthOn.Clear();
            m_transparentDepthOff.Clear();
        }

        CommandBuffer const& GetOpaqueDepthTestEnabledBuffer() const { return m_opaqueDepthOn; }
        CommandBuffer const& GetOpaqueDepthTestDisabledBuffer() const { return m_opaqueDepthOff; }
        CommandBuffer const& GetTransparentDepthTestEnabledBuffer() const { return m_transparentDepthOn; }
        CommandBuffer const& GetTransparentDepthTestDisabledBuffer() const { return m_transparentDepthOff; }

    private:

        inline CommandBuffer* GetCommandBuffer( DepthTest depthTestState, bool isTransparent )
        {
            CommandBuffer* pBuffer = nullptr;

            if ( depthTestState == DepthTest::Enable )
            {
                pBuffer = isTransparent ? &m_transparentDepthOn : &m_opaqueDepthOn;
            }
            else // Disable depth test
            {
                pBuffer = isTransparent ? &m_transparentDepthOff : &m_opaqueDepthOff;
            }

            return pBuffer;
        }

    private:

        int64                       m_ID;
        CommandBuffer               m_opaqueDepthOn;
        CommandBuffer               m_opaqueDepthOff;
        CommandBuffer               m_transparentDepthOn;
        CommandBuffer               m_transparentDepthOff;
    };

    //-------------------------------------------------------------------------
    // Frame Buffer
    //-------------------------------------------------------------------------
    // This contains all the commands we need to actually draw this frame
    // Any command with a TTL, will be left in this buffer at the end of the frame to be drawn again

    class SE_API_RUNTIME FrameCommandBuffer
    {
    public:

        void AddThreadCommands( ThreadCommandBuffer const& threadCommands );

        // Empties the command buffer ignoring any TTL state
        FORCE_INLINE void Clear()
        {
            opaqueDepthOn.Clear();
            opaqueDepthOff.Clear();
            transparentDepthOn.Clear();
            transparentDepthOff.Clear();
        }

        // Resets the buffer, will remove all commands with an expired TTL
        FORCE_INLINE void Reset()
        {
            opaqueDepthOn.Reset();
            opaqueDepthOff.Reset();
            transparentDepthOn.Reset();
            transparentDepthOff.Reset();
        }

    public:

        CommandBuffer               opaqueDepthOn;
        CommandBuffer               opaqueDepthOff;
        CommandBuffer               transparentDepthOn;
        CommandBuffer               transparentDepthOff;
    };


    void SE_API_RUNTIME GetFrameCommandBuffer(FrameCommandBuffer& frameCmd);
    void SE_API_RUNTIME ClearCommandBuffer();

    //-------------------------------------------------------------------------
    // Basic Primitives
    //-------------------------------------------------------------------------

    void SE_API_RUNTIME DrawPoint( Float3 const& position, Float4 const& color, float thickness = s_defaultPointThickness, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawLine( Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawLine( Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float lineThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color0, Float4 const& color1, Float4 const& color2, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawWireTriangle( Float3 const& v0, Float3 const& v1, Float3 const& v2, Float4 const& color, float lineThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawText2D( Float2 const& screenPosition, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawText3D( Float3 const& worldPosition, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawTextBox2D( Float2 const& screenPos, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawTextBox3D( Float3 const& worldPos, char const* pText, Float4 const& color, FontSize size = FontSize::FontNormal, TextAlignment alignment = TextAlignment::AlignMiddleLeft, DepthTest depthTestState = DepthTest::Disable);

    //-------------------------------------------------------------------------
    // Boxes / Volumes / Planes
    //-------------------------------------------------------------------------

    void SE_API_RUNTIME DrawPlane( Float4 const& planeEquation, Float4 const& color, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawBox(Float3 const& position, Quaternion const& rotation, Float3 const& size, Color const& color, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawWireBox( Float3 const& position, Quaternion const& rotation, Float3 const& halfsize, Color const& color, float lineThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    //-------------------------------------------------------------------------
    // Sphere / Circle / Cylinder / Capsule
    //-------------------------------------------------------------------------

    void SE_API_RUNTIME DrawCircle( Float3 const& worldPosition, Float3 const& upAxis, float radius, Color const& color, float lineThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawSphere( Float3 const& position, float radius, Color const& color, float lineThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    //-------------------------------------------------------------------------

    // Disc align to the XY plane
    void SE_API_RUNTIME DrawDisc( Float3 const& worldPoint, float radius, Float4 const& color, DepthTest depthTestState = DepthTest::Disable);

    // Cylinder with radius on the XY plane and half-height along Z
    void SE_API_RUNTIME DrawCylinder( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    // Capsule with radius on the XY plane and half-height along Z, total capsule height = 2 * ( halfHeight + radius )
    void SE_API_RUNTIME DrawCapsule( Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    //-------------------------------------------------------------------------
    // Complex Shapes
    //-------------------------------------------------------------------------

    void SE_API_RUNTIME DrawAxis(Float3 const& worldPosition, Float3 const& up, Float3 const& left, float axisLength = 0.05f, float axisThickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    void SE_API_RUNTIME DrawArrow( Float3 const& startPoint, Float3 const& endPoint, Float4 const& color, float thickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    // Draw a cone, originating at the transform, aligned to the -Y axis of the transform
    void SE_API_RUNTIME DrawCone( Transform const& transform, float coneAngle, float length, Float4 const& color, float thickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

    // Draw a cone originating at the start point, aligned to the specified
    void SE_API_RUNTIME DrawCone( Float3 const& startPoint, Float3 const& direction, float coneAngle, float length, Float4 const& color, float thickness = s_defaultLineThickness, DepthTest depthTestState = DepthTest::Disable);

}
