#include "SlangShaderCompiler.h"
#include "Runtime/Core/Types/Collections/ChunkedArray.h"

#include <slang-com-ptr.h>

namespace SE
{
	namespace
	{
		struct StageUsageContext
		{
			List<ShaderStage> Stages;
			List<Slang::ComPtr<slang::IMetadata>> Metadata;
		};

		// Builder-only association between one user-visible logical resource range and
		// the Slang binding ranges that provide its primary and optional counter descriptors.
		struct LogicalBindingRangeMap
		{
			uint32 PrimaryRangeIndex = Max_uint32;
			int32 CounterRangeIndex = -1;
		};

		struct TypeBuildEntry
		{
			slang::TypeLayoutReflection* Layout = nullptr;
			uint32 TypeId = 0;
			List<LogicalBindingRangeMap> LogicalRanges;
		};

		struct BindingPathNode
		{
			slang::VariableLayoutReflection* VariableLayout = nullptr;
			const BindingPathNode* Outer = nullptr;
		};

		// Slang 的 offset 都是相对当前 VariableLayout 的。路径保存具体 occurrence，
		// 两个边界分别决定 Uniform byte 和 descriptor binding 从哪里停止向外累计。
		struct BindingPath
		{
			const BindingPathNode* Leaf = nullptr;
			const BindingPathNode* DeepestConstantBuffer = nullptr;
			const BindingPathNode* DeepestParameterBlock = nullptr;
		};

		struct BlockLayoutContext
		{
			slang::TypeLayoutReflection* ContainerTypeLayout = nullptr;
			slang::TypeLayoutReflection* ElementTypeLayout = nullptr;
			BindingPath ContainerPath;
			bool HasContainerPath = false;
			BindingPath ElementPath;
		};

		struct ResolvedCategoryOffset
		{
			uint64 Value = 0;
			uint64 Space = 0;
		};

		using BindingPathNodeArena = ChunkedArray<BindingPathNode, 64>;

		String ToTypeKind(const slang::TypeReflection::Kind kind)
		{
			switch (kind)
			{
			case slang::TypeReflection::Kind::Scalar:
			case slang::TypeReflection::Kind::Vector:
			case slang::TypeReflection::Kind::Matrix:
				return SE_TEXT("Basic");
			case slang::TypeReflection::Kind::Struct:
				return SE_TEXT("Struct");
			case slang::TypeReflection::Kind::Array:
				return SE_TEXT("Array");
			case slang::TypeReflection::Kind::Resource:
			case slang::TypeReflection::Kind::SamplerState:
			case slang::TypeReflection::Kind::TextureBuffer:
			case slang::TypeReflection::Kind::ShaderStorageBuffer:
				return SE_TEXT("Resource");
			case slang::TypeReflection::Kind::ConstantBuffer:
			case slang::TypeReflection::Kind::ParameterBlock:
				return SE_TEXT("ParameterBlock");
			default:
				return SE_TEXT("Unsupported");
			}
		}

		String ToBindingTypeString(const slang::BindingType type)
		{
			// Slang encodes writable resources by adding MutableFlag to the
			// descriptor type. SLC2 keeps the base descriptor category; write
			// capability remains represented by the reflected resource access.
			const uint32 baseType = static_cast<uint32>(type) & static_cast<uint32>(slang::BindingType::BaseMask);
			switch (static_cast<slang::BindingType>(baseType))
			{
			case slang::BindingType::Sampler:
				return SE_TEXT("Sampler");
			case slang::BindingType::Texture:
				return SE_TEXT("Texture");
			case slang::BindingType::ConstantBuffer:
				return SE_TEXT("ConstantBuffer");
			case slang::BindingType::ParameterBlock:
				return SE_TEXT("ParameterBlock");
			case slang::BindingType::TypedBuffer:
				return SE_TEXT("TypedBuffer");
			case slang::BindingType::RawBuffer:
				return SE_TEXT("RawBuffer");
			case slang::BindingType::CombinedTextureSampler:
				return SE_TEXT("CombinedTextureSampler");
			case slang::BindingType::InputRenderTarget:
				return SE_TEXT("InputRenderTarget");
			case slang::BindingType::InlineUniformData:
				return SE_TEXT("InlineUniformData");
			case slang::BindingType::RayTracingAccelerationStructure:
				return SE_TEXT("RayTracingAccelerationStructure");
			case slang::BindingType::PushConstant:
				return SE_TEXT("PushConstant");
			default:
				return SE_TEXT("Unknown");
			}
		}

		String ToParameterCategoryString(const slang::ParameterCategory category)
		{
			switch (category)
			{
			case slang::ParameterCategory::Uniform:
				return SE_TEXT("Uniform");
			case slang::ParameterCategory::ConstantBuffer:
				return SE_TEXT("ConstantBuffer");
			case slang::ParameterCategory::ShaderResource:
				return SE_TEXT("ShaderResource");
			case slang::ParameterCategory::UnorderedAccess:
				return SE_TEXT("UnorderedAccess");
			case slang::ParameterCategory::SamplerState:
				return SE_TEXT("SamplerState");
			case slang::ParameterCategory::DescriptorTableSlot:
				return SE_TEXT("DescriptorTableSlot");
			case slang::ParameterCategory::PushConstantBuffer:
				return SE_TEXT("PushConstantBuffer");
			case slang::ParameterCategory::RegisterSpace:
				return SE_TEXT("RegisterSpace");
			case slang::ParameterCategory::SubElementRegisterSpace:
				return SE_TEXT("SubElementRegisterSpace");
			case slang::ParameterCategory::VaryingInput:
				return SE_TEXT("VaryingInput");
			case slang::ParameterCategory::VaryingOutput:
				return SE_TEXT("VaryingOutput");
			default:
				return SE_TEXT("Unknown");
			}
		}

		String ToResourceKindString(const slang::TypeReflection::Kind kind)
		{
			switch (kind)
			{
			case slang::TypeReflection::Kind::SamplerState:
				return SE_TEXT("Sampler");
			case slang::TypeReflection::Kind::TextureBuffer:
			case slang::TypeReflection::Kind::Resource:
				return SE_TEXT("Resource");
			case slang::TypeReflection::Kind::ShaderStorageBuffer:
				return SE_TEXT("StorageBuffer");
			default:
				return SE_TEXT("");
			}
		}

		String ToResourceShapeString(const SlangResourceShape shape)
		{
			return String::Format(SE_TEXT("{0}"), static_cast<int32>(shape));
		}

		String ToResourceAccessString(const SlangResourceAccess access)
		{
			return String::Format(SE_TEXT("{0}"), static_cast<int32>(access));
		}

		bool IsSupportedTypeKind(const slang::TypeReflection::Kind kind)
		{
			switch (kind)
			{
			case slang::TypeReflection::Kind::Scalar:
			case slang::TypeReflection::Kind::Vector:
			case slang::TypeReflection::Kind::Matrix:
			case slang::TypeReflection::Kind::Struct:
			case slang::TypeReflection::Kind::Array:
			case slang::TypeReflection::Kind::Resource:
			case slang::TypeReflection::Kind::SamplerState:
			case slang::TypeReflection::Kind::TextureBuffer:
			case slang::TypeReflection::Kind::ShaderStorageBuffer:
			case slang::TypeReflection::Kind::ConstantBuffer:
			case slang::TypeReflection::Kind::ParameterBlock:
				return true;
			default:
				return false;
			}
		}

		bool IsUniformLikeEntryPointCategory(const slang::ParameterCategory category)
		{
			switch (category)
			{
			case slang::ParameterCategory::VaryingInput:
			case slang::ParameterCategory::VaryingOutput:
			case slang::ParameterCategory::None:
				return false;
			default:
				return true;
			}
		}

		int32 GetStageOrder(const ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::Vertex:
				return 0;
            case ShaderStage::Hull:
				return 1;
            case ShaderStage::Domain:
				return 2;
            case ShaderStage::Geometry:
				return 3;
            case ShaderStage::Pixel:
				return 4;
            case ShaderStage::Compute:
				return 5;
			default:
				return -1;
			}
		}

