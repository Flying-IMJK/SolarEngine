#pragma once

#include "TextLayoutOptions.h"
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Types/BitFlags.h"
#include "Runtime/Graphics/Viewport.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"

namespace SE
{
    class Texture;
    struct SpriteHandle;
    struct TextRange;
    class MaterialBase;
    class Font;
    class GPUTextureView;
    class GPUContext;
    class TextureBase;
    class GPUTexture;

    /// <summary>
    /// Rendering 2D shapes and text using Graphics Device.
    /// </summary>
    SE_CLASS(API, Static)
    class SE_API_RUNTIME Render2D
    {
    public:
        /// <summary>
        /// The rendering features and options flags.
        /// </summary>
        SE_ENUM(API)
        enum class RenderingFeatures
        {
            /// <summary>
            /// The none.
            /// </summary>
            None = 0,

            /// <summary>
            /// Enables automatic geometry vertices snapping to integer coordinates in screen space. Reduces aliasing and sampling artifacts. Might be disabled for 3D projection viewport or for complex UI transformations.
            /// </summary>
            VertexSnapping = 1,

            /// <summary>
            /// Enables automatic characters usage from fallback fonts.
            /// </summary>
            FallbackFonts = 2,
        };

        struct CustomData
        {
            Matrix ViewProjection;
            Float2 ViewSize;
        };


        /// <summary>
        /// Checks if interface is during rendering phrase (Draw calls may be performed without failing).
        /// </summary>
        static bool IsRendering();

        /// <summary>
        /// Gets the current rendering viewport.
        /// </summary>
        static const Viewport& GetViewport();

        /// <summary>
        /// The active rendering features flags.
        /// </summary>
        static EnumFlags<RenderingFeatures> Features;

        /// <summary>
        /// Called when frame rendering begins by the graphics device.
        /// </summary>
        static void BeginFrame();

        /// <summary>
        /// Begins the rendering phrase.
        /// </summary>
        /// <param name="context">The GPU commands context to use.</param>
        /// <param name="output">The output target.</param>
        /// <param name="depthBuffer">The depth buffer.</param>
        SE_FUNCTION(API)
        static void Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer = nullptr);

