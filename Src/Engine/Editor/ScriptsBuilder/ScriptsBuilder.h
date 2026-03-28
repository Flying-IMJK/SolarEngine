#pragma once

#ifdef SE_EDITOR

#include "Editor/API.h"

namespace SE
{
    /// <summary>
    /// Forward declaration placeholder for ScriptsBuilder.
    /// Full implementation is provided in Task 14 (Editor ScriptsBuilder / Hot-Reload).
    ///
    /// Monitors the Scripts/ directory for .cs file changes and triggers
    /// automatic recompilation via dotnet build, supporting hot-reload.
    /// Only compiled when SE_EDITOR is defined.
    /// </summary>
    class SE_API_EDITOR ScriptsBuilder
    {
    public:
        // TODO: Implement in Task 14
        // - Monitor Scripts/ directory for .cs file changes
        // - Trigger dotnet build on change
        // - Implement hot-reload: ScriptsReloading event -> unload old ALC -> load new assembly -> ScriptsReloaded event
        // - Expose CompileScripts() for manual trigger from editor UI
        // - Display compile errors (filename, line, description) in editor log window
        // - Preserve scene object state across hot-reload via serialization/deserialization

        /// <summary>
        /// Manually triggers script compilation.
        /// </summary>
        static void CompileScripts();
    };

} // namespace SE

#endif // SE_EDITOR
