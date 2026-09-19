
using System;

namespace SE
{
    partial struct SpriteHandle
    {
        /// <summary>
        /// Invalid sprite handle.
        /// </summary>
        public static SpriteHandle Invalid;

        /// <summary>
        /// Initializes a new instance of the <see cref="Sprite"/> struct.
        /// </summary>
        /// <param name="atlas">The atlas.</param>
        /// <param name="index">The index.</param>
        public SpriteHandle(SpriteAtlas atlas, int index)
        {
            Atlas = atlas;
            Index = index;
        }

        /// <summary>
        /// Returns true if sprite is valid.
        /// </summary>
        public bool IsValid => Atlas != null && Index != -1;

        /// <summary>
        /// Gets or sets the sprite name.
        /// </summary>
        public string Name
        {
            get
            {
                if (Atlas == null)
                {
                    throw new InvalidOperationException("Cannot use invalid sprite.");
                }
                return Atlas.GetSprite(Index).Name;
            }
            set
            {
                if (Atlas == null)
                {
                    throw new InvalidOperationException("Cannot use invalid sprite.");
                }
                var sprite = Atlas.GetSprite(Index);
                sprite.Name = value;
                Atlas.SetSprite(Index, sprite);
            }
        }

        /// <summary>
        /// Gets or sets the sprite location (in pixels).
        /// </summary>
        public Float2 Location
        {
            get => Area.Location * Atlas.Size();
            set
            {
                var area = Area;
                area.Location = value / Atlas.Size();
                Area = area;
            }
        }

        /// <summary>
        /// Gets or sets the sprite size (in pixels).
        /// </summary>
        public Float2 Size
        {
            get
            {
                if (Atlas == null)
                {
                    throw new InvalidOperationException("Cannot use invalid sprite.");
                }

                Rectangle area = Rectangle.Empty;
                Atlas.GetSpriteArea(Index, ref area);
                return area.Size * Atlas.Size();
            }
            set
            {
                var area = Area;
                area.Size = value / Atlas.Size();
                Area = area;
            }
        }

        /// <summary>
        /// Gets or sets the sprite area in atlas (in normalized atlas coordinates [0;1]).
        /// </summary>
        public Rectangle Area
        {
            get
            {
                if (Atlas == null)
                {
                    throw new InvalidOperationException("Cannot use invalid sprite.");
                }

                Rectangle area = Rectangle.Empty;
                Atlas.GetSpriteArea(Index, ref area);
                return area;
            }
            set
            {
                if (Atlas == null)
                {
                    throw new InvalidOperationException("Cannot use invalid sprite.");
                }
                var sprite = Atlas.GetSprite(Index);
                sprite.Area = value;
                Atlas.SetSprite(Index, sprite);
            }
        }
    }
}
