#pragma once

#include "Editor/API.h"
#include "Core/Types/Collections/List.h"
#include "Core/Math/Matrix.h"
#include "Core/Types/Strings/StringID.h"

//-------------------------------------------------------------------------

namespace SGE::Editor::Import
{
    class SE_API_EDITOR ImportedData
    {

    public:

        ImportedData() = default;
        ImportedData( ImportedData const& ) = default;
        virtual ~ImportedData() = default;

        ImportedData& operator=( ImportedData const& rhs ) = default;

        virtual bool IsValid() const = 0;

        inline bool HasWarnings() const { return !m_warnings.IsEmpty(); }
        inline List<String> const& GetWarnings() const { return m_warnings; }

        inline bool HasErrors() const { return !m_errors.IsEmpty(); }
        inline List<String> const& GetErrors() const { return m_errors; }

        inline String const& GetSourcePath() const { return m_sourcePath; }
    protected:

		String                 		m_sourcePath;
        List<String>                m_warnings;
		List<String>                m_errors;
    };
}