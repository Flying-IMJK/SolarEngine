#pragma once

#include "Core/Types/Variable.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Transform.h"

#include "Runtime/API.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Render/Assets/Geometry/ModelData.h"
#include "../../Resource/Assets/Materials/MaterialBase.h"

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

    typedef EnumFlags<ImportDataTypes> ImportDataTypeFlags;

    class ModelUtils
    {
        // Optional: inputModel or modelData
        // Optional: outputSDF or null, outputStream or null
        //    static bool GenerateModelSDF(class Model* inputModel, class ModelData* modelData, float resolutionScale, int32 lodIndex, ModelBase::SDFData* outputSDF, class MemoryWriteStream* outputStream, const StringView& assetName, float backfacesThreshold = 0.6f);

#if SE_EDITOR

    public:
        /// <summary>
        /// Declares the imported data type.
        /// </summary>
        enum class ModelType : int32
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
        enum class AnimationDuration : int32
        {
            // The imported duration.
            Imported = 0,
            // The custom duration specified via keyframes range.
            Custom = 1,
        };

        /// <summary>
        /// Declares the imported animation Root Motion modes.
        /// </summary>
        enum class RootMotionMode
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
        struct SE_API_RUNTIME Options : public ISerializable
        {
            // Type of the imported asset.
            ModelType Type = ModelType::Model;

        public: // Geometry

            // Enable model normal vectors re-calculating.
            bool CalculateNormals = false;
            // Specifies the maximum angle (in degrees) that may be between two face normals at the same vertex position before they are smoothed together. The default value is 175.
            // 0 ~ 175
            float SmoothingNormalsAngle = 175.0f;
            // If checked, the imported normal vectors of the mesh will be flipped (scaled by -1).
            bool FlipNormals = false;
            // Enable model tangent vectors re-calculating.
            bool CalculateTangents = false;
            // Specifies the maximum angle (in degrees) that may be between two vertex tangents before their tangents and bi-tangents are smoothed. The default value is 45.
            // 0 ~ 45
            float SmoothingTangentsAngle = 45.0f;
            // Enable/disable meshes geometry optimization.
            bool OptimizeMeshes = true;
            // Enable/disable geometry merge for meshes with the same materials. Index buffer will be reordered to improve performance and other modifications will be applied. However, importing time will be increased.
            bool MergeMeshes = true;
            // Enable/disable importing meshes Level of Details.
            bool ImportLODs = true;
            // Enable/disable importing vertex colors (channel 0 only).
            bool ImportVertexColors = true;
            // Enable/disable importing blend shapes (morph targets).
            bool ImportBlendShapes = false;
            // Enable skeleton bones offset matrices re-calculating.
            bool CalculateBoneOffsetMatrices = false;
            // The lightmap UVs source.
            //        ModelLightmapUVsSource LightmapUVsSource = ModelLightmapUVsSource::Disable;
            // If specified, all meshes that name starts with this prefix in the name will be imported as a separate collision data asset (excluded used for rendering).
            String CollisionMeshesPrefix = SE_TEXT("");
            // The type of collision that should be generated if the mesh has a collision prefix specified.
            //        CollisionDataType CollisionType = CollisionDataType::ConvexMesh;

        public: // Transform

            // Custom uniform import scale.
            float Scale = 1.0f;
            // Custom import geometry rotation.
            Quaternion Rotation = Quaternion::Identity;
            // Custom import geometry offset.
            Float3 Translation = Float3::Zero;
            // If checked, the imported geometry will be shifted to its local transform origin.
            bool UseLocalOrigin = false;
            // If checked, the imported geometry will be shifted to the center of mass.
            bool CenterGeometry = false;

        public: // Animation

            // Imported animation duration mode. Can use the original value or be overriden by settings.
            AnimationDuration Duration = AnimationDuration::Imported;
            // Imported animation first/last frame index. Used only if Duration mode is set to Custom.
            // 0
            Float2 FramesRange = Float2::Zero;
            // The imported animation default frame rate. Can specify the default frames per second amount for imported animations. If the value is 0 then the original animation frame rate will be used.
            // 0 ~ 1000
            float DefaultFrameRate = 0.0f;
            // The imported animation sampling rate. If value is 0 then the original animation speed will be used.
            // 0 ~ 1000
            float SamplingRate = 0.0f;
            // The imported animation will have tracks with no keyframes or unspecified data removed.
            bool SkipEmptyCurves = true;
            // The imported animation channels will be optimized to remove redundant keyframes.
            bool OptimizeKeyframes = true;
            // If checked, the importer will import scale animation tracks (otherwise scale animation will be ignored).
            bool ImportScaleTracks = false;
            // Enables root motion extraction support from this animation.
            RootMotionMode RootMotion = RootMotionMode::None;
            // Adjusts root motion applying flags. Can customize how root node animation can affect target actor movement (eg. apply both position and rotation changes).
            //        AnimationRootMotionFlags RootMotionFlags = AnimationRootMotionFlags::RootPositionXZ;
            // The custom node name to be used as a root motion source. If not specified the actual root node will be used.
            String RootNodeName = SE_TEXT("");

        public: // Level Of Detail

            // If checked, the importer will generate a sequence of LODs based on the base LOD index.
            bool GenerateLODs = false;
            // The index of the LOD from the source model data to use as a reference for following LODs generation.
            // - ~ 5
            int32 BaseLOD = 0;
            // The amount of LODs to include in the model (all remaining ones starting from Base LOD will be generated).
            // 1 ~ 6
            int32 LODCount = 4;
            // The target amount of triangles for the generated LOD (based on the higher LOD). Normalized to range 0-1. For instance 0.4 cuts the triangle count to 40%.
            // 0 ~ 1
            float TriangleReduction = 0.5f;
            // Whether to do a sloppy mesh optimization. This is faster but does not follow the topology of the original mesh.
            bool SloppyOptimization = false;
            // Only used if Sloppy is false. Target error is an approximate measure of the deviation from the original mesh using distance normalized to [0..1] range (e.g. 1e-2f means that simplifier will try to maintain the error to be below 1% of the mesh extents).
            // 0.01f ~ 1
            float LODTargetError = 0.05f;

        public: // Materials

            // If checked, the importer will create materials for model meshes as specified in the file.
            bool ImportMaterials = true;
            // If checked, the importer will create the model's materials as instances of a base material.
            bool ImportMaterialsAsInstances = false;
            // The material used as the base material that will be instanced as the imported model's material.
            AssetRef<MaterialBase> InstanceToImportAs;
            // If checked, the importer will import texture files used by the model and any embedded texture resources.
            bool ImportTextures = true;
            // If checked, the importer will try to keep the model's current overridden material slots, instead of importing materials from the source file.
            bool RestoreMaterialsOnReimport = true;
            // If checked, the importer will not reimport any material from this model which already exist in the sub-asset folder.
            bool SkipExistingMaterialsOnReimport = true;

        public: // SDF

            // If checked, enables generation of Signed Distance Field (SDF).
            bool GenerateSDF = false;
            // Resolution scale for generated Signed Distance Field (SDF) texture. Higher values improve accuracy but increase memory usage and reduce performance.
            // 0.0001f, 100.0f
            float SDFResolution = 1.0f;

        public: // Splitting

            // If checked, the imported mesh/animations are split into separate assets. Used if ObjectIndex is set to -1.
            bool SplitObjects = false;
            // The zero-based index for the mesh/animation clip to import. If the source file has more than one mesh/animation it can be used to pick a desired object. Default -1 imports all objects.
            int32 ObjectIndex = -1;

        public: // Other

            // If specified, the specified folder will be used as sub-directory name for automatically imported sub assets such as textures and materials. Set to whitespace (single space) to import to the same directory.
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

    public:
        /// <summary>
        /// Imports the model source file data.
        /// </summary>
        /// <param name="path">The file path.</param>
        /// <param name="data">The output data.</param>
        /// <param name="options">The import options.</param>
        /// <param name="errorMsg">The error message container.</param>
        /// <returns>True if fails, otherwise false.</returns>
        static bool ImportData(const String& path, ModelData& data, Options& options, String& errorMsg);

        /// <summary>
        /// Imports the model.
        /// </summary>
        /// <param name="path">The file path.</param>
        /// <param name="data">The output data.</param>
        /// <param name="options">The import options.</param>
        /// <param name="errorMsg">The error message container.</param>
        /// <param name="autoImportOutput">The output folder for the additional imported data - optional. Used to auto-import textures and material assets.</param>
        /// <returns>True if fails, otherwise false.</returns>
        static bool ImportModel(const String& path, ModelData& data, Options& options, String& errorMsg, const String& autoImportOutput = String::Empty);

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

        static bool ImportDataAssimp(const String& path, ModelData& data, Options& options, String& errorMsg);
    };
#endif

} // SE