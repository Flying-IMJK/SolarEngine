
add_subdirectory(Factories)
add_subdirectory(Loading)
add_subdirectory(Storage)
add_subdirectory(Importers)

RUNTIME_ADD(
        Asset.h
        Asset.cpp
        BinaryAsset.h
        BinaryAsset.cpp
        AssetInfo.h
        AssetConfig.h

        AssetReference.h
        AssetContent.h
        AssetContent.cpp
        AssetsCache.h
        AssetsCache.cpp

        WeakAssetReference.h

        IResource.h
        ResourceID.h
        ResourceID.cpp
        ResourceLoader.h
        ResourceLoader.cpp
        ResourceProvider.h
        ResourcePtr.h
        ResourceRecord.h
        ResourceRecord.cpp
        ResourceRequest.h
        ResourceRequest.cpp
        ResourceRequesterID.h
        ResourceDescriptor.h
        ResourceDescriptor.cpp

        PackagedResourceProvider.h
        PackagedResourceProvider.cpp
)