        /// <summary>
        /// Begins the rendering phrase.
        /// </summary>
        /// <param name="context">The GPU commands context to use.</param>
        /// <param name="output">The output target.</param>
        /// <param name="depthBuffer">The depth buffer.</param>
        /// <param name="viewProjection">The View*Projection matrix. Allows to render GUI in 3D or with custom transformations.</param>
        SE_FUNCTION(API)
        static void Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer, const Matrix& viewProjection);

        /// <summary>
        /// Begins the rendering phrase.
        /// </summary>
        /// <param name="context">The GPU commands context to use.</param>
        /// <param name="output">The output target.</param>
        /// <param name="depthBuffer">The depth buffer.</param>
        /// <param name="viewport">The output viewport.</param>
        SE_FUNCTION(API)
        static void Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, const Viewport& viewport);

        /// <summary>
        /// Begins the rendering phrase.
        /// </summary>
        /// <param name="context">The GPU commands context to use.</param>
        /// <param name="output">The output target.</param>
        /// <param name="depthBuffer">The depth buffer.</param>
        /// <param name="viewport">The output viewport.</param>
        /// <param name="viewProjection">The View*Projection matrix. Allows to render GUI in 3D or with custom transformations.</param>
        SE_FUNCTION(API)
        static void Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, const Viewport& viewport, const Matrix& viewProjection);

        /// <summary>
        /// Ends the rendering phrase.
        /// </summary>
        SE_FUNCTION(API)
        static void End();

        /// <summary>
        /// Called when frame rendering ends by the graphics device.
        /// </summary>
        static void EndFrame();

    public:
        /// <summary>
        /// Pushes transformation layer.
        /// </summary>
        /// <param name="transform">The transformation to apply.</param>
        SE_FUNCTION(API)
        static void PushTransform(const Matrix3x3& transform);

        /// <summary>
        /// Peeks the current transformation layer.
        /// </summary>
        /// <param name="transform">The output transformation to apply combined from all the transformations together (pushed into the transformation stack).</param>
        SE_FUNCTION(API)
        static void PeekTransform(Matrix3x3& transform);

        /// <summary>
        /// Pops transformation layer.
        /// </summary>
        SE_FUNCTION(API)
        static void PopTransform();

        /// <summary>
        /// Pushes clipping rectangle mask.
        /// </summary>
        /// <param name="clipRect">The axis aligned clipping mask rectangle.</param>
        SE_FUNCTION(API)
        static void PushClip(const Rectangle& clipRect);

        /// <summary>
        /// Peeks the current clipping rectangle mask.
        /// </summary>
        /// <param name="clipRect">The output axis aligned clipping mask rectangle combined from all the masks together (pushed into the masking stack).</param>
        SE_FUNCTION(API)
        static void PeekClip(Rectangle& clipRect);

        /// <summary>
        /// Pops clipping rectangle mask.
        /// </summary>
        SE_FUNCTION(API)
        static void PopClip();

        /// <summary>
        /// Pushes tint color.
        /// </summary>
        /// <param name="tint">The tint color.</param>
        /// <param name="inherit">Multiply <paramref ref="tint"/> by the last tint on the stack.</param>
        SE_FUNCTION(API)
        static void PushTint(const Color& tint, bool inherit = true);

        /// <summary>
        /// Peeks the current tint color.
        /// </summary>
        /// <param name="tint">The output tint color.</param>
        SE_FUNCTION(API)
        static void PeekTint(Color& tint);

        /// <summary>
        /// Pops tint color.
        /// </summary>
        SE_FUNCTION(API)
        static void PopTint();

    public:
        /// <summary>
        /// Draws a text.
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="layoutRect">The size and position of the area in which the text is drawn.</param>
        /// <param name="color">The text color.</param>
        /// <param name="horizontalAlignment">The horizontal alignment of the text in a layout rectangle.</param>
        /// <param name="verticalAlignment">The vertical alignment of the text in a layout rectangle.</param>
        /// <param name="textWrapping">Describes how wrap text inside a layout rectangle.</param>
        /// <param name="baseLinesGapScale">The scale for distance one baseline from another. Default is 1.</param>
        /// <param name="scale">The text drawing scale. Default is 1.</param>
        SE_FUNCTION(API)
        static void RenderText(Font* font, const StringView& text, const Rectangle& layoutRect, const Color& color, TextAlignment horizontalAlignment = TextAlignment::Near,
            TextAlignment verticalAlignment = TextAlignment::Near, TextWrapping textWrapping = TextWrapping::NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f);


        /// <summary>
        /// Draws a text using a custom material shader. Given material must have GUI domain and a public parameter named Font (texture parameter used for a font atlas sampling).
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="customMaterial">Custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="layoutRect">The size and position of the area in which the text is drawn.</param>
        /// <param name="color">The text color.</param>
        /// <param name="horizontalAlignment">The horizontal alignment of the text in a layout rectangle.</param>
        /// <param name="verticalAlignment">The vertical alignment of the text in a layout rectangle.</param>
        /// <param name="textWrapping">Describes how wrap text inside a layout rectangle.</param>
        /// <param name="baseLinesGapScale">The scale for distance one baseline from another. Default is 1.</param>
        /// <param name="scale">The text drawing scale. Default is 1.</param>
        SE_FUNCTION(API)
        static void RenderText(Font* font, MaterialBase* customMaterial, const StringView& text, const Rectangle& layoutRect, const Color& color,
            TextAlignment horizontalAlignment = TextAlignment::Near, TextAlignment verticalAlignment = TextAlignment::Near,
            TextWrapping textWrapping = TextWrapping::NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f);

        /// <summary>
        /// Draws a text.
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="color">The text color.</param>
        /// <param name="location">The text location.</param>
        /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
        SE_FUNCTION(API)
        static void RenderText(Font* font, const StringView& text, const Color& color, const Float2& location, MaterialBase* customMaterial = nullptr);

        /// <summary>
        /// Draws a text.
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
        /// <param name="color">The text color.</param>
        /// <param name="location">The text location.</param>
        /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
        SE_FUNCTION(API)
        static void RenderText(Font* font, const StringView& text, const TextRange& textRange, const Color& color, const Float2& location, MaterialBase* customMaterial = nullptr);

        /// <summary>
        /// Draws a text with formatting.
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="color">The text color.</param>
        /// <param name="layout">The text layout properties.</param>
        /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
        SE_FUNCTION(API)
        static void RenderText(Font* font, const StringView& text, const Color& color, const TextLayoutOptions& layout, MaterialBase* customMaterial = nullptr);

        /// <summary>
        /// Draws a text with formatting.
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
        /// <param name="color">The text color.</param>
        /// <param name="layout">The text layout properties.</param>
        /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
        SE_FUNCTION(API)
        static void RenderText(Font* font, const StringView& text, const TextRange& textRange, const Color& color, const TextLayoutOptions& layout, MaterialBase* customMaterial = nullptr);

        /// <summary>
        /// Fills a rectangle area.
        /// </summary>
        /// <param name="rect">The rectangle to fill.</param>
        /// <param name="color">The color to use.</param>
        SE_FUNCTION(API)
        static void FillRectangle(const Rectangle& rect, const Color& color);

        /// <summary>
        /// Fills a rectangle area.
        /// </summary>
        /// <param name="rect">The rectangle to fill.</param>
        /// <param name="color1">The color to use for upper left vertex.</param>
        /// <param name="color2">The color to use for upper right vertex.</param>
        /// <param name="color3">The color to use for bottom right vertex.</param>
        /// <param name="color4">The color to use for bottom left vertex.</param>
        SE_FUNCTION(API)
        static void FillRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4);

        /// <summary>
        /// Draws a rectangle borders.
        /// </summary>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to use.</param>
        /// <param name="thickness">The line thickness.</param>
        SE_FUNCTION(API)
        FORCE_INLINE static void DrawRectangle(const Rectangle& rect, const Color& color, float thickness = 1.0f)
        {
            DrawRectangle(rect, color, color, color, color, thickness);
        }

        /// <summary>
        /// Draws a rectangle borders.
        /// </summary>
        /// <param name="rect">The rectangle to fill.</param>
        /// <param name="color1">The color to use for upper left vertex.</param>
        /// <param name="color2">The color to use for upper right vertex.</param>
        /// <param name="color3">The color to use for bottom right vertex.</param>
        /// <param name="color4">The color to use for bottom left vertex.</param>
        /// <param name="thickness">The line thickness.</param>
        SE_FUNCTION(API)
        static void DrawRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4, float thickness = 1.0f);

        /// <summary>
        /// Draws the render target.
        /// </summary>
        /// <param name="rt">The render target handle to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawTexture(GPUTextureView* rt, const Rectangle& rect, const Color& color = Colors::White);

        /// <summary>
        /// Draws the texture.
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawTexture(GPUTexture* t, const Rectangle& rect, const Color& color = Colors::White);

        /// <summary>
        /// Draws the texture.
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawTexture(TextureBase* t, const Rectangle& rect, const Color& color = Colors::White);

        SE_FUNCTION(API)
        static void DrawTexture(Texture* t, const Rectangle& rect, const Color& color = Colors::White);

        /// <summary>
        /// Draws a sprite.
        /// </summary>
        /// <param name="spriteHandle">The sprite to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color = Colors::White);

        /// <summary>
        /// Draws the texture (uses point sampler).
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawTexturePoint(GPUTexture* t, const Rectangle& rect, const Color& color = Colors::White);

        /// <summary>
        /// Draws a sprite (uses point sampler).
        /// </summary>
        /// <param name="spriteHandle">The sprite to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color = Colors::White);

        /// <summary>
        /// Draws the texture using 9-slicing.
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="border">The borders for 9-slicing (inside rectangle, ordered: left, right, top, bottom).</param>
        /// <param name="borderUVs">The borders UVs for 9-slicing (inside rectangle UVs, ordered: left, right, top, bottom).</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void Draw9SlicingTexture(TextureBase* t, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color = Colors::White);

        /// <summary>
        /// Draws the texture using 9-slicing (uses point sampler).
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="border">The borders for 9-slicing (inside rectangle, ordered: left, right, top, bottom).</param>
        /// <param name="borderUVs">The borders UVs for 9-slicing (inside rectangle UVs, ordered: left, right, top, bottom).</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void Draw9SlicingTexturePoint(TextureBase* t, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color = Colors::White);

        /// <summary>
        /// Draws a sprite using 9-slicing.
        /// </summary>
        /// <param name="spriteHandle">The sprite to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="border">The borders for 9-slicing (inside rectangle, ordered: left, right, top, bottom).</param>
        /// <param name="borderUVs">The borders UVs for 9-slicing (inside rectangle UVs, ordered: left, right, top, bottom).</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        // static void Draw9SlicingSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color = Colors::White);

        /// <summary>
        /// Draws a sprite using 9-slicing (uses point sampler).
        /// </summary>
        /// <param name="spriteHandle">The sprite to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        /// <param name="border">The borders for 9-slicing (inside rectangle, ordered: left, right, top, bottom).</param>
        /// <param name="borderUVs">The borders UVs for 9-slicing (inside rectangle UVs, ordered: left, right, top, bottom).</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        // static void Draw9SlicingSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color = Colors::White);

        /// <summary>
        /// Performs custom rendering.
        /// </summary>
        /// <param name="t">The texture to use.</param>
        /// <param name="rect">The rectangle area to draw.</param>
        /// <param name="ps">The custom pipeline state to use (input must match default Render2D vertex shader and can use single texture).</param>
        /// <param name="color">The color to multiply all texture pixels.</param>
        SE_FUNCTION(API)
        static void DrawCustom(GPUTexture* t, const Rectangle& rect, GPUPipelineState* ps, const Color& color = Colors::White);

        /// <summary>
        /// Draws a line.
        /// </summary>
        /// <param name="p1">The start point.</param>
        /// <param name="p2">The end point.</param>
        /// <param name="color">The line color.</param>
        /// <param name="thickness">The line thickness.</param>
        SE_FUNCTION(API)
        FORCE_INLINE static void DrawLine(const Float2& p1, const Float2& p2, const Color& color, float thickness = 1.0f)
        {
            DrawLine(p1, p2, color, color, thickness);
        }

        /// <summary>
        /// Draws a line.
        /// </summary>
        /// <param name="p1">The start point.</param>
        /// <param name="p2">The end point.</param>
        /// <param name="color1">The line start color.</param>
        /// <param name="color2">The line end color.</param>
        /// <param name="thickness">The line thickness.</param>
        SE_FUNCTION(API)
        static void DrawLine(const Float2& p1, const Float2& p2, const Color& color1, const Color& color2, float thickness = 1.0f);

        /// <summary>
        /// Draws a Bezier curve.
        /// </summary>
        /// <param name="p1">The start point.</param>
        /// <param name="p2">The first control point.</param>
        /// <param name="p3">The second control point.</param>
        /// <param name="p4">The end point.</param>
        /// <param name="color">The line color</param>
        /// <param name="thickness">The line thickness.</param>
        SE_FUNCTION(API)
        static void DrawBezier(const Float2& p1, const Float2& p2, const Float2& p3, const Float2& p4, const Color& color, float thickness = 1.0f);

        /// <summary>
        /// Draws the GUI material.
        /// </summary>
        /// <param name="material">The material to render. Must be a GUI material type.</param>
        /// <param name="rect">The target rectangle to draw.</param>
        /// <param name="color">The color to use.</param>
        SE_FUNCTION(API)
        static void DrawMaterial(MaterialBase* material, const Rectangle& rect, const Color& color);

        /// <summary>
        /// Draws the background blur.
        /// </summary>
        /// <param name="rect">The target rectangle to draw (blurs its background).</param>
        /// <param name="blurStrength">The blur strength defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.</param>
        SE_FUNCTION(API)
        static void DrawBlur(const Rectangle& rect, float blurStrength);

        /// <summary>
        /// Draws vertices array.
        /// </summary>
        /// <param name="t">The texture.</param>
        /// <param name="vertices">The vertices array.</param>
        /// <param name="uvs">The uvs array.</param>
        SE_FUNCTION(API)
        static void DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs);

        /// <summary>
        /// Draws vertices array.
        /// </summary>
        /// <param name="t">The texture.</param>
        /// <param name="vertices">The vertices array.</param>
        /// <param name="uvs">The uvs array.</param>
        /// <param name="color">The color.</param>
        SE_FUNCTION(API)
        static void DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs, const Color& color);

        /// <summary>
        /// Draws vertices array.
        /// </summary>
        /// <param name="t">The texture.</param>
        /// <param name="vertices">The vertices array.</param>
        /// <param name="uvs">The uvs array.</param>
        /// <param name="colors">The colors array.</param>
        SE_FUNCTION(API)
        static void DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs, const Span<Color>& colors);

        /// <summary>
        /// Draws indexed vertices array.
        /// </summary>
        /// <param name="t">The texture.</param>
        /// <param name="indices">The indices array.</param>
        /// <param name="vertices">The vertices array.</param>
        /// <param name="uvs">The uvs array.</param>
        /// <param name="colors">The colors array.</param>
        SE_FUNCTION(API)
        static void DrawTexturedTriangles(GPUTexture* t, const Span<uint16>& indices, const Span<Float2>& vertices, const Span<Float2>& uvs, const Span<Color>& colors);

        /// <summary>
        /// Draws vertices array.
        /// </summary>
        /// <param name="vertices">The vertices array.</param>
        /// <param name="colors">The colors array.</param>
        /// <param name="useAlpha">If true alpha blending will be enabled.</param>
        SE_FUNCTION(API)
        static void FillTriangles(const Span<Float2>& vertices, const Span<Color>& colors, bool useAlpha);

        /// <summary>
        /// Fills a triangular area.
        /// </summary>
        /// <param name="p0">The first point.</param>
        /// <param name="p1">The second point.</param>
        /// <param name="p2">The third point.</param>
        /// <param name="color">The color.</param>
        SE_FUNCTION(API)
        static void FillTriangle(const Float2& p0, const Float2& p1, const Float2& p2, const Color& color);

        /// <summary>
        /// Calls drawing GUI to the texture.
        /// </summary>
        /// <param name="drawableElement">The root container for Draw methods.</param>
        /// <param name="context">The GPU context to handle graphics commands.</param>
        /// <param name="output">The output render target.</param>
        static void CallDrawing(Function<void()> drawableElement, GPUContext* context, GPUTexture* output);

        /// <summary>
        /// Calls drawing GUI to the texture using custom View*Projection matrix.
        /// If depth buffer texture is provided there will be depth test performed during rendering.
        /// </summary>
        /// <param name="drawableElement">The root container for Draw methods.</param>
        /// <param name="context">The GPU context to handle graphics commands.</param>
        /// <param name="output">The output render target.</param>
        /// <param name="depthBuffer">The depth buffer render target. It's optional parameter but if provided must match output texture.</param>
        /// <param name="viewProjection">The View*Projection matrix used to transform all rendered vertices.</param>
        static void CallDrawing(Function<void()> drawableElement, GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer, Matrix &viewProjection);
    };


} // SE

