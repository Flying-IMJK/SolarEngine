
#include "ModelUtils.h"

#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Types/DateTime.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Runtime/Render/Assets/Geometry/Model.h"
#include "Runtime/Resource/AssetConfig.h"
#include "Runtime/Resource/Assets/Materials/MaterialInstance.h"
#include "Runtime/Resource/AssetRefSerialize.h"

#include "meshoptimizer/meshoptimizer.h"

namespace SE
{
#if SE_EDITOR

    void ModelUtils::Options::Serialize(SerializeContext& context)
    {
        SERIALIZE_GET_OTHER_OBJ(ModelUtils::Options, context.otherObj);

        SERIALIZE(Type);
        SERIALIZE(CalculateNormals);
        SERIALIZE(SmoothingNormalsAngle);
        SERIALIZE(FlipNormals);
        SERIALIZE(CalculateTangents);
        SERIALIZE(SmoothingTangentsAngle);
        SERIALIZE(OptimizeMeshes);
        SERIALIZE(MergeMeshes);
        SERIALIZE(ImportLODs);
        SERIALIZE(ImportVertexColors);
        SERIALIZE(ImportBlendShapes);
        SERIALIZE(CalculateBoneOffsetMatrices);
        // SERIALIZE(LightmapUVsSource);
        SERIALIZE(CollisionMeshesPrefix);
        SERIALIZE(Scale);
        SERIALIZE(Rotation);
        SERIALIZE(Translation);
        SERIALIZE(UseLocalOrigin);
        SERIALIZE(CenterGeometry);
        SERIALIZE(Duration);
        SERIALIZE(FramesRange);
        SERIALIZE(DefaultFrameRate);
        SERIALIZE(SamplingRate);
        SERIALIZE(SkipEmptyCurves);
        SERIALIZE(OptimizeKeyframes);
        SERIALIZE(ImportScaleTracks);
        SERIALIZE(RootMotion);
        // SERIALIZE(RootMotionFlags);
        SERIALIZE(RootNodeName);
        SERIALIZE(GenerateLODs);
        SERIALIZE(BaseLOD);
        SERIALIZE(LODCount);
        SERIALIZE(TriangleReduction);
        SERIALIZE(SloppyOptimization);
        SERIALIZE(LODTargetError);
        SERIALIZE(ImportMaterials);
        SERIALIZE(ImportMaterialsAsInstances);
        SERIALIZE(InstanceToImportAs);
        SERIALIZE(ImportTextures);
        SERIALIZE(RestoreMaterialsOnReimport);
        SERIALIZE(SkipExistingMaterialsOnReimport);
        SERIALIZE(GenerateSDF);
        SERIALIZE(SDFResolution);
        SERIALIZE(SplitObjects);
        SERIALIZE(ObjectIndex);
        SERIALIZE(SubAssetFolder);
    }

    void ModelUtils::Options::Deserialize(DeserializeContext& context)
    {
        DESERIALIZE(Type);
        DESERIALIZE(CalculateNormals);
        DESERIALIZE(SmoothingNormalsAngle);
        DESERIALIZE(FlipNormals);
        DESERIALIZE(CalculateTangents);
        DESERIALIZE(SmoothingTangentsAngle);
        DESERIALIZE(OptimizeMeshes);
        DESERIALIZE(MergeMeshes);
        DESERIALIZE(ImportLODs);
        DESERIALIZE(ImportVertexColors);
        DESERIALIZE(ImportBlendShapes);
        DESERIALIZE(CalculateBoneOffsetMatrices);
        // DESERIALIZE(LightmapUVsSource);
        DESERIALIZE(CollisionMeshesPrefix);
        DESERIALIZE(Scale);
        DESERIALIZE(Rotation);
        DESERIALIZE(Translation);
        DESERIALIZE(UseLocalOrigin);
        DESERIALIZE(CenterGeometry);
        DESERIALIZE(Duration);
        DESERIALIZE(FramesRange);
        DESERIALIZE(DefaultFrameRate);
        DESERIALIZE(SamplingRate);
        DESERIALIZE(SkipEmptyCurves);
        DESERIALIZE(OptimizeKeyframes);
        DESERIALIZE(ImportScaleTracks);
        DESERIALIZE(RootMotion);
        // DESERIALIZE(RootMotionFlags);
        DESERIALIZE(RootNodeName);
        DESERIALIZE(GenerateLODs);
        DESERIALIZE(BaseLOD);
        DESERIALIZE(LODCount);
        DESERIALIZE(TriangleReduction);
        DESERIALIZE(SloppyOptimization);
        DESERIALIZE(LODTargetError);
        DESERIALIZE(ImportMaterials);
        DESERIALIZE(ImportMaterialsAsInstances);
        DESERIALIZE(InstanceToImportAs);
        DESERIALIZE(ImportTextures);
        DESERIALIZE(RestoreMaterialsOnReimport);
        DESERIALIZE(SkipExistingMaterialsOnReimport);
        DESERIALIZE(GenerateSDF);
        DESERIALIZE(SDFResolution);
        DESERIALIZE(SplitObjects);
        DESERIALIZE(ObjectIndex);
        DESERIALIZE(SubAssetFolder);

        // [Deprecated on 23.11.2021, expires on 21.11.2023]
        int32 AnimationIndex = -1;
        DESERIALIZE(AnimationIndex);
        if (AnimationIndex != -1)
            ObjectIndex = AnimationIndex;

        // [Deprecated on 08.02.2024, expires on 08.02.2026]
        bool EnableRootMotion = false;
        DESERIALIZE(EnableRootMotion);
        if (EnableRootMotion)
        {
            RootMotion = RootMotionMode::ExtractNode;
            // RootMotionFlags = AnimationRootMotionFlags::RootPositionXZ;
        }
    }

    void RemoveNamespace(String& name)
    {
        const int32 namespaceStart = name.Find(':');
        if (namespaceStart != -1)
            name = name.Substring(namespaceStart + 1);
    }

