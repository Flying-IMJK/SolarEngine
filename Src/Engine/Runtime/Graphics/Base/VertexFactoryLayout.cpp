#include "VertexFactoryLayout.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "Runtime/Core/Types/Hash.h"
#include "Runtime/Graphics/GlobalSettings_GPU.h"
#include "Runtime/Graphics/Shaders/Config.h"

namespace SE
{
	namespace
	{
		void AppendHashUint32(List<byte>& data, const uint32 value)
		{
			// 固定使用小端字节序，避免主机结构体布局和 padding 影响哈希。
			data.Add(static_cast<byte>(value));
			data.Add(static_cast<byte>(value >> 8));
			data.Add(static_cast<byte>(value >> 16));
			data.Add(static_cast<byte>(value >> 24));
		}

		void AppendHashString(List<byte>& data, const String& value)
		{
			AppendHashUint32(data, static_cast<uint32>(value.Length()));
			for (int32 index = 0; index < value.Length(); index++)
			{
				AppendHashUint32(data, static_cast<uint32>(value[index]));
			}
		}

		bool IsBindingLess(const VertexFactoryBufferBinding& left, const VertexFactoryBufferBinding& right)
		{
			return left.Slot < right.Slot;
		}

		bool IsElementLess(const VertexFactoryInputElement& left, const VertexFactoryInputElement& right)
		{
			if (left.Slot != right.Slot)
			{
				return left.Slot < right.Slot;
			}
			if (left.Offset != right.Offset)
			{
				return left.Offset < right.Offset;
			}
			if (left.Semantic != right.Semantic)
			{
				return left.Semantic < right.Semantic;
			}
			return left.SemanticIndex < right.SemanticIndex;
		}

		uint64 BuildLayoutHash(
			const List<VertexFactoryBufferBinding>& bindings,
			const List<VertexFactoryInputElement>& elements)
		{
			List<byte> data;
			data.EnsureCapacity(16 + bindings.Count() * 16 + elements.Count() * 32);

			// 版本标记隔离后续字段语义调整，列表已在进入此函数前完成规范化排序。
			AppendHashUint32(data, 1);
			AppendHashUint32(data, static_cast<uint32>(bindings.Count()));
			for (int32 index = 0; index < bindings.Count(); index++)
			{
				const VertexFactoryBufferBinding& binding = bindings[index];
				AppendHashUint32(data, binding.Slot);
				AppendHashUint32(data, binding.Stride);
				AppendHashUint32(data, static_cast<uint32>(binding.InputRate));
				AppendHashUint32(data, binding.InstanceStepRate);
			}

			AppendHashUint32(data, static_cast<uint32>(elements.Count()));
			for (int32 index = 0; index < elements.Count(); index++)
			{
				const VertexFactoryInputElement& element = elements[index];
				AppendHashString(data, element.Semantic);
				AppendHashUint32(data, element.SemanticIndex);
				AppendHashUint32(data, static_cast<uint32>(element.Format));
				AppendHashUint32(data, element.Slot);
				AppendHashUint32(data, element.Offset);
			}

			return Hash::XXHash::GetHash64(data);
		}
	}

