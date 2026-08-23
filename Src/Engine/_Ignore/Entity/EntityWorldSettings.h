#pragma once
#include "Runtime/API.h"
#include "Runtime/Settings/Settings.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME IEntityWorldSettings : public ISettings
    {
        SE_CLASS( IEntityWorldSettings, ISettings);
    };
}