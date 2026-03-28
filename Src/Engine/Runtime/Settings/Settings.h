#pragma once
#include "Core/TypeSystem/IType.h"
#include "Runtime/API.h"
//-------------------------------------------------------------------------

namespace SE
{
    class IniFile;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME ISettings : public IType
    {
        SE_CLASS_DEFAULT(ISettings, IType);
    };

    //-------------------------------------------------------------------------
    // System Settings
    //-------------------------------------------------------------------------
    // Derived from this type to create a set of global setting that will be
    // saved to, and loaded from, the engine ini file.
    //
    // Derived global settings objects will be automatically created and registered!

    class SE_API_RUNTIME GlobalSettings : public ISettings
    {
        SE_CLASS_DEFAULT(GlobalSettings, ISettings);

        virtual bool LoadSettings( IniFile const& ini ) = 0;
        virtual bool SaveSettings( IniFile& ini ) const = 0;
    };
}