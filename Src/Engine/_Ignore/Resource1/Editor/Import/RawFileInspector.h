#pragma once

#include "Editor/API.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/StringID.h"
#include "Core/TypeSystem/IReflectedType.h"
#include "Runtime/Resource/ResourceID.h"
#include "Core/Tools/Time.h"

//-------------------------------------------------------------------------

namespace SGE::Editor
{
    class EditorContext;
}

//-------------------------------------------------------------------------

namespace SGE::Editor::Import
{
    //-------------------------------------------------------------------------
    // Describes an importable piece of data present in a raw file
    //-------------------------------------------------------------------------

    struct ImportableItem : public IReflectedType
    {
        SE_CLASS( ImportableItem, IReflectedType)

        ImportableItem() = default;
        ImportableItem( ResPath const& path, StringID nameID ) : m_sourceFile ( path ), m_nameID( nameID ) {}

        inline bool IsValid() const { return m_sourceFile.IsValid() && m_nameID.IsValid(); }
        virtual String GetDescription() const { return String::Empty; }

    public:

        ResPath                    m_sourceFile;
        StringID                        m_nameID;
    };

    //-------------------------------------------------------------------------
    // Importable Data
    //-------------------------------------------------------------------------
    // TODO: Make this extensible once we have a lot of different types

    struct ImportableMesh : public ImportableItem
    {
        SE_CLASS( ImportableMesh, ImportableItem)

        virtual String GetDescription() const override
        {
            return String(m_materialID.IsValid() ? m_materialID.ToString() : SE_TEXT("No Material Set"));
        }

        StringID                        m_materialID;
        String                          m_extraInfo;
        bool                            m_isSkeletalMesh = false;
    };

    //-------------------------------------------------------------------------

    struct ImportableSkeleton : public ImportableItem
    {
        SE_CLASS( ImportableSkeleton, ImportableItem);

        inline bool IsNullOrLocatorNode() const { return !m_childSkeletonRoots.IsEmpty(); }
        inline bool IsSkeletonNode() const { return m_childSkeletonRoots.IsEmpty(); }

        virtual String GetDescription() const override
        {
            return String(IsNullOrLocatorNode() ? SE_TEXT("Null/Locator") : SE_TEXT("Skeleton Node") );
        }

    public:

        List<StringID>               m_childSkeletonRoots; // Only filled if this is a null root
    };

    //-------------------------------------------------------------------------

    struct ImportableAnimation : public ImportableItem
    {
        SE_CLASS(ImportableAnimation, ImportableItem)

        virtual String GetDescription() const override
        {
            float const numFrames = m_duration.ToFloat() * m_frameRate;
            return String::Format(SE_TEXT("{:.2f} Frames, {.2f}s, {.2f} fps"), numFrames, m_duration.ToFloat(), m_frameRate );
        }

    public:

        Seconds                         m_duration = 0.0f;
        float                           m_frameRate = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct ImportableImage : public ImportableItem
    {
        SE_CLASS( ImportableImage, ImportableItem);

        virtual String GetDescription() const override
        { 
            return String::Format(SE_TEXT("{0} x {1}, channels: {2}"), m_dimensions.x, m_dimensions.y, m_numChannels );
        }

    public:

        Int2                            m_dimensions = Int2::Zero;
        int32_t                         m_numChannels = 0;
    };

    //-------------------------------------------------------------------------
    // Inspector
    //-------------------------------------------------------------------------

    struct InspectorContext
    {
        inline bool IsValid() const{ return m_warningDelegate.IsBinded() && m_errorDelegate.IsBinded() && !m_rawResourceDirectoryPath.IsEmpty(); }

    public:
        template<typename... Args>
        inline void LogWarning(Char const* pFormat, Args... args) const
        {
            String str = String::Format(pFormat, args...);
            m_warningDelegate(str);
        }

        template<typename... Args>
        inline void LogError( Char const* pFormat, Args... args) const
        {
            String str = String::Format(pFormat, args...);
            m_errorDelegate(str);
        }

        InspectorContext() : m_warningDelegate(), m_errorDelegate(), m_rawResourceDirectoryPath(){}
    public:

		Function<void(String)>    m_warningDelegate;
		Function<void(String)>    m_errorDelegate;
        String			          m_rawResourceDirectoryPath;
    };

    //-------------------------------------------------------------------------

    enum class InspectionResult
    {
        Uninspected,
        Success,
        Failure,
        UnsupportedExtension
    };

    SE_API_EDITOR InspectionResult InspectFile( InspectorContext const& ctx, String const& sourceFilePath, List<ImportableItem*>& outFileInfo);

    SE_API_EDITOR bool IsImportableFileType(String const& extension );
}