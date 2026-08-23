#pragma once
//-------------------------------------------------------------------------
// Auto-generated managed/native ABI bridge - do not edit manually.
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Math/Vector2.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Math/Vector3.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Math/Vector4.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Platform/CreateWindowSettings.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Platform/CPUInfo.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Platform/MemoryStats.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/Font.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/TextLayoutOptions.h"
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"

namespace SE::BindingsInterop
{
    struct SE_CreateWindowSettings
    {
        CLRString* Title;
        ::SE::Vector2Base<float> Position;
        ::SE::Vector2Base<float> Size;
        ::SE::Vector2Base<float> MinimumSize;
        ::SE::Vector2Base<float> MaximumSize;
        ::SE::WindowStartPosition StartPosition;
        bool Fullscreen;
        bool HasBorder;
        bool SupportsTransparency;
        bool ShowInTaskbar;
        bool ActivateWhenFirstShown;
        bool AllowInput;
        bool AllowMinimize;
        bool AllowMaximize;
        bool AllowDragAndDrop;
        bool IsTopmost;
        bool IsRegularWindow;
        bool HasSizingFrame;
        bool ShowAfterFirstPaint;
    };

    inline ::SE::CreateWindowSettings ToNative(const SE_CreateWindowSettings& value)
    {
        ::SE::CreateWindowSettings result{};
        result.Title = CLRUtils::ToString((CLRString*)value.Title);
        result.Position = value.Position;
        result.Size = value.Size;
        result.MinimumSize = value.MinimumSize;
        result.MaximumSize = value.MaximumSize;
        result.StartPosition = value.StartPosition;
        result.Fullscreen = value.Fullscreen;
        result.HasBorder = value.HasBorder;
        result.SupportsTransparency = value.SupportsTransparency;
        result.ShowInTaskbar = value.ShowInTaskbar;
        result.ActivateWhenFirstShown = value.ActivateWhenFirstShown;
        result.AllowInput = value.AllowInput;
        result.AllowMinimize = value.AllowMinimize;
        result.AllowMaximize = value.AllowMaximize;
        result.AllowDragAndDrop = value.AllowDragAndDrop;
        result.IsTopmost = value.IsTopmost;
        result.IsRegularWindow = value.IsRegularWindow;
        result.HasSizingFrame = value.HasSizingFrame;
        result.ShowAfterFirstPaint = value.ShowAfterFirstPaint;
        return result;
    }

    inline SE_CreateWindowSettings ToManaged(const ::SE::CreateWindowSettings& value)
    {
        SE_CreateWindowSettings result{};
        result.Title = CLRUtils::ToString(value.Title);
        result.Position = value.Position;
        result.Size = value.Size;
        result.MinimumSize = value.MinimumSize;
        result.MaximumSize = value.MaximumSize;
        result.StartPosition = value.StartPosition;
        result.Fullscreen = value.Fullscreen;
        result.HasBorder = value.HasBorder;
        result.SupportsTransparency = value.SupportsTransparency;
        result.ShowInTaskbar = value.ShowInTaskbar;
        result.ActivateWhenFirstShown = value.ActivateWhenFirstShown;
        result.AllowInput = value.AllowInput;
        result.AllowMinimize = value.AllowMinimize;
        result.AllowMaximize = value.AllowMaximize;
        result.AllowDragAndDrop = value.AllowDragAndDrop;
        result.IsTopmost = value.IsTopmost;
        result.IsRegularWindow = value.IsRegularWindow;
        result.HasSizingFrame = value.HasSizingFrame;
        result.ShowAfterFirstPaint = value.ShowAfterFirstPaint;
        return result;
    }

    struct SE_Sprite
    {
        ::SE::Rectangle Area;
        CLRString* Name;
    };

    inline ::SE::Sprite ToNative(const SE_Sprite& value)
    {
        ::SE::Sprite result{};
        result.Area = value.Area;
        result.Name = CLRUtils::ToString((CLRString*)value.Name);
        return result;
    }

    inline SE_Sprite ToManaged(const ::SE::Sprite& value)
    {
        SE_Sprite result{};
        result.Area = value.Area;
        result.Name = CLRUtils::ToString(value.Name);
        return result;
    }

    struct SE_SpriteHandle
    {
        ::SE::AssetRef<::SE::SpriteAtlas> Atlas;
        int32 Index;
    };

    inline ::SE::SpriteHandle ToNative(const SE_SpriteHandle& value)
    {
        ::SE::SpriteHandle result{};
        result.Atlas = value.Atlas;
        result.Index = value.Index;
        return result;
    }

