#pragma once
//-------------------------------------------------------------------------
// Auto-generated managed/native ABI bridge - do not edit manually.
//-------------------------------------------------------------------------
#include "E:/EngineProject/SolarEngine/Src/Engine//Runtime/Core/Platform/CreateWindowSettings.h"
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