		bool PrepareStageUsage(slang::IComponentType* linked, const List<ShaderStage>& stages, const SlangInt targetIndex, StageUsageContext& output, String& error)
		{
			if (linked == nullptr || targetIndex < 0 || stages.IsEmpty())
			{
				error = SE_TEXT("Slang linked component, target index, or stage list is invalid.");
				return false;
			}
			output = StageUsageContext();
			for (int32 stageIndex = 0; stageIndex < stages.Count(); stageIndex++)
			{
				if (GetStageOrder(stages[stageIndex]) < 0)
				{
					error = SE_TEXT("Slang stage usage contains an unsupported shader stage.");
					return false;
				}
				for (int32 previousIndex = 0; previousIndex < stageIndex; previousIndex++)
				{
					if (stages[previousIndex] == stages[stageIndex])
					{
						error = SE_TEXT("Slang stage usage contains a duplicate shader stage.");
						return false;
					}
				}
				Slang::ComPtr<slang::IMetadata> metadata;
				Slang::ComPtr<slang::IBlob> diagnostics;
				if (SLANG_FAILED(linked->getEntryPointMetadata(stageIndex, targetIndex, metadata.writeRef(), diagnostics.writeRef())) || metadata == nullptr)
				{
					error = String::Format(SE_TEXT("Failed to get Slang entry-point metadata for stage {0}."), ToString(stages[stageIndex]));
					return false;
				}
				output.Stages.Add(stages[stageIndex]);
				output.Metadata.Add(metadata);
			}
			return true;
		}

		bool BuildStageMask(const StageUsageContext& usage, const slang::ParameterCategory category, const uint32 set, const uint32 binding, String& output, String& error)
		{
			output = String::Empty;
			for (int32 stageOrder = 0; stageOrder < 6; stageOrder++)
			{
				int32 stageIndex = -1;
				for (int32 candidateIndex = 0; candidateIndex < usage.Stages.Count(); candidateIndex++)
				{
					if (GetStageOrder(usage.Stages[candidateIndex]) == stageOrder)
					{
						stageIndex = candidateIndex;
						break;
					}
				}
				if (stageIndex < 0)
				{
					continue;
				}
				bool used = false;
				if (usage.Metadata[stageIndex] == nullptr || SLANG_FAILED(usage.Metadata[stageIndex]->isParameterLocationUsed(static_cast<SlangParameterCategory>(category), set, binding, used)))
				{
					error = String::Format(SE_TEXT("Failed to query Slang parameter usage (category={0}, set={1}, binding={2})."), ToParameterCategoryString(category), set, binding);
					return false;
				}
				if (!used)
				{
					continue;
				}
				const String stageName = ToString(usage.Stages[stageIndex]);
				if (!output.IsEmpty())
				{
					output += SE_TEXT("|");
				}
				output += stageName;
			}
			return true;
		}

		int32 FindTypeEntry(const List<TypeBuildEntry>& entries, slang::TypeLayoutReflection* layout)
		{
			for (int32 i = 0; i < entries.Count(); i++)
			{
				if (entries[i].Layout == layout)
				{
					return i;
				}
			}
			return -1;
		}

		uint32 AddResourceRange(ShaderIRTypeRecord& record, const String& resourceKind, const uint32 count)
		{
			ShaderIRResourceRange range;
			range.ResourceKind = resourceKind;
			range.Count = count;
			range.BaseIndex = 0;
			for (int32 i = 0; i < record.ResourceRanges.Count(); i++)
			{
				range.BaseIndex += record.ResourceRanges[i].Count;
			}
			record.ResourceRanges.Add(range);
			return static_cast<uint32>(record.ResourceRanges.Count() - 1);
		}

		void RecomputeResourceRangeBases(ShaderIRTypeRecord& record)
		{
			uint32 baseIndex = 0;
			for (int32 rangeIndex = 0; rangeIndex < record.ResourceRanges.Count(); rangeIndex++)
			{
				record.ResourceRanges[rangeIndex].BaseIndex = baseIndex;
				baseIndex += record.ResourceRanges[rangeIndex].Count;
			}
		}

		void AppendResourceRanges(ShaderIRTypeRecord& destination, const ShaderIRTypeRecord& source, const uint32 multiplier)
		{
			for (int32 rangeIndex = 0; rangeIndex < source.ResourceRanges.Count(); rangeIndex++)
			{
				ShaderIRResourceRange range = source.ResourceRanges[rangeIndex];
				range.Count *= multiplier;
				destination.ResourceRanges.Add(range);
			}
			RecomputeResourceRangeBases(destination);
		}

		bool ReadBindingRangeCount(slang::TypeLayoutReflection* typeLayout, const uint32 rangeIndex, uint32& output, String& error)
		{
			const SlangInt rangeCount = typeLayout->getBindingRangeCount();
			if (rangeCount < 0 || rangeIndex >= static_cast<uint32>(rangeCount))
			{
				error = SE_TEXT("Slang logical resource references an invalid binding range.");
				return false;
			}
			const SlangInt count = typeLayout->getBindingRangeBindingCount(rangeIndex);
			if (count <= 0 || static_cast<uint64>(count) > Max_uint32)
			{
				error = SE_TEXT("Slang binding range has an invalid resource count.");
				return false;
			}
			output = static_cast<uint32>(count);
			return true;
		}

		bool ValidateLogicalBindingRanges(slang::TypeLayoutReflection* typeLayout,
			const ShaderIRTypeRecord& record,
			const List<LogicalBindingRangeMap>& logicalRanges,
			String& error)
		{
			const SlangInt rawRangeCount = typeLayout->getBindingRangeCount();
			if (rawRangeCount < 0 || rawRangeCount > Max_int32 ||
				logicalRanges.Count() != record.ResourceRanges.Count())
			{
				error = SE_TEXT("Slang binding ranges do not match the logical resource graph.");
				return false;
			}

			List<byte> ownedRanges;
			ownedRanges.Resize(static_cast<int32>(rawRangeCount));
			for (int32 logicalIndex = 0; logicalIndex < logicalRanges.Count(); logicalIndex++)
			{
				const LogicalBindingRangeMap& mapping = logicalRanges[logicalIndex];
				const ShaderIRResourceRange& logicalRange = record.ResourceRanges[logicalIndex];
				if (mapping.PrimaryRangeIndex >= static_cast<uint32>(rawRangeCount) ||
					ownedRanges[mapping.PrimaryRangeIndex] != 0)
				{
					error = SE_TEXT("A logical resource has an invalid or multiply-owned primary binding range.");
					return false;
				}
				uint32 primaryCount;
				if (!ReadBindingRangeCount(typeLayout, mapping.PrimaryRangeIndex, primaryCount, error) ||
					primaryCount != logicalRange.Count)
				{
					if (error.IsEmpty())
					{
						error = SE_TEXT("Primary binding range count does not match its logical resource range.");
					}
					return false;
				}
				ownedRanges[mapping.PrimaryRangeIndex] = 1;

				if (mapping.CounterRangeIndex >= 0)
				{
					const uint32 counterRangeIndex = static_cast<uint32>(mapping.CounterRangeIndex);
					if (counterRangeIndex >= static_cast<uint32>(rawRangeCount) ||
						counterRangeIndex == mapping.PrimaryRangeIndex || ownedRanges[counterRangeIndex] != 0)
					{
						error = SE_TEXT("A logical resource has an invalid or multiply-owned counter binding range.");
						return false;
					}
					uint32 counterCount;
					if (!ReadBindingRangeCount(typeLayout, counterRangeIndex, counterCount, error) ||
						counterCount != logicalRange.Count)
					{
						if (error.IsEmpty())
						{
							error = SE_TEXT("Counter binding range count does not match its parent logical resource range.");
						}
						return false;
					}
					ownedRanges[counterRangeIndex] = 1;
				}
			}

			for (int32 rangeIndex = 0; rangeIndex < ownedRanges.Count(); rangeIndex++)
			{
				if (ownedRanges[rangeIndex] == 0)
				{
					error = SE_TEXT("Slang binding range has no logical resource owner.");
					return false;
				}
			}
			return true;
		}

