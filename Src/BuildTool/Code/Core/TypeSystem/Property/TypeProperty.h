#pragma once


namespace SE
{
    struct TypeProperty
    {
        enum class Flags
        {
            None = 0,
            IsStructure = 1 << 0,
            IsEnum = 1 << 1,
            IsBitFlags = 1 << 2,
            IsArray = 1 << 3,
            IsDynamicArray = 1 << 4,
        };
    };
}

