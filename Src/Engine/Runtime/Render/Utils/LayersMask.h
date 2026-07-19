#pragma once
#include "Runtime/Core/Platform/Compiler.h"
#include "Runtime/Core/Serialization/ISerializable.h"
#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/Serialization/JsonWriters.hpp"
#include "Runtime/Core/Types/Variable.h"
#include "Runtime/API.h"

namespace SE
{
    /// <summary>
    /// The objects layers selection mask (from layers and tags settings). Uses 1 bit per layer (up to 32 layers).
    /// </summary>
    struct SE_API_RUNTIME LayersMask
    {
        /// <summary>
        /// The layers selection mask.
        /// </summary>
        uint32 Mask = Max_uint32;

    public:
        FORCE_INLINE LayersMask()
        {
        }

        FORCE_INLINE LayersMask(uint32 mask)
            : Mask(mask)
        {
        }

        FORCE_INLINE bool HasLayer(int32 layerIndex) const
        {
            return (Mask & (1 << layerIndex)) != 0;
        }

        bool HasLayer(const StringView& layerName) const;

        operator uint32() const
        {
            return Mask;
        }

        bool operator==(const LayersMask& other) const
        {
            return Mask == other.Mask;
        }

        bool operator!=(const LayersMask& other) const
        {
            return Mask != other.Mask;
        }

        LayersMask operator+(const LayersMask& other) const
        {
            return Mask | other.Mask;
        }

        LayersMask operator-(const LayersMask& other) const
        {
            return Mask & ~other.Mask;
        }

        LayersMask operator&(const LayersMask& other) const
        {
            return Mask & other.Mask;
        }

        LayersMask operator|(const LayersMask& other) const
        {
            return Mask | other.Mask;
        }

        LayersMask operator^(const LayersMask& other) const
        {
            return Mask ^ other.Mask;
        }

        LayersMask operator-() const
        {
            return ~Mask;
        }
    };

    // @formatter:off
    namespace Serialization
    {
        inline bool ShouldSerialize(const LayersMask& v, const void* otherObj)
        {
            return !otherObj || v != *(LayersMask*)otherObj;
        }

        inline void Serialize(SerializeContext& context, const LayersMask& v, const void* otherObj)
        {
            context.stream.Uint(v.Mask);
        }
        inline void Deserialize(DeserializeContext& context, LayersMask& v)
        {
            v.Mask = context.stream->GetUint();
        }
    }
    // @formatter:on

}
