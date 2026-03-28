#pragma once
#include "ImportFileEntry.h"
#include "Runtime/Utilities/Texture/TextureUtils.h"

namespace SE::Editor
{

    /// <summary>
    /// Texture asset import entry.
    /// </summary>
    class TextureImportEntry : public ImportFileEntry
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="TextureImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        TextureImportEntry(ImportRequest& request);

        /// <inheritdoc />
        bool Import() override;

    private:
        TextureUtils::Options m_Setting;
    };

} // SE

