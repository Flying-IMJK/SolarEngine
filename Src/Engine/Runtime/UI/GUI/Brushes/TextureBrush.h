#pragma once
#include "IBrush.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/UI/GUI/Margin.h"

namespace SE
{
    class Texture;

    class SE_API_RUNTIME TextureBrush final : public IBrush
    {
        SE_DEFINE_CLASS(TextureBrush, IBrush)
    public:
        /// <summary>
        /// The texture.
        /// </summary>
        AssetRef<Texture> Texture;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        BrushFilter Filter = BrushFilter::Linear;


        TextureBrush() = default;
        /// <summary>
        /// Initializes a new instance of the <see cref="TextureBrush"/> struct.
        /// </summary>
        /// <param name="texture">The texture.</param>
        TextureBrush(AssetRef<::SE::Texture> texture);

        /// <inheritdoc />
        void Draw(Rectangle rect, Color color) override;

    protected:
        Float2 __GetSize() override;
    };

    /// <summary>
    /// Implementation of <see cref="IBrush"/> for <see cref="FlaxEngine.Texture"/> using 9-slicing.
    /// </summary>
    /// <seealso cref="IBrush" />
    class SE_API_RUNTIME Texture9SlicingBrush : public IBrush
    {
    public:
        /// <summary>
        /// The texture.
        /// </summary>
        AssetRef<Texture> Texture;

        /// <summary>
        /// The texture sampling filter mode.
        /// </summary>
        BrushFilter Filter = BrushFilter::Linear;

        /// <summary>
        /// The border size.
        /// </summary>
        float BorderSize = 10.0f;

        /// <summary>
        /// The texture borders (in texture space, range 0-1).
        /// </summary>
        Margin Border = Margin(0.1f);

#if SE_EDITOR
        /// <summary>
        /// Displays borders (editor only).
        /// </summary>
        bool ShowBorders;
#endif

        /// <summary>
        /// Initializes a new instance of the <see cref="Texture9SlicingBrush"/> class.
        /// </summary>
        Texture9SlicingBrush()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Texture9SlicingBrush"/> struct.
        /// </summary>
        /// <param name="texture">The texture.</param>
        Texture9SlicingBrush(AssetRef<::SE::Texture> texture);

        /// <inheritdoc />
        void Draw(Rectangle rect, Color color) override;

    protected:
        Float2 __GetSize() override;
    };

} // SE

