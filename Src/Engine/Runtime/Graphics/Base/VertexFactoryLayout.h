#pragma once

#include "PixelFormat.h"
#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	enum class VertexFactoryInputRate : byte
	{
		PerVertex = 0,
		PerInstance = 1,
	};

	/// <summary>
	/// 描述一个 VertexFactory 顶点流的物理读取方式。
	/// </summary>
	struct SE_API_RUNTIME VertexFactoryBufferBinding
	{
		/// <summary>
		/// 插槽
		/// </summary>
		uint32 Slot = 0;
		/// <summary>
		/// 顶点流偏移步长
		/// </summary>
		uint32 Stride = 0;
		/// <summary>
		/// 顶点流读取方式
		/// </summary>
		VertexFactoryInputRate InputRate = VertexFactoryInputRate::PerVertex;
		uint32 InstanceStepRate = 0;
	};

	/// <summary>
	/// 描述一个标准顶点语义在物理顶点流中的位置和格式。
	/// </summary>
	struct SE_API_RUNTIME VertexFactoryInputElement
	{
		String Semantic;
		uint32 SemanticIndex = 0;
		PixelFormat Format = PixelFormat::Undefined;
		uint32 Slot = 0;
		uint32 Offset = 0;
	};

	/// <summary>
	/// VertexFactory 的不可变物理顶点布局。布局只能通过 Create 完整构造。
	/// </summary>
	class SE_API_RUNTIME VertexFactoryLayout
	{
	public:
		static bool Create(const List<VertexFactoryBufferBinding>& bindings, const List<VertexFactoryInputElement>& elements, VertexFactoryLayout& output);

		static bool IsVertexFormatSupported(PixelFormat format);

		bool IsValid() const
		{
			return m_IsValid;
		}

		uint64 GetHash() const
		{
			return m_LayoutHash;
		}

		const List<VertexFactoryBufferBinding>& GetBindings() const
		{
			return m_Bindings;
		}

		const List<VertexFactoryInputElement>& GetElements() const
		{
			return m_Elements;
		}

		const VertexFactoryBufferBinding* FindBinding(uint32 slot) const;
		const VertexFactoryInputElement* FindElement(const StringView& semantic, uint32 semanticIndex) const;

	private:
		List<VertexFactoryBufferBinding> m_Bindings;
		List<VertexFactoryInputElement> m_Elements;
		uint64 m_LayoutHash = 0;
		bool m_IsValid = false;
	};
}
