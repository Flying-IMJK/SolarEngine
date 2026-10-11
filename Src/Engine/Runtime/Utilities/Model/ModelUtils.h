#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Math/Quaternion.h"
#include "Runtime/Core/Math/Transform.h"

#include "Runtime/API.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Render/Assets/Geometry/ModelData.h"
#include "Runtime/Resource/Assets/Materials/MaterialBase.h"

namespace SE
{
    /// <summary>
    /// The model file import data types (used as flags).
    /// </summary>
    enum class ImportDataTypes : int32
    {
        None = 0,

        /// <summary>
        /// Imports meshes (and LODs).
        /// </summary>
        Geometry = 1 << 0,

        /// <summary>
        /// Imports the skeleton bones hierarchy.
        /// </summary>
        Skeleton = 1 << 1,

        /// <summary>
        /// Imports the animations.
        /// </summary>
        Animations = 1 << 2,

        /// <summary>
        /// Imports the scene nodes hierarchy.
        /// </summary>
        Nodes = 1 << 3,

        /// <summary>
        /// Imports the materials.
        /// </summary>
        Materials = 1 << 4,

        /// <summary>
        /// Imports the textures.
        /// </summary>
        Textures = 1 << 5,
    };

    typedef ImportDataTypes ImportDataTypeFlags;

    SE_ENUM_OPERATORS(ImportDataTypeFlags);

    /// <summary>
    /// Declares the imported data type.
    /// </summary>
    SE_ENUM(API())
    enum class ModelUtilsModelType : int32
    {
        // The model asset.
        Model = 0,
        // The skinned model asset.
        SkinnedModel = 1,
        // The animation asset.
        Animation = 2,
        // The prefab scene.
        Prefab = 3,
    };

    /// <summary>
    /// Declares the imported animation clip duration.
    /// </summary>
    SE_ENUM(API())
    enum class ModelUtilsAnimationDuration : int32
    {
        // The imported duration.
        Imported = 0,
        // The custom duration specified via keyframes range.
        Custom = 1,
    };

    /// <summary>
    /// Declares the imported animation Root Motion modes.
    /// </summary>
    SE_ENUM(API())
    enum class ModelUtilsRootMotionMode
    {
        // Root Motion feature is disabled.
        None = 0,
        // Motion is extracted from the root node (or node specified by name).
        ExtractNode = 1,
        // Motion is extracted from the center of mass movement (estimated based on the skeleton pose animation).
        ExtractCenterOfMass = 2,
    };

    /// <summary>
    /// Model import options.
    /// </summary>
    SE_STRUCT(API(Attributes="HideInEditor"))
    struct SE_API_RUNTIME ModelUtilsOptions : public ISerializable
    { 
        SCRIPTING_TYPE_MIN(ModelUtilsOptions);

    public:
        // Type of the imported asset.
        SE_FIELD(API(Attributes="EditorOrder(0)"))
        ModelUtilsModelType Type = ModelUtilsModelType::Model;

