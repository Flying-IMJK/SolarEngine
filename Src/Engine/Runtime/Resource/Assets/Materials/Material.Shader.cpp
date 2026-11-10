#include <Runtime/Resource/Assets/Materials/Material.h>

#include <Runtime/Core/Platform/FileSystem.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Resource/Storage/StorageChunk.h>

namespace SE
{
#if SE_EDITOR
    bool Material::LoadMaterialSourceFiles()
    {
        m_SourceFiles.Clear();
        if (!HasChunk(MATERIAL_FILE_CHUNK_SOURCE_FILES) ||
            !LoadChunks(GET_CHUNK_FLAG(MATERIAL_FILE_CHUNK_SOURCE_FILES)))
            return false;
        const StorageChunk* chunk = GetChunk(MATERIAL_FILE_CHUNK_SOURCE_FILES);
        if (!chunk || chunk->Size() < 2 * sizeof(uint32))
            return false;
        MemoryReadStream stream(chunk->Get(), chunk->Size());
        uint32           version, count;
        stream.Read(version);
        stream.Read(count);
        if (version != 1 || count == 0 ||
            count > (stream.GetLength() - stream.GetPosition()) / (sizeof(uint32) + sizeof(DateTime)))
            return false;
        bool current = true;
        for (uint32 i = 0; i < count; ++i)
        {
            if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
                return false;
            uint32 length;
            stream.Read(length);
            if (length > (stream.GetLength() - stream.GetPosition()) / sizeof(Char) ||
                stream.GetLength() - stream.GetPosition() - length * sizeof(Char) < sizeof(DateTime))
                return false;
            String path(reinterpret_cast<const Char*>(stream.GetPositionHandle()), length);
            stream.SetPosition(stream.GetPosition() + length * sizeof(Char));
            DateTime timestamp;
            stream.Read(timestamp);
            m_SourceFiles.Add(path);
            current &= FileSystem::FileExists(path) && FileSystem::GetFileLastEditTime(path) == timestamp;
        }
        return current && stream.GetPosition() == stream.GetLength();
    }

    bool Material::SaveMaterialSourceFiles()
    {
        MemoryWriteStream stream;
        stream.Write(static_cast<uint32>(1));
        stream.Write(static_cast<uint32>(m_SourceFiles.Count()));
        for (const String& path : m_SourceFiles)
        {
            stream.Write(static_cast<uint32>(path.Length()));
            stream.WriteBytes(reinterpret_cast<const byte*>(path.Get()), path.Length() * sizeof(Char));
            stream.Write(FileSystem::GetFileLastEditTime(path));
        }
        StorageChunk* chunk = GetOrCreateChunk(MATERIAL_FILE_CHUNK_SOURCE_FILES);
        if (!chunk)
            return false;
        chunk->Data.Copy(stream.GetHandle(), stream.GetLength());
        return true;
    }
#endif

} // namespace SE