		bool BuildResourceRanges(slang::TypeLayoutReflection* typeLayout,
			ShaderIRTypeRecord& record,
			List<LogicalBindingRangeMap>& logicalRanges,
			String& error)
		{
			const SlangInt rawRangeCount = typeLayout->getBindingRangeCount();
			if (rawRangeCount <= 0 || rawRangeCount > Max_int32)
			{
				error = SE_TEXT("Shader resource type has no finite binding range.");
				return false;
			}

			LogicalBindingRangeMap mapping;
			slang::VariableLayoutReflection* explicitCounter = typeLayout->getExplicitCounter();
			if (explicitCounter != nullptr)
			{
				// Counter existence comes from the explicit counter layout. Its numeric offset
				// is used only after that semantic relationship has been established.
				const SlangInt counterRangeIndex = typeLayout->getExplicitCounterBindingRangeOffset();
				if (typeLayout->getResourceShape() != SLANG_STRUCTURED_BUFFER || rawRangeCount != 2 ||
					counterRangeIndex < 0 || counterRangeIndex >= rawRangeCount)
				{
					error = SE_TEXT("Structured buffer counter reflection has an invalid binding range shape.");
					return false;
				}
				mapping.CounterRangeIndex = static_cast<int32>(counterRangeIndex);
				mapping.PrimaryRangeIndex = counterRangeIndex == 0 ? 1u : 0u;
			}
			else
			{
				if (rawRangeCount != 1)
				{
					error = SE_TEXT("Shader resource has multiple binding ranges without an explicit counter owner.");
					return false;
				}
				mapping.PrimaryRangeIndex = 0;
			}

			uint32 resourceCount;
			if (!ReadBindingRangeCount(typeLayout, mapping.PrimaryRangeIndex, resourceCount, error))
			{
				return false;
			}
			AddResourceRange(record, record.ResourceKind, resourceCount);
			logicalRanges.Add(mapping);
			return true;
		}

		uint32 BuildType(slang::TypeLayoutReflection* typeLayout, List<TypeBuildEntry>& entries, ShaderReflectionIR& output, String& error)
		{
			if (typeLayout == nullptr)
			{
				error = SE_TEXT("Slang type layout is null.");
				return Max_uint32;
			}

			const int32 existingIndex = FindTypeEntry(entries, typeLayout);
			if (existingIndex != -1)
			{
				return entries[existingIndex].TypeId;
			}

			// TypeId 按首次发现顺序稳定分配；递归类型先占位，后续再回填完整记录。
			const slang::TypeReflection::Kind slangKind = typeLayout->getKind();
			if (!IsSupportedTypeKind(slangKind))
			{
				error = SE_TEXT("Unsupported Slang type kind in reflection builder: ") + ToTypeKind(slangKind);
				return Max_uint32;
			}

			TypeBuildEntry entry;
			entry.Layout = typeLayout;
			entry.TypeId = static_cast<uint32>(output.Types.Count());
			entries.Add(entry);
			const int32 entryIndex = entries.Count() - 1;

			ShaderIRTypeRecord record;
			record.Id = entry.TypeId;
			record.Kind = ToTypeKind(slangKind);
			record.Name = String(typeLayout->getName());
			record.UniformByteSize = static_cast<uint32>(typeLayout->getSize(slang::ParameterCategory::Uniform));
			output.Types.Add(record);
			List<LogicalBindingRangeMap> logicalRanges;

			switch (slangKind)
			{
			case slang::TypeReflection::Kind::Struct:
				for (unsigned int fieldIndex = 0; fieldIndex < typeLayout->getFieldCount(); fieldIndex++)
				{
					slang::VariableLayoutReflection* field = typeLayout->getFieldByIndex(fieldIndex);
					if (field == nullptr || field->getTypeLayout() == nullptr)
					{
						error = SE_TEXT("Slang struct field layout is null.");
						return Max_uint32;
					}

					ShaderIRMember member;
					member.Name = String(field->getName());
					member.TypeId = BuildType(field->getTypeLayout(), entries, output, error);
					if (member.TypeId == Max_uint32)
					{
						return Max_uint32;
					}
					// Slang 返回的字段 UniformOffset 是相对直接父结构的偏移，不写成根块绝对偏移。
					member.UniformOffset = static_cast<uint32>(field->getOffset(slang::ParameterCategory::Uniform));
					member.ResourceRangeOffset = static_cast<uint32>(record.ResourceRanges.Count());
					member.ResourceIndexOffset = 0;
					record.Members.Add(member);
					AppendResourceRanges(record, output.Types[member.TypeId], 1);

					const int32 childEntryIndex = FindTypeEntry(entries, field->getTypeLayout());
					if (childEntryIndex < 0)
					{
						error = SE_TEXT("Slang struct field has no completed logical binding map.");
						return Max_uint32;
					}
					const TypeBuildEntry& childEntry = entries[childEntryIndex];
					if (childEntry.LogicalRanges.HasItems())
					{
						const SlangInt fieldRangeOffset = typeLayout->getFieldBindingRangeOffset(fieldIndex);
						if (fieldRangeOffset < 0 || static_cast<uint64>(fieldRangeOffset) > Max_uint32)
						{
							error = SE_TEXT("Slang struct field has an invalid binding range offset.");
							return Max_uint32;
						}
						for (int32 rangeIndex = 0; rangeIndex < childEntry.LogicalRanges.Count(); rangeIndex++)
						{
							LogicalBindingRangeMap mapping = childEntry.LogicalRanges[rangeIndex];
							const uint64 primaryRangeIndex = static_cast<uint64>(fieldRangeOffset) + mapping.PrimaryRangeIndex;
							const int64 counterRangeIndex = mapping.CounterRangeIndex >= 0
								? static_cast<int64>(fieldRangeOffset) + mapping.CounterRangeIndex
								: -1;
							if (primaryRangeIndex > Max_uint32 || counterRangeIndex > Max_int32)
							{
								error = SE_TEXT("Slang struct field binding range mapping overflows.");
								return Max_uint32;
							}
							mapping.PrimaryRangeIndex = static_cast<uint32>(primaryRangeIndex);
							mapping.CounterRangeIndex = static_cast<int32>(counterRangeIndex);
							logicalRanges.Add(mapping);
						}
					}
				}
				break;
			case slang::TypeReflection::Kind::Array:
				record.ElementTypeId = BuildType(typeLayout->getElementTypeLayout(), entries, output, error);
				if (record.ElementTypeId == Max_uint32)
				{
					return Max_uint32;
				}
				record.ElementCount = static_cast<uint32>(typeLayout->getElementCount());
                record.ElementByteStride = static_cast<uint32>(typeLayout->getElementStride(SlangParameterCategory::SLANG_PARAMETER_CATEGORY_UNIFORM));
				AppendResourceRanges(record, output.Types[record.ElementTypeId], record.ElementCount);
				{
					const int32 elementEntryIndex = FindTypeEntry(entries, typeLayout->getElementTypeLayout());
					if (elementEntryIndex < 0)
					{
						error = SE_TEXT("Slang array element has no completed logical binding map.");
						return Max_uint32;
					}
					logicalRanges = entries[elementEntryIndex].LogicalRanges;
				}
				break;
			case slang::TypeReflection::Kind::Resource:
			case slang::TypeReflection::Kind::SamplerState:
			case slang::TypeReflection::Kind::TextureBuffer:
			case slang::TypeReflection::Kind::ShaderStorageBuffer:
				// 单个资源类型先形成逻辑 ResourceRange，后续由 ParameterBlock 的 descriptor 范围映射到物理绑定。
				record.ResourceKind = ToResourceKindString(slangKind);
				record.Shape = ToResourceShapeString(typeLayout->getResourceShape());
				record.Access = ToResourceAccessString(typeLayout->getResourceAccess());
				if (!BuildResourceRanges(typeLayout, record, logicalRanges, error))
				{
					return Max_uint32;
				}
				break;
			case slang::TypeReflection::Kind::ConstantBuffer:
			case slang::TypeReflection::Kind::ParameterBlock:
				record.ElementTypeId = BuildType(typeLayout->getElementTypeLayout(), entries, output, error);
				if (record.ElementTypeId == Max_uint32)
				{
					return Max_uint32;
				}
				record.ResourceKind = record.Kind;
				{
					uint32 blockCount;
					if (typeLayout->getBindingRangeCount() != 1 ||
						!ReadBindingRangeCount(typeLayout, 0, blockCount, error) || blockCount != 1)
					{
						if (error.IsEmpty())
						{
							error = SE_TEXT("ConstantBuffer or ParameterBlock type must have one binding range.");
						}
						return Max_uint32;
					}
					AddResourceRange(record, record.Kind, blockCount);
					LogicalBindingRangeMap mapping;
					mapping.PrimaryRangeIndex = 0;
					logicalRanges.Add(mapping);
				}
				break;
			default:
				break;
			}

			if (!ValidateLogicalBindingRanges(typeLayout, record, logicalRanges, error))
			{
				return Max_uint32;
			}
			output.Types[entry.TypeId] = record;
			entries[entryIndex].LogicalRanges = logicalRanges;
			return entry.TypeId;
		}

