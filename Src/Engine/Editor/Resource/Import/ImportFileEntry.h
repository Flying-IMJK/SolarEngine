#pragma once
#include <any>

#include "Runtime/Core/Types/Strings/String.h"

namespace SE::Editor
{
    struct ImportRequest;
    class ImportFileEntry;

    using ImportFileEntryHandler = Function<ImportFileEntry*(ImportRequest&)>;

    /// <summary>
    /// File entry action (import or create).
    /// </summary>
    class IFileEntryAction
    {
    public:
        /// <summary>
        /// The source file path (may be empty or null).
        /// </summary>
        virtual StringView GetSourceUrl() = 0;

        /// <summary>
        /// The result file path.
        /// </summary>
        virtual StringView GetResultUrl() = 0;

        /// <summary>
        /// Executes this action.
        /// </summary>
        /// <returns>True if, failed, otherwise false.</returns>
        virtual bool Execute() = 0;
    };

    /// <summary>
    /// File import entry.
    /// </summary>
    class ImportFileEntry : IFileEntryAction
    {
    public:
        /// <inheritdoc />
        String SourceUrl;

        /// <inheritdoc />
        String ResultUrl;

        virtual std::any& GetSetting() { return empty; }

        bool HasSetting() { return GetSetting().has_value(); }

        virtual bool TryOverrideSetting(std::any& setting) { return false; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ImportFileEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        ImportFileEntry(ImportRequest& request);

        /// <summary>
        /// Modifies the result URL filename (keeps destination folder and extension).
        /// </summary>
        /// <param name="filename">The new filename.</param>
        void ModifyResultFilename(String filename);

        /// <summary>
        /// Performs file importing.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        virtual bool Import();

        StringView GetSourceUrl() override;
        StringView GetResultUrl() override;

        /// <inheritdoc />
        bool Execute() override
        {
            return Import();
        }

    private:
        static std::any empty;
    };

} // SE