    public: // Geometry
        // Enable model normal vectors re-calculating.
        SE_FIELD(API(Attributes="EditorOrder(20), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        bool CalculateNormals = false;
        // Specifies the maximum angle (in degrees) that may be between two face normals at the same vertex position before they are smoothed together. The default value is 175.
        SE_FIELD(API(Attributes="EditorOrder(30), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSmoothingNormalsAngle)), Limit(0, 175, 0.1f)"))
        float SmoothingNormalsAngle = 175.0f;
        // If checked, the imported normal vectors of the mesh will be flipped (scaled by -1).
        SE_FIELD(API(Attributes="EditorOrder(35), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        bool FlipNormals = false;
        // Enable model tangent vectors re-calculating.
        SE_FIELD(API(Attributes="EditorOrder(40), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        bool CalculateTangents = false;
        // Specifies the maximum angle (in degrees) that may be between two vertex tangents before their tangents and bi-tangents are smoothed. The default value is 45.
        // 0 ~ 45
        SE_FIELD(API(Attributes="EditorOrder(45), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSmoothingTangentsAngle)), Limit(0, 45, 0.1f)"))
        float SmoothingTangentsAngle = 45.0f;
        // Enable/disable meshes geometry optimization.
        SE_FIELD(API(Attributes="EditorOrder(50), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        bool OptimizeMeshes = true;
        // Enable/disable geometry merge for meshes with the same materials. Index buffer will be reordered to improve performance and other modifications will be applied. However, importing time will be increased.
        SE_FIELD(API(Attributes="EditorOrder(60), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        bool MergeMeshes = true;
        // Enable/disable importing meshes Level of Details.
        SE_FIELD(API(Attributes="EditorOrder(70), EditorDisplay(\"Geometry\", \"Import LODs\"), VisibleIf(nameof(ShowGeometry))"))
        bool ImportLODs = true;
        // Enable/disable importing vertex colors (channel 0 only).
        SE_FIELD(API(Attributes="EditorOrder(80), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        bool ImportVertexColors = true;
        // Enable/disable importing blend shapes (morph targets).
        SE_FIELD(API(Attributes="EditorOrder(85), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSkinnedModel))"))
        bool ImportBlendShapes = false;
        // Enable skeleton bones offset matrices re-calculating.
        SE_FIELD(API(Attributes="EditorOrder(86), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSkinnedModel))"))
        bool CalculateBoneOffsetMatrices = false;
        // The lightmap UVs source.
        //        ModelLightmapUVsSource LightmapUVsSource = ModelLightmapUVsSource::Disable;
        // If specified, all meshes that name starts with this prefix in the name will be imported as a separate collision data asset (excluded used for rendering).
        SE_FIELD(API(Attributes="EditorOrder(100), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))"))
        String CollisionMeshesPrefix = SE_TEXT("");
        // The type of collision that should be generated if the mesh has a collision prefix specified.
        //        CollisionDataType CollisionType = CollisionDataType::ConvexMesh;

    public: // Transform

        // Custom uniform import scale.
        SE_FIELD(API(Attributes="EditorOrder(500), EditorDisplay(\"Transform\")"))
        float Scale = 1.0f;
        // Custom import geometry rotation.
        SE_FIELD(API(Attributes="EditorOrder(510), EditorDisplay(\"Transform\")"))
        Quaternion Rotation = Quaternion::Identity;
        // Custom import geometry offset.
        SE_FIELD(API(Attributes="EditorOrder(520), EditorDisplay(\"Transform\")"))
        Float3 Translation = Float3::Zero;
        // If checked, the imported geometry will be shifted to its local transform origin.
        SE_FIELD(API(Attributes="EditorOrder(530), EditorDisplay(\"Transform\")"))
        bool UseLocalOrigin = false;
        // If checked, the imported geometry will be shifted to the center of mass.
        SE_FIELD(API(Attributes="EditorOrder(540), EditorDisplay(\"Transform\")"))
        bool CenterGeometry = false;

    public: // Animation

        // Imported animation duration mode. Can use the original value or be overriden by settings.
        SE_FIELD(API(Attributes="EditorOrder(1000), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))"))
        ModelUtilsAnimationDuration Duration = ModelUtilsAnimationDuration::Imported;
        // Imported animation first/last frame index. Used only if Duration mode is set to Custom.
        // 0
        SE_FIELD(API(Attributes="EditorOrder(1010), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowFramesRange)), Limit(0)"))
        Float2 FramesRange = Float2::Zero;
        // The imported animation default frame rate. Can specify the default frames per second amount for imported animations. If the value is 0 then the original animation frame rate will be used.
        // 0 ~ 1000
        SE_FIELD(API(Attributes="EditorOrder(1020), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation)), Limit(0, 1000, 0.01f)"))
        float DefaultFrameRate = 0.0f;
        // The imported animation sampling rate. If value is 0 then the original animation speed will be used.
        // 0 ~ 1000
        SE_FIELD(API(Attributes="EditorOrder(1030), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation)), Limit(0, 1000, 0.01f)"))
        float SamplingRate = 0.0f;
        // The imported animation will have tracks with no keyframes or unspecified data removed.
        SE_FIELD(API(Attributes="EditorOrder(1040), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))"))
        bool SkipEmptyCurves = true;
        // The imported animation channels will be optimized to remove redundant keyframes.
        SE_FIELD(API(Attributes="EditorOrder(1050), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))"))
        bool OptimizeKeyframes = true;
        // If checked, the importer will import scale animation tracks (otherwise scale animation will be ignored).
        SE_FIELD(API(Attributes="EditorOrder(1055), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))"))
        bool ImportScaleTracks = false;
        // Enables root motion extraction support from this animation.
        SE_FIELD(API(Attributes="EditorOrder(1060), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))"))
        ModelUtilsRootMotionMode RootMotion = ModelUtilsRootMotionMode::None;
        // Adjusts root motion applying flags. Can customize how root node animation can affect target actor movement (eg. apply both position and rotation changes).
        //        AnimationRootMotionFlags RootMotionFlags = AnimationRootMotionFlags::RootPositionXZ;
        // The custom node name to be used as a root motion source. If not specified the actual root node will be used.
        SE_FIELD(API(Attributes="EditorOrder(1070), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowRootMotion))"))
        String RootNodeName = SE_TEXT("");

    public: // Level Of Detail

        // If checked, the importer will generate a sequence of LODs based on the base LOD index.
        SE_FIELD(API(Attributes="EditorOrder(1100), EditorDisplay(\"Level Of Detail\", \"Generate LODs\"), VisibleIf(nameof(ShowGeometry))"))
        bool GenerateLODs = false;
        // The index of the LOD from the source model data to use as a reference for following LODs generation.
        // - ~ 5
        SE_FIELD(API(Attributes="EditorOrder(1110), EditorDisplay(\"Level Of Detail\", \"Base LOD\"), VisibleIf(nameof(ShowGenerateLODs)), Limit(0, 5, 0.065f)"))
        int32 BaseLOD = 0;
        // The amount of LODs to include in the model (all remaining ones starting from Base LOD will be generated).
        // 1 ~ 6
        SE_FIELD(API(Attributes="EditorOrder(1120), EditorDisplay(\"Level Of Detail\", \"LOD Count\"), VisibleIf(nameof(ShowGenerateLODs)), Limit(1, 6, 0.065f)"))
        int32 LODCount = 4;
        // The target amount of triangles for the generated LOD (based on the higher LOD). Normalized to range 0-1. For instance 0.4 cuts the triangle count to 40%.
        // 0 ~ 1
        SE_FIELD(API(Attributes="EditorOrder(1130), EditorDisplay(\"Level Of Detail\"), VisibleIf(nameof(ShowGenerateLODs)), Limit(0, 1, 0.001f)"))
        float TriangleReduction = 0.5f;
        // Whether to do a sloppy mesh optimization. This is faster but does not follow the topology of the original mesh.
        SE_FIELD(API(Attributes="EditorOrder(1140), EditorDisplay(\"Level Of Detail\"), VisibleIf(nameof(ShowGenerateLODs))"))
        bool SloppyOptimization = false;
        // Only used if Sloppy is false. Target error is an approximate measure of the deviation from the original mesh using distance normalized to [0..1] range (e.g. 1e-2f means that simplifier will try to maintain the error to be below 1% of the mesh extents).
        // 0.01f ~ 1
        SE_FIELD(API(Attributes="EditorOrder(1150), EditorDisplay(\"Level Of Detail\", \"LOD Target Error\"), VisibleIf(nameof(SloppyOptimization), true), VisibleIf(nameof(ShowGenerateLODs)), Limit(0.01f, 1, 0.001f)"))
        float LODTargetError = 0.05f;

    public: // Materials

        // If checked, the importer will create materials for model meshes as specified in the file.
        SE_FIELD(API(Attributes="EditorOrder(399), EditorDisplay(\"Materials\"), VisibleIf(nameof(ShowGeometry))"))
        bool ImportMaterials = true;
        // If checked, the importer will create the model's materials as instances of a base material.
        SE_FIELD(API(Attributes="EditorOrder(401), EditorDisplay(\"Materials\"), VisibleIf(nameof(ImportMaterials)), VisibleIf(nameof(ShowGeometry))"))
        bool ImportMaterialsAsInstances = false;
        // The material used as the base material that will be instanced as the imported model's material.
        SE_FIELD(API(Attributes="EditorOrder(402), EditorDisplay(\"Materials\"), VisibleIf(nameof(ImportMaterialsAsInstances)), VisibleIf(nameof(ShowGeometry))"))
        AssetRef<MaterialBase> InstanceToImportAs;
        // If checked, the importer will import texture files used by the model and any embedded texture resources.
        SE_FIELD(API(Attributes="EditorOrder(410), EditorDisplay(\"Materials\"), VisibleIf(nameof(ShowGeometry))"))
        bool ImportTextures = true;
        // If checked, the importer will try to keep the model's current overridden material slots, instead of importing materials from the source file.
        SE_FIELD(API(Attributes="EditorOrder(420), EditorDisplay(\"Materials\", \"Keep Overridden Materials\"), VisibleIf(nameof(ShowGeometry))"))
        bool RestoreMaterialsOnReimport = true;
        // If checked, the importer will not reimport any material from this model which already exist in the sub-asset folder.
        SE_FIELD(API(Attributes="EditorOrder(421), EditorDisplay(\"Materials\", \"Skip Existing Materials\"), VisibleIf(nameof(ShowGeometry))"))
        bool SkipExistingMaterialsOnReimport = true;

    public: // SDF

        // If checked, enables generation of Signed Distance Field (SDF).
        SE_FIELD(API(Attributes="EditorOrder(1500), EditorDisplay(\"SDF\"), VisibleIf(nameof(ShowModel))"))
        bool GenerateSDF = false;
        // Resolution scale for generated Signed Distance Field (SDF) texture. Higher values improve accuracy but increase memory usage and reduce performance.
        // 0.0001f, 100.0f
        SE_FIELD(API(Attributes="EditorOrder(1510), EditorDisplay(\"SDF\"), VisibleIf(nameof(ShowModel)), Limit(0.0001f, 100.0f)"))
        float SDFResolution = 1.0f;

    public: // Splitting

        // If checked, the imported mesh/animations are split into separate assets. Used if ObjectIndex is set to -1.
        SE_FIELD(API(Attributes="EditorOrder(2000), EditorDisplay(\"Splitting\"), VisibleIf(nameof(ShowSplitting))"))
        bool SplitObjects = false;
        // The zero-based index for the mesh/animation clip to import. If the source file has more than one mesh/animation it can be used to pick a desired object. Default -1 imports all objects.
        SE_FIELD(API(Attributes="EditorOrder(2010), EditorDisplay(\"Splitting\"), VisibleIf(nameof(ShowSplitting)), Limit(int.MinValue, int.MaxValue, 0.065f)"))
        int32 ObjectIndex = -1;

    public: // Other

        // If specified, the specified folder will be used as sub-directory name for automatically imported sub assets such as textures and materials. Set to whitespace (single space) to import to the same directory.
        SE_FIELD(API(Attributes="EditorOrder(3030), EditorDisplay(\"Other\")"))
        String SubAssetFolder = SE_TEXT("");

    public: // Internals

        // Internal flags for objects to import.
        ImportDataTypeFlags ImportTypes = ImportDataTypes::None;

        struct CachedData
        {
            ModelData* Data = nullptr;
            void* MeshesByName = nullptr;
        };
        // Cached model data - used when performing nested importing (eg. via objects splitting). Allows to read and process source file only once and use those results for creation of multiple assets (permutation via ObjectIndex).
        CachedData* Cached = nullptr;

    public:
        // [ISerializable]
        void Serialize(SerializeContext &context) override;
        void Deserialize(DeserializeContext &context) override;
    };


    class ModelUtils
    {
        // Optional: inputModel or modelData
        // Optional: outputSDF or null, outputStream or null
        //    static bool GenerateModelSDF(class Model* inputModel, class ModelData* modelData, float resolutionScale, int32 lodIndex, ModelBase::SDFData* outputSDF, class MemoryWriteStream* outputStream, const StringView& assetName, float backfacesThreshold = 0.6f);
    public:
        /// <summary>
        /// Imports the model source file data.
        /// </summary>
        /// <param name="path">The file path.</param>
        /// <param name="data">The output data.</param>
        /// <param name="options">The import options.</param>
        /// <param name="errorMsg">The error message container.</param>
        /// <returns>True if fails, otherwise false.</returns>
        static bool ImportData(const String& path, ModelData& data, ModelUtilsOptions& options, String& errorMsg);

        /// <summary>
        /// Imports the model.
        /// </summary>
        /// <param name="path">The file path.</param>
        /// <param name="data">The output data.</param>
        /// <param name="options">The import options.</param>
        /// <param name="errorMsg">The error message container.</param>
        /// <param name="autoImportOutput">The output folder for the additional imported data - optional. Used to auto-import textures and material assets.</param>
        /// <returns>True if fails, otherwise false.</returns>
        static bool ImportModel(const String& path, ModelData& data, ModelUtilsOptions& options, String& errorMsg, const String& autoImportOutput = String::Empty);

    public:
        static int32 DetectLodIndex(const String& nodeName);
        static bool FindTexture(const String& sourcePath, const String& file, String& path);

        /// <summary>
        /// Gets the local transformations to go from rootIndex to index.
        /// </summary>
        /// <param name="nodes">The nodes containing the local transformations.</param>
        /// <param name="rootIndex">The root index.</param>
        /// <param name="index">The current index.</param>
        /// <returns>The transformation at this index.</returns>
        template<typename Node>
        static Transform CombineTransformsFromNodeIndices(List<Node>& nodes, int32 rootIndex, int32 index)
        {
            if (index == -1 || index == rootIndex)
                return Transform::Identity;

            auto result = nodes[index].LocalTransform;
            if (index != rootIndex)
            {
                const auto parentTransform = CombineTransformsFromNodeIndices(nodes, rootIndex, nodes[index].ParentIndex);
                result = parentTransform.LocalToWorld(result);
            }

            return result;
        }

    private:
        // static void CalculateBoneOffsetMatrix(const List<SkeletonNode>& nodes, Matrix& offsetMatrix, int32 nodeIndex);

        static bool ImportDataAssimp(const String& path, ModelData& data, ModelUtilsOptions& options, String& errorMsg);
    };


} // SE