		bool ValidateEntryPointUniformParameters(slang::ProgramLayout* layout, String& error)
		{
			// 当前设计不支持 EntryPoint uniform/resource 参数；入口只允许 varying 输入输出。
			for (SlangUInt entryPointIndex = 0; entryPointIndex < layout->getEntryPointCount(); entryPointIndex++)
			{
				slang::EntryPointReflection* entryPoint = layout->getEntryPointByIndex(entryPointIndex);
				if (entryPoint == nullptr)
				{
					error = SE_TEXT("Slang entry point reflection is null.");
					return false;
				}

				for (unsigned int parameterIndex = 0; parameterIndex < entryPoint->getParameterCount(); parameterIndex++)
				{
					slang::VariableLayoutReflection* parameter = entryPoint->getParameterByIndex(parameterIndex);
					if (parameter == nullptr)
					{
						error = SE_TEXT("Slang entry point parameter layout is null.");
						return false;
					}

					for (unsigned int categoryIndex = 0; categoryIndex < parameter->getCategoryCount(); categoryIndex++)
					{
						const slang::ParameterCategory category = parameter->getCategoryByIndex(categoryIndex);
						if (IsUniformLikeEntryPointCategory(category))
						{
							error = SE_TEXT("EntryPoint uniform/resource parameters are not supported: ") + String(entryPoint->getName());
							return false;
						}
					}
				}
			}
			return true;
		}

		bool ValidateNoPushConstant(slang::ProgramLayout* layout, String& error)
		{
			// 当前只拒绝 PushConstant 分类，不在这里额外限制其它合法的 uniform/resource 布局。
			for (unsigned int parameterIndex = 0; parameterIndex < layout->getParameterCount(); parameterIndex++)
			{
				slang::VariableLayoutReflection* parameter = layout->getParameterByIndex(parameterIndex);
				if (parameter == nullptr)
				{
					continue;
				}

				for (unsigned int categoryIndex = 0; categoryIndex < parameter->getCategoryCount(); categoryIndex++)
				{
					if (parameter->getCategoryByIndex(categoryIndex) == slang::ParameterCategory::PushConstantBuffer)
					{
						error = SE_TEXT("Push Constant is not supported by SolarEngine Slang reflection builder.");
						return false;
					}
				}
			}
			return true;
		}

		bool ReadFiniteUint32(const SlangInt value, uint32& output, String& error)
		{
			if (value < 0 || static_cast<uint64>(value) > Max_uint32)
			{
				error = SE_TEXT("Slang reflection returned an unknown, negative, or oversized binding value.");
				return false;
			}
			output = static_cast<uint32>(value);
			return true;
		}

		bool AddFiniteOffset(const size_t value, uint64& accumulator, String& error)
		{
			if (value >= SLANG_UNBOUNDED_SIZE - 1 || static_cast<uint64>(value) > Max_uint64 - accumulator)
			{
				error = SE_TEXT("Slang reflection returned an unknown, unbounded, or overflowing offset.");
				return false;
			}
			accumulator += static_cast<uint64>(value);
			return true;
		}

		bool IsNodeInPath(const BindingPathNode* target, const BindingPathNode* leaf)
		{
			if (target == nullptr)
			{
				return true;
			}
			for (const BindingPathNode* node = leaf; node != nullptr; node = node->Outer)
			{
				if (node == target)
				{
					return true;
				}
			}
			return false;
		}

		bool AppendBindingPath(const BindingPath& base, slang::VariableLayoutReflection* variableLayout, BindingPathNodeArena& arena, BindingPath& output, String& error)
		{
			if (variableLayout == nullptr)
			{
				error = SE_TEXT("Slang binding path variable layout is null.");
				return false;
			}
			BindingPathNode node;
			node.VariableLayout = variableLayout;
			node.Outer = base.Leaf;
			output = base;
			output.Leaf = arena.Add(node);
			return true;
		}

		bool ResolveCategory(const BindingPath& path, const slang::ParameterCategory category, ResolvedCategoryOffset& output, String& error)
		{
			if (!IsNodeInPath(path.DeepestConstantBuffer, path.Leaf) || !IsNodeInPath(path.DeepestParameterBlock, path.Leaf))
			{
				error = SE_TEXT("Slang binding path has a container boundary outside its node chain.");
				return false;
			}

			output = ResolvedCategoryOffset();
			if (category == slang::ParameterCategory::Uniform)
			{
				for (const BindingPathNode* node = path.Leaf; node != path.DeepestConstantBuffer; node = node->Outer)
				{
					if (node == nullptr || !AddFiniteOffset(node->VariableLayout->getOffset(category), output.Value, error))
					{
						if (error.IsEmpty())
						{
							error = SE_TEXT("Slang Uniform binding path does not reach its ConstantBuffer boundary.");
						}
						return false;
					}
				}
				return true;
			}

			const bool spaceAware = category == slang::ParameterCategory::ConstantBuffer ||
				category == slang::ParameterCategory::ShaderResource ||
				category == slang::ParameterCategory::UnorderedAccess ||
				category == slang::ParameterCategory::SamplerState ||
				category == slang::ParameterCategory::DescriptorTableSlot;
			if (!spaceAware)
			{
				error = SE_TEXT("Slang descriptor uses an unsupported parameter category.");
				return false;
			}

			// ParameterBlock 内的 binding 是块局部值；set 则继续累计所有外层
			// ParameterBlock occurrence 的 SubElementRegisterSpace offset。
			for (const BindingPathNode* node = path.Leaf; node != path.DeepestParameterBlock; node = node->Outer)
			{
				if (node == nullptr || !AddFiniteOffset(node->VariableLayout->getOffset(category), output.Value, error) || !AddFiniteOffset(node->VariableLayout->getBindingSpace(category), output.Space, error))
				{
					if (error.IsEmpty())
					{
						error = SE_TEXT("Slang descriptor binding path does not reach its ParameterBlock boundary.");
					}
					return false;
				}
			}
			for (const BindingPathNode* node = path.DeepestParameterBlock; node != nullptr; node = node->Outer)
			{
				if (!AddFiniteOffset(node->VariableLayout->getOffset(slang::ParameterCategory::SubElementRegisterSpace), output.Space, error))
				{
					return false;
				}
			}
			return true;
		}

