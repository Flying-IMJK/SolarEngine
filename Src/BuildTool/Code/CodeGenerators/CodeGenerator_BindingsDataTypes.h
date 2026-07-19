#pragma once

// CodeGenerator_BindingsDataTypes.h
// Retained types that are generation-only concerns (not part of DataType).

#include "Database/DataTypes.h"

namespace SE::BuildTool
{
    /// Raw source code injected into generated binding files.
    struct ApiInjectedCode
    {
        std::string lang;
        std::string code;
        int         lineNumber = -1;
    };

    /// All API-annotated items extracted from a single header file.
    /// Temporarily retained during the merge - will be removed once all generators
    /// consume DataType::bindingInfo directly.
    struct BindingsHeaderInfo
    {
        std::string         filePath;
        std::string         assemblyName;  // e.g. "SE.Core", "SE.Runtime", "SE.Editor"
        std::string         assemblyDir;   // assembly TargetDir absolute path
        uint64             contentHash = 0;  // for incremental build
        std::vector<ApiClass>     classes;
        std::vector<ApiEnum>      enums;
        std::vector<ApiInterface> interfaces;
        std::vector<ApiEvent>     events;
        std::vector<ApiInjectedCode> injectedCode;
    };

    /// Information about a binary module for generating module-level files.
    struct BinaryModuleInfo
    {
        std::string name;           // e.g. "SE.Runtime"
        std::string assemblyType;    // e.g. "Runtime" (stripped from name)
        std::string assemblyDir;     // output directory
        std::vector<const BindingsHeaderInfo*> headers;
    };

} // namespace SE::BuildTool
