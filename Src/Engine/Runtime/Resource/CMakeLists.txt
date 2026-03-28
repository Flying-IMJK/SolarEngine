
add_subdirectory(Assets)
add_subdirectory(Factories)
add_subdirectory(Loading)
add_subdirectory(Storage)
add_subdirectory(Importers)
add_subdirectory(Streaming)

RUNTIME_ADD(
        Asset.h
        Asset.cpp
        BinaryAsset.h
        BinaryAsset.cpp
        JsonAsset.h
        JsonAsset.cpp
        AssetInfo.h
        AssetConfig.h

        AssetRef.h
        AssetContent.h
        AssetContent.cpp
        AssetsCache.h
        AssetsCache.cpp
        AssetsContainer.h
        AssetRefSerialize.h
)