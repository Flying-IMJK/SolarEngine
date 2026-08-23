#include "Importer.h"
// #include "ImportedMesh.h"
// #include "ImportedSkeleton.h"
// #include "ImportedAnimation.h"
#include "ImportedImage.h"
#include "Core/Memory/Memory.h"
// #include "Formats/FBX.h"
// #include "Formats/GLTF.h"

#include "stb/stb_image.h"


//-------------------------------------------------------------------------

namespace SGE::Editor::Import
{
    static bool ValidateRawAsset(ReaderContext const &ctx, ImportedData const *pRawAsset)
    {
        if (pRawAsset)
        {
            for (auto const &warning : pRawAsset->GetWarnings())
            {
//                ctx.m_warningDelegate(warning.Get());
            }

            for (auto const &error : pRawAsset->GetErrors())
            {
//                ctx.m_errorDelegate(error.Get());
            }

            return !pRawAsset->HasErrors() && pRawAsset->IsValid();
        }

        return false;
    }

    //-------------------------------------------------------------------------

    // Scope<ImportedMesh> ReadStaticMesh(ReaderContext const &ctx, FileSystem::Path const &sourceFilePath, Vector<String> const &meshesToInclude)
    // {
    //     ENGINE_ASSERT(sourceFilePath.IsValid() && ctx.IsValid());

    //     Scope<ImportedMesh> pImportedMesh = nullptr;

    //     auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
    //     if (extension == "fbx")
    //     {
    //         pImportedMesh = Fbx::ReadStaticMesh(sourceFilePath, meshesToInclude);
    //     }
    //     else if (extension == "gltf" || extension == "glb")
    //     {
    //         pImportedMesh = gltf::ReadStaticMesh(sourceFilePath, meshesToInclude);
    //     }
    //     else
    //     {
    //         char buffer[512];
    //         Printf(buffer, 512, "unsupported extension: %s", sourceFilePath.c_str());
    //         ctx.m_errorDelegate(buffer);
    //     }

    //     //-------------------------------------------------------------------------

    //     if (!ValidateRawAsset(ctx, pImportedMesh.get()))
    //     {
    //         pImportedMesh = nullptr;
    //     }

    //     //-------------------------------------------------------------------------

    //     return pImportedMesh;
    // }

    // //-------------------------------------------------------------------------

    // Scope<ImportedMesh> ReadSkeletalMesh(ReaderContext const &ctx, FileSystem::Path const &sourceFilePath, Vector<String> const &meshesToInclude, int32_t maxBoneInfluences)
    // {
    //     ENGINE_ASSERT(sourceFilePath.IsValid() && ctx.IsValid());

    //     Scope<ImportedMesh> pImportedMesh = nullptr;

    //     auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
    //     if (extension == "fbx")
    //     {
    //         pImportedMesh = Fbx::ReadSkeletalMesh(sourceFilePath, meshesToInclude, maxBoneInfluences);
    //     }
    //     else if (extension == "gltf" || extension == "glb")
    //     {
    //         pImportedMesh = gltf::ReadSkeletalMesh(sourceFilePath, meshesToInclude, maxBoneInfluences);
    //     }
    //     else
    //     {
    //         char buffer[512];
    //         Printf(buffer, 512, "unsupported extension: %s", sourceFilePath.c_str());
    //         ctx.m_errorDelegate(buffer);
    //     }

    //     //-------------------------------------------------------------------------

    //     if (!ValidateRawAsset(ctx, pImportedMesh.get()))
    //     {
    //         pImportedMesh = nullptr;
    //     }

    //     //-------------------------------------------------------------------------

    //     return pImportedMesh;
    // }

    // //-------------------------------------------------------------------------

    // Scope<ImportedSkeleton> ReadSkeleton(ReaderContext const &ctx, FileSystem::Path const &sourceFilePath, String const &skeletonRootBoneName, Vector<StringID> const &listOfHighLODBones)
    // {
    //     ENGINE_ASSERT(sourceFilePath.IsValid() && ctx.IsValid());

