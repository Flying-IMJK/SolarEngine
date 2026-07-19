
#include "ModelLOD.h"

#include <Runtime/Core/Math/Transform.h>

#include "MeshDeformation.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"

namespace SE
{
	bool ModelLOD::Load(MemoryReadStream& stream)
	{
		// Load LOD for each mesh
		_verticesCount = 0;
		for (int32 i = 0; i < Meshes.Count(); i++)
		{
			// #MODEL_DATA_FORMAT_USAGE
			uint32 vertices;
			stream.ReadUint32(&vertices);
			_verticesCount += vertices;
			uint32 triangles;
			stream.ReadUint32(&triangles);
			uint32 indicesCount = triangles * 3;
			bool use16BitIndexBuffer = indicesCount <= Max_uint16;
			uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
			if (vertices == 0 || triangles == 0)
				return false;
			const VB0ElementType* vb0 = stream.Move<VB0ElementType>(vertices);
			const VB1ElementType* vb1 = stream.Move<VB1ElementType>(vertices);
			bool hasColors = stream.ReadBool();
			VB2ElementType* vb2 = nullptr;
			if (hasColors)
			{
				vb2 = stream.Move<VB2ElementType>(vertices);
			}
			const auto ib = stream.Move<byte>(indicesCount * ibStride);

			// Setup GPU resources
			if (!Meshes[i].Load(vertices, triangles, vb0, vb1, vb2, ib, use16BitIndexBuffer))
			{
				LOG_WARNING("Render", "Cannot initialize mesh {0}. Vertices: {1}, triangles: {2}", i, vertices, triangles);
				return false;
			}
		}

		return true;
	}

	void ModelLOD::Unload()
	{
		// Unload LOD for each mesh
		for (int32 i = 0; i < Meshes.Count(); i++)
		{
			Meshes[i].Unload();
		}
	}

	void ModelLOD::Dispose()
	{
		_model = nullptr;
		ScreenSize = 0.0f;
		Meshes.Resize(0);
	}

	bool ModelLOD::Intersects(const Ray& ray, const Matrix& world, float& distance, Float3& normal, Mesh** mesh)
	{
		bool result = false;
		float closest = Max_float;
		Float3 closestNormal = Float3::Up;
		for (int32 i = 0; i < Meshes.Count(); i++)
		{
			float dst;
			Float3 nrm;
			if (Meshes[i].Intersects(ray, world, dst, nrm) && dst < closest)
			{
				result = true;
				*mesh = &Meshes[i];
				closest = dst;
				closestNormal = nrm;
			}
		}
		distance = closest;
		normal = closestNormal;
		return result;
	}

	bool ModelLOD::Intersects(const Ray& ray, const Transform& transform, float& distance, Float3& normal, Mesh** mesh)
	{
		bool result = false;
		float closest = Max_float;
		Float3 closestNormal = Float3::Up;
		for (int32 i = 0; i < Meshes.Count(); i++)
		{
			float dst;
			Float3 nrm;
			if (Meshes[i].Intersects(ray, transform, dst, nrm) && dst < closest)
			{
				result = true;
				*mesh = &Meshes[i];
				closest = dst;
				closestNormal = nrm;
			}
		}
		distance = closest;
		normal = closestNormal;
		return result;
	}

	BoundingBox ModelLOD::GetBox(const Matrix& world) const
	{
		Float3 tmp, min = Float3::Maximum, max = Float3::Minimum;
		Float3 corners[8];
		for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
		{
			const auto& mesh = Meshes[meshIndex];
			mesh.GetBox().GetCorners(corners);
			for (int32 i = 0; i < 8; i++)
			{
				Float3::Transform(corners[i], world, tmp);
				min = Float3::Min(min, tmp);
				max = Float3::Max(max, tmp);
			}
		}
		return BoundingBox(min, max);
	}

	BoundingBox ModelLOD::GetBox(const Transform& transform, const MeshDeformation* deformation) const
	{
		Float3 tmp, min = Float3::Maximum, max = Float3::Minimum;
		Float3 corners[8];
		for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
		{
			const auto& mesh = Meshes[meshIndex];
			BoundingBox box = mesh.GetBox();
			if (deformation)
			{
				deformation->GetBounds(_lodIndex, meshIndex, box);
			}
			box.GetCorners(corners);
			for (int32 i = 0; i < 8; i++)
			{
				transform.LocalToWorld(corners[i], tmp);
				min = Float3::Min(min, tmp);
				max = Float3::Max(max, tmp);
			}
		}
		return BoundingBox(min, max);
	}

	BoundingBox ModelLOD::GetBox() const
	{
		Float3 min = Float3::Maximum, max = Float3::Minimum;
		Float3 corners[8];
		for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
		{
			Meshes[meshIndex].GetBox().GetCorners(corners);
			for (int32 i = 0; i < 8; i++)
			{
				min = Float3::Min(min, corners[i]);
				max = Float3::Max(max, corners[i]);
			}
		}
		return BoundingBox(min, max);
	}

	void ModelLOD::Render(GPUContext* context)
	{
		for (int32 i = 0; i < Meshes.Count(); i++)
			Meshes.Get()[i].Render(context);
	}

	void ModelLOD::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, EnumFlags<StaticMask> flags,
		bool receiveDecals, DrawPass drawModes, float perInstanceRandom, int16 sortOrder) const
	{
		for (int32 i = 0; i < Meshes.Count(); i++)
			Meshes.Get()[i].Draw(renderContext, material, world, flags, receiveDecals, drawModes, perInstanceRandom, sortOrder);
	}

	void ModelLOD::Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info, float lodDitherFactor) const
	{
		for (int32 i = 0; i < Meshes.Count(); i++)
			Meshes.Get()[i].Draw(renderContext, info, lodDitherFactor);
	}

	void ModelLOD::Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info, float lodDitherFactor) const
	{
		for (int32 i = 0; i < Meshes.Count(); i++)
			Meshes.Get()[i].Draw(renderContextBatch, info, lodDitherFactor);
	}
} // SE