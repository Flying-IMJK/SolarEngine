// Copyright (c) Wojciech Figat. All rights reserved.

using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Basic editor/viewer for <see cref="GPUTexture"/>.
    /// </summary>
    [CustomEditor(typeof(GPUTexture)), DefaultEditor]
    public class GPUTextureEditor : CustomEditor
    {
        private Image m_Image;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            m_Image = new Image
            {
                Brush = new GPUTextureBrush(),
                Size = new Float2(200, 100),
                Parent = layout.ContainerControl,
            };
            m_Image.Clicked += OnImageClicked;
        }

        private void OnImageClicked(Image image, MouseButton button)
        {
            GPUTexture texture = Values[0] as GPUTexture;
            if (!texture || button != MouseButton.Right)
            {
                return;
            }
            ContextMenu menu = new ContextMenu();
            menu.AddButton("Enlarge", () => m_Image.Size *= 2);
            menu.AddButton("Shrink", () => m_Image.Size /= 2).Enabled = m_Image.Height > 32;
            Float2 location = image.PointFromScreen(Input.MouseScreenPosition);
            menu.Show(image, location);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            GPUTexture texture = Values[0] as GPUTexture;
            ((GPUTextureBrush)m_Image.Brush).Texture = texture;
            if (texture)
            {
                GPUTextureDescription desc = texture.Description;
#if BUILD_RELEASE
                string name = string.Empty;
#else
                string name = texture.ToString();
#endif
                m_Image.TooltipText =
                    $"{name}\nType: {texture.Dimensions}\nSize: {desc.Width}x{desc.Height}\nFormat: {desc.Format}\nMemory: {texture.MemoryUsage}";
            }
            else
            {
                m_Image.TooltipText = "None";
            }
        }
    }
}
