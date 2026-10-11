using System;
using System.ComponentModel;
using System.Reflection;

namespace SE
{
    public unsafe partial struct TextureUtilsOptions
    {
        public enum CustomMaxSizes
        {
            _32 = 32,
            _64 = 64,
            _128 = 128,
            _256 = 256,
            _512 = 512,
            _1024 = 1024,
            _2048 = 2048,
            _4096 = 4096,
            _8192 = 8192,
            _16384 = 16384,
        }

        /// <summary>
        /// The size of the imported texture. If Resize property is set to true then texture will be resized during the import to this value. Otherwise it will be ignored.
        /// </summary>
        [EditorOrder(110), DefaultValue(typeof(Int2), "1024,1024")]
        public Int2 Size
        {
            get => new Int2(SizeX, SizeY);
            set
            {
                SizeX = value.X;
                SizeY = value.Y;
            }
        }

        /// <summary>
        /// Maximum size of the texture (for both width and height). Higher resolution textures will be resized during importing process.
        /// </summary>
        [EditorOrder(90), DefaultValue(CustomMaxSizes._8192), EditorDisplay(null, "Max Size")]
        public CustomMaxSizes CustomMaxSize
        {
            get
            {
                var value = MaxSize;
                if (!Mathf.IsPowerOfTwo(value))
                {
                    value = Mathf.NextPowerOfTwo(value);
                }

                FieldInfo[] fields = typeof(CustomMaxSizes).GetFields();
                for (int i = 0; i < fields.Length; i++)
                {
                    var @field = fields[i];
                    if (@field.Name.Equals("value__"))
                    {
                        continue;
                    }

                    if (value == (int)@field.GetRawConstantValue())
                    {
                        return (CustomMaxSizes)value;
                    }
                }
                return CustomMaxSizes._8192;
            }
            set => MaxSize = (int)value;
        }

        private static bool m__DefaultValid = false;

        private static TextureUtilsOptions m_Default;

        /// <summary>
        /// The default <see cref="Options"/>.
        /// </summary>
        public static TextureUtilsOptions Default
        {
            get
            {
                if (!m__DefaultValid)
                {
                    m__DefaultValid = true;
                    object obj = m_Default;
                    Utils.InitStructure(obj, typeof(TextureUtilsOptions));
                    m_Default = (TextureUtilsOptions)obj;
                }
                return m_Default;
            }
        }
    }
}

