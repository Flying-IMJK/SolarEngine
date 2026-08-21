#include "SlangReflectionBuilder.h"

#include "Runtime/Core/Types/Hash.h"

namespace SE
{
	namespace
	{
		struct TypeBuildEntry
		{
			slang::TypeLayoutReflection* Layout = nullptr;
			uint32 TypeId = 0;
		};

		String MakeFingerprint(const String& programId, const String& targetKey, const String& variant)
		{
			const String text = programId + SE_TEXT("|") + targetKey + SE_TEXT("|") + variant;
			const StringAnsi ansi(text);
			const uint64 hash = Hash::XXHash::GetHash64(ansi.Get(), ansi.Length());
			return String::Format(SE_TEXT("SOLAR-PIPELINE-LAYOUT/v1:{0:x}"), hash);
		}

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
					member.ResourceRangeOffset = static_cast<uint32>(typeLayout->getFieldBindingRangeOffset(fieldIndex));
					record.Members.Add(member);
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

		void AddDescriptorRanges(slang::TypeLayoutReflection* typeLayout, ShaderIRParameterBlock& block)
		{
			// descriptor 范围是当前 Target 的物理后端布局；逻辑资源名称仍从 TypeRecord 成员树解析。
			for (SlangInt setIndex = 0; setIndex < typeLayout->getDescriptorSetCount(); setIndex++)
			{
				const uint32 set = static_cast<uint32>(typeLayout->getDescriptorSetSpaceOffset(setIndex));
				for (SlangInt rangeIndex = 0; rangeIndex < typeLayout->getDescriptorSetDescriptorRangeCount(setIndex); rangeIndex++)
				{
					ShaderIRDescriptorRange range;
					range.Set = set;
					range.Binding = static_cast<uint32>(typeLayout->getDescriptorSetDescriptorRangeIndexOffset(setIndex, rangeIndex));
					range.DescriptorCount = static_cast<uint32>(typeLayout->getDescriptorSetDescriptorRangeDescriptorCount(setIndex, rangeIndex));
					range.DescriptorType = ToBindingTypeString(typeLayout->getDescriptorSetDescriptorRangeType(setIndex, rangeIndex));
					range.StageMask = SE_TEXT("all");
					block.DescriptorRanges.Add(range);
				}
			}
		}
	}

	bool SlangReflectionBuilder::Build(
		const String& programId,
		const String& targetKey,
		const String& variant,
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

			output.RootBlockId = 0;
			output.PipelineLayoutFingerprint = MakeFingerprint(programId, targetKey, variant);

			// 以 Slang 全局参数布局作为反射根，运行时从该根构建 Falcor 风格的按名称绑定路径。
			List<TypeBuildEntry> typeEntries;
			const uint32 rootTypeId = BuildType(rootVarLayout->getTypeLayout(), typeEntries, output, error);
			if (rootTypeId == Max_uint32)
		{
			output = ShaderReflectionIR();
			return false;
		}

		ShaderIRParameterBlock rootBlock;
		rootBlock.Id = 0;
		rootBlock.Name = SE_TEXT("$root");
		rootBlock.ElementTypeId = rootTypeId;
		rootBlock.UniformByteSize = static_cast<uint32>(layout->getGlobalConstantBufferSize());
		AddDescriptorRanges(rootVarLayout->getTypeLayout(), rootBlock);
		output.ParameterBlocks.Add(rootBlock);

		return true;
	}
}