    //     Scope<ImportedSkeleton> pImportedSkeleton = nullptr;

    //     auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
    //     if (extension == "fbx")
    //     {
    //         pImportedSkeleton = Fbx::ReadSkeleton(sourceFilePath, skeletonRootBoneName);
    //     }
    //     else if (extension == "gltf" || extension == "glb")
    //     {
    //         pImportedSkeleton = gltf::ReadSkeleton(sourceFilePath, skeletonRootBoneName);
    //     }
    //     else
    //     {
    //         char buffer[512];
    //         Printf(buffer, 512, "unsupported extension: %s", sourceFilePath.c_str());
    //         ctx.m_errorDelegate(buffer);
    //     }

    //     pImportedSkeleton->Finalize(listOfHighLODBones);

    //     //-------------------------------------------------------------------------

    //     if (!ValidateRawAsset(ctx, pImportedSkeleton.get()))
    //     {
    //         pImportedSkeleton = nullptr;
    //     }

    //     //-------------------------------------------------------------------------

    //     return pImportedSkeleton;
    // }

    // //-------------------------------------------------------------------------

    // Scope<ImportedAnimation> ReadAnimation(ReaderContext const &ctx, FileSystem::Path const &sourceFilePath, ImportedSkeleton const &importedSkeleton, String const &animationName)
    // {
    //     ENGINE_ASSERT(ctx.IsValid() && sourceFilePath.IsValid() && importedSkeleton.IsValid());

    //     Scope<ImportedAnimation> pImportedAnimation = nullptr;

    //     auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
    //     if (extension == "fbx")
    //     {
    //         pImportedAnimation = Fbx::ReadAnimation(sourceFilePath, importedSkeleton, animationName);
    //     }
    //     else if (extension == "gltf" || extension == "glb")
    //     {
    //         pImportedAnimation = gltf::ReadAnimation(sourceFilePath, importedSkeleton, animationName);
    //     }
    //     else
    //     {
    //         TInlineString<512> errorString;
    //         errorString.sprintf("unsupported extension: %s", sourceFilePath.c_str());
    //         ctx.m_errorDelegate(errorString.c_str());
    //     }

    //     //-------------------------------------------------------------------------

    //     if (pImportedAnimation != nullptr)
    //     {
    //         if (!pImportedAnimation->HasErrors())
    //         {
    //             pImportedAnimation->Finalize();
    //         }

    //         //-------------------------------------------------------------------------

    //         if (!ValidateRawAsset(ctx, pImportedAnimation.get()))
    //         {
    //             pImportedAnimation = nullptr;
    //         }
    //     }

    //     //-------------------------------------------------------------------------

    //     return pImportedAnimation;
    // }

    //-------------------------------------------------------------------------

    class StbImportedImage : public ImportedImage
    {
        friend Scope<ImportedImage> ReadImage(ReaderContext const &ctx, String const &sourceFilePath);

    public:
        using ImportedImage::ImportedImage;
    };

    Scope<ImportedImage> ReadImage(ReaderContext const &ctx, String const &sourceFilePath)
    {
        ENGINE_ASSERT(ctx.IsValid() && !sourceFilePath.IsEmpty());

        Scope<ImportedImage> pImportedImage(New<StbImportedImage>());
        StbImportedImage *pImage = (StbImportedImage *)pImportedImage.get();

        //-------------------------------------------------------------------------

        pImage->m_pImageData = stbi_load(sourceFilePath.ToStringAnsi().Get(), &pImage->m_width, &pImage->m_height, &pImage->m_channels, STBI_rgb_alpha);
        if (pImage->m_pImageData == nullptr)
        {
            pImportedImage = nullptr;
        }
        else
        {
            pImage->m_stride = pImage->m_width * STBI_rgb_alpha;
        }

        //-------------------------------------------------------------------------

        return pImportedImage;
    }
}