		bool EnterSingleElementContainer(const BindingPath& occurrencePath, slang::TypeLayoutReflection* containerTypeLayout, BindingPathNodeArena& arena, BlockLayoutContext& output, String& error)
		{
			if (containerTypeLayout == nullptr || occurrencePath.Leaf == nullptr ||
				(containerTypeLayout->getKind() != slang::TypeReflection::Kind::ConstantBuffer && containerTypeLayout->getKind() != slang::TypeReflection::Kind::ParameterBlock))
			{
				error = SE_TEXT("Slang single-element container context is invalid.");
				return false;
			}
			slang::VariableLayoutReflection* containerVarLayout = containerTypeLayout->getContainerVarLayout();
			slang::VariableLayoutReflection* elementVarLayout = containerTypeLayout->getElementVarLayout();
			if (containerVarLayout == nullptr || containerVarLayout->getTypeLayout() == nullptr || elementVarLayout == nullptr || elementVarLayout->getTypeLayout() == nullptr)
			{
				error = SE_TEXT("Slang single-element container has no container or element layout.");
				return false;
			}

			BindingPath innerPath = occurrencePath;
			innerPath.DeepestConstantBuffer = occurrencePath.Leaf;
			const size_t subSpaceSize = containerVarLayout->getTypeLayout()->getSize(slang::ParameterCategory::SubElementRegisterSpace);
			if (subSpaceSize >= SLANG_UNBOUNDED_SIZE - 1 || subSpaceSize > Max_uint32)
			{
				error = SE_TEXT("Slang single-element container has an invalid descriptor-set size.");
				return false;
			}
			if (subSpaceSize != 0)
			{
				innerPath.DeepestParameterBlock = occurrencePath.Leaf;
			}

			output = BlockLayoutContext();
			output.ContainerTypeLayout = containerTypeLayout;
			output.ElementTypeLayout = elementVarLayout->getTypeLayout();
			output.HasContainerPath = true;
			if (!AppendBindingPath(innerPath, containerVarLayout, arena, output.ContainerPath, error) || !AppendBindingPath(innerPath, elementVarLayout, arena, output.ElementPath, error))
			{
				return false;
			}
			return true;
		}

		bool BuildDescriptorRange(slang::TypeLayoutReflection* typeLayout, const BindingPath& ownerPath, const SlangInt descriptorSetIndex, const SlangInt descriptorRangeIndex, const StringAnsi& role, const StageUsageContext& stageUsage, ShaderIRDescriptorRange& output, String& error)
		{
			uint32 relativeSet;
			uint32 relativeBinding;
			uint32 count;
			if (!ReadFiniteUint32(typeLayout->getDescriptorSetSpaceOffset(descriptorSetIndex), relativeSet, error) || !ReadFiniteUint32(typeLayout->getDescriptorSetDescriptorRangeIndexOffset(descriptorSetIndex, descriptorRangeIndex), relativeBinding, error) || !ReadFiniteUint32(typeLayout->getDescriptorSetDescriptorRangeDescriptorCount(descriptorSetIndex, descriptorRangeIndex), count, error) || count == 0)
			{
				if (error.IsEmpty())
				{
					error = SE_TEXT("Slang descriptor range has zero count.");
				}
				return false;
			}
			const slang::ParameterCategory category = typeLayout->getDescriptorSetDescriptorRangeCategory(descriptorSetIndex, descriptorRangeIndex);
			ResolvedCategoryOffset base;
			if (!ResolveCategory(ownerPath, category, base, error) || base.Space + relativeSet > Max_uint32 || base.Value + relativeBinding > Max_uint32)
			{
				if (error.IsEmpty())
				{
					error = SE_TEXT("Slang descriptor location is oversized.");
				}
				return false;
			}
			output.Role = role;
			output.Set = static_cast<uint32>(base.Space + relativeSet);
			output.Binding = static_cast<uint32>(base.Value + relativeBinding);
			output.ArrayElementBase = 0;
			output.LogicalElementStride = 1;
			output.DescriptorCount = count;
			output.DescriptorType = ToBindingTypeString(typeLayout->getDescriptorSetDescriptorRangeType(descriptorSetIndex, descriptorRangeIndex));
			if (!BuildStageMask(stageUsage, category, output.Set, output.Binding, output.StageMask, error))
			{
				return false;
			}
			if (output.DescriptorType == SE_TEXT("Unknown"))
			{
				error = String::Format(SE_TEXT("Slang descriptor range uses an unsupported binding type (kind={0}, set={1}, range={2}, type={3}, category={4})."),
					ToTypeKind(typeLayout->getKind()), descriptorSetIndex, descriptorRangeIndex,
					static_cast<int32>(typeLayout->getDescriptorSetDescriptorRangeType(descriptorSetIndex, descriptorRangeIndex)),
					ToParameterCategoryString(category));
				return false;
			}
			return true;
		}

		bool HasVariableCategory(slang::VariableLayoutReflection* variableLayout, const slang::ParameterCategory category)
		{
			if (variableLayout == nullptr)
			{
				return false;
			}
			for (unsigned int categoryIndex = 0; categoryIndex < variableLayout->getCategoryCount(); categoryIndex++)
			{
				if (variableLayout->getCategoryByIndex(categoryIndex) == category)
				{
					return true;
				}
			}
			return false;
		}

		bool BuildDefaultUniformBuffer(const BlockLayoutContext& context, const uint32 uniformByteSize, const StageUsageContext& stageUsage, ShaderIRParameterBlock& block, String& error)
		{
			if (uniformByteSize == 0)
			{
				return true;
			}
			if (context.ContainerTypeLayout == nullptr || !context.HasContainerPath || context.ContainerPath.Leaf == nullptr)
			{
				error = SE_TEXT("Shader parameter block has uniform data but no descriptor-backed container path.");
				return false;
			}

			// PB/CB 的 ordinary data 都由容器 VariableLayout 定位。PB 的隐式 UBO
			// 不属于元素 TypeLayout 的普通 descriptor range，不能从元素 range 猜测。
			slang::VariableLayoutReflection* containerLayout = context.ContainerPath.Leaf->VariableLayout;
			slang::ParameterCategory category = slang::ParameterCategory::DescriptorTableSlot;
			if (!HasVariableCategory(containerLayout, category))
			{
				category = slang::ParameterCategory::ConstantBuffer;
				if (!HasVariableCategory(containerLayout, category))
				{
					error = SE_TEXT("Shader parameter block uniform data has no container descriptor category.");
					return false;
				}
			}
			ResolvedCategoryOffset descriptorLocation;
			if (!ResolveCategory(context.ContainerPath, category, descriptorLocation, error) || descriptorLocation.Space > Max_uint32 || descriptorLocation.Value > Max_uint32)
			{
				if (error.IsEmpty())
				{
					error = SE_TEXT("Shader parameter block uniform descriptor location is oversized.");
				}
				return false;
			}

			block.DefaultUniformBuffer.Role = SE_TEXT("uniformData");
			block.DefaultUniformBuffer.Set = static_cast<uint32>(descriptorLocation.Space);
			block.DefaultUniformBuffer.Binding = static_cast<uint32>(descriptorLocation.Value);
			block.DefaultUniformBuffer.ArrayElementBase = 0;
			block.DefaultUniformBuffer.LogicalElementStride = 1;
			block.DefaultUniformBuffer.DescriptorCount = 1;
			block.DefaultUniformBuffer.DescriptorType = SE_TEXT("ConstantBuffer");
			if (!BuildStageMask(stageUsage, category, block.DefaultUniformBuffer.Set, block.DefaultUniformBuffer.Binding, block.DefaultUniformBuffer.StageMask, error))
			{
				return false;
			}
			block.HasDefaultUniformBuffer = true;
			return true;
		}

