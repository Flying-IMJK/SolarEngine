#pragma once

#include "Core/Math/BoundingVolumes.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Runtime/Graphics/DynamicBuffer.h"
#include "Runtime/Render/RenderEnum.h"

namespace SE
{
	class MeshBase;

	/// <summary>
	/// The mesh deformation data container.
	/// </summary>
	struct SE_API_RUNTIME MeshDeformationData
	{
		NON_COPYABLE(MeshDeformationData)
		uint64 Key;
		MeshBufferType Type;
		uint32 DirtyMinIndex = 0;
		uint32 DirtyMaxIndex = Max_uint32 - 1;
		bool Dirty = true;
		BoundingBox Bounds;
		DynamicVertexBuffer VertexBuffer;

		MeshDeformationData(uint64 key, MeshBufferType type, uint32 stride) :
			Key(key),
			Type(type),
			Bounds(),
			VertexBuffer(0, stride, SE_TEXT("MeshDeformation"))
		{
		}

		~MeshDeformationData()
		{
		}
	};

	class SE_API_RUNTIME MeshDeformation
	{
	private:
		Dictionary<uint32, Delegate<const MeshBase*, MeshDeformationData&>> _deformers;
		List<MeshDeformationData*> _deformations;

	public:
		~MeshDeformation()
		{
			Clear();
		}

		void GetBounds(int32 lodIndex, int32 meshIndex, BoundingBox& bounds) const;
		void Clear();
		void Dirty();
		void Dirty(int32 lodIndex, int32 meshIndex, MeshBufferType type);
		void Dirty(int32 lodIndex, int32 meshIndex, MeshBufferType type, const BoundingBox& bounds);
		void AddDeformer(int32 lodIndex, int32 meshIndex, MeshBufferType type, const Function<void(const MeshBase* mesh, MeshDeformationData& deformation)>& deformer);
		void RemoveDeformer(int32 lodIndex, int32 meshIndex, MeshBufferType type, const Function<void(const MeshBase* mesh, MeshDeformationData& deformation)>& deformer);
		void RunDeformers(const MeshBase* mesh, MeshBufferType type, GPUBuffer*& vertexBuffer);
	};
} // SE


