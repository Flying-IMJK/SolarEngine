#pragma once

#include "Runtime/API.h"
#include "Core/Math/Math.h"
#include "Core/Math/NumericRange.h"
#include "Core/Math/Vector2.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/BitFlags.h"
#include "Core/Thread/Threading.h"
#include "Core/Tools/Time.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SE::Drawing
{
    enum class DepthTest : uint8
    {
        Enable,
        Disable
    };

    //-------------------------------------------------------------------------

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
		PointCommand() : m_position(), m_thickness(), m_color(), m_TTL()
		{
		}

        PointCommand(Float3 const &position, Float4 const &color, float pointThickness, Seconds TTL)
            : m_position(position), m_thickness(pointThickness), m_color(color), m_TTL(TTL)
        {
        }

        inline bool IsTransparent() const { return m_color.Raw[3] != 1.0f; }

        Float3 m_position;
        float m_thickness;
        Float4 m_color;
        Seconds m_TTL;
    };

    //-------------------------------------------------------------------------

    struct LineCommand
    {
		LineCommand() : m_startPosition(), m_startThickness(), m_startColor(), m_endPosition(), m_endThickness(), m_endColor(), m_TTL()
		{
		}

        LineCommand(Float3 const &startPosition, Float3 const &endPosition, Float4 const &color, float lineThickness, Seconds TTL)
            : m_startPosition(startPosition), m_startThickness(lineThickness), m_startColor(color), m_endPosition(endPosition), m_endThickness(lineThickness), m_endColor(color), m_TTL(TTL)
        {
        }

        LineCommand(Float3 const &startPosition, Float3 const &endPosition, Float4 const &startColor, Float4 const &endColor, float lineThickness, Seconds TTL)
            : m_startPosition(startPosition), m_startThickness(lineThickness), m_startColor(startColor), m_endPosition(endPosition), m_endThickness(lineThickness), m_endColor(endColor), m_TTL(TTL)
        {
        }

        LineCommand(Float3 const &startPosition, Float3 const &endPosition, Float4 const &color, float startThickness, float endThickness, Seconds TTL)
            : m_startPosition(startPosition), m_startThickness(startThickness), m_startColor(color), m_endPosition(endPosition), m_endThickness(endThickness), m_endColor(color), m_TTL(TTL)
        {
        }

        LineCommand(Float3 const &startPosition, Float3 const &endPosition, Float4 const &startColor, Float4 const &endColor, float startThickness, float endThickness, Seconds TTL)
            : m_startPosition(startPosition), m_startThickness(startThickness), m_startColor(startColor), m_endPosition(endPosition), m_endThickness(endThickness), m_endColor(endColor), m_TTL(TTL)
        {
        }

        inline bool IsTransparent() const { return m_startColor.Raw[3] != 1.0f || m_endColor.Raw[3] != 1.0f; }

        Float3 m_startPosition;
        float m_startThickness;
        Float4 m_startColor;
        float m_padding; // Needed for VB upload - since each command is 2 vertices
        Float3 m_endPosition;
        float m_endThickness;
        Float4 m_endColor;
        Seconds m_TTL;
    };

    //-------------------------------------------------------------------------

    struct TriangleCommand
    {
		TriangleCommand() : m_vertex0(), m_color0(), m_vertex1(), m_color1(), m_vertex2(), m_color2(), m_TTL()
		{
		}

        TriangleCommand(Float3 const &V0, Float3 const &V1, Float3 const &V2, Float4 const &color, Seconds TTL)
            : m_vertex0(V0), m_color0(color), m_vertex1(V1), m_color1(color), m_vertex2(V2), m_color2(color), m_TTL(TTL)
        {
        }

        TriangleCommand(Float3 const &V0, Float3 const &V1, Float3 const &V2, Float4 const &color0, Float4 const &color1, Float4 const &color2, Seconds TTL)
            : m_vertex0(V0), m_color0(color0), m_vertex1(V1), m_color1(color1), m_vertex2(V2), m_color2(color2), m_TTL(TTL)
        {
        }

        inline bool IsTransparent() const { return m_color0.Raw[3] != 1.0f || m_color1.Raw[3] != 1.0f || m_color2.Raw[3] != 1.0f; }

        Float4 m_vertex0;
        Float4 m_color0;
        float m_padding0; // Needed for VB upload - since each command is 3 vertices
        Float4 m_vertex1;
        Float4 m_color1;
        float m_padding1; // Needed for VB upload - since each command is 3 vertices
        Float4 m_vertex2;
        Float4 m_color2;
        Seconds m_TTL;
    };

    //-------------------------------------------------------------------------

    struct TextCommand
    {
		TextCommand()
			: m_color(), m_position(), m_fontSize(), m_alignment(), m_isScreenText(true), m_hasBackground(), m_text(), m_TTL()
		{
		}

        TextCommand(Float2 const &position, char const *pText, Float4 const &color, FontSize size, TextAlignment alignment, bool background, Seconds TTL)
            : m_color(color), m_position(position.x, position.y, 0), m_fontSize(size), m_alignment(alignment), m_isScreenText(true), m_hasBackground(background), m_text(pText), m_TTL(TTL)
        {
        }

        TextCommand(Float3 const &position, char const *pText, Float4 const &color, FontSize size, TextAlignment alignment, bool background, Seconds TTL)
            : m_color(color), m_position(position), m_fontSize(size), m_alignment(alignment), m_isScreenText(false), m_hasBackground(background), m_text(pText), m_TTL(TTL)
        {
        }

        inline bool IsTransparent() const { return m_color.Raw[3] != 1.0f; }

        Float4 m_color;
        Float3 m_position;
        FontSize m_fontSize;
        TextAlignment m_alignment;
        bool m_isScreenText;
        bool m_hasBackground;
        String m_text;
        Seconds m_TTL;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME CommandBuffer
    {
        inline void Append(CommandBuffer const &buffer)
        {
            m_pointCommands.Resize(m_pointCommands.Count() + buffer.m_pointCommands.Count());
            m_pointCommands.Add(m_pointCommands);

            m_lineCommands.Resize(m_lineCommands.Count() + buffer.m_lineCommands.Count());
            m_lineCommands.Add(m_lineCommands);

            m_triangleCommands.Resize(m_triangleCommands.Count() + buffer.m_triangleCommands.Count());
            m_triangleCommands.Add(buffer.m_triangleCommands);

            m_textCommands.Resize(m_textCommands.Count() + buffer.m_textCommands.Count());
            m_textCommands.Add(m_textCommands);
        }

        inline void Clear()
        {
            m_pointCommands.Clear();
            m_lineCommands.Clear();
            m_triangleCommands.Clear();
            m_textCommands.Clear();
        }

        void Reset(Seconds deltaTime);

    public:
        List<PointCommand> m_pointCommands;
		List<LineCommand> m_lineCommands;
		List<TriangleCommand> m_triangleCommands;
		List<TextCommand> m_textCommands;
    };

    //-------------------------------------------------------------------------
    // Per-Thread command buffer
    //-------------------------------------------------------------------------
    // These are fully cleared each frame

    class SE_API_RUNTIME ThreadCommandBuffer
    {

    public:
        ThreadCommandBuffer(int64 threadID)
            : m_ID(threadID)
        {
        }

        inline int64 GetThreadID() const { return m_ID; }

        inline void AddCommand(PointCommand &&cmd, DepthTest depthTestState)
        {
            CommandBuffer *pBuffer = GetCommandBuffer(depthTestState, cmd.IsTransparent());
            pBuffer->m_pointCommands.Add(std::move(cmd));
        }

        inline void AddCommand(LineCommand &&cmd, DepthTest depthTestState)
        {
            CommandBuffer *pBuffer = GetCommandBuffer(depthTestState, cmd.IsTransparent());
            pBuffer->m_lineCommands.Add(std::move(cmd));
        }

        inline void AddCommand(TriangleCommand &&cmd, DepthTest depthTestState)
        {
            CommandBuffer *pBuffer = GetCommandBuffer(depthTestState, cmd.IsTransparent());
            pBuffer->m_triangleCommands.Add(std::move(cmd));
        }

        inline void AddCommand(TextCommand &&cmd, DepthTest depthTestState)
        {
            CommandBuffer *pBuffer = GetCommandBuffer(depthTestState, cmd.IsTransparent());
            pBuffer->m_textCommands.Add(std::move(cmd));
        }

        inline void Clear()
        {
            m_opaqueDepthOn.Clear();
            m_opaqueDepthOff.Clear();
            m_transparentDepthOn.Clear();
            m_transparentDepthOff.Clear();
        }

        CommandBuffer const &GetOpaqueDepthTestEnabledBuffer() const { return m_opaqueDepthOn; }
        CommandBuffer const &GetOpaqueDepthTestDisabledBuffer() const { return m_opaqueDepthOff; }
        CommandBuffer const &GetTransparentDepthTestEnabledBuffer() const { return m_transparentDepthOn; }
        CommandBuffer const &GetTransparentDepthTestDisabledBuffer() const { return m_transparentDepthOff; }

    private:
        inline CommandBuffer *GetCommandBuffer(DepthTest depthTestState, bool isTransparent)
        {
            CommandBuffer *pBuffer = nullptr;

            if (depthTestState == DepthTest::Enable)
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
		int64 m_ID;
        CommandBuffer m_opaqueDepthOn;
        CommandBuffer m_opaqueDepthOff;
        CommandBuffer m_transparentDepthOn;
        CommandBuffer m_transparentDepthOff;
    };

    //-------------------------------------------------------------------------
    // Frame Buffer
    //-------------------------------------------------------------------------
    // This contains all the commands we need to actually draw this frame
    // Any command with a TTL, will be left in this buffer at the end of the frame to be drawn again

    class SE_API_RUNTIME FrameCommandBuffer
    {
    public:
        void AddThreadCommands(ThreadCommandBuffer const &threadCommands);

        // Empties the command buffer ignoring any TTL state
        inline void Clear()
        {
            m_opaqueDepthOn.Clear();
            m_opaqueDepthOff.Clear();
            m_transparentDepthOn.Clear();
            m_transparentDepthOff.Clear();
        }

        // Resets the buffer, will remove all commands with an expired TTL
        inline void Reset(Seconds deltaTime)
        {
            m_opaqueDepthOn.Reset(deltaTime);
            m_opaqueDepthOff.Reset(deltaTime);
            m_transparentDepthOn.Reset(deltaTime);
            m_transparentDepthOff.Reset(deltaTime);
        }

    public:
        CommandBuffer m_opaqueDepthOn;
        CommandBuffer m_opaqueDepthOff;
        CommandBuffer m_transparentDepthOn;
        CommandBuffer m_transparentDepthOff;
    };
}
#endif