		bool BuildSimpleDescriptor(slang::TypeLayoutReflection* elementTypeLayout,
			const BindingPath& elementPath,
			const uint32 rawRangeIndex,
			const StringAnsi& role,
			const uint32 logicalCount,
			const StageUsageContext& stageUsage,
			ShaderIRDescriptorRange& descriptor,
			String& error)
		{
			const SlangInt descriptorSetIndex = elementTypeLayout->getBindingRangeDescriptorSetIndex(rawRangeIndex);
			const SlangInt firstDescriptorRangeIndex =
				elementTypeLayout->getBindingRangeFirstDescriptorRangeIndex(rawRangeIndex);
			const SlangInt descriptorRangeCount =
				elementTypeLayout->getBindingRangeDescriptorRangeCount(rawRangeIndex);
			if (descriptorSetIndex < 0 || descriptorSetIndex >= elementTypeLayout->getDescriptorSetCount() ||
				firstDescriptorRangeIndex < 0 || descriptorRangeCount != 1 ||
				firstDescriptorRangeIndex >= elementTypeLayout->getDescriptorSetDescriptorRangeCount(descriptorSetIndex))
			{
				error = SE_TEXT("Simple binding range must map to exactly one physical descriptor range.");
				return false;
			}
			if (!BuildDescriptorRange(elementTypeLayout, elementPath, descriptorSetIndex,
				firstDescriptorRangeIndex, role, stageUsage, descriptor, error))
			{
				return false;
			}
			if (descriptor.DescriptorCount != logicalCount)
			{
				error = SE_TEXT("Descriptor count does not match the logical resource range count.");
				return false;
			}
			return true;
		}

		bool BuildParameterBlock(const BlockLayoutContext& context, BindingPathNodeArena& pathArena, const String& name, const StageUsageContext& stageUsage, List<TypeBuildEntry>& typeEntries, ShaderReflectionIR& output, uint32& blockId, String& error)
		{
			slang::TypeLayoutReflection* elementTypeLayout = context.ElementTypeLayout;
			if (elementTypeLayout == nullptr)
			{
				error = SE_TEXT("Shader parameter block element layout is null.");
				return false;
			}
			const uint32 elementTypeId = BuildType(elementTypeLayout, typeEntries, output, error);
			if (elementTypeId == Max_uint32)
			{
				return false;
			}
			const SlangInt bindingRangeCount = elementTypeLayout->getBindingRangeCount();
			const int32 elementEntryIndex = FindTypeEntry(typeEntries, elementTypeLayout);
			if (bindingRangeCount < 0 || bindingRangeCount > Max_int32 || elementEntryIndex < 0 ||
				typeEntries[elementEntryIndex].LogicalRanges.Count() != output.Types[elementTypeId].ResourceRanges.Count())
			{
				error = SE_TEXT("Slang binding ranges do not match the logical resource range graph.");
				return false;
			}
			// Copy the builder-only map because recursive block construction may grow the type-entry array.
			const List<LogicalBindingRangeMap> logicalMappings = typeEntries[elementEntryIndex].LogicalRanges;

			ShaderIRParameterBlock block;
			block.Id = static_cast<uint32>(output.ParameterBlocks.Count());
			block.Name = name;
			block.ElementTypeId = elementTypeId;
			block.UniformByteSize = static_cast<uint32>(elementTypeLayout->getSize(slang::ParameterCategory::Uniform));
			if (!BuildDefaultUniformBuffer(context, block.UniformByteSize, stageUsage, block, error))
			{
				return false;
			}
			output.ParameterBlocks.Add(block);
			blockId = block.Id;

			List<int32> subObjectByBindingRange;
			subObjectByBindingRange.Resize(static_cast<int32>(bindingRangeCount));
			for (int32 rangeIndex = 0; rangeIndex < subObjectByBindingRange.Count(); rangeIndex++)
			{
				subObjectByBindingRange[rangeIndex] = -1;
			}
			for (SlangInt subObjectRangeIndex = 0; subObjectRangeIndex < elementTypeLayout->getSubObjectRangeCount(); subObjectRangeIndex++)
			{
				const SlangInt bindingRangeIndex = elementTypeLayout->getSubObjectRangeBindingRangeIndex(subObjectRangeIndex);
				if (bindingRangeIndex < 0 || bindingRangeIndex >= bindingRangeCount || subObjectByBindingRange[static_cast<int32>(bindingRangeIndex)] >= 0 || elementTypeLayout->getSubObjectRangeOffset(subObjectRangeIndex) == nullptr || elementTypeLayout->getSubObjectRangeSpaceOffset(subObjectRangeIndex) < 0)
				{
					error = SE_TEXT("Slang sub-object range mapping is invalid.");
					return false;
				}
				subObjectByBindingRange[static_cast<int32>(bindingRangeIndex)] = static_cast<int32>(subObjectRangeIndex);
			}

			for (int32 logicalRangeIndex = 0; logicalRangeIndex < logicalMappings.Count(); logicalRangeIndex++)
			{
				const LogicalBindingRangeMap& mapping = logicalMappings[logicalRangeIndex];
				const uint32 primaryRangeIndex = mapping.PrimaryRangeIndex;
				const slang::BindingType bindingType = elementTypeLayout->getBindingRangeType(primaryRangeIndex);
				const bool isBlock = bindingType == slang::BindingType::ConstantBuffer || bindingType == slang::BindingType::ParameterBlock;
				ShaderIRRangeBinding binding;
				binding.RangeIndex = static_cast<uint32>(logicalRangeIndex);
				if (isBlock)
				{
					uint32 blockCount;
					const int32 subObjectRangeIndex = subObjectByBindingRange[primaryRangeIndex];
					slang::TypeLayoutReflection* childContainerTypeLayout = elementTypeLayout->getBindingRangeLeafTypeLayout(primaryRangeIndex);
					slang::VariableLayoutReflection* childElementOffsetLayout = childContainerTypeLayout != nullptr ? childContainerTypeLayout->getElementVarLayout() : nullptr;
					if (mapping.CounterRangeIndex >= 0 ||
						!ReadFiniteUint32(elementTypeLayout->getBindingRangeBindingCount(primaryRangeIndex), blockCount, error) || blockCount != 1 || subObjectRangeIndex < 0 || childContainerTypeLayout == nullptr || childElementOffsetLayout == nullptr || childElementOffsetLayout->getTypeLayout() == nullptr)
					{
						error = SE_TEXT("ConstantBuffer or ParameterBlock binding range must have one sub-object instance.");
						return false;
					}

					slang::VariableLayoutReflection* subObjectOffsetLayout = elementTypeLayout->getSubObjectRangeOffset(subObjectRangeIndex);
					uint32 ignoredSpaceOffset;
					if (!ReadFiniteUint32(elementTypeLayout->getSubObjectRangeSpaceOffset(subObjectRangeIndex), ignoredSpaceOffset, error))
					{
						return false;
					}
					BindingPath childOccurrencePath;
					if (!AppendBindingPath(context.ElementPath, subObjectOffsetLayout, pathArena, childOccurrencePath, error))
					{
						return false;
					}
					BlockLayoutContext childContext;
					if (!EnterSingleElementContainer(childOccurrencePath, childContainerTypeLayout, pathArena, childContext, error))
					{
						return false;
					}
					uint32 subBlockId;
					slang::VariableReflection* leafVariable = elementTypeLayout->getBindingRangeLeafVariable(primaryRangeIndex);
					const String childName = leafVariable != nullptr ? String(leafVariable->getName()) : String::Format(SE_TEXT("$block{0}"), logicalRangeIndex);
					if (!BuildParameterBlock(childContext, pathArena, childName, stageUsage, typeEntries, output, subBlockId, error))
					{
						return false;
					}
					binding.Flavor = bindingType == slang::BindingType::ConstantBuffer ? "constantBuffer" : "parameterBlock";
					binding.SubBlockId = static_cast<int32>(subBlockId);
				}
				else
				{
					const ShaderIRResourceRange& logicalRange =
						output.Types[elementTypeId].ResourceRanges[logicalRangeIndex];
					ShaderIRDescriptorRange primaryDescriptor;
					if (!BuildSimpleDescriptor(elementTypeLayout, context.ElementPath, primaryRangeIndex, "primary", logicalRange.Count, stageUsage, primaryDescriptor, error))
					{
						return false;
					}
					binding.Flavor = SE_TEXT("simple");
					binding.DescriptorRanges.Add(primaryDescriptor);

					if (mapping.CounterRangeIndex >= 0)
					{
						ShaderIRDescriptorRange counterDescriptor;
						if (!BuildSimpleDescriptor(elementTypeLayout, context.ElementPath,
							static_cast<uint32>(mapping.CounterRangeIndex), "counter",
							logicalRange.Count, stageUsage, counterDescriptor, error))
						{
							return false;
						}
						if (primaryDescriptor.StageMask != counterDescriptor.StageMask)
						{
							error = SE_TEXT("Primary and counter descriptors must have identical stage usage.");
							return false;
						}
						if (counterDescriptor.DescriptorType != SE_TEXT("RawBuffer") &&
							counterDescriptor.DescriptorType != SE_TEXT("StorageBuffer"))
						{
							error = SE_TEXT("Structured buffer counter must use a storage-buffer descriptor.");
							return false;
						}
						binding.DescriptorRanges.Add(counterDescriptor);
					}
				}
				output.ParameterBlocks[blockId].RangeBindings.Add(binding);
			}
			return true;
		}