    bool ModelUtils::ImportData(const String& path, ModelData& data, Options& options, String& errorMsg)
    {
        PROFILE_CPU();

        // Validate options
        options.Scale = Math::Clamp(options.Scale, 0.0001f, 100000.0f);
        options.SmoothingNormalsAngle = Math::Clamp(options.SmoothingNormalsAngle, 0.0f, 175.0f);
        options.SmoothingTangentsAngle = Math::Clamp(options.SmoothingTangentsAngle, 0.0f, 45.0f);
        options.FramesRange.y = Math::Max(options.FramesRange.y, options.FramesRange.x);
        options.DefaultFrameRate = Math::Max(0.0f, options.DefaultFrameRate);
        options.SamplingRate = Math::Max(0.0f, options.SamplingRate);
        if (options.SplitObjects || options.Type == ModelType::Prefab)
            options.MergeMeshes = false; // Meshes merging doesn't make sense when we want to import each mesh individually
        // TODO: maybe we could update meshes merger to collapse meshes within the same name if splitting is enabled?

        // Call importing backend

        if (!ImportDataAssimp(path, data, options, errorMsg))
        {
            return false;
        }


        // Remove namespace prefixes from the nodes names
        {
            for (auto& node : data.Nodes)
            {
                RemoveNamespace(node.Name);
            }
            /*for (auto& node : data.Skeleton.Nodes)
            {
                RemoveNamespace(node.Name);
            }*/
            /*for (auto& animation : data.Animations)
            {
                for (auto& channel : animation.Channels)
                    RemoveNamespace(channel.NodeName);
            }*/
            for (auto& lod : data.LODs)
            {
                for (auto& mesh : lod.Meshes)
                {
                    RemoveNamespace(mesh->Name);
                    for (auto& blendShape : mesh->BlendShapes)
                        RemoveNamespace(blendShape.Name);
                }
            }
        }

        // Validate the animation channels
        /*for (auto& animation : data.Animations)
        {
            auto& channels = animation.Channels;
            if (channels.IsEmpty())
                continue;

            // Validate bone animations uniqueness
            for (int32 i = 0; i < channels.Count(); i++)
            {
                for (int32 j = i + 1; j < channels.Count(); j++)
                {
                    if (channels[i].NodeName == channels[j].NodeName)
                    {
                        LOG_WARNING("Resource", "Animation uses two nodes with the same name ({0}). Removing duplicated channel.", channels[i].NodeName);
                        channels.RemoveAtKeepOrder(j);
                        j--;
                    }
                }
            }

            // Remove channels/animations with empty tracks
            if (options.SkipEmptyCurves)
            {
                for (int32 i = 0; i < channels.Count(); i++)
                {
                    auto& channel = channels[i];

                    // Remove identity curves (with single keyframe and no actual animated change)
                    if (channel.Position.GetKeyframes().Count() == 1 && channel.Position.GetKeyframes()[0].Value.IsZero())
                    {
                        channel.Position.Clear();
                    }
                    if (channel.Rotation.GetKeyframes().Count() == 1 && channel.Rotation.GetKeyframes()[0].Value.IsIdentity())
                    {
                        channel.Rotation.Clear();
                    }
                    if (channel.Scale.GetKeyframes().Count() == 1 && channel.Scale.GetKeyframes()[0].Value.IsOne())
                    {
                        channel.Scale.Clear();
                    }

                    // Remove whole channel if has no effective data
                    if (channel.Position.IsEmpty() && channel.Rotation.IsEmpty() && channel.Scale.IsEmpty())
                    {
                        LOG_WARNING("Resource", "Removing empty animation channel ({0}).", channel.NodeName);
                        channels.RemoveAtKeepOrder(i);
                    }
                }
            }
        }*/

        // Flip normals of the imported geometry
        if (options.FlipNormals && options.ImportTypes.IsFlag(ImportDataTypes::Geometry))
        {
            for (auto& lod : data.LODs)
            {
                for (auto& mesh : lod.Meshes)
                {
                    for (auto& n : mesh->Normals)
                        n *= -1;
                    for (auto& shape : mesh->BlendShapes)
                        for (auto& v : shape.Vertices)
                            v.NormalDelta *= -1;
                }
            }
        }

        return true;
    }

    // Disabled by default (not finished and Assimp importer outputs nodes in a fine order)
#define USE_SKELETON_NODES_SORTING 0

#if USE_SKELETON_NODES_SORTING

    bool SortDepths(const Pair<int32, int32>& a, const Pair<int32, int32>& b)
    {
        return a.First < b.First;
    }

    void CreateLinearListFromTree(List<SkeletonNode>& nodes, List<int32>& mapping)
    {
        // Customized breadth first tree algorithm (each node has no direct reference to the children so we build the cache for the nodes depth level)
        const int32 count = nodes.Count();
        List<Pair<int32, int32>> depths(count); // Pair.First = Depth, Pair.Second = Node Index
        depths.Resize(count);
        depths.SetAll(-1);
        for (int32 i = 0; i < count; i++)
        {
            // Skip evaluated nodes
            if (depths[i].First != -1)
                continue;

            // Find the first node with calculated depth and get the distance to it
            int32 end = i;
            int32 lastDepth;
            int32 relativeDepth = 0;
            do
            {
                lastDepth = depths[end].First;
                end = nodes[end].ParentIndex;
                relativeDepth++;
            } while (end != -1 && lastDepth == -1);

            // Set the depth (second item is the node index)
            depths[i] = ToPair(lastDepth + relativeDepth, i);
        }
        for (int32 i = 0; i < count; i++)
        {
            // Strange divide by 2 but works
            depths[i].First = depths[i].First >> 1;
        }

        // Order nodes by depth O(n*log(n))
        depths.Sort(SortDepths);

        // Extract nodes mapping O(n^2)
        mapping.EnsureCapacity(count, false);
        mapping.Resize(count);
        for (int32 i = 0; i < count; i++)
        {
            int32 newIndex = -1;
            for (int32 j = 0; j < count; j++)
            {
                if (depths[j].Second == i)
                {
                    newIndex = j;
                    break;
                }
            }
            ASSERT(newIndex != -1);
            mapping[i] = newIndex;
        }
    }

#endif

    /*
    template<typename T>
    void OptimizeCurve(LinearCurve<T>& curve)
    {
        auto& oldKeyframes = curve.GetKeyframes();
        const int32 keyCount = oldKeyframes.Count();
        typename LinearCurve<T>::KeyFrameCollection newKeyframes(keyCount);
        bool lastWasEqual = false;

        for (int32 i = 0; i < keyCount; i++)
        {
            bool isEqual = false;
            const auto& curKey = oldKeyframes[i];
            if (i > 0)
            {
                const auto& prevKey = newKeyframes.Last();
                isEqual = Math::NearEqual(prevKey.Value, curKey.Value);
            }

            // More than two keys in a row are equal, remove the middle key by replacing it with this one
            if (lastWasEqual && isEqual)
            {
                auto& prevKey = newKeyframes.Last();
                prevKey = curKey;
                continue;
            }

            newKeyframes.Add(curKey);
            lastWasEqual = isEqual;
        }

        // Special case if animation has only two the same keyframes after cleaning
        if (newKeyframes.Count() == 2 && Math::NearEqual(newKeyframes[0].Value, newKeyframes[1].Value))
        {
            newKeyframes.RemoveAt(1);
        }

        // Special case if animation has only one identity keyframe (does not introduce any animation)
        if (newKeyframes.Count() == 1 && Math::NearEqual(newKeyframes[0].Value, curve.GetDefaultValue()))
        {
            newKeyframes.RemoveAt(0);
        }

        // Update keyframes if size changed
        if (keyCount != newKeyframes.Count())
        {
            curve.SetKeyframes(newKeyframes);
        }
    }
    */

    void* MeshOptAllocate(size_t size)
    {
        return PlatformAllocator::Allocate(size);
    }

    void MeshOptDeallocate(void* ptr)
    {
        PlatformAllocator::Free(ptr);
    }

    void TrySetupMaterialParameter(MaterialInstance* instance, Span<const Char*> paramNames, const Variant& value, MaterialParameterType type)
    {
        for (const Char* name : paramNames)
        {
            for (MaterialParameter& param : instance->Params)
            {
                const MaterialParameterType paramType = param.GetParameterType();
                if (type != paramType)
                {
                    if (type == MaterialParameterType::Color)
                    {
                        if (paramType != MaterialParameterType::Vector3 ||
                            paramType != MaterialParameterType::Vector4)
                            continue;
                    }
                    else
                        continue;
                }
                if (StringUtils::CompareIgnoreCase(name, param.GetName().Get()) != 0)
                    continue;
                param.SetValue(value);
                return;
            }
        }
    }

    String GetAdditionalImportPath(const String& autoImportOutput, List<String>& importedFileNames, const String& name)
    {
        String filename = name;
        for (int32 j = filename.Length() - 1; j >= 0; j--)
        {
            if (FileSystem::IsInvalidPathChar(filename[j]))
                filename[j] = ' ';
        }
        if (importedFileNames.Contains(filename))
        {
            int32 counter = 1;
            do
            {
                filename = name + SE_TEXT(" ") + StringUtils::ToString(counter);
                counter++;
            } while (importedFileNames.Contains(filename));
        }
        importedFileNames.Add(filename);
        return autoImportOutput / filename + ASSET_FILES_EXTENSION_WITH_DOT;
    }

