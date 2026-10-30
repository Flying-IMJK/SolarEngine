#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialInstance.h>

#include <Runtime/Utilities/Variant.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Resource/AssetContent.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialStorage.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/Resource/Storage/StorageChunk.h>

namespace SE
{
    BINARY_ASSET_FACTORY(SLC2MaterialInstance, true);

    namespace
    {
        bool ValidateParameters(const MaterialParams& parameters)
        {
            for (int32 index = 0; index < parameters.Count(); ++index)
            {
                const MaterialParameter& parameter = parameters[index];
                if (parameter.GetParameterID() == UID::Empty ||
                    parameter.GetParameterType() <= MaterialParameterType::Invalid ||
                    parameter.GetParameterType() > MaterialParameterType::GlobalSDF)
                    return false;
                for (int32 other = index + 1; other < parameters.Count(); ++other)
                {
                    if (parameters[other].GetParameterID() == parameter.GetParameterID())
                        return false;
                }
            }
            return true;
        }

        bool RefreshParametersVersion(MaterialParams& parameters)
        {
            if (parameters.IsEmpty())
            {
                parameters.Dispose();
                return true;
            }
            MemoryWriteStream stream(256);
            parameters.Save(&stream);
            if (stream.GetPosition() == 0)
                return false;
            MemoryReadStream reader(stream.GetHandle(), stream.GetPosition());
            MaterialParams   refreshed;
            if (refreshed.Load(&reader) || reader.GetPosition() != reader.GetLength())
                return false;
            parameters = MoveTemp(refreshed);
            return true;
        }

        bool SynchronizeParameters(MaterialParams& source, MaterialParams& previous, MaterialParams& output)
        {
            source.Clone(output);
            for (MaterialParameter& parameter : output)
                parameter.SetIsOverride(false);
            for (const MaterialParameter& oldParameter : previous)
            {
                if (!oldParameter.IsOverride())
                    continue;
                MaterialParameter* parameter = output.Get(oldParameter.GetParameterID());
                if (parameter == nullptr || parameter->GetParameterType() != oldParameter.GetParameterType())
                    continue;
                if (parameter->TrySetValue(oldParameter.GetValue()) != MaterialParameterSetResult::Success)
                    return false;
                parameter->SetIsOverride(true);
            }
            return RefreshParametersVersion(output) && ValidateParameters(output);
        }
    } // namespace

    SLC2MaterialInstance::SLC2MaterialInstance(const SpawnParams& params, const AssetInfo* info) :
        BinaryAsset(params, info)
    {}

    bool SLC2MaterialInstance::ResolveParameters(MaterialParams&                    output,
                                                 AssetRef<SLC2Material>&            baseMaterial,
                                                 uint64&                            baseRevision,
                                                 List<const SLC2MaterialInstance*>& stack) const
    {
        if (stack.Count() >= 64)
            return false;
        for (const SLC2MaterialInstance* item : stack)
        {
            if (item == this)
                return false;
        }
        stack.Add(this);
        const bool result = [&]() {
            AssetRef<SLC2Material>         base;
            AssetRef<SLC2MaterialInstance> parent;
            uint64                         revision;
            {
                Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
                base     = m_Base;
                parent   = m_Parent;
                revision = m_Revision;
            }

            MaterialParams         source;
            int32                  sourceVersion;
            AssetRef<SLC2Material> resolvedBase;
            uint64                 resolvedBaseRevision;
            if (base)
            {
                if (!base->WaitForLoaded() || !base->SnapshotParameters(source, sourceVersion, resolvedBaseRevision))
                    return false;
                resolvedBase = base;
            }
            else if (parent)
            {
                if (!parent->WaitForLoaded() ||
                    !parent->ResolveParameters(source, resolvedBase, resolvedBaseRevision, stack))
                    return false;
                sourceVersion = source.GetVersionHash();
            }
            else
            {
                return false;
            }

            Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
            if (m_Revision != revision || m_Base.Get() != base.Get() || m_Parent.Get() != parent.Get())
                return false;
            if (!m_Synchronized || m_SourceVersion != sourceVersion)
            {
                MaterialParams previous;
                m_Parameters.Clone(previous);
                MaterialParams synchronized;
                if (!SynchronizeParameters(source, previous, synchronized))
                    return false;
                m_Parameters    = MoveTemp(synchronized);
                m_SourceVersion = sourceVersion;
                m_Synchronized  = true;
                ++m_Revision;
            }
            m_Parameters.Clone(output);
            baseMaterial = MoveTemp(resolvedBase);
            baseRevision = resolvedBaseRevision;
            return true;
        }();
        stack.RemoveLast();
        return result;
    }

