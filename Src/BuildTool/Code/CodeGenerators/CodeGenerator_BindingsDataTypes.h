#pragma once

// CodeGenerator_BindingsDataTypes.h
// Retained types that are generation-only concerns (not part of DataType).

#include "../Database/DataTypes.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/List.h"

namespace SE::ReflectTool
{
    /// All API-annotated items extracted from a single header file.
    /// Temporarily retained during the merge — will be removed once all generators
    /// consume DataType::bindingInfo directly.
    struct BindingsHeaderInfo
    {
        StringAnsi         filePath;
        StringAnsi         assemblyName;  // e.g. "SE.Core", "SE.Runtime", "SE.Editor"
        StringAnsi         assemblyDir;   // assembly TargetDir absolute path
        uint64             contentHash = 0;  // for incremental build
        List<ApiClass>     classes;
        List<ApiEnum>      enums;
        List<ApiInterface> interfaces;
        List<ApiEvent>     events;
    };

    /// Information about a binary module for generating module-level files.
    struct BinaryModuleInfo
    {
        StringAnsi name;           // e.g. "SE.Runtime"
        StringAnsi assemblyType;    // e.g. "Runtime" (stripped from name)
        StringAnsi assemblyDir;     // output directory
        List<const BindingsHeaderInfo*> headers;
    };

} // namespace SE::ReflectTool