    bool ModelUtils::ImportModel(const String& path, ModelData& data, Options& options, String& errorMsg, const String& autoImportOutput)
    {
        PROFILE_CPU();
        LOG_INFO("Resource", "Importing model from \'{0}\'", path);
        const auto startTime = DateTime::NowUTC();

        // Import data
        switch (options.Type)
        {
        case ModelType::Model:
            options.ImportTypes.SetFlags(ImportDataTypes::Geometry, ImportDataTypes::Nodes);
            if (options.ImportMaterials)
                options.ImportTypes.SetFlag(ImportDataTypes::Materials);
            if (options.ImportTextures)
                options.ImportTypes.SetFlag(ImportDataTypes::Textures);
            break;
        case ModelType::SkinnedModel:
            options.ImportTypes.SetFlags(ImportDataTypes::Geometry, ImportDataTypes::Nodes, ImportDataTypes::Skeleton);
            if (options.ImportMaterials)
                options.ImportTypes.SetFlag(ImportDataTypes::Materials);
            if (options.ImportTextures)
                options.ImportTypes.SetFlag(ImportDataTypes::Textures);
            break;
        case ModelType::Animation:
            options.ImportTypes = ImportDataTypes::Animations;
            if (options.RootMotion == RootMotionMode::ExtractCenterOfMass)
                options.ImportTypes.SetFlag(ImportDataTypes::Skeleton);
            break;
        case ModelType::Prefab:
            options.ImportTypes.SetFlags(ImportDataTypes::Geometry, ImportDataTypes::Nodes, ImportDataTypes::Animations);
            if (options.ImportMaterials)
                options.ImportTypes.SetFlag(ImportDataTypes::Materials);
            if (options.ImportTextures)
                options.ImportTypes.SetFlag(ImportDataTypes::Textures);
            break;
        default:
            return false;
        }
        if (!ImportData(path, data, options, errorMsg))
            return false;
        // Validate result data
        if (options.ImportTypes.IsFlag(ImportDataTypes::Geometry))
        {
            LOG_INFO("Resource", "Imported model has {0} LODs, {1} meshes (in LOD0) and {2} materials", data.LODs.Count(), data.LODs.Count() != 0 ? data.LODs[0].Meshes.Count() : 0, data.Materials.Count());

            // Process blend shapes
            for (auto& lod : data.LODs)
            {
                for (auto& mesh : lod.Meshes)
                {
                    for (int32 blendShapeIndex = mesh->BlendShapes.Count() - 1; blendShapeIndex >= 0; blendShapeIndex--)
                    {
                        auto& blendShape = mesh->BlendShapes[blendShapeIndex];

                        // Remove blend shape vertices with empty deltas
                        for (int32 i = blendShape.Vertices.Count() - 1; i >= 0; i--)
                        {
                            auto& v = blendShape.Vertices.Get()[i];
                            if (v.PositionDelta.IsZero() && v.NormalDelta.IsZero())
                            {
                                blendShape.Vertices.RemoveAt(i);
                            }
                        }

                        // Remove empty blend shapes
                        if (blendShape.Vertices.IsEmpty() || blendShape.Name.IsEmpty())
                        {
                            LOG_INFO("Resource", "Removing empty blend shape '{0}' from mesh '{1}'", blendShape.Name, mesh->Name);
                            mesh->BlendShapes.RemoveAt(blendShapeIndex);
                        }
                    }
                }
            }
        }
        /*if (options.ImportTypes.IsFlagSet(ImportDataTypes::Skeleton))
        {
            LOG_INFO("Resource", "Imported skeleton has {0} bones and {1} nodes", data.Skeleton.Bones.Count(), data.Nodes.Count());

            // Add single node if imported skeleton is empty
            if (data.Skeleton.Nodes.IsEmpty())
            {
                data.Skeleton.Nodes.Resize(1);
                data.Skeleton.Nodes[0].Name = SE_TEXT("Root");
                data.Skeleton.Nodes[0].LocalTransform = Transform::Identity;
                data.Skeleton.Nodes[0].ParentIndex = -1;
            }

            // Special case if imported model has no bones but has valid skeleton and meshes.
            // We assume that every mesh uses a single bone. Copy nodes to bones.
            if (data.Skeleton.Bones.IsEmpty() && Math::IsInRange(data.Skeleton.Nodes.Count(), 1, MAX_BONES_PER_MODEL))
            {
                data.Skeleton.Bones.Resize(data.Skeleton.Nodes.Count());
                for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
                {
                    auto& node = data.Skeleton.Nodes[i];
                    auto& bone = data.Skeleton.Bones[i];

                    bone.ParentIndex = node.ParentIndex;
                    bone.NodeIndex = i;
                    bone.LocalTransform = node.LocalTransform;

                    Matrix t = Matrix::Identity;
                    int32 idx = bone.NodeIndex;
                    do
                    {
                        t *= data.Skeleton.Nodes[idx].LocalTransform.GetWorld();
                        idx = data.Skeleton.Nodes[idx].ParentIndex;
                    } while (idx != -1);
                    t.Invert();
                    bone.OffsetMatrix = t;
                }
            }

            // Check bones limit currently supported by the engine
            if (data.Skeleton.Bones.Count() > MAX_BONES_PER_MODEL)
            {
                errorMsg = String::Format(SE_TEXT("Imported model skeleton has too many bones. Imported: {0}, maximum supported: {1}. Please optimize your asset."), data.Skeleton.Bones.Count(), MAX_BONES_PER_MODEL);
                return true;
            }

            // Ensure that root node is at index 0
            int32 rootIndex = -1;
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                const auto idx = data.Skeleton.Nodes.Get()[i].ParentIndex;
                if (idx == -1 && rootIndex == -1)
                {
                    // Found root
                    rootIndex = i;
                }
                else if (idx == -1)
                {
                    // Found multiple roots
                    errorMsg = SE_TEXT("Imported skeleton has more than one root node.");
                    return true;
                }
            }
            if (rootIndex == -1)
            {
                // Missing root node (more additional validation that possible error)
                errorMsg = SE_TEXT("Imported skeleton has missing root node.");
                return true;
            }
            if (rootIndex != 0)
            {
                // Map the root node to index 0 (more optimized for runtime)
                LOG_WARNING("Resource", "Imported skeleton root node is not at index 0. Performing the remmaping.");
                const int32 prevRootIndex = rootIndex;
                rootIndex = 0;
                Swap(data.Skeleton.Nodes[rootIndex], data.Skeleton.Nodes[prevRootIndex]);
                for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
                {
                    auto& node = data.Skeleton.Nodes.Get()[i];
                    if (node.ParentIndex == prevRootIndex)
                        node.ParentIndex = rootIndex;
                    else if (node.ParentIndex == rootIndex)
                        node.ParentIndex = prevRootIndex;
                }
                for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
                {
                    auto& bone = data.Skeleton.Bones.Get()[i];
                    if (bone.NodeIndex == prevRootIndex)
                        bone.NodeIndex = rootIndex;
                    else if (bone.NodeIndex == rootIndex)
                        bone.NodeIndex = prevRootIndex;
                }
            }

#if BUILD_DEBUG
            // Validate that nodes and bones hierarchies are valid (no cyclic references because its mean to be a tree)
            {
                for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
                {
                    int32 j = i;
                    int32 testsLeft = data.Skeleton.Nodes.Count();
                    do
                    {
                        j = data.Skeleton.Nodes[j].ParentIndex;
                    } while (j != -1 && testsLeft-- > 0);
                    if (testsLeft <= 0)
                    {
                        Platform::Fatal(SE_TEXT("Skeleton importer issue!"));
                    }
                }
                for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
                {
                    int32 j = i;
                    int32 testsLeft = data.Skeleton.Bones.Count();
                    do
                    {
                        j = data.Skeleton.Bones[j].ParentIndex;
                    } while (j != -1 && testsLeft-- > 0);
                    if (testsLeft <= 0)
                    {
                        Platform::Fatal(SE_TEXT("Skeleton importer issue!"));
                    }
                }
                for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
                {
                    if (data.Skeleton.Bones[i].NodeIndex == -1)
                    {
                        Platform::Fatal(SE_TEXT("Skeleton importer issue!"));
                    }
                }
            }
#endif
        }*/
        /*if (options.ImportTypes.IsFlagSet(ImportDataTypes::Geometry) && options.ImportTypes.IsFlagSet(ImportDataTypes::Skeleton))
        {
            // Validate skeleton bones used by the meshes
            const int32 meshesCount = data.LODs.Count() != 0 ? data.LODs[0].Meshes.Count() : 0;
            for (int32 i = 0; i < meshesCount; i++)
            {
                const auto mesh = data.LODs[0].Meshes[i];
                if (mesh->BlendIndices.IsEmpty() || mesh->BlendWeights.IsEmpty())
                {
                    auto indices = Int4::Zero;
                    auto weights = Float4::UnitX;

                    // Check if use a single bone for skinning
                    auto nodeIndex = data.Skeleton.FindNode(mesh->Name);
                    auto boneIndex = data.Skeleton.FindBone(nodeIndex);
                    if (boneIndex == -1 && nodeIndex != -1 && data.Skeleton.Bones.Count() < MAX_BONES_PER_MODEL)
                    {
                        // Add missing bone to be used by skinned model from animated nodes pose
                        boneIndex = data.Skeleton.Bones.Count();
                        auto& bone = data.Skeleton.Bones.AddOne();
                        bone.ParentIndex = -1;
                        bone.NodeIndex = nodeIndex;
                        bone.LocalTransform = CombineTransformsFromNodeIndices(data.Nodes, -1, nodeIndex);
                        CalculateBoneOffsetMatrix(data.Skeleton.Nodes, bone.OffsetMatrix, bone.NodeIndex);
                        LOG_WARNING("Resource", "Using auto-created bone {0} (index {1}) for mesh \'{2}\'", data.Skeleton.Nodes[nodeIndex].Name, boneIndex, mesh->Name);
                        indices.X = boneIndex;
                    }
                    else if (boneIndex != -1)
                    {
                        // Fallback to already added bone
                        LOG_WARNING("Resource", "Using auto-detected bone {0} (index {1}) for mesh \'{2}\'", data.Skeleton.Nodes[nodeIndex].Name, boneIndex, mesh->Name);
                        indices.X = boneIndex;
                    }
                    else
                    {
                        // No bone
                        LOG_WARNING("Resource", "Imported mesh \'{0}\' has missing skinning data. It may result in invalid rendering.", mesh->Name);
                    }

                    mesh->BlendIndices.Resize(mesh->Positions.Count());
                    mesh->BlendWeights.Resize(mesh->Positions.Count());
                    mesh->BlendIndices.SetAll(indices);
                    mesh->BlendWeights.SetAll(weights);
                }
#if BUILD_DEBUG
                else
                {
                    auto& indices = mesh->BlendIndices;
                    for (int32 j = 0; j < indices.Count(); j++)
                    {
                        const int32 min = indices[j].MinValue();
                        const int32 max = indices[j].MaxValue();
                        if (min < 0 || max >= data.Skeleton.Bones.Count())
                        {
                            LOG_WARNING("Resource", "Imported mesh \'{0}\' has invalid blend indices. It may result in invalid rendering.", mesh->Name);
                        }
                    }

                    auto& weights = mesh->BlendWeights;
                    for (int32 j = 0; j < weights.Count(); j++)
                    {
                        const float sum = weights[j].SumValues();
                        if (Math::Abs(sum - 1.0f) > ZeroTolerance)
                        {
                            LOG_WARNING("Resource", "Imported mesh \'{0}\' has invalid blend weights. It may result in invalid rendering.", mesh->Name);
                        }
                    }
                }
#endif
            }
        }*/

        /*if (options.ImportTypes.IsFlagSet(ImportDataTypes::Animations))
        {
            for (auto& animation : data.Animations)
            {
                LOG_INFO("Resource", "Imported animation '{}' has {} channels, duration: {} frames, frames per second: {}", animation.Name, animation.Channels.Count(), animation.Duration, animation.FramesPerSecond);
                if (animation.Duration <= Math::ZeroTolerance || animation.FramesPerSecond <= Math::ZeroTolerance)
                {
                    errorMsg = SE_TEXT("Invalid animation duration.");
                    return true;
                }
            }
        }*/

        switch (options.Type)
        {
        case ModelType::Model:
            if (data.LODs.IsEmpty() || data.LODs[0].Meshes.IsEmpty())
            {
                errorMsg = SE_TEXT("Imported model has no valid geometry.");
                return false;
            }
            if (data.Nodes.IsEmpty())
            {
                errorMsg = SE_TEXT("Missing model nodes.");
                return false;
            }
            break;
        case ModelType::SkinnedModel:
            if (data.LODs.Count() > 1)
            {
                LOG_WARNING("Resource", "Imported skinned model has more than one LOD. Removing the lower LODs. Only single one is supported.");
                data.LODs.Resize(1);
            }
            break;
        case ModelType::Animation:
            /*if (data.Animations.IsEmpty())
            {
                errorMsg = SE_TEXT("Imported file has no valid animations.");
                return false;
            }*/
            break;
        }

        // Keep additionally imported files well organized
        List<String> importedFileNames;

        // Prepare textures
        for (int32 i = 0; i < data.Textures.Count(); i++)
        {
            auto& texture = data.Textures[i];

            // Auto-import textures
            if (autoImportOutput.IsEmpty() || options.ImportTypes.IsNotFlag(ImportDataTypes::Textures) || texture.FilePath.IsEmpty())
                continue;
            String assetPath = GetAdditionalImportPath(autoImportOutput, importedFileNames, FileSystem::GetFileNameWithoutExtension(texture.FilePath));
#if COMPILE_WITH_ASSETS_IMPORTER
            TextureTool::Options textureOptions;
            switch (texture.Type)
            {
            case TextureEntry::TypeHint::ColorRGB:
                textureOptions.Type = TextureFormatType::ColorRGB;
                break;
            case TextureEntry::TypeHint::ColorRGBA:
                textureOptions.Type = TextureFormatType::ColorRGBA;
                break;
            case TextureEntry::TypeHint::Normals:
                textureOptions.Type = TextureFormatType::NormalMap;
                break;
            }
            AssetsImportingManager::ImportIfEdited(texture.FilePath, assetPath, texture.AssetID, &textureOptions);
#endif
        }

        // Prepare materials
        for (int32 i = 0; i < data.Materials.Count(); i++)
        {
            auto& material = data.Materials[i];

            if (material.Name.IsEmpty())
                material.Name = SE_TEXT("Material ") + StringUtils::ToString(i);

            // Auto-import materials
            if (autoImportOutput.IsEmpty() || options.ImportTypes.IsNotFlag(ImportDataTypes::Materials) || !material.UsesProperties())
                continue;
            String assetPath = GetAdditionalImportPath(autoImportOutput, importedFileNames, material.Name);
#if COMPILE_WITH_ASSETS_IMPORTER
            // When splitting imported meshes allow only the first mesh to import assets (mesh[0] is imported after all following ones so import assets during mesh[1])
            if (!options.SplitObjects && options.ObjectIndex != 1 && options.ObjectIndex != -1)
            {
                // Find that asset created previously
                AssetInfo info;
                if (Content::GetAssetInfo(assetPath, info))
                    material.AssetID = info.ID;
                continue;
            }

            // Skip any materials that already exist from the model.
            // This allows the use of "import as material instances" without material properties getting overridden on each import.
            if (options.SkipExistingMaterialsOnReimport)
            {
                AssetInfo info;
                if (Content::GetAssetInfo(assetPath, info))
                {
                    material.AssetID = info.ID;
                    continue;
                }
            }

            if (options.ImportMaterialsAsInstances)
            {
                // Create material instance
                AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialInstanceTag, assetPath, material.AssetID);
                if (auto* materialInstance = Content::Load<MaterialInstance>(assetPath))
                {
                    materialInstance->SetBaseMaterial(options.InstanceToImportAs);

                    // Customize base material based on imported material (blind guess based on the common names used in materials)
                    const Char* diffuseColorNames[] = { SE_TEXT("color"), SE_TEXT("col"), SE_TEXT("diffuse"), SE_TEXT("basecolor"), SE_TEXT("base color") };
                    TrySetupMaterialParameter(materialInstance, ToSpan(diffuseColorNames, ARRAY_COUNT(diffuseColorNames)), material.Diffuse.Color, MaterialParameterType::Color);
                    const Char* emissiveColorNames[] = { SE_TEXT("emissive"), SE_TEXT("emission"), SE_TEXT("light") };
                    TrySetupMaterialParameter(materialInstance, ToSpan(emissiveColorNames, ARRAY_COUNT(emissiveColorNames)), material.Emissive.Color, MaterialParameterType::Color);
                    const Char* opacityValueNames[] = { SE_TEXT("opacity"), SE_TEXT("alpha") };
                    TrySetupMaterialParameter(materialInstance, ToSpan(opacityValueNames, ARRAY_COUNT(opacityValueNames)), material.Opacity.Value, MaterialParameterType::Float);

                    materialInstance->Save();
                }
                else
                {
                    LOG_ERROR("Resource", "Failed to load material instance after creation. ({0})", assetPath);
                }
            }
            else
            {
                // Create material
                CreateMaterial::Options materialOptions;
                materialOptions.Diffuse.Color = material.Diffuse.Color;
                if (material.Diffuse.TextureIndex != -1)
                    materialOptions.Diffuse.Texture = data.Textures[material.Diffuse.TextureIndex].AssetID;
                materialOptions.Diffuse.HasAlphaMask = material.Diffuse.HasAlphaMask;
                materialOptions.Emissive.Color = material.Emissive.Color;
                if (material.Emissive.TextureIndex != -1)
                    materialOptions.Emissive.Texture = data.Textures[material.Emissive.TextureIndex].AssetID;
                materialOptions.Opacity.Value = material.Opacity.Value;
                if (material.Opacity.TextureIndex != -1)
                    materialOptions.Opacity.Texture = data.Textures[material.Opacity.TextureIndex].AssetID;
                materialOptions.Roughness.Value = material.Roughness.Value;
                if (material.Roughness.TextureIndex != -1)
                    materialOptions.Roughness.Texture = data.Textures[material.Roughness.TextureIndex].AssetID;
                if (material.Normals.TextureIndex != -1)
                    materialOptions.Normals.Texture = data.Textures[material.Normals.TextureIndex].AssetID;
                if (material.TwoSided || material.Diffuse.HasAlphaMask)
                    materialOptions.Info.CullMode = CullMode::TwoSided;
                if (!Math::IsOne(material.Opacity.Value) || material.Opacity.TextureIndex != -1)
                    materialOptions.Info.BlendMode = MaterialBlendMode::Transparent;
                AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialTag, assetPath, material.AssetID, &materialOptions);
            }
#endif
        }

        // Prepare import transformation
        Transform importTransform(options.Translation, options.Rotation, Float3(options.Scale));
        if (options.UseLocalOrigin && data.LODs.HasItems() && data.LODs[0].Meshes.HasItems())
        {
            importTransform.Translation -= importTransform.Orientation * data.LODs[0].Meshes[0]->OriginTranslation * importTransform.Scale;
        }
        if (options.CenterGeometry && data.LODs.HasItems() && data.LODs[0].Meshes.HasItems())
        {
            // Calculate the bounding box (use LOD0 as a reference)
            BoundingBox box = data.LODs[0].GetBox();
            auto center = data.LODs[0].Meshes[0]->OriginOrientation * importTransform.Orientation * box.GetCenter() * importTransform.Scale * data.LODs[0].Meshes[0]->Scaling;
            importTransform.Translation -= center;
        }

        // Apply the import transformation
        if (!importTransform.IsIdentity() && data.Nodes.HasItems())
        {
            if (options.Type == ModelType::SkinnedModel)
            {
                // Transform the root node using the import transformation
                /*auto& root = data.Skeleton.RootNode();
                Transform meshTransform = root.LocalTransform.WorldToLocal(importTransform).LocalToWorld(root.LocalTransform);
                root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);

                // Apply import transform on meshes
                Matrix meshTransformMatrix;
                meshTransform.GetWorld(meshTransformMatrix);
                for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
                {
                    auto& lod = data.LODs[lodIndex];
                    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                    {
                        lod.Meshes[meshIndex]->TransformBuffer(meshTransformMatrix);
                    }
                }

                // Apply import transform on bones
                Matrix importMatrixInv;
                importTransform.GetWorld(importMatrixInv);
                importMatrixInv.Invert();
                for (SkeletonBone& bone : data.Skeleton.Bones)
                {
                    if (bone.ParentIndex == -1)
                        bone.LocalTransform = importTransform.LocalToWorld(bone.LocalTransform);
                    bone.OffsetMatrix = importMatrixInv * bone.OffsetMatrix;
                }*/
            }
            else
            {
                // Transform the root node using the import transformation
                auto& root = data.Nodes[0];
                root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);
            }
        }

        // Post-process imported data
        if (options.ImportTypes.IsFlag(ImportDataTypes::Skeleton))
        {
            /*if (options.CalculateBoneOffsetMatrices)
            {
                // Calculate offset matrix (inverse bind pose transform) for every bone manually
                for (SkeletonBone& bone : data.Skeleton.Bones)
                {
                    CalculateBoneOffsetMatrix(data.Skeleton.Nodes, bone.OffsetMatrix, bone.NodeIndex);
                }
            }*/

#if USE_SKELETON_NODES_SORTING
            // Sort skeleton nodes and bones hierarchy (parents first)
            // Then it can be used with a simple linear loop update
            {
                const int32 nodesCount = data.Skeleton.Nodes.Count();
                const int32 bonesCount = data.Skeleton.Bones.Count();
                List<int32> mapping;
                CreateLinearListFromTree(data.Skeleton.Nodes, mapping);
                for (int32 i = 0; i < nodesCount; i++)
                {
                    auto& node = data.Skeleton.Nodes[i];
                    node.ParentIndex = mapping[node.ParentIndex];
                }
                for (int32 i = 0; i < bonesCount; i++)
                {
                    auto& bone = data.Skeleton.Bones[i];
                    bone.NodeIndex = mapping[bone.NodeIndex];
                }
            }
            reorder_nodes_and_test_it_out
            !
    #endif
        }
        if (options.ImportTypes.IsFlag(ImportDataTypes::Geometry) && options.Type != ModelType::Prefab)
        {
            // Perform simple nodes mapping to single node (will transform meshes to model local space)
            /*SkeletonMapping<ModelDataNode> skeletonMapping(data.Nodes, nullptr);

            // Refresh skeleton updater with model skeleton
            SkeletonUpdater<ModelDataNode> hierarchyUpdater(data.Nodes);
            hierarchyUpdater.UpdateMatrices();

            // Move meshes in the new nodes
            for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
            {
                for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
                {
                    auto& mesh = *data.LODs[lodIndex].Meshes[meshIndex];

                    // Check if there was a remap using model skeleton
                    if (skeletonMapping.SourceToSource[mesh.NodeIndex] != mesh.NodeIndex)
                    {
                        // Transform vertices
                        const Matrix transformationMatrix = hierarchyUpdater.CombineMatricesFromNodeIndices(skeletonMapping.SourceToSource[mesh.NodeIndex], mesh.NodeIndex);

                        if (!transformationMatrix.IsIdentity())
                            mesh.TransformBuffer(transformationMatrix);
                    }

                    // Update new node index using real asset skeleton
                    mesh.NodeIndex = skeletonMapping.SourceToTarget[mesh.NodeIndex];
                }
            }*/
        }
        if (options.ImportTypes.IsFlag(ImportDataTypes::Geometry) && options.Type == ModelType::Prefab)
        {
            // Apply just the scale and rotations.
            for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
            {
                for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
                {
                    auto& mesh = *data.LODs[lodIndex].Meshes[meshIndex];
                    auto& node = data.Nodes[mesh.NodeIndex];
                    auto currentNode = &data.Nodes[mesh.NodeIndex];

                    Float3 scale = Float3::One;
                    Quaternion rotation = Quaternion::Identity;
                    while (true)
                    {
                        scale *= currentNode->LocalTransform.Scale;
                        rotation *= currentNode->LocalTransform.Orientation;
                        if (currentNode->ParentIndex == -1)
                            break;
                        currentNode = &data.Nodes[currentNode->ParentIndex];
                    }

                    // Transform vertices
                    auto transformationMatrix = Matrix::Identity;
                    transformationMatrix.SetScaleVector(scale);
                    transformationMatrix = transformationMatrix * Matrix::RotationQuaternion(rotation);

                    if (!transformationMatrix.IsIdentity())
                        mesh.TransformBuffer(transformationMatrix);
                }
            }
        }

        if (options.ImportTypes.IsFlag(ImportDataTypes::Animations))
        {
            /*for (auto& animation : data.Animations)
            {
                // Trim the animation keyframes range if need to
                if (options.Duration == AnimationDuration::Custom)
                {
                    // Custom animation import, frame index start and end
                    const float start = options.FramesRange.X;
                    const float end = options.FramesRange.y;
                    for (int32 i = 0; i < animation.Channels.Count(); i++)
                    {
                        auto& anim = animation.Channels[i];
                        anim.Position.Trim(start, end);
                        anim.Rotation.Trim(start, end);
                        anim.Scale.Trim(start, end);
                    }
                    animation.Duration = end - start;
                }

                // Change the sampling rate if need to
                if (!Math::IsZero(options.SamplingRate))
                {
                    const float timeScale = (float)(animation.FramesPerSecond / options.SamplingRate);
                    if (!Math::IsOne(timeScale))
                    {
                        animation.FramesPerSecond = options.SamplingRate;
                        for (int32 i = 0; i < animation.Channels.Count(); i++)
                        {
                            auto& anim = animation.Channels[i];
                            anim.Position.TransformTime(timeScale, 0.0f);
                            anim.Rotation.TransformTime(timeScale, 0.0f);
                            anim.Scale.TransformTime(timeScale, 0.0f);
                        }
                    }
                }

                // Process root motion setup
                animation.RootMotionFlags = options.RootMotion != RootMotionMode::None ? options.RootMotionFlags : AnimationRootMotionFlags::None;
                animation.RootNodeName = options.RootNodeName.TrimTrailing();
                if (animation.RootMotionFlags != AnimationRootMotionFlags::None && animation.Channels.HasItems())
                {
                    if (options.RootMotion == RootMotionMode::ExtractNode)
                    {
                        if (animation.RootNodeName.HasChars() && animation.GetChannel(animation.RootNodeName) == nullptr)
                        {
                            LOG_WARNING("Resource", "Missing Root Motion node '{}'", animation.RootNodeName);
                        }
                    }
                    else if (options.RootMotion == RootMotionMode::ExtractCenterOfMass && data.Skeleton.Nodes.HasItems()) // TODO: finish implementing this
                    {
                        // Setup root node animation track
                        const auto& rootName = data.Skeleton.Nodes.First().Name;
                        auto rootChannelPtr = animation.GetChannel(rootName);
                        if (!rootChannelPtr)
                        {
                            animation.Channels.Insert(0, NodeAnimationData());
                            rootChannelPtr = &animation.Channels[0];
                            rootChannelPtr->NodeName = rootName;
                        }
                        animation.RootNodeName = rootName;
                        auto& rootChannel = *rootChannelPtr;
                        rootChannel.Position.Clear();

                        // Calculate skeleton center of mass position over the animation frames
                        const int32 frames = (int32)animation.Duration;
                        const int32 nodes = data.Skeleton.Nodes.Count();
                        List<Float3> centerOfMass;
                        centerOfMass.Resize(frames);
                        for (int32 frame = 0; frame < frames; frame++)
                        {
                            auto& key = centerOfMass[frame];

                            // Evaluate skeleton pose at the animation frame
                            AnimGraphImpulse pose;
                            pose.Nodes.Resize(nodes);
                            for (int32 nodeIndex = 0; nodeIndex < nodes; nodeIndex++)
                            {
                                Transform srcNode = data.Skeleton.Nodes[nodeIndex].LocalTransform;
                                auto& node = data.Skeleton.Nodes[nodeIndex];
                                if (auto* channel = animation.GetChannel(node.Name))
                                    channel->Evaluate((float)frame, &srcNode, false);
                                pose.Nodes[nodeIndex] = srcNode;
                            }

                            // Calculate average location of the pose (center of mass)
                            key = Float3::Zero;
                            for (int32 nodeIndex = 0; nodeIndex < nodes; nodeIndex++)
                                key += pose.GetNodeModelTransformation(data.Skeleton, nodeIndex).Translation;
                            key /= (float)nodes;
                        }

                        // Calculate skeleton center of mass movement over the animation frames
                        rootChannel.Position.Resize(frames);
                        const Float3 centerOfMassRefPose = centerOfMass[0];
                        for (int32 frame = 0; frame < frames; frame++)
                        {
                            auto& key = rootChannel.Position[frame];
                            key.Time = (float)frame;
                            key.Value = centerOfMass[frame] - centerOfMassRefPose;
                        }

                        // Remove root motion from the children (eg. if Root moves, then Hips should skip that movement delta)
                        Float3 maxMotion = Float3::Zero;
                        for (int32 i = 0; i < animation.Channels.Count(); i++)
                        {
                            auto& anim = animation.Channels[i];
                            const int32 animNodeIndex = data.Skeleton.FindNode(anim.NodeName);

                            // Skip channels that have one of their parents already animated
                            {
                                int32 nodeIndex = animNodeIndex;
                                nodeIndex = data.Skeleton.Nodes[nodeIndex].ParentIndex;
                                while (nodeIndex > 0)
                                {
                                    const String& nodeName = data.Skeleton.Nodes[nodeIndex].Name;
                                    if (animation.GetChannel(nodeName) != nullptr)
                                        break;
                                    nodeIndex = data.Skeleton.Nodes[nodeIndex].ParentIndex;
                                }
                                if (nodeIndex > 0 || &anim == rootChannelPtr)
                                    continue;
                            }

                            // Remove motion
                            auto& animPos = anim.Position.GetKeyframes();
                            for (int32 frame = 0; frame < animPos.Count(); frame++)
                            {
                                auto& key = animPos[frame];

                                // Evaluate root motion at the keyframe location
                                Float3 rootMotion;
                                rootChannel.Position.Evaluate(rootMotion, key.Time, false);
                                Float3::Max(maxMotion, rootMotion, maxMotion);

                                // Evaluate skeleton pose at the animation frame
                                AnimGraphImpulse pose;
                                pose.Nodes.Resize(nodes);
                                pose.Nodes[0] = data.Skeleton.Nodes[0].LocalTransform; // Use ref pose of root
                                for (int32 nodeIndex = 1; nodeIndex < nodes; nodeIndex++) // Skip new root
                                {
                                    Transform srcNode = data.Skeleton.Nodes[nodeIndex].LocalTransform;
                                    auto& node = data.Skeleton.Nodes[nodeIndex];
                                    if (auto* channel = animation.GetChannel(node.Name))
                                        channel->Evaluate((float)frame, &srcNode, false);
                                    pose.Nodes[nodeIndex] = srcNode;
                                }

                                // Convert root motion to the local space of this node so the node stays at the same location after adding new root channel
                                Transform parentNodeTransform = pose.GetNodeModelTransformation(data.Skeleton, data.Skeleton.Nodes[animNodeIndex].ParentIndex);
                                rootMotion = parentNodeTransform.WorldToLocal(rootMotion);

                                // Remove motion
                                key.Value -= rootMotion;
                            }
                        }
                        LOG_INFO("Resource", "Calculated root motion: {}", maxMotion);
                    }
                }

                // Optimize the keyframes
                if (options.OptimizeKeyframes)
                {
                    const int32 before = animation.GetKeyframesCount();
                    for (int32 i = 0; i < animation.Channels.Count(); i++)
                    {
                        auto& anim = animation.Channels[i];

                        // Optimize keyframes
                        OptimizeCurve(anim.Position);
                        OptimizeCurve(anim.Rotation);
                        OptimizeCurve(anim.Scale);

                        // Remove empty channels
                        if (anim.GetKeyframesCount() == 0)
                        {
                            animation.Channels.RemoveAt(i--);
                        }
                    }
                    const int32 after = animation.GetKeyframesCount();
                    LOG_INFO("Resource", "Optimized {0} animation keyframe(s). Before: {1}, after: {2}, Ratio: {3}%", before - after, before, after, Utilities::RoundTo2DecimalPlaces((float)after / before));
                }
            }*/
        }

        // Collision mesh output
        if (options.CollisionMeshesPrefix.HasChars())
        {
            // Extract collision meshes from the model
            ModelData collisionModel;
            for (auto& lod : data.LODs)
            {
                for (int32 i = lod.Meshes.Count() - 1; i >= 0; i--)
                {
                    auto mesh = lod.Meshes[i];
                    if (mesh->Name.StartsWith(options.CollisionMeshesPrefix, StringSearchCase::IgnoreCase))
                    {
                        if (collisionModel.LODs.Count() == 0)
                            collisionModel.LODs.AddOne();
                        collisionModel.LODs[0].Meshes.Add(mesh);
                        lod.Meshes.RemoveAtKeepOrder(i);
                        if (lod.Meshes.IsEmpty())
                            break;
                    }
                }
            }
#if COMPILE_WITH_PHYSICS_COOKING
            if (collisionModel.LODs.HasItems() && options.CollisionType != CollisionDataType::None)
            {
                // Cook collision
                String assetPath = GetAdditionalImportPath(autoImportOutput, importedFileNames, SE_TEXT("Collision"));
                CollisionCooking::Argument arg;
                arg.Type = options.CollisionType;
                arg.OverrideModelData = &collisionModel;
                if (CreateCollisionData::CookMeshCollision(assetPath, arg))
                {
                    LOG_ERROR("Resource", "Failed to create collision mesh.");
                }
            }
#endif
        }

        // Merge meshes with the same parent nodes, material and skinning
        if (options.MergeMeshes)
        {
            int32 meshesMerged = 0;
            for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
            {
                auto& meshes = data.LODs[lodIndex].Meshes;

                // Group meshes that can be merged together
                typedef Pair<int32, int32> MeshGroupKey;
                const Function<MeshGroupKey(MeshData* const&)> f = [](MeshData* const& x) -> MeshGroupKey
                {
                    return MeshGroupKey(x->NodeIndex, x->MaterialSlotIndex);
                };
                List<IGrouping<MeshGroupKey, MeshData*>> meshesByGroup;
                ListExtensions::GroupBy(meshes, f, meshesByGroup);

                for (int32 groupIndex = 0; groupIndex < meshesByGroup.Count(); groupIndex++)
                {
                    auto& group = meshesByGroup[groupIndex];
                    if (group.Count() <= 1)
                        continue;

                    // Merge group meshes with the first one
                    auto targetMesh = group[0];
                    for (int32 i = 1; i < group.Count(); i++)
                    {
                        auto mesh = group[i];
                        meshes.Remove(mesh);
                        targetMesh->Merge(*mesh);
                        meshesMerged++;
                        Delete(mesh);
                    }
                }
            }
            if (meshesMerged)
                LOG_INFO("Resource", "Merged {0} meshes", meshesMerged);
        }

        // Automatic LOD generation
        if (options.GenerateLODs && options.LODCount > 1 && data.LODs.HasItems() && options.TriangleReduction < 1.0f - Math::ZeroTolerance)
        {
            auto lodStartTime = DateTime::NowUTC();
            meshopt_setAllocator(MeshOptAllocate, MeshOptDeallocate);
            float triangleReduction = Math::Saturate(options.TriangleReduction);
            int32 lodCount = Math::Max(options.LODCount, data.LODs.Count());
            int32 baseLOD = Math::Clamp(options.BaseLOD, 0, lodCount - 1);
            data.LODs.Resize(lodCount);
            int32 generatedLod = 0, baseLodTriangleCount = 0, baseLodVertexCount = 0;
            for (auto& mesh : data.LODs[baseLOD].Meshes)
            {
                baseLodTriangleCount += mesh->Indices.Count() / 3;
                baseLodVertexCount += mesh->Positions.Count();
            }
            List<unsigned int> indices;
            for (int32 lodIndex = Math::Clamp(baseLOD + 1, 1, lodCount - 1); lodIndex < lodCount; lodIndex++)
            {
                auto& dstLod = data.LODs[lodIndex];
                const auto& srcLod = data.LODs[lodIndex - 1];

                int32 lodTriangleCount = 0, lodVertexCount = 0;
                dstLod.Meshes.Resize(srcLod.Meshes.Count());
                for (int32 meshIndex = 0; meshIndex < dstLod.Meshes.Count(); meshIndex++)
                {
                    auto& dstMesh = dstLod.Meshes[meshIndex] = New<MeshData>();
                    const auto& srcMesh = srcLod.Meshes[meshIndex];

                    // Setup mesh
                    dstMesh->MaterialSlotIndex = srcMesh->MaterialSlotIndex;
                    dstMesh->NodeIndex = srcMesh->NodeIndex;
                    dstMesh->Name = srcMesh->Name;

                    // Simplify mesh using meshoptimizer
                    int32 srcMeshIndexCount = srcMesh->Indices.Count();
                    int32 srcMeshVertexCount = srcMesh->Positions.Count();
                    int32 dstMeshIndexCountTarget = int32(srcMeshIndexCount * triangleReduction) / 3 * 3;
                    if (dstMeshIndexCountTarget < 3 || dstMeshIndexCountTarget >= srcMeshIndexCount)
                        continue;
                    indices.Clear();
                    indices.Resize(srcMeshIndexCount);
                    int32 dstMeshIndexCount = {};
                    if (options.SloppyOptimization)
                        dstMeshIndexCount = (int32)meshopt_simplifySloppy(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Float3), dstMeshIndexCountTarget, options.LODTargetError);
                    else
                        dstMeshIndexCount = (int32)meshopt_simplify(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Float3), dstMeshIndexCountTarget, options.LODTargetError);
                    if (dstMeshIndexCount <= 0 || dstMeshIndexCount > indices.Count())
                        continue;
                    indices.Resize(dstMeshIndexCount);

                    // Generate simplified vertex buffer remapping table (use only vertices from LOD index buffer)
                    List<unsigned int> remap;
                    remap.Resize(srcMeshVertexCount);
                    int32 dstMeshVertexCount = (int32)meshopt_optimizeVertexFetchRemap(remap.Get(), indices.Get(), dstMeshIndexCount, srcMeshVertexCount);

                    // Remap index buffer
                    dstMesh->Indices.Resize(dstMeshIndexCount);
                    meshopt_remapIndexBuffer(dstMesh->Indices.Get(), indices.Get(), dstMeshIndexCount, remap.Get());

                    // Remap vertex buffer
#define REMAP_VERTEX_BUFFER(name, type) \
if (srcMesh->name.HasItems()) \
{ \
ASSERT(srcMesh->name.Count() == srcMeshVertexCount); \
dstMesh->name.Resize(dstMeshVertexCount); \
meshopt_remapVertexBuffer(dstMesh->name.Get(), srcMesh->name.Get(), srcMeshVertexCount, sizeof(type), remap.Get()); \
}
                    REMAP_VERTEX_BUFFER(Positions, Float3);
                    REMAP_VERTEX_BUFFER(UVs, Float2);
                    REMAP_VERTEX_BUFFER(Normals, Float3);
                    REMAP_VERTEX_BUFFER(Tangents, Float3);
                    REMAP_VERTEX_BUFFER(Tangents, Float3);
                    REMAP_VERTEX_BUFFER(LightmapUVs, Float2);
                    REMAP_VERTEX_BUFFER(Colors, Color);
                    REMAP_VERTEX_BUFFER(BlendIndices, Int4);
                    REMAP_VERTEX_BUFFER(BlendWeights, Float4);
#undef REMAP_VERTEX_BUFFER

                    // Remap blend shapes
                    dstMesh->BlendShapes.Resize(srcMesh->BlendShapes.Count());
                    for (int32 blendShapeIndex = 0; blendShapeIndex < srcMesh->BlendShapes.Count(); blendShapeIndex++)
                    {
                        const auto& srcBlendShape = srcMesh->BlendShapes[blendShapeIndex];
                        auto& dstBlendShape = dstMesh->BlendShapes[blendShapeIndex];

                        dstBlendShape.Name = srcBlendShape.Name;
                        dstBlendShape.Weight = srcBlendShape.Weight;
                        dstBlendShape.Vertices.EnsureCapacity(srcBlendShape.Vertices.Count());
                        for (int32 i = 0; i < srcBlendShape.Vertices.Count(); i++)
                        {
                            auto v = srcBlendShape.Vertices[i];
                            v.VertexIndex = remap[v.VertexIndex];
                            if (v.VertexIndex != ~0u)
                            {
                                dstBlendShape.Vertices.Add(v);
                            }
                        }
                    }

                    // Remove empty blend shapes
                    for (int32 blendShapeIndex = dstMesh->BlendShapes.Count() - 1; blendShapeIndex >= 0; blendShapeIndex--)
                    {
                        if (dstMesh->BlendShapes[blendShapeIndex].Vertices.IsEmpty())
                            dstMesh->BlendShapes.RemoveAt(blendShapeIndex);
                    }

                    // Optimize generated LOD
                    meshopt_optimizeVertexCache(dstMesh->Indices.Get(), dstMesh->Indices.Get(), dstMeshIndexCount, dstMeshVertexCount);
                    meshopt_optimizeOverdraw(dstMesh->Indices.Get(), dstMesh->Indices.Get(), dstMeshIndexCount, (const float*)dstMesh->Positions.Get(), dstMeshVertexCount, sizeof(Float3), 1.05f);

                    lodTriangleCount += dstMeshIndexCount / 3;
                    lodVertexCount += dstMeshVertexCount;
                    generatedLod++;
                }

                // Remove empty meshes (no LOD was generated for them)
                for (int32 i = dstLod.Meshes.Count() - 1; i >= 0; i--)
                {
                    MeshData* mesh = dstLod.Meshes[i];
                    if (mesh->Indices.IsEmpty() || mesh->Positions.IsEmpty())
                    {
                        Delete(mesh);
                        dstLod.Meshes.RemoveAtKeepOrder(i);
                    }
                }

                LOG_INFO("Resource", "Generated LOD{0}: triangles: {1} ({2}% of base LOD), verticies: {3} ({4}% of base LOD)",
                    lodIndex,
                    lodTriangleCount, (int32)(lodTriangleCount * 100 / baseLodTriangleCount),
                    lodVertexCount, (int32)(lodVertexCount * 100 / baseLodVertexCount));
            }
            for (int32 lodIndex = data.LODs.Count() - 1; lodIndex > 0; lodIndex--)
            {
                if (data.LODs[lodIndex].Meshes.IsEmpty())
                    data.LODs.RemoveAt(lodIndex);
                else
                    break;
            }
            if (generatedLod)
            {
                auto lodEndTime = DateTime::NowUTC();
                LOG_INFO("Resource", "Generated LODs for {1} meshes in {0} ms", static_cast<int32>((lodEndTime - lodStartTime).GetTotalMilliseconds()), generatedLod);
            }
        }

        // Calculate blend shapes vertices ranges
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (auto& blendShape : mesh->BlendShapes)
                {
                    // Compute min/max for used vertex indices
                    blendShape.MinVertexIndex = Max_uint32;
                    blendShape.MaxVertexIndex = 0;
                    blendShape.UseNormals = false;
                    for (int32 i = 0; i < blendShape.Vertices.Count(); i++)
                    {
                        auto& v = blendShape.Vertices[i];
                        blendShape.MinVertexIndex = Math::Min(blendShape.MinVertexIndex, v.VertexIndex);
                        blendShape.MaxVertexIndex = Math::Max(blendShape.MaxVertexIndex, v.VertexIndex);
                        blendShape.UseNormals |= !v.NormalDelta.IsZero();
                    }
                }
            }
        }

        // Auto calculate LODs transition settings
        data.CalculateLODsScreenSizes();

        const auto endTime = DateTime::NowUTC();
        LOG_INFO("Resource", "Model file imported in {0} ms", static_cast<int32>((endTime - startTime).GetTotalMilliseconds()));

        return true;
    }

    int32 ModelUtils::DetectLodIndex(const String& nodeName)
    {
        int32 index = nodeName.FindLast(SE_TEXT("LOD"), 0, StringSearchCase::IgnoreCase);
        if (index != -1)
        {
            // Some models use LOD_0 to identify LOD levels
            if (nodeName.Length() > index + 4 && nodeName[index + 3] == '_')
                index++;

            int32 num;
            if (!StringUtils::Parse(nodeName.Get() + index + 3, &num))
            {
                if (num >= 0 && num < MODEL_MAX_LODS)
                    return num;
                LOG_WARNING("Resource", "Invalid mesh level of detail index at node \'{0}\'. Maximum supported amount of LODs is {1}.", nodeName, MODEL_MAX_LODS);
            }
        }
        return 0;
    }

    bool ModelUtils::FindTexture(const String& sourcePath, const String& file, String& path)
    {
        const String sourceFolder = FileSystem::GetDirectoryName(sourcePath);
        path = sourceFolder / file;
        if (!FileSystem::FileExists(path))
        {
            const String filename = FileSystem::GetFileName(file);
            path = sourceFolder / filename;
            if (!FileSystem::FileExists(path))
            {
                path = sourceFolder / SE_TEXT("textures") / filename;
                if (!FileSystem::FileExists(path))
                {
                    path = sourceFolder / SE_TEXT("Textures") / filename;
                    if (!FileSystem::FileExists(path))
                    {
                        path = sourceFolder / SE_TEXT("texture") / filename;
                        if (!FileSystem::FileExists(path))
                        {
                            path = sourceFolder / SE_TEXT("Texture") / filename;
                            if (!FileSystem::FileExists(path))
                            {
                                path = sourceFolder / SE_TEXT("../textures") / filename;
                                if (!FileSystem::FileExists(path))
                                {
                                    path = sourceFolder / SE_TEXT("../Textures") / filename;
                                    if (!FileSystem::FileExists(path))
                                    {
                                        path = sourceFolder / SE_TEXT("../texture") / filename;
                                        if (!FileSystem::FileExists(path))
                                        {
                                            path = sourceFolder / SE_TEXT("../Texture") / filename;
                                            if (!FileSystem::FileExists(path))
                                            {
                                                return false;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        FileSystem::NormalizePath(path);
        return true;
    }

    /*void ModelUtils::CalculateBoneOffsetMatrix(const List<SkeletonNode>& nodes, Matrix& offsetMatrix, int32 nodeIndex)
    {
        offsetMatrix = Matrix::Identity;
        int32 idx = nodeIndex;
        do
        {
            const SkeletonNode& node = nodes[idx];
            offsetMatrix *= node.LocalTransform.GetWorld();
            idx = node.ParentIndex;
        } while (idx != -1);
        offsetMatrix.Invert();
    }*/

#endif
} // SE