    bool SLC2MaterialInstance::GetParameters(MaterialParams& output) const
    {
        AssetRef<SLC2Material>            baseMaterial;
        uint64                            baseRevision;
        List<const SLC2MaterialInstance*> stack;
        return ResolveParameters(output, baseMaterial, baseRevision, stack);
    }

    bool SLC2MaterialInstance::SetParameterOverride(const UID& parameterId, const Variant& value)
    {
        MaterialParams                    synchronized;
        AssetRef<SLC2Material>            baseMaterial;
        uint64                            baseRevision;
        List<const SLC2MaterialInstance*> stack;
        if (!ResolveParameters(synchronized, baseMaterial, baseRevision, stack))
            return false;

        Threading::ScopeLock lock(Locker);
        MaterialParams       candidate;
        m_Parameters.Clone(candidate);
        MaterialParameter* parameter = candidate.Get(parameterId);
        if (parameter == nullptr || parameter->TrySetValue(value) != MaterialParameterSetResult::Success)
            return false;
        parameter->SetIsOverride(true);
        if (!RefreshParametersVersion(candidate) || !ValidateParameters(candidate))
            return false;
        m_Parameters = MoveTemp(candidate);
        ++m_Revision;
        return true;
    }

    bool SLC2MaterialInstance::ClearParameterOverride(const UID& parameterId)
    {
        AssetRef<SLC2Material>         base;
        AssetRef<SLC2MaterialInstance> parent;
        MaterialParams                 previous;
        uint64                         revision;
        {
            Threading::ScopeLock lock(Locker);
            base   = m_Base;
            parent = m_Parent;
            m_Parameters.Clone(previous);
            revision = m_Revision;
        }

        MaterialParams source;
        int32          sourceVersion;
        uint64         baseRevision;
        if (base)
        {
            if (!base->WaitForLoaded() || !base->SnapshotParameters(source, sourceVersion, baseRevision))
                return false;
        }
        else if (parent)
        {
            AssetRef<SLC2Material>            baseMaterial;
            List<const SLC2MaterialInstance*> stack;
            stack.Add(this);
            if (!parent->WaitForLoaded() || !parent->ResolveParameters(source, baseMaterial, baseRevision, stack))
                return false;
            sourceVersion = source.GetVersionHash();
        }
        else
        {
            return false;
        }

        MaterialParameter* sourceParameter = source.Get(parameterId);
        MaterialParameter* parameter       = previous.Get(parameterId);
        if (sourceParameter == nullptr || parameter == nullptr ||
            sourceParameter->GetParameterType() != parameter->GetParameterType() ||
            parameter->TrySetValue(sourceParameter->GetValue()) != MaterialParameterSetResult::Success)
            return false;
        parameter->SetIsOverride(false);
        if (!RefreshParametersVersion(previous) || !ValidateParameters(previous))
            return false;

        Threading::ScopeLock lock(Locker);
        if (m_Revision != revision || m_Base.Get() != base.Get() || m_Parent.Get() != parent.Get())
            return false;
        m_Parameters    = MoveTemp(previous);
        m_SourceVersion = sourceVersion;
        m_Synchronized  = true;
        ++m_Revision;
        return true;
    }

    bool SLC2MaterialInstance::SetBaseMaterial(SLC2Material* pMaterial)
    {
        if (pMaterial == nullptr || !pMaterial->WaitForLoaded())
            return false;
        MaterialParams source;
        int32          sourceVersion;
        uint64         baseRevision;
        if (!pMaterial->SnapshotParameters(source, sourceVersion, baseRevision))
            return false;

        MaterialParams previous;
        uint64         revision;
        {
            Threading::ScopeLock lock(Locker);
            m_Parameters.Clone(previous);
            revision = m_Revision;
        }
        MaterialParams synchronized;
        if (!SynchronizeParameters(source, previous, synchronized))
            return false;

        Threading::ScopeLock lock(Locker);
        if (m_Revision != revision)
            return false;
        m_Base          = pMaterial;
        m_Parent        = nullptr;
        m_Parameters    = MoveTemp(synchronized);
        m_SourceVersion = sourceVersion;
        m_Synchronized  = true;
        ++m_Revision;
        return true;
    }

