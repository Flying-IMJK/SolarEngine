#pragma once

#include <any>
#include "Runtime/Core/Types/Strings/String.h"

namespace SE::Editor
{
    struct ImportRequest
    {
        /// <summary>
        /// The input item path (folder or file).
        /// </summary>
        String InputPath;

        /// <summary>
        /// The output path (folder or file).
        /// </summary>
        String OutputPath;

        /// <summary>
        /// Flag set to true for the assets handled by the engine internally.
        /// </summary>
        bool IsInBuilt;

        /// <summary>
        /// Flag used to skip showing import settings dialog to used. Can be used for importing assets from code by plugins.
        /// </summary>
        bool SkipSettingsDialog;

        /// <summary>
        /// The custom settings.
        /// </summary>
        std::any Settings;
    };
};
