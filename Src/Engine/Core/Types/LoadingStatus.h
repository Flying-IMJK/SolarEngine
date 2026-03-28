#pragma once

//-------------------------------------------------------------------------

namespace SE
{
    enum class LoadingStatus
    {
        Unloaded = 0,
        Loading,
        Loaded,
        Unloading,
        Failed,
    };
}