    bool SLC2MaterialInstance::SetParentInstance(SLC2MaterialInstance* pInstance)
    {
        if (pInstance == nullptr || pInstance == this || !pInstance->WaitForLoaded())
            return false;
        MaterialParams                    source;
        AssetRef<SLC2Material>            baseMaterial;
        uint64                            baseRevision;
        List<const SLC2MaterialInstance*> stack;
        stack.Add(this);
        if (!pInstance->ResolveParameters(source, baseMaterial, baseRevision, stack))
            return false;
        const int32 sourceVersion = source.GetVersionHash();

        MaterialParams previous;
        uint64         revision;
        {
            Threading::ScopeLock lock(Locker);
            m_Parameters.Clone(previous);
            revision = m_Revision;
        }
        MaterialParams synchronized;
        if (!SynchronizeParameters(source, previous, synchronized))
            return false;

        Threading::ScopeLock lock(Locker);
        if (m_Revision != revision)
            return false;
        m_Base          = nullptr;
        m_Parent        = pInstance;
        m_Parameters    = MoveTemp(synchronized);
        m_SourceVersion = sourceVersion;
        m_Synchronized  = true;
        ++m_Revision;
        return true;
    }

    AssetRef<SLC2Material> SLC2MaterialInstance::GetBaseMaterial() const
    {
        Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
        return m_Base;
    }

    AssetRef<SLC2MaterialInstance> SLC2MaterialInstance::GetParentInstance() const
    {
        Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
        return m_Parent;
    }

    bool SLC2MaterialInstance::PrepareDraw(const String&                             passName,
                                           const SLC2MaterialGeometryMode            geometryMode,
                                           const List<String>&                       defines,
                                           const SLC2MaterialDrawContext&            context,
                                           std::unique_ptr<SLC2MaterialDrawProgram>& output) const
    {
        output.reset();
        MaterialParams                    parameters;
        AssetRef<SLC2Material>            baseMaterial;
        uint64                            baseRevision;
        List<const SLC2MaterialInstance*> stack;
        if (!ResolveParameters(parameters, baseMaterial, baseRevision, stack) || !baseMaterial)
            return false;
        return baseMaterial->PrepareDrawWithParameters(
            passName, geometryMode, defines, parameters, baseRevision, context, output);
    }

    uint32 SLC2MaterialInstance::GetSerializedVersion() const { return SLC2MaterialStorage::Version; }

    bool SLC2MaterialInstance::OnInit(AssetInitData& initData)
    {
        return initData.SerializedVersion == SLC2MaterialStorage::Version;
    }

    AssetChunksFlag SLC2MaterialInstance::GetChunksToPreload() const
    {
        return GET_CHUNK_FLAG(SLC2MaterialStorage::Chunk);
    }

    Asset::LoadResult SLC2MaterialInstance::load()
    {
        StorageChunk*  chunk = GetChunk(SLC2MaterialStorage::Chunk);
        uint32         referenceKind;
        UID            referenceId;
        MaterialParams parameters;
        if (chunk == nullptr || !chunk->IsValid() ||
            !SLC2MaterialStorage::DecodeInstance(chunk->Get(), chunk->Size(), referenceKind, referenceId, parameters) ||
            !ValidateParameters(parameters) || referenceKind > 1 || referenceId == GetID())
            return LoadResult::InvalidData;

        AssetRef<SLC2Material>         base;
        AssetRef<SLC2Material>         resolvedBase;
        AssetRef<SLC2MaterialInstance> parent;
        MaterialParams                 source;
        int32                          sourceVersion;
        uint64                         baseRevision;
        if (referenceKind == 0)
        {
            base = AssetContent::LoadAsync<SLC2Material>(referenceId);
            if (!base || !base->WaitForLoaded() || !base->SnapshotParameters(source, sourceVersion, baseRevision))
                return LoadResult::InvalidData;
        }
        else
        {
            parent = AssetContent::LoadAsync<SLC2MaterialInstance>(referenceId);
            List<const SLC2MaterialInstance*> stack;
            stack.Add(this);
            if (!parent || !parent->WaitForLoaded() ||
                !parent->ResolveParameters(source, resolvedBase, baseRevision, stack))
                return LoadResult::InvalidData;
            sourceVersion = source.GetVersionHash();
        }
        MaterialParams synchronized;
        if (!SynchronizeParameters(source, parameters, synchronized))
            return LoadResult::InvalidData;

        Threading::ScopeLock lock(Locker);
        m_Base          = MoveTemp(base);
        m_Parent        = MoveTemp(parent);
        m_Parameters    = MoveTemp(synchronized);
        m_SourceVersion = sourceVersion;
        m_Synchronized  = true;
        ++m_Revision;
        return LoadResult::Ok;
    }

