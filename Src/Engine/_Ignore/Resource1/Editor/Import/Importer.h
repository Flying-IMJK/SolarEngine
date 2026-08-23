#pragma once

#include "Editor/API.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/StringID.h"

//-------------------------------------------------------------------------

namespace SGE::Editor::Import
{
    class ImportedMesh;
    class ImportedSkeleton;
    class ImportedAnimation;
    class ImportedImage;

    //-------------------------------------------------------------------------

    struct ReaderContext
    {
        inline bool IsValid() const{ return m_warningDelegate.IsBinded() && m_errorDelegate.IsBinded(); }

        Function<void(Char const*)>  m_warningDelegate;
		Function<void(Char const*)>  m_errorDelegate;
    };

    //-------------------------------------------------------------------------

    // SE_API_EDITOR Scope<ImportedSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName = String(), Vector<StringID> const& listOfHighLODBones = Vector<StringID>() );
    // SE_API_EDITOR Scope<ImportedAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, Import::ImportedSkeleton const& importedSkeleton, String const& animationName = String() );
    // SE_API_EDITOR Scope<ImportedMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, Vector<String> const& meshesToInclude = Vector<String>() );
    // SE_API_EDITOR Scope<ImportedMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, Vector<String> const& meshesToInclude = Vector<String>(), int32 maxBoneInfluences = 4 );
    SE_API_EDITOR Scope<ImportedImage> ReadImage( ReaderContext const& ctx, String const& sourceFilePath );
}