		bool HasActiveRange(const List<byte>& activity)
		{
			for (int32 index = 0; index < activity.Count(); index++)
			{
				if (activity[index] != 0)
				{
					return true;
				}
			}
			return false;
		}

		bool ClonePrunedType(const ShaderReflectionIR& source, const uint32 sourceTypeId, const List<byte>& activity, const List<uint32>& subBlockElementTypes, const bool keepUniformData, const bool forceRecord, ShaderReflectionIR& output, uint32& outputTypeId, bool& hasContent, String& error)
		{
			if (sourceTypeId >= static_cast<uint32>(source.Types.Count()))
			{
				error = SE_TEXT("Pruned type references an unknown source type.");
				return false;
			}
			const ShaderIRTypeRecord& sourceRecord = source.Types[sourceTypeId];
			if (activity.Count() != sourceRecord.ResourceRanges.Count() || subBlockElementTypes.Count() != sourceRecord.ResourceRanges.Count())
			{
				error = SE_TEXT("Pruned type activity does not match the source resource range graph.");
				return false;
			}

			ShaderIRTypeRecord record;
			record.Kind = sourceRecord.Kind;
			record.Name = sourceRecord.Name;
			record.UniformByteSize = keepUniformData ? sourceRecord.UniformByteSize : 0;
			record.ElementCount = sourceRecord.ElementCount;
			record.ElementByteStride = sourceRecord.ElementByteStride;
			record.ResourceKind = sourceRecord.ResourceKind;
			record.Shape = sourceRecord.Shape;
			record.Access = sourceRecord.Access;

			if (sourceRecord.Kind == SE_TEXT("Struct"))
			{
				for (int32 memberIndex = 0; memberIndex < sourceRecord.Members.Count(); memberIndex++)
				{
					const ShaderIRMember& sourceMember = sourceRecord.Members[memberIndex];
					if (sourceMember.TypeId >= static_cast<uint32>(source.Types.Count()))
					{
						error = SE_TEXT("Pruned struct member references an unknown source type.");
						return false;
					}
					const ShaderIRTypeRecord& sourceMemberType = source.Types[sourceMember.TypeId];
					if (sourceMember.ResourceRangeOffset > static_cast<uint32>(activity.Count()) || sourceMemberType.ResourceRanges.Count() > activity.Count() - static_cast<int32>(sourceMember.ResourceRangeOffset))
					{
						error = SE_TEXT("Pruned struct member resource range offset is invalid.");
						return false;
					}
					List<byte> memberActivity;
					List<uint32> memberSubBlockElementTypes;
					memberActivity.Resize(sourceMemberType.ResourceRanges.Count());
					memberSubBlockElementTypes.Resize(sourceMemberType.ResourceRanges.Count());
					for (int32 rangeIndex = 0; rangeIndex < memberActivity.Count(); rangeIndex++)
					{
						memberActivity[rangeIndex] = activity[static_cast<int32>(sourceMember.ResourceRangeOffset) + rangeIndex];
						memberSubBlockElementTypes[rangeIndex] = subBlockElementTypes[static_cast<int32>(sourceMember.ResourceRangeOffset) + rangeIndex];
					}
					uint32 memberTypeId;
					bool memberHasContent;
					if (!ClonePrunedType(source, sourceMember.TypeId, memberActivity, memberSubBlockElementTypes, keepUniformData, false, output, memberTypeId, memberHasContent, error))
					{
						return false;
					}
					if (!memberHasContent)
					{
						continue;
					}
					ShaderIRMember member = sourceMember;
					member.TypeId = memberTypeId;
					member.ResourceRangeOffset = static_cast<uint32>(record.ResourceRanges.Count());
					record.Members.Add(member);
					AppendResourceRanges(record, output.Types[memberTypeId], 1);
				}
			}
			else if (sourceRecord.Kind == SE_TEXT("Array"))
			{
				if (sourceRecord.ElementTypeId >= static_cast<uint32>(source.Types.Count()) || sourceRecord.ElementCount == 0)
				{
					error = SE_TEXT("Pruned array type is invalid.");
					return false;
				}
				const ShaderIRTypeRecord& sourceElementType = source.Types[sourceRecord.ElementTypeId];
				const int32 elementRangeCount = sourceElementType.ResourceRanges.Count();
				if (activity.Count() != elementRangeCount || subBlockElementTypes.Count() != elementRangeCount)
				{
					error = SE_TEXT("Pruned array activity does not match the source element range graph.");
					return false;
				}
				// Activity is tracked per logical resource range; the range count itself carries the array length.
				uint32 elementTypeId;
				bool elementHasContent;
				if (!ClonePrunedType(
					source, sourceRecord.ElementTypeId, activity, subBlockElementTypes, keepUniformData, false,
					output, elementTypeId, elementHasContent, error))
				{
					return false;
				}
				if (elementHasContent)
				{
					record.ElementTypeId = elementTypeId;
					AppendResourceRanges(record, output.Types[elementTypeId], sourceRecord.ElementCount);
				}
			}
			else if (sourceRecord.Kind == SE_TEXT("ParameterBlock"))
			{
				if (sourceRecord.ResourceRanges.Count() != 1)
				{
					error = SE_TEXT("Pruned parameter block type does not have one logical range.");
					return false;
				}
				if (activity[0] != 0)
				{
					if (subBlockElementTypes[0] == Max_uint32)
					{
						error = SE_TEXT("Pruned parameter block type does not have an active child block.");
						return false;
					}
					record.ElementTypeId = subBlockElementTypes[0];
					AddResourceRange(record, sourceRecord.ResourceRanges[0].ResourceKind, sourceRecord.ResourceRanges[0].Count);
				}
			}
			else
			{
				List<int32> rangeRemap;
				rangeRemap.Resize(sourceRecord.ResourceRanges.Count());
				for (int32 rangeIndex = 0; rangeIndex < rangeRemap.Count(); rangeIndex++)
				{
					rangeRemap[rangeIndex] = -1;
				}
				for (int32 rangeIndex = 0; rangeIndex < sourceRecord.ResourceRanges.Count(); rangeIndex++)
				{
					if (activity[rangeIndex] != 0)
					{
						const ShaderIRResourceRange& sourceRange = sourceRecord.ResourceRanges[rangeIndex];
						rangeRemap[rangeIndex] = record.ResourceRanges.Count();
						record.ResourceRanges.Add(sourceRange);
					}
				}
				RecomputeResourceRangeBases(record);
			}

			hasContent = record.UniformByteSize != 0 || record.ResourceRanges.Count() != 0 || record.Members.Count() != 0;
			if (!hasContent && !forceRecord)
			{
				outputTypeId = Max_uint32;
				return true;
			}
			record.Id = static_cast<uint32>(output.Types.Count());
			output.Types.Add(record);
			outputTypeId = record.Id;
			return true;
		}

		bool BuildPrunedParameterBlock(const ShaderReflectionIR& source, const uint32 sourceBlockId, const bool isRoot, ShaderReflectionIR& output, uint32& outputBlockId, String& error)
		{
			if (sourceBlockId >= static_cast<uint32>(source.ParameterBlocks.Count()))
			{
				error = SE_TEXT("Pruned parameter block references an unknown source block.");
				return false;
			}
			const ShaderIRParameterBlock& sourceBlock = source.ParameterBlocks[sourceBlockId];
			if (sourceBlock.ElementTypeId >= static_cast<uint32>(source.Types.Count()))
			{
				error = SE_TEXT("Pruned parameter block references an unknown source type.");
				return false;
			}
			const ShaderIRTypeRecord& sourceType = source.Types[sourceBlock.ElementTypeId];
			if (sourceBlock.RangeBindings.Count() != sourceType.ResourceRanges.Count())
			{
				error = SE_TEXT("Pruned parameter block source range mappings are incomplete.");
				return false;
			}

			ShaderIRParameterBlock block;
			block.Id = static_cast<uint32>(output.ParameterBlocks.Count());
			output.ParameterBlocks.Add(block);
			outputBlockId = block.Id;

			List<byte> activity;
			List<uint32> subBlockElementTypes;
			List<ShaderIRRangeBinding> bindings;
			activity.Resize(sourceType.ResourceRanges.Count());
			subBlockElementTypes.Resize(sourceType.ResourceRanges.Count());
			bindings.Resize(sourceType.ResourceRanges.Count());
			for (int32 rangeIndex = 0; rangeIndex < activity.Count(); rangeIndex++)
			{
				activity[rangeIndex] = 0;
				subBlockElementTypes[rangeIndex] = Max_uint32;
				const ShaderIRRangeBinding& sourceBinding = sourceBlock.RangeBindings[rangeIndex];
				const bool isBlock = sourceBinding.Flavor == SE_TEXT("constantBuffer") || sourceBinding.Flavor == SE_TEXT("parameterBlock");
				if (isBlock)
				{
					uint32 childBlockId;
					if (sourceBinding.SubBlockId < 0 || !BuildPrunedParameterBlock(source, static_cast<uint32>(sourceBinding.SubBlockId), false, output, childBlockId, error))
					{
						return false;
					}
					if (childBlockId != Max_uint32)
					{
						activity[rangeIndex] = 1;
						bindings[rangeIndex] = sourceBinding;
						bindings[rangeIndex].SubBlockId = static_cast<int32>(childBlockId);
						subBlockElementTypes[rangeIndex] = output.ParameterBlocks[childBlockId].ElementTypeId;
					}
					continue;
				}

				ShaderIRRangeBinding binding = sourceBinding;
				binding.DescriptorRanges.Clear();
				for (int32 descriptorIndex = 0; descriptorIndex < sourceBinding.DescriptorRanges.Count(); descriptorIndex++)
				{
					const ShaderIRDescriptorRange& descriptor = sourceBinding.DescriptorRanges[descriptorIndex];
					if (!descriptor.StageMask.IsEmpty())
					{
						binding.DescriptorRanges.Add(descriptor);
					}
				}
				if (!binding.DescriptorRanges.IsEmpty())
				{
					activity[rangeIndex] = 1;
					bindings[rangeIndex] = binding;
				}
			}

			const bool keepUniformData = sourceBlock.HasDefaultUniformBuffer && !sourceBlock.DefaultUniformBuffer.StageMask.IsEmpty();
			if (!isRoot && !keepUniformData && !HasActiveRange(activity))
			{
				output.ParameterBlocks.RemoveLast();
				outputBlockId = Max_uint32;
				return true;
			}

			uint32 elementTypeId;
			bool hasTypeContent;
			if (!ClonePrunedType(source, sourceBlock.ElementTypeId, activity, subBlockElementTypes, keepUniformData, isRoot, output, elementTypeId, hasTypeContent, error))
			{
				return false;
			}
			if (elementTypeId == Max_uint32)
			{
				error = SE_TEXT("Pruned root parameter block has no element type.");
				return false;
			}
			if (!isRoot && !hasTypeContent)
			{
				error = SE_TEXT("Pruned non-root parameter block has no reflected content.");
				return false;
			}

			block.Name = sourceBlock.Name;
			block.ElementTypeId = elementTypeId;
			block.UniformByteSize = keepUniformData ? sourceBlock.UniformByteSize : 0;
			block.HasDefaultUniformBuffer = keepUniformData;
			if (keepUniformData)
			{
				block.DefaultUniformBuffer = sourceBlock.DefaultUniformBuffer;
			}
			for (int32 rangeIndex = 0; rangeIndex < bindings.Count(); rangeIndex++)
			{
				if (activity[rangeIndex] == 0)
				{
					continue;
				}
				ShaderIRRangeBinding binding = bindings[rangeIndex];
				binding.RangeIndex = static_cast<uint32>(block.RangeBindings.Count());
				block.RangeBindings.Add(binding);
			}
			if (block.RangeBindings.Count() != output.Types[elementTypeId].ResourceRanges.Count())
			{
				error = SE_TEXT("Pruned parameter block range mappings do not match its element type.");
				return false;
			}
			output.ParameterBlocks[outputBlockId] = block;
			return true;
		}
	}

	bool SlangShaderCompiler::BuildReflection(
		slang::IComponentType* linked,
		const List<ShaderStage>& stages,
		const SlangInt targetIndex,
		slang::ProgramLayout* layout,
		ShaderReflectionIR& output,
		String& error)
	{
		output = ShaderReflectionIR();

		if (layout == nullptr)
		{
			error = SE_TEXT("Slang ProgramLayout is null.");
			return false;
		}
		StageUsageContext stageUsage;
		if (!PrepareStageUsage(linked, stages, targetIndex, stageUsage, error))
		{
			return false;
		}

		if (!ValidateEntryPointUniformParameters(layout, error))
		{
			return false;
		}

		if (!ValidateNoPushConstant(layout, error))
		{
			return false;
		}

		slang::VariableLayoutReflection* rootVarLayout = layout->getGlobalParamsVarLayout();
		if (rootVarLayout == nullptr || rootVarLayout->getTypeLayout() == nullptr)
		{
			error = SE_TEXT("Slang global params root layout is null.");
			return false;
		}

		// 根 occurrence 也必须进入 BindingPath；否则顶层显式 space/set 会在递归前丢失。
		ShaderReflectionIR source;
		List<TypeBuildEntry> typeEntries;
		BindingPathNodeArena pathArena;
		BindingPath rootOccurrencePath;
		BindingPath emptyPath;
		if (!AppendBindingPath(emptyPath, rootVarLayout, pathArena, rootOccurrencePath, error))
		{
			return false;
		}
		BlockLayoutContext rootContext;
		slang::TypeLayoutReflection* rootTypeLayout = rootVarLayout->getTypeLayout();
		if (rootTypeLayout->getKind() == slang::TypeReflection::Kind::ConstantBuffer || rootTypeLayout->getKind() == slang::TypeReflection::Kind::ParameterBlock)
		{
			if (!EnterSingleElementContainer(rootOccurrencePath, rootTypeLayout, pathArena, rootContext, error))
			{
				return false;
			}
		}
		else
		{
			rootContext.ElementTypeLayout = rootTypeLayout;
			rootContext.ElementPath = rootOccurrencePath;
		}
		if (!BuildParameterBlock(rootContext, pathArena, SE_TEXT("$root"), stageUsage, typeEntries, source, source.RootBlockId, error))
		{
			output = ShaderReflectionIR();
			return false;
		}
		if (!BuildPrunedParameterBlock(source, source.RootBlockId, true, output, output.RootBlockId, error))
		{
			output = ShaderReflectionIR();
			return false;
		}
		if (!ValidateShaderPhysicalDescriptorLayout(output, error))
		{
			output = ShaderReflectionIR();
			return false;
		}
		output.PipelineLayoutFingerprint = BuildPipelineLayoutFingerprint(output);

		return true;
	}
}