    void SLC2MaterialInstance::Unload(const bool isReloading)
    {
        // Reload keeps the published relation and overrides until a complete candidate replaces them.
        if (isReloading)
            return;
        Threading::ScopeLock lock(Locker);
        m_Base   = nullptr;
        m_Parent = nullptr;
        m_Parameters.Dispose();
        m_SourceVersion = 0;
        m_Synchronized  = false;
        ++m_Revision;
    }

#if SE_EDITOR
    bool SLC2MaterialInstance::SaveAssetData(const StringView&     path,
                                             SLC2Material*         pMaterial,
                                             SLC2MaterialInstance* pParent,
                                             MaterialParams&       parameters)
    {
        if ((pMaterial == nullptr) == (pParent == nullptr) || !ValidateParameters(parameters))
            return false;

        MaterialParams source;
        uint64         baseRevision;
        if (pMaterial != nullptr)
        {
            int32 sourceVersion;
            if (!pMaterial->WaitForLoaded() || !pMaterial->SnapshotParameters(source, sourceVersion, baseRevision))
                return false;
            (void)sourceVersion;
        }
        else
        {
            AssetInfo destination;
            const UID destinationId = AssetContent::GetAssetInfo(path, destination) ? destination.id : UID::Empty;
            AssetRef<SLC2MaterialInstance> ancestor = pParent;
            List<UID>                      visited;
            while (ancestor)
            {
                if (visited.Count() >= 64 || ancestor.GetID() == destinationId)
                    return false;
                for (const UID& id : visited)
                {
                    if (id == ancestor.GetID())
                        return false;
                }
                visited.Add(ancestor.GetID());
                if (!ancestor->WaitForLoaded())
                    return false;
                ancestor = ancestor->GetParentInstance();
            }

            AssetRef<SLC2Material>            baseMaterial;
            List<const SLC2MaterialInstance*> stack;
            if (!pParent->ResolveParameters(source, baseMaterial, baseRevision, stack))
                return false;
        }
        (void)baseRevision;

        MaterialParams synchronized;
        if (!SynchronizeParameters(source, parameters, synchronized))
            return false;

        const uint32 referenceKind = pParent != nullptr ? 1u : 0u;
        const UID    referenceId   = pParent != nullptr ? pParent->GetID() : pMaterial->GetID();
        List<byte>   bytes;
        SLC2MaterialStorage::EncodeInstance(referenceKind, referenceId, synchronized, bytes);
        StorageChunk chunk;
        chunk.Data.Copy(bytes.Get(), bytes.Count());
        AssetInitData data;
        data.Header.TypeID                             = Typeof<SLC2MaterialInstance>();
        data.Header.Chunks[SLC2MaterialStorage::Chunk] = &chunk;
        data.SerializedVersion                         = SLC2MaterialStorage::Version;
        return SaveToAsset(path, data);
    }

    void SLC2MaterialInstance::GetReferences(List<UID>& output) const
    {
        BinaryAsset::GetReferences(output);
        AssetRef<SLC2Material>         base;
        AssetRef<SLC2MaterialInstance> parent;
        MaterialParams                 parameters;
        {
            Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
            base   = m_Base;
            parent = m_Parent;
            const_cast<MaterialParams&>(m_Parameters).Clone(parameters);
        }
        if (base)
            output.Add(base.GetID());
        if (parent)
            output.Add(parent.GetID());
        parameters.GetReferences(output);
    }
#endif
} // namespace SE