	bool VertexFactoryLayout::Create(
		const List<VertexFactoryBufferBinding>& bindings,
		const List<VertexFactoryInputElement>& elements,
		VertexFactoryLayout& output)
	{
		if (bindings.Count() > GPU_MAX_VB_BINDED)
		{
			LOG_ERROR("Graphics", "VertexFactory layout has too many bindings. Maximum is {0}.", GPU_MAX_VB_BINDED);
			return false;
		}
		if (elements.Count() > VERTEX_SHADER_MAX_INPUT_ELEMENTS)
		{
			LOG_ERROR("Graphics", "VertexFactory layout has too many elements. Maximum is {0}.", VERTEX_SHADER_MAX_INPUT_ELEMENTS);
			return false;
		}

		VertexFactoryLayout candidate;
		candidate.m_Bindings = bindings;
		candidate.m_Elements = elements;

		// Semantic 不区分大小写，构造时统一为大写并据此建立唯一键。
		for (int32 index = 0; index < candidate.m_Elements.Count(); index++)
		{
			VertexFactoryInputElement& element = candidate.m_Elements[index];
			element.Semantic.ToUpper();
			if (element.Semantic.IsEmpty())
			{
				LOG_ERROR("Graphics", "VertexFactory element semantic cannot be empty.");
				return false;
			}
			if (!IsVertexFormatSupported(element.Format))
			{
                LOG_ERROR("Graphics", "VertexFactory element {0}{1} uses an unsupported vertex format {2}.",
					element.Semantic,
					element.SemanticIndex,
					PixelFormatGetString(element.Format));
				return false;
			}
		}

		Sorting::QuickSort(candidate.m_Bindings, IsBindingLess);
		Sorting::QuickSort(candidate.m_Elements, IsElementLess);

		for (int32 bindingIndex = 0; bindingIndex < candidate.m_Bindings.Count(); bindingIndex++)
		{
			const VertexFactoryBufferBinding& binding = candidate.m_Bindings[bindingIndex];
			if (binding.Slot >= GPU_MAX_VB_BINDED)
			{
				LOG_ERROR("Graphics", "VertexFactory binding slot {0} exceeds the engine limit.", binding.Slot);
				return false;
			}
			if (binding.Stride == 0)
			{
				LOG_ERROR("Graphics", "VertexFactory binding slot {0} has zero stride.", binding.Slot);
				return false;
			}
			if ((binding.InputRate == VertexFactoryInputRate::PerVertex && binding.InstanceStepRate != 0) ||
				(binding.InputRate == VertexFactoryInputRate::PerInstance && binding.InstanceStepRate == 0))
			{
				LOG_ERROR("Graphics", "VertexFactory binding slot {0} has an invalid instance step rate.", binding.Slot);
				return false;
			}
			if (bindingIndex > 0 && candidate.m_Bindings[bindingIndex - 1].Slot == binding.Slot)
			{
				LOG_ERROR("Graphics", "VertexFactory layout contains duplicate binding slot {0}.", binding.Slot);
				return false;
			}
		}

		for (int32 elementIndex = 0; elementIndex < candidate.m_Elements.Count(); elementIndex++)
		{
			const VertexFactoryInputElement& element = candidate.m_Elements[elementIndex];
			const VertexFactoryBufferBinding* binding = nullptr;
			for (int32 bindingIndex = 0; bindingIndex < candidate.m_Bindings.Count(); bindingIndex++)
			{
				if (candidate.m_Bindings[bindingIndex].Slot == element.Slot)
				{
					binding = &candidate.m_Bindings[bindingIndex];
					break;
				}
			}
			if (binding == nullptr)
			{
				LOG_ERROR("Graphics", "VertexFactory element {0}{1} references unknown binding slot {2}.", element.Semantic, element.SemanticIndex, element.Slot);
				return false;
			}

			const uint32 elementSize = PixelFormatGetSizeInBytes(element.Format);
			if (element.Offset > binding->Stride || elementSize > binding->Stride - element.Offset)
			{
				LOG_ERROR("Graphics", "VertexFactory element {0}{1} exceeds binding slot {2} stride.", element.Semantic, element.SemanticIndex, element.Slot);
				return false;
			}

			for (int32 previousIndex = 0; previousIndex < elementIndex; previousIndex++)
			{
				const VertexFactoryInputElement& previous = candidate.m_Elements[previousIndex];
				if (previous.Semantic == element.Semantic && previous.SemanticIndex == element.SemanticIndex)
				{
					LOG_ERROR("Graphics", "VertexFactory layout contains duplicate semantic {0}{1}.", element.Semantic, element.SemanticIndex);
					return false;
				}
				if (previous.Slot == element.Slot)
				{
					const uint32 previousEnd = previous.Offset + PixelFormatGetSizeInBytes(previous.Format);
					const uint32 elementEnd = element.Offset + elementSize;
					if (previous.Offset < elementEnd && element.Offset < previousEnd)
					{
                        LOG_ERROR("Graphics", "VertexFactory elements {0}{1} and {2}{3} overlap in slot {4}.",
							previous.Semantic,
							previous.SemanticIndex,
							element.Semantic,
							element.SemanticIndex,
							element.Slot);
						return false;
					}
				}
			}
		}

		candidate.m_LayoutHash = BuildLayoutHash(candidate.m_Bindings, candidate.m_Elements);
		candidate.m_IsValid = true;
		output = MoveTemp(candidate);
		return true;
	}

	bool VertexFactoryLayout::IsVertexFormatSupported(const PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R32G32B32A32_Float:
		case PixelFormat::R32G32B32A32_UInt:
		case PixelFormat::R32G32B32A32_SInt:
		case PixelFormat::R32G32B32_Float:
		case PixelFormat::R32G32B32_UInt:
		case PixelFormat::R32G32B32_SInt:
		case PixelFormat::R16G16B16A16_Float:
		case PixelFormat::R16G16B16A16_UNorm:
		case PixelFormat::R16G16B16A16_UInt:
		case PixelFormat::R16G16B16A16_SNorm:
		case PixelFormat::R16G16B16A16_SInt:
		case PixelFormat::R32G32_Float:
		case PixelFormat::R32G32_UInt:
		case PixelFormat::R32G32_SInt:
		case PixelFormat::R10G10B10A2_UNorm:
		case PixelFormat::R10G10B10A2_UInt:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UInt:
		case PixelFormat::R8G8B8A8_SNorm:
		case PixelFormat::R8G8B8A8_SInt:
		case PixelFormat::R16G16_Float:
		case PixelFormat::R16G16_UNorm:
		case PixelFormat::R16G16_UInt:
		case PixelFormat::R16G16_SNorm:
		case PixelFormat::R16G16_SInt:
		case PixelFormat::R32_Float:
		case PixelFormat::R32_UInt:
		case PixelFormat::R32_SInt:
		case PixelFormat::R8G8_UNorm:
		case PixelFormat::R8G8_UInt:
		case PixelFormat::R8G8_SNorm:
		case PixelFormat::R8G8_SInt:
		case PixelFormat::R16_Float:
		case PixelFormat::R16_UNorm:
		case PixelFormat::R16_UInt:
		case PixelFormat::R16_SNorm:
		case PixelFormat::R16_SInt:
		case PixelFormat::R8_UNorm:
		case PixelFormat::R8_UInt:
		case PixelFormat::R8_SNorm:
		case PixelFormat::R8_SInt:
			return true;
		default:
			return false;
		}
	}

	const VertexFactoryBufferBinding* VertexFactoryLayout::FindBinding(const uint32 slot) const
	{
		for (int32 index = 0; index < m_Bindings.Count(); index++)
		{
			if (m_Bindings[index].Slot == slot)
			{
				return &m_Bindings[index];
			}
		}
		return nullptr;
	}

	const VertexFactoryInputElement* VertexFactoryLayout::FindElement(
		const StringView& semantic,
		const uint32 semanticIndex) const
	{
		String normalizedSemantic(semantic);
		normalizedSemantic.ToUpper();
		for (int32 index = 0; index < m_Elements.Count(); index++)
		{
			const VertexFactoryInputElement& element = m_Elements[index];
			if (element.Semantic == normalizedSemantic && element.SemanticIndex == semanticIndex)
			{
				return &element;
			}
		}
		return nullptr;
	}
}
