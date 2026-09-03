#include "SlangReflectionBuilder.h"

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

		struct TypeBuildEntry
		{
			slang::TypeLayoutReflection* Layout = nullptr;
			uint32 TypeId = 0;
		};

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
			switch (type)
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

		void AppendResourceRanges(ShaderIRTypeRecord& destination, const ShaderIRTypeRecord& source, const uint32 multiplier)
		{
			for (int32 rangeIndex = 0; rangeIndex < source.ResourceRanges.Count(); rangeIndex++)
			{
				ShaderIRResourceRange range = source.ResourceRanges[rangeIndex];
				range.Count *= multiplier;
				range.BaseIndex = 0;
				for (int32 existingIndex = 0; existingIndex < destination.ResourceRanges.Count(); existingIndex++)
				{
					range.BaseIndex += destination.ResourceRanges[existingIndex].Count;
				}
				destination.ResourceRanges.Add(range);
			}
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

			ShaderIRTypeRecord record;
			record.Id = entry.TypeId;
			record.Kind = ToTypeKind(slangKind);
			record.Name = String(typeLayout->getName());
			record.UniformByteSize = static_cast<uint32>(typeLayout->getSize(slang::ParameterCategory::Uniform));
			output.Types.Add(record);

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
				break;
			case slang::TypeReflection::Kind::Resource:
			case slang::TypeReflection::Kind::SamplerState:
			case slang::TypeReflection::Kind::TextureBuffer:
			case slang::TypeReflection::Kind::ShaderStorageBuffer:
				// 单个资源类型先形成逻辑 ResourceRange，后续由 ParameterBlock 的 descriptor 范围映射到物理绑定。
				record.ResourceKind = ToResourceKindString(slangKind);
				record.Shape = ToResourceShapeString(typeLayout->getResourceShape());
				record.Access = ToResourceAccessString(typeLayout->getResourceAccess());
				AddResourceRange(record, record.ResourceKind, 1);
				break;
			case slang::TypeReflection::Kind::ConstantBuffer:
			case slang::TypeReflection::Kind::ParameterBlock:
				record.ElementTypeId = BuildType(typeLayout->getElementTypeLayout(), entries, output, error);
				if (record.ElementTypeId == Max_uint32)
				{
					return Max_uint32;
				}
				record.ResourceKind = record.Kind;
				AddResourceRange(record, record.Kind, 1);
				break;
			default:
				break;
			}

			output.Types[entry.TypeId] = record;
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

		bool BuildDescriptorRange(slang::TypeLayoutReflection* typeLayout, const SlangInt descriptorSetIndex, const SlangInt descriptorRangeIndex, const String& role, const StageUsageContext& stageUsage, ShaderIRDescriptorRange& output, String& error)
		{
			uint32 set;
			uint32 binding;
			uint32 count;
			if (!ReadFiniteUint32(typeLayout->getDescriptorSetSpaceOffset(descriptorSetIndex), set, error) || !ReadFiniteUint32(typeLayout->getDescriptorSetDescriptorRangeIndexOffset(descriptorSetIndex, descriptorRangeIndex), binding, error) || !ReadFiniteUint32(typeLayout->getDescriptorSetDescriptorRangeDescriptorCount(descriptorSetIndex, descriptorRangeIndex), count, error) || count == 0)
			{
				if (error.IsEmpty())
				{
					error = SE_TEXT("Slang descriptor range has zero count.");
				}
				return false;
			}
			output.Role = role;
			output.Set = set;
			output.Binding = binding;
			output.ArrayElementBase = 0;
			output.LogicalElementStride = 1;
			output.DescriptorCount = count;
			output.DescriptorType = ToBindingTypeString(typeLayout->getDescriptorSetDescriptorRangeType(descriptorSetIndex, descriptorRangeIndex));
			if (!BuildStageMask(stageUsage, typeLayout->getDescriptorSetDescriptorRangeCategory(descriptorSetIndex, descriptorRangeIndex), set, binding, output.StageMask, error))
			{
				return false;
			}
			if (output.DescriptorType == SE_TEXT("Unknown"))
			{
				error = SE_TEXT("Slang descriptor range uses an unsupported binding type.");
				return false;
			}
			return true;
		}

		bool BuildDefaultUniformBuffer(slang::TypeLayoutReflection* containerTypeLayout, const uint32 uniformByteSize, const StageUsageContext& stageUsage, ShaderIRParameterBlock& block, String& error)
		{
			if (uniformByteSize == 0)
			{
				return true;
			}
			for (SlangInt setIndex = 0; setIndex < containerTypeLayout->getDescriptorSetCount(); setIndex++)
			{
				for (SlangInt descriptorRangeIndex = 0; descriptorRangeIndex < containerTypeLayout->getDescriptorSetDescriptorRangeCount(setIndex); descriptorRangeIndex++)
				{
					if (containerTypeLayout->getDescriptorSetDescriptorRangeType(setIndex, descriptorRangeIndex) != slang::BindingType::ConstantBuffer)
					{
						continue;
					}
					if (!BuildDescriptorRange(containerTypeLayout, setIndex, descriptorRangeIndex, SE_TEXT("uniformData"), stageUsage, block.DefaultUniformBuffer, error))
					{
						return false;
					}
					block.HasDefaultUniformBuffer = true;
					return true;
				}
			}
			error = SE_TEXT("Shader parameter block has uniform data but no ConstantBuffer descriptor range.");
			return false;
		}

		bool BuildParameterBlock(slang::TypeLayoutReflection* containerTypeLayout, slang::TypeLayoutReflection* elementTypeLayout, const String& name, const StageUsageContext& stageUsage, List<TypeBuildEntry>& typeEntries, ShaderReflectionIR& output, uint32& blockId, String& error)
		{
			if (containerTypeLayout == nullptr || elementTypeLayout == nullptr)
			{
				error = SE_TEXT("Shader parameter block container or element layout is null.");
				return false;
			}
			const uint32 elementTypeId = BuildType(elementTypeLayout, typeEntries, output, error);
			if (elementTypeId == Max_uint32)
			{
				return false;
			}
			const SlangInt bindingRangeCount = elementTypeLayout->getBindingRangeCount();
			if (bindingRangeCount < 0 || static_cast<uint32>(bindingRangeCount) != static_cast<uint32>(output.Types[elementTypeId].ResourceRanges.Count()))
			{
				error = SE_TEXT("Slang binding range count does not match the logical resource range graph.");
				return false;
			}

			ShaderIRParameterBlock block;
			block.Id = static_cast<uint32>(output.ParameterBlocks.Count());
			block.Name = name;
			block.ElementTypeId = elementTypeId;
			block.UniformByteSize = static_cast<uint32>(elementTypeLayout->getSize(slang::ParameterCategory::Uniform));
			if (!BuildDefaultUniformBuffer(containerTypeLayout, block.UniformByteSize, stageUsage, block, error))
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

			for (SlangInt rangeIndex = 0; rangeIndex < bindingRangeCount; rangeIndex++)
			{
				const slang::BindingType bindingType = elementTypeLayout->getBindingRangeType(rangeIndex);
				const bool isBlock = bindingType == slang::BindingType::ConstantBuffer || bindingType == slang::BindingType::ParameterBlock;
				ShaderIRRangeBinding binding;
				binding.RangeIndex = static_cast<uint32>(rangeIndex);
				if (isBlock)
				{
					uint32 blockCount;
					const int32 subObjectRangeIndex = subObjectByBindingRange[static_cast<int32>(rangeIndex)];
					slang::TypeLayoutReflection* childContainerTypeLayout = elementTypeLayout->getBindingRangeLeafTypeLayout(rangeIndex);
					if (!ReadFiniteUint32(elementTypeLayout->getBindingRangeBindingCount(rangeIndex), blockCount, error) || blockCount != 1 || subObjectRangeIndex < 0 || childContainerTypeLayout == nullptr || childContainerTypeLayout->getElementTypeLayout() == nullptr)
					{
						error = SE_TEXT("ConstantBuffer or ParameterBlock binding range must have one sub-object instance.");
						return false;
					}
					uint32 subBlockId;
					slang::VariableReflection* leafVariable = elementTypeLayout->getBindingRangeLeafVariable(rangeIndex);
					const String childName = leafVariable != nullptr ? String(leafVariable->getName()) : String::Format(SE_TEXT("$block{0}"), rangeIndex);
					if (!BuildParameterBlock(childContainerTypeLayout, childContainerTypeLayout->getElementTypeLayout(), childName, stageUsage, typeEntries, output, subBlockId, error))
					{
						return false;
					}
					binding.Flavor = bindingType == slang::BindingType::ConstantBuffer ? SE_TEXT("constantBuffer") : SE_TEXT("parameterBlock");
					binding.SubBlockId = static_cast<int32>(subBlockId);
				}
				else
				{
					const SlangInt descriptorSetIndex = elementTypeLayout->getBindingRangeDescriptorSetIndex(rangeIndex);
					const SlangInt firstDescriptorRangeIndex = elementTypeLayout->getBindingRangeFirstDescriptorRangeIndex(rangeIndex);
					const SlangInt descriptorRangeCount = elementTypeLayout->getBindingRangeDescriptorRangeCount(rangeIndex);
					if (descriptorSetIndex < 0 || descriptorSetIndex >= elementTypeLayout->getDescriptorSetCount() || firstDescriptorRangeIndex < 0 || descriptorRangeCount <= 0 || firstDescriptorRangeIndex + descriptorRangeCount > elementTypeLayout->getDescriptorSetDescriptorRangeCount(descriptorSetIndex))
					{
						error = SE_TEXT("Simple binding range has no descriptor range mapping.");
						return false;
					}
					binding.Flavor = SE_TEXT("simple");
					for (SlangInt descriptorOffset = 0; descriptorOffset < descriptorRangeCount; descriptorOffset++)
					{
						ShaderIRDescriptorRange descriptor;
						if (!BuildDescriptorRange(elementTypeLayout, descriptorSetIndex, firstDescriptorRangeIndex + descriptorOffset, SE_TEXT("primary"), stageUsage, descriptor, error))
						{
							return false;
						}
						if (descriptor.DescriptorCount != output.Types[elementTypeId].ResourceRanges[static_cast<int32>(rangeIndex)].Count)
						{
							error = SE_TEXT("Descriptor count does not match the logical resource range count.");
							return false;
						}
						binding.DescriptorRanges.Add(descriptor);
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
				if (activity.Count() != elementRangeCount * static_cast<int32>(sourceRecord.ElementCount))
				{
					error = SE_TEXT("Pruned array activity does not match the source element range graph.");
					return false;
				}
				List<byte> elementActivity;
				List<uint32> elementSubBlockElementTypes;
				elementActivity.Resize(elementRangeCount);
				elementSubBlockElementTypes.Resize(elementRangeCount);
				for (int32 rangeIndex = 0; rangeIndex < elementRangeCount; rangeIndex++)
				{
					elementActivity[rangeIndex] = activity[rangeIndex];
					elementSubBlockElementTypes[rangeIndex] = subBlockElementTypes[rangeIndex];
				}
				for (uint32 elementIndex = 1; elementIndex < sourceRecord.ElementCount; elementIndex++)
				{
					for (int32 rangeIndex = 0; rangeIndex < elementRangeCount; rangeIndex++)
					{
						const int32 sourceRangeIndex = static_cast<int32>(elementIndex) * elementRangeCount + rangeIndex;
						if (activity[sourceRangeIndex] != elementActivity[rangeIndex] || subBlockElementTypes[sourceRangeIndex] != elementSubBlockElementTypes[rangeIndex])
						{
							error = SE_TEXT("Pruned resource array has inconsistent element activity.");
							return false;
						}
					}
				}
				uint32 elementTypeId;
				bool elementHasContent;
				if (!ClonePrunedType(source, sourceRecord.ElementTypeId, elementActivity, elementSubBlockElementTypes, keepUniformData, false, output, elementTypeId, elementHasContent, error))
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
				for (int32 rangeIndex = 0; rangeIndex < sourceRecord.ResourceRanges.Count(); rangeIndex++)
				{
					if (activity[rangeIndex] != 0)
					{
						const ShaderIRResourceRange& sourceRange = sourceRecord.ResourceRanges[rangeIndex];
						AddResourceRange(record, sourceRange.ResourceKind, sourceRange.Count);
					}
				}
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

	bool SlangReflectionBuilder::Build(
		const String& programId,
		const String& targetKey,
		const String& variant,
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

		// 以 Slang 全局参数布局作为反射根。后续所有 ConstantBuffer/ParameterBlock
		// 都由 binding range → sub-object range 的显式关系递归创建。
		ShaderReflectionIR source;
		List<TypeBuildEntry> typeEntries;
		if (!BuildParameterBlock(rootVarLayout->getTypeLayout(), rootVarLayout->getTypeLayout(), SE_TEXT("$root"), stageUsage, typeEntries, source, source.RootBlockId, error))
		{
			output = ShaderReflectionIR();
			return false;
		}
		if (!BuildPrunedParameterBlock(source, source.RootBlockId, true, output, output.RootBlockId, error))
		{
			output = ShaderReflectionIR();
			return false;
		}
		output.PipelineLayoutFingerprint = BuildPipelineLayoutFingerprint(output);

		return true;
	}
}