    inline SE_SpriteHandle ToManaged(const ::SE::SpriteHandle& value)
    {
        SE_SpriteHandle result{};
        result.Atlas = value.Atlas;
        result.Index = value.Index;
        return result;
    }

}

namespace SE
{
    template<>
    struct CLRConverter<::SE::Float2>
    {
        CLRObject* Box(const ::SE::Float2& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Float2& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Float2));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Float2>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Float2));
        }

        void ToNativeArray(Span<::SE::Float2>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Float2));
        }
    };

    template<>
    struct CLRConverter<::SE::Double2>
    {
        CLRObject* Box(const ::SE::Double2& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Double2& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Double2));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Double2>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Double2));
        }

        void ToNativeArray(Span<::SE::Double2>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Double2));
        }
    };

    template<>
    struct CLRConverter<::SE::Int2>
    {
        CLRObject* Box(const ::SE::Int2& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Int2& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Int2));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Int2>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Int2));
        }

        void ToNativeArray(Span<::SE::Int2>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Int2));
        }
    };

    template<>
    struct CLRConverter<::SE::Float3>
    {
        CLRObject* Box(const ::SE::Float3& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Float3& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Float3));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Float3>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Float3));
        }

        void ToNativeArray(Span<::SE::Float3>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Float3));
        }
    };

    template<>
    struct CLRConverter<::SE::Double3>
    {
        CLRObject* Box(const ::SE::Double3& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Double3& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Double3));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Double3>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Double3));
        }

        void ToNativeArray(Span<::SE::Double3>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Double3));
        }
    };

    template<>
    struct CLRConverter<::SE::Int3>
    {
        CLRObject* Box(const ::SE::Int3& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Int3& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Int3));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Int3>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Int3));
        }

        void ToNativeArray(Span<::SE::Int3>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Int3));
        }
    };

    template<>
    struct CLRConverter<::SE::Float4>
    {
        CLRObject* Box(const ::SE::Float4& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Float4& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Float4));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Float4>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Float4));
        }

        void ToNativeArray(Span<::SE::Float4>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Float4));
        }
    };

    template<>
    struct CLRConverter<::SE::Double4>
    {
        CLRObject* Box(const ::SE::Double4& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Double4& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Double4));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Double4>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Double4));
        }

        void ToNativeArray(Span<::SE::Double4>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Double4));
        }
    };

    template<>
    struct CLRConverter<::SE::Int4>
    {
        CLRObject* Box(const ::SE::Int4& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::Int4& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::Int4));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Int4>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::Int4));
        }

        void ToNativeArray(Span<::SE::Int4>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::Int4));
        }
    };

    template<>
    struct CLRConverter<::SE::CreateWindowSettings>
    {
        CLRObject* Box(const ::SE::CreateWindowSettings& data, const CLRClass* klass)
        {
            auto managed = BindingsInterop::ToManaged(data);
            return CLRCore::Object::Box((void*)&managed, klass);
        }

        void Unbox(::SE::CreateWindowSettings& result, CLRObject* data)
        {
            if (data)
                result = BindingsInterop::ToNative(*reinterpret_cast<BindingsInterop::SE_CreateWindowSettings*>(CLRCore::Object::Unbox(data)));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::CreateWindowSettings>& data)
        {
            if (result == nullptr)
                return;
            auto* resultItems = CLRCore::Array::GetAddress<BindingsInterop::SE_CreateWindowSettings>(result);
            const CLRClass* elementClass = ::SE::CreateWindowSettings::TypeInitializer.GetClass();
            for (int32 i = 0; i < data.Length(); ++i)
            {
                auto managed = BindingsInterop::ToManaged(data[i]);
                CLRCore::GC::WriteValue(&resultItems[i], &managed, 1, elementClass);
            }
        }

        void ToNativeArray(Span<::SE::CreateWindowSettings>& result, const CLRArray* data)
        {
            if (data == nullptr)
                return;
            auto* dataItems = CLRCore::Array::GetAddress<BindingsInterop::SE_CreateWindowSettings>(data);
            for (int32 i = 0; i < result.Length(); ++i)
                result[i] = BindingsInterop::ToNative(dataItems[i]);
        }
    };

    template<>
    struct CLRConverter<::SE::CPUInfo>
    {
        CLRObject* Box(const ::SE::CPUInfo& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::CPUInfo& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::CPUInfo));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::CPUInfo>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::CPUInfo));
        }

        void ToNativeArray(Span<::SE::CPUInfo>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::CPUInfo));
        }
    };

    template<>
    struct CLRConverter<::SE::MemoryStats>
    {
        CLRObject* Box(const ::SE::MemoryStats& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::MemoryStats& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::MemoryStats));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::MemoryStats>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::MemoryStats));
        }

        void ToNativeArray(Span<::SE::MemoryStats>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::MemoryStats));
        }
    };

    template<>
    struct CLRConverter<::SE::ProcessMemoryStats>
    {
        CLRObject* Box(const ::SE::ProcessMemoryStats& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::ProcessMemoryStats& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::ProcessMemoryStats));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::ProcessMemoryStats>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::ProcessMemoryStats));
        }

        void ToNativeArray(Span<::SE::ProcessMemoryStats>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::ProcessMemoryStats));
        }
    };

    template<>
    struct CLRConverter<::SE::TextRange>
    {
        CLRObject* Box(const ::SE::TextRange& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::TextRange& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::TextRange));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::TextRange>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::TextRange));
        }

        void ToNativeArray(Span<::SE::TextRange>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::TextRange));
        }
    };

    template<>
    struct CLRConverter<::SE::TextLayoutOptions>
    {
        CLRObject* Box(const ::SE::TextLayoutOptions& data, const CLRClass* klass)
        {
            return CLRCore::Object::Box((void*)&data, klass);
        }

        void Unbox(::SE::TextLayoutOptions& result, CLRObject* data)
        {
            if (data)
                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::SE::TextLayoutOptions));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::TextLayoutOptions>& data)
        {
            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::SE::TextLayoutOptions));
        }

        void ToNativeArray(Span<::SE::TextLayoutOptions>& result, const CLRArray* data)
        {
            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::SE::TextLayoutOptions));
        }
    };

    template<>
    struct CLRConverter<::SE::Sprite>
    {
        CLRObject* Box(const ::SE::Sprite& data, const CLRClass* klass)
        {
            auto managed = BindingsInterop::ToManaged(data);
            return CLRCore::Object::Box((void*)&managed, klass);
        }

        void Unbox(::SE::Sprite& result, CLRObject* data)
        {
            if (data)
                result = BindingsInterop::ToNative(*reinterpret_cast<BindingsInterop::SE_Sprite*>(CLRCore::Object::Unbox(data)));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::Sprite>& data)
        {
            if (result == nullptr)
                return;
            auto* resultItems = CLRCore::Array::GetAddress<BindingsInterop::SE_Sprite>(result);
            const CLRClass* elementClass = ::SE::Sprite::TypeInitializer.GetClass();
            for (int32 i = 0; i < data.Length(); ++i)
            {
                auto managed = BindingsInterop::ToManaged(data[i]);
                CLRCore::GC::WriteValue(&resultItems[i], &managed, 1, elementClass);
            }
        }

        void ToNativeArray(Span<::SE::Sprite>& result, const CLRArray* data)
        {
            if (data == nullptr)
                return;
            auto* dataItems = CLRCore::Array::GetAddress<BindingsInterop::SE_Sprite>(data);
            for (int32 i = 0; i < result.Length(); ++i)
                result[i] = BindingsInterop::ToNative(dataItems[i]);
        }
    };

    template<>
    struct CLRConverter<::SE::SpriteHandle>
    {
        CLRObject* Box(const ::SE::SpriteHandle& data, const CLRClass* klass)
        {
            auto managed = BindingsInterop::ToManaged(data);
            return CLRCore::Object::Box((void*)&managed, klass);
        }

        void Unbox(::SE::SpriteHandle& result, CLRObject* data)
        {
            if (data)
                result = BindingsInterop::ToNative(*reinterpret_cast<BindingsInterop::SE_SpriteHandle*>(CLRCore::Object::Unbox(data)));
        }

        void ToManagedArray(CLRArray* result, const Span<::SE::SpriteHandle>& data)
        {
            if (result == nullptr)
                return;
            auto* resultItems = CLRCore::Array::GetAddress<BindingsInterop::SE_SpriteHandle>(result);
            const CLRClass* elementClass = ::SE::SpriteHandle::TypeInitializer.GetClass();
            for (int32 i = 0; i < data.Length(); ++i)
            {
                auto managed = BindingsInterop::ToManaged(data[i]);
                CLRCore::GC::WriteValue(&resultItems[i], &managed, 1, elementClass);
            }
        }

        void ToNativeArray(Span<::SE::SpriteHandle>& result, const CLRArray* data)
        {
            if (data == nullptr)
                return;
            auto* dataItems = CLRCore::Array::GetAddress<BindingsInterop::SE_SpriteHandle>(data);
            for (int32 i = 0; i < result.Length(); ++i)
                result[i] = BindingsInterop::ToNative(dataItems[i]);
        }
    };

}
