
using System;

namespace SE
{
    /// <summary>
    /// Draw method within this interface is used for <see cref="Render2D"/>.CallDrawing single DrawCall
    /// <remarks>Each frame new Queue is sent to GPU from this CPU bound method</remarks>
    /// </summary>
    /// <seealso cref="Render2D.CallDrawing(IDrawable,GPUContext,GPUTexture)"/>
    /// <seealso cref="PostProcessEffect.Render"/>
    public interface IDrawable
    {
        /// <summary>
        /// Render2D drawing methods should be used within this method during render phase to be visible. 
        /// </summary>
        void Draw();
    }


    static partial class Render2D
    {
        /// <summary>
        /// Draws the GUI material.
        /// </summary>
        /// <param name="material">The material to render. Must be a GUI material type.</param>
        /// <param name="rect">The target rectangle to draw.</param>
        public static void DrawMaterial(MaterialBase material, Rectangle rect)
        {
            var color = Color.White;
            Internal_DrawMaterial(Object.GetUnmanagedPtr(material), rect, color);
        }

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
        public static void DrawText(Font font, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f)
        {
            var layout = new TextLayoutOptions
            {
                Bounds = layoutRect,
                HorizontalAlignment = horizontalAlignment,
                VerticalAlignment = verticalAlignment,
                TextWrapping = textWrapping,
                Scale = scale,
                BaseLinesGapScale = baseLinesGapScale,
            };
            RenderText(font, text, color, layout);
        }

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
        public static void DrawText(Font font, MaterialBase customMaterial, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f)
        {
            var layout = new TextLayoutOptions
            {
                Bounds = layoutRect,
                HorizontalAlignment = horizontalAlignment,
                VerticalAlignment = verticalAlignment,
                TextWrapping = textWrapping,
                Scale = scale,
                BaseLinesGapScale = baseLinesGapScale,
            };
            RenderText(font, text, color, layout, customMaterial);
        }

        /// <summary>
        /// Calls drawing GUI to the texture.
        /// </summary>
        /// <param name="drawableElement">The root container for Draw methods.</param>
        /// <param name="context">The GPU context to handle graphics commands.</param>
        /// <param name="output">The output render target.</param>
        public static void CallDrawing(IDrawable drawableElement, GPUContext context, GPUTexture output)
        {
            if (context == null || output == null || drawableElement == null)
                throw new ArgumentNullException();

            Begin(context, output);
            try
            {
                drawableElement.Draw();
            }
            finally
            {
                End();
            }
        }

        /// <summary>
        /// Calls drawing GUI to the texture using custom View*Projection matrix.
        /// If depth buffer texture is provided there will be depth test performed during rendering.
        /// </summary>
        /// <param name="drawableElement">The root container for Draw methods.</param>
        /// <param name="context">The GPU context to handle graphics commands.</param>
        /// <param name="output">The output render target.</param>
        /// <param name="depthBuffer">The depth buffer render target. It's optional parameter but if provided must match output texture.</param>
        /// <param name="viewProjection">The View*Projection matrix used to transform all rendered vertices.</param>
        public static void CallDrawing(IDrawable drawableElement, GPUContext context, GPUTexture output, GPUTexture depthBuffer, ref Matrix viewProjection)
        {
            if (context == null || output == null || drawableElement == null)
                throw new ArgumentNullException();
            if (depthBuffer != null)
            {
                if (!depthBuffer.IsAllocated)
                    throw new InvalidOperationException("Depth buffer is not allocated. Use GPUTexture.Init before rendering.");
                if (output.Size != depthBuffer.Size)
                    throw new InvalidOperationException("Output buffer and depth buffer dimensions must be equal.");
            }

            Begin(context, output, depthBuffer, viewProjection);
            try
            {
                drawableElement.Draw();
            }
            finally
            {
                End();
            }
        }
    }
}
