#include "SLC2Reader.h"

#include "Runtime/Core/Encoding.h"
#include "Runtime/Core/Serialization/Json.h"

namespace SE
{
	namespace
	{
		bool RequireMember(const Json::Value& object, const char* name, Json::Value::ConstMemberIterator& member, String& error)
		{
			if (!object.IsObject())
			{
				error = SE_TEXT("SLC2 node is not an object.");
				return false;
			}
			member = object.FindMember(name);
			if (member == object.MemberEnd())
			{
				error = String(SE_TEXT("SLC2 missing field: ")) + StringAnsi(name).ToString();
				return false;
			}
			return true;
		}

		bool ReadString(const Json::Value& object, const char* name, String& value, String& error)
		{
			Json::Value::ConstMemberIterator member;
			if (!RequireMember(object, name, member, error) || !member->value.IsString())
			{
				if (error.IsEmpty())
				{
					error = String(SE_TEXT("SLC2 field must be string: ")) + StringAnsi(name).ToString();
				}
				return false;
			}
			value = StringAnsi(member->value.GetString(), static_cast<int32>(member->value.GetStringLength())).ToString();
			return true;
		}

		bool ReadUint(const Json::Value& object, const char* name, uint32& value, String& error)
		{
			Json::Value::ConstMemberIterator member;
			if (!RequireMember(object, name, member, error) || !member->value.IsUint())
			{
				if (error.IsEmpty())
				{
					error = String(SE_TEXT("SLC2 field must be uint: ")) + StringAnsi(name).ToString();
				}
				return false;
			}
			value = member->value.GetUint();
			return true;
		}

		bool ReadArray(const Json::Value& object, const char* name, const Json::Value*& value, String& error)
		{
			Json::Value::ConstMemberIterator member;
			if (!RequireMember(object, name, member, error) || !member->value.IsArray())
			{
				if (error.IsEmpty())
				{
					error = String(SE_TEXT("SLC2 field must be array: ")) + StringAnsi(name).ToString();
				}
				return false;
			}
			value = &member->value;
			return true;
		}

		int32 GetStageMaskOrder(const String& stage)
		{
			if (stage == SE_TEXT("vs"))
			{
				return 0;
			}
			if (stage == SE_TEXT("hs"))
			{
				return 1;
			}
			if (stage == SE_TEXT("ds"))
			{
				return 2;
			}
			if (stage == SE_TEXT("gs"))
			{
				return 3;
			}
			if (stage == SE_TEXT("ps"))
			{
				return 4;
			}
			if (stage == SE_TEXT("cs"))
			{
				return 5;
			}
			return -1;
		}

		bool ValidateStageMask(const String& value, String& error)
		{
			const StringAnsi ansi(value);
			int32 tokenStart = 0;
			int32 previousOrder = -1;
			for (int32 index = 0; index <= ansi.Length(); index++)
			{
				if (index != ansi.Length() && ansi[index] != '|')
				{
					continue;
				}
				if (index == tokenStart)
				{
					error = SE_TEXT("SLC2 stage mask contains an empty token.");
					return false;
				}
				const String token = StringAnsi(ansi.Get() + tokenStart, index - tokenStart).ToString();
				const int32 order = GetStageMaskOrder(token);
				if (order < 0 || order <= previousOrder)
				{
					error = SE_TEXT("SLC2 stage mask is unknown, duplicated, or unordered.");
					return false;
				}
				previousOrder = order;
				tokenStart = index + 1;
			}
			return true;
		}

		bool ParseDescriptorRange(const Json::Value& value, ShaderIRDescriptorRange& output, String& error)
		{
			const Json::Value* flags = nullptr;
			if (!ReadString(value, "role", output.Role, error) || !ReadUint(value, "set", output.Set, error) || !ReadUint(value, "binding", output.Binding, error) || !ReadUint(value, "arrayElementBase", output.ArrayElementBase, error) || !ReadUint(value, "logicalElementStride", output.LogicalElementStride, error) || !ReadUint(value, "descriptorCount", output.DescriptorCount, error) || !ReadString(value, "descriptorType", output.DescriptorType, error) || !ReadString(value, "stageMask", output.StageMask, error) || !ReadArray(value, "flags", flags, error))
			{
				return false;
			}
			if (output.Role.IsEmpty() || output.DescriptorType.IsEmpty() || output.StageMask.IsEmpty() || output.LogicalElementStride == 0 || output.DescriptorCount == 0 || !ValidateStageMask(output.StageMask, error))
			{
				error = SE_TEXT("SLC2 descriptor range has invalid required values.");
				return false;
			}
			for (auto it = flags->Begin(); it != flags->End(); ++it)
			{
				if (!it->IsString())
				{
					error = SE_TEXT("SLC2 descriptor flag is not a string.");
					return false;
				}
				output.Flags.Add(StringAnsi(it->GetString(), static_cast<int32>(it->GetStringLength())).ToString());
			}
			return true;
		}

		bool ParseType(const Json::Value& value, ShaderIRTypeRecord& output, String& error)
		{
			const Json::Value* members = nullptr;
			const Json::Value* ranges = nullptr;
			if (!ReadUint(value, "id", output.Id, error) || !ReadString(value, "kind", output.Kind, error) || !ReadString(value, "name", output.Name, error) || !ReadUint(value, "uniformByteSize", output.UniformByteSize, error) || !ReadUint(value, "elementTypeId", output.ElementTypeId, error) || !ReadUint(value, "elementCount", output.ElementCount, error) || !ReadUint(value, "elementByteStride", output.ElementByteStride, error) || !ReadString(value, "resourceKind", output.ResourceKind, error) || !ReadString(value, "shape", output.Shape, error) || !ReadString(value, "access", output.Access, error) || !ReadArray(value, "members", members, error) || !ReadArray(value, "resourceRanges", ranges, error))
			{
				return false;
			}
			for (auto it = members->Begin(); it != members->End(); ++it)
			{
				ShaderIRMember member;
				if (!ReadString(*it, "name", member.Name, error) || !ReadUint(*it, "typeId", member.TypeId, error) || !ReadUint(*it, "uniformOffset", member.UniformOffset, error) || !ReadUint(*it, "resourceRangeOffset", member.ResourceRangeOffset, error) || !ReadUint(*it, "resourceIndexOffset", member.ResourceIndexOffset, error))
				{
					return false;
				}
				output.Members.Add(member);
			}
			for (auto it = ranges->Begin(); it != ranges->End(); ++it)
			{
				ShaderIRResourceRange range;
				if (!ReadString(*it, "resourceKind", range.ResourceKind, error) || !ReadUint(*it, "count", range.Count, error) || !ReadUint(*it, "baseIndex", range.BaseIndex, error) || !ReadString(*it, "internalRole", range.InternalRole, error) || !ReadUint(*it, "ownerRangeOffset", range.OwnerRangeOffset, error))
				{
					return false;
				}
				if (range.Count == 0)
				{
					error = SE_TEXT("SLC2 resource range count must be non-zero.");
					return false;
				}
				output.ResourceRanges.Add(range);
			}
			return true;
		}

		bool ParseBlock(const Json::Value& value, ShaderIRParameterBlock& output, String& error)
		{
			const Json::Value* bindings = nullptr;
			if (!ReadUint(value, "id", output.Id, error) || !ReadString(value, "name", output.Name, error) || !ReadUint(value, "elementTypeId", output.ElementTypeId, error) || !ReadUint(value, "uniformByteSize", output.UniformByteSize, error) || !ReadArray(value, "rangeBindings", bindings, error))
			{
				return false;
			}
			Json::Value::ConstMemberIterator defaultUniform;
			if (!RequireMember(value, "defaultUniformBuffer", defaultUniform, error))
			{
				return false;
			}
			if (!defaultUniform->value.IsNull())
			{
				output.HasDefaultUniformBuffer = true;
				if (!ParseDescriptorRange(defaultUniform->value, output.DefaultUniformBuffer, error) || output.DefaultUniformBuffer.Role != SE_TEXT("uniformData"))
				{
					if (error.IsEmpty())
					{
						error = SE_TEXT("SLC2 default uniform buffer must use uniformData role.");
					}
					return false;
				}
			}
			if ((output.UniformByteSize == 0) == output.HasDefaultUniformBuffer)
			{
				error = SE_TEXT("SLC2 uniform byte size and default uniform buffer do not agree.");
				return false;
			}
			for (auto it = bindings->Begin(); it != bindings->End(); ++it)
			{
				ShaderIRRangeBinding binding;
				const Json::Value* descriptors = nullptr;
				if (!ReadUint(*it, "rangeIndex", binding.RangeIndex, error) || !ReadString(*it, "flavor", binding.Flavor, error) || !ReadArray(*it, "descriptorRanges", descriptors, error))
				{
					return false;
				}
				Json::Value::ConstMemberIterator subBlock;
				if (!RequireMember(*it, "subBlockId", subBlock, error))
				{
					return false;
				}
				if (!subBlock->value.IsNull())
				{
					if (!subBlock->value.IsUint())
					{
						error = SE_TEXT("SLC2 subBlockId must be uint or null.");
						return false;
					}
					binding.SubBlockId = static_cast<int32>(subBlock->value.GetUint());
				}
				for (auto descriptor = descriptors->Begin(); descriptor != descriptors->End(); ++descriptor)
				{
					ShaderIRDescriptorRange range;
					if (!ParseDescriptorRange(*descriptor, range, error))
					{
						return false;
					}
					binding.DescriptorRanges.Add(range);
				}
				const bool isSimple = binding.Flavor == SE_TEXT("simple");
				const bool isBlock = binding.Flavor == SE_TEXT("constantBuffer") || binding.Flavor == SE_TEXT("parameterBlock");
				if ((!isSimple && !isBlock) || (isSimple && (binding.SubBlockId >= 0 || binding.DescriptorRanges.Count() == 0)) || (isBlock && (binding.SubBlockId < 0 || binding.DescriptorRanges.Count() != 0)))
				{
					error = SE_TEXT("SLC2 range binding shape is invalid.");
					return false;
				}
				output.RangeBindings.Add(binding);
			}
			return true;
		}

		bool ValidateBlockTree(const ShaderReflectionIR& layout, const uint32 blockId, List<byte>& visitState, String& error)
		{
			if (blockId >= static_cast<uint32>(layout.ParameterBlocks.Count()))
			{
				error = SE_TEXT("SLC2 parameter block references an unknown block.");
				return false;
			}
			if (visitState[blockId] != 0)
			{
				error = visitState[blockId] == 1 ? SE_TEXT("SLC2 parameter block graph contains a cycle.") : SE_TEXT("SLC2 parameter block is reached more than once.");
				return false;
			}
			visitState[blockId] = 1;
			const ShaderIRParameterBlock& block = layout.ParameterBlocks[blockId];
			const ShaderIRTypeRecord& type = layout.Types[block.ElementTypeId];
			if (block.RangeBindings.Count() != type.ResourceRanges.Count())
			{
				error = SE_TEXT("SLC2 parameter block does not map every logical resource range.");
				return false;
			}
			for (int32 rangeIndex = 0; rangeIndex < type.ResourceRanges.Count(); rangeIndex++)
			{
				const ShaderIRRangeBinding& binding = block.RangeBindings[rangeIndex];
				const ShaderIRResourceRange& range = type.ResourceRanges[rangeIndex];
				const bool isBlock = binding.Flavor == SE_TEXT("constantBuffer") || binding.Flavor == SE_TEXT("parameterBlock");
				if (binding.RangeIndex != static_cast<uint32>(rangeIndex) || (range.ResourceKind == SE_TEXT("ParameterBlock")) != isBlock)
				{
					error = SE_TEXT("SLC2 logical range and range binding flavor do not agree.");
					return false;
				}
				if (isBlock)
				{
					if (binding.SubBlockId < 0 || !ValidateBlockTree(layout, static_cast<uint32>(binding.SubBlockId), visitState, error))
					{
						return false;
					}
				}
				else
				{
					for (int32 descriptorIndex = 0; descriptorIndex < binding.DescriptorRanges.Count(); descriptorIndex++)
					{
						if (binding.DescriptorRanges[descriptorIndex].DescriptorCount != range.Count)
						{
							error = SE_TEXT("SLC2 descriptor count does not match its logical resource range.");
							return false;
						}
					}
				}
			}
			visitState[blockId] = 2;
			return true;
		}

		bool ValidateTypeTree(const ShaderReflectionIR& layout, const uint32 typeId, List<byte>& visitState, String& error)
		{
			if (typeId >= static_cast<uint32>(layout.Types.Count()))
			{
				error = SE_TEXT("SLC2 type references an unknown type.");
				return false;
			}
			if (visitState[typeId] == 1)
			{
				error = SE_TEXT("SLC2 type graph contains a cycle.");
				return false;
			}
			if (visitState[typeId] == 2)
			{
				return true;
			}
			visitState[typeId] = 1;
			const ShaderIRTypeRecord& type = layout.Types[typeId];
			if ((type.Kind == SE_TEXT("Array") || type.Kind == SE_TEXT("ParameterBlock")) && !ValidateTypeTree(layout, type.ElementTypeId, visitState, error))
			{
				return false;
			}
			for (int32 memberIndex = 0; memberIndex < type.Members.Count(); memberIndex++)
			{
				if (!ValidateTypeTree(layout, type.Members[memberIndex].TypeId, visitState, error))
				{
					return false;
				}
			}
			visitState[typeId] = 2;
			return true;
		}

		bool ValidateLayout(ShaderReflectionIR& layout, String& error)
		{
			if (layout.Schema != 2 || layout.Types.Count() == 0 || layout.ParameterBlocks.Count() == 0 || layout.RootBlockId >= static_cast<uint32>(layout.ParameterBlocks.Count()))
			{
				error = SE_TEXT("SLC2 layout header is invalid.");
				return false;
			}
			for (int32 typeIndex = 0; typeIndex < layout.Types.Count(); typeIndex++)
			{
				const ShaderIRTypeRecord& type = layout.Types[typeIndex];
				if (type.Id != static_cast<uint32>(typeIndex) || type.Kind.IsEmpty())
				{
					error = SE_TEXT("SLC2 type IDs are not contiguous.");
					return false;
				}
				for (int32 memberIndex = 0; memberIndex < type.Members.Count(); memberIndex++)
				{
					if (type.Members[memberIndex].TypeId >= static_cast<uint32>(layout.Types.Count()))
					{
						error = SE_TEXT("SLC2 member references an unknown type.");
						return false;
					}
				}
				uint64 expectedBaseIndex = 0;
				for (int32 rangeIndex = 0; rangeIndex < type.ResourceRanges.Count(); rangeIndex++)
				{
					const ShaderIRResourceRange& range = type.ResourceRanges[rangeIndex];
					if (range.Count == 0 || expectedBaseIndex > Max_uint32 || range.BaseIndex != expectedBaseIndex)
					{
						error = SE_TEXT("SLC2 resource range count or base index is invalid.");
						return false;
					}
					expectedBaseIndex += range.Count;
				}
			}
			for (int32 blockIndex = 0; blockIndex < layout.ParameterBlocks.Count(); blockIndex++)
			{
				const ShaderIRParameterBlock& block = layout.ParameterBlocks[blockIndex];
				if (block.Id != static_cast<uint32>(blockIndex) || block.ElementTypeId >= static_cast<uint32>(layout.Types.Count()))
				{
					error = SE_TEXT("SLC2 parameter block is invalid.");
					return false;
				}
				for (int32 left = 0; left < block.RangeBindings.Count(); left++)
				{
					const ShaderIRRangeBinding& binding = block.RangeBindings[left];
					if (binding.SubBlockId >= static_cast<int32>(layout.ParameterBlocks.Count()))
					{
						error = SE_TEXT("SLC2 range binding references an unknown block.");
						return false;
					}
					for (int32 right = left + 1; right < block.RangeBindings.Count(); right++)
					{
						if (binding.RangeIndex == block.RangeBindings[right].RangeIndex)
						{
							error = SE_TEXT("SLC2 range binding index is duplicated.");
							return false;
						}
					}
				}
			}
			List<byte> visitState;
			visitState.Resize(layout.ParameterBlocks.Count());
			for (int32 blockIndex = 0; blockIndex < visitState.Count(); blockIndex++)
			{
				visitState[blockIndex] = 0;
			}
			if (!ValidateBlockTree(layout, layout.RootBlockId, visitState, error))
			{
				return false;
			}
			for (int32 blockIndex = 0; blockIndex < visitState.Count(); blockIndex++)
			{
				if (visitState[blockIndex] != 2)
				{
					error = SE_TEXT("SLC2 parameter block is not reachable from rootBlockId.");
					return false;
				}
			}
			List<byte> typeVisitState;
			typeVisitState.Resize(layout.Types.Count());
			for (int32 typeIndex = 0; typeIndex < typeVisitState.Count(); typeIndex++)
			{
				typeVisitState[typeIndex] = 0;
			}
			if (!ValidateTypeTree(layout, layout.ParameterBlocks[layout.RootBlockId].ElementTypeId, typeVisitState, error))
			{
				return false;
			}
			for (int32 typeIndex = 0; typeIndex < typeVisitState.Count(); typeIndex++)
			{
				if (typeVisitState[typeIndex] != 2)
				{
					error = SE_TEXT("SLC2 type is not reachable from rootBlockId.");
					return false;
				}
			}
			if (BuildPipelineLayoutFingerprint(layout) != layout.PipelineLayoutFingerprint)
			{
				error = SE_TEXT("SLC2 pipeline layout fingerprint mismatch.");
				return false;
			}
			return true;
		}

		bool ParseStage(const Json::Value& value, SLC2StageRecord& output, String& error)
		{
			String stageName;
			String encodedCode;
			String codeHash;
			uint32 outputControlPoints = 0;
			if (!ReadString(value, "stage", stageName, error) || !ReadString(value, "entryPoint", output.EntryPoint, error) || !ReadUint(value, "outputControlPoints", outputControlPoints, error) || !ReadString(value, "code", encodedCode, error) || !ReadString(value, "codeHash", codeHash, error))
			{
				return false;
			}
			const StringAnsi stageAnsi(stageName);
            if (!ParseShaderStage(stageAnsi.Get(), output.Stage) || output.Stage == ShaderStage::Max ||
                encodedCode.IsEmpty())
			{
				error = SE_TEXT("SLC2 stage is invalid.");
				return false;
			}
			if ((output.Stage == ShaderStage::Hull && (outputControlPoints == 0 || outputControlPoints > 32)) ||
				(output.Stage != ShaderStage::Hull && outputControlPoints != 0))
			{
				error = SE_TEXT("SLC2 stage output control points are invalid.");
				return false;
			}
			output.OutputControlPoints = static_cast<int32>(outputControlPoints);
			const StringAnsi encodedAnsi(encodedCode);
			Encoding::Base64::Decode(encodedAnsi.Get(), encodedAnsi.Length(), output.Code);
			if (output.Code.Count() < 4 || (output.Code.Count() & 3) != 0 || output.Code[0] != 0x03 || output.Code[1] != 0x02 || output.Code[2] != 0x23 || output.Code[3] != 0x07 || ComputeSHA256Hex(output.Code.Get(), output.Code.Count()) != codeHash)
			{
				error = SE_TEXT("SLC2 stage code is invalid.");
				return false;
			}
			return true;
		}

		bool ParseTarget(const Json::Value& value, SLC2TargetRecord& output, String& error)
		{
			String platform;
			String shaderProfile;
			String featureLevel;
			const Json::Value* variants = nullptr;
			if (!ReadString(value, "targetKey", output.TargetKey, error) || !ReadString(value, "platform", platform, error) || !ReadString(value, "shaderProfile", shaderProfile, error) || !ReadString(value, "featureLevel", featureLevel, error) || !ReadArray(value, "variants", variants, error))
			{
				return false;
			}
			bool featureLevelFound = false;
			for (int32 index = 0; index <= static_cast<int32>(ShaderTargetPlatform::PS5); index++)
			{
				if (ToString(static_cast<ShaderTargetPlatform>(index)) == platform)
				{
					output.Target.Platform = static_cast<ShaderTargetPlatform>(index);
				}
			}
			for (int32 index = 0; index < static_cast<int32>(ShaderProfile::MAX); index++)
			{
				if (ToString(static_cast<ShaderProfile>(index)) == shaderProfile)
				{
					output.Target.Profile = static_cast<ShaderProfile>(index);
				}
			}
			for (int32 index = 0; index < static_cast<int32>(FeatureLevel::MAX); index++)
			{
				if (String(ToString(static_cast<FeatureLevel>(index))) == featureLevel)
				{
					output.Target.Feature = static_cast<FeatureLevel>(index);
					featureLevelFound = true;
				}
			}
			if (output.Target.Platform == ShaderTargetPlatform::Unknown || output.Target.Profile == ShaderProfile::Unknown || !featureLevelFound || BuildTargetKey(output.Target) != output.TargetKey || variants->Empty())
			{
				error = SE_TEXT("SLC2 target identity is invalid.");
				return false;
			}
			for (auto it = variants->Begin(); it != variants->End(); ++it)
			{
				SLC2VariantRecord variant;
				const Json::Value* stages = nullptr;
				Json::Value::ConstMemberIterator layout;
				if (!ReadString(*it, "variant", variant.Variant, error) || !RequireMember(*it, "layout", layout, error) || !layout->value.IsObject() || !ReadArray(*it, "stages", stages, error) || stages->Empty())
				{
					return false;
				}
				const Json::Value* types = nullptr;
				const Json::Value* blocks = nullptr;
				if (!ReadUint(layout->value, "schema", variant.Layout.Schema, error) || !ReadUint(layout->value, "rootBlockId", variant.Layout.RootBlockId, error) || !ReadString(layout->value, "pipelineLayoutFingerprint", variant.Layout.PipelineLayoutFingerprint, error) || !ReadArray(layout->value, "types", types, error) || !ReadArray(layout->value, "parameterBlocks", blocks, error))
				{
					return false;
				}
				for (auto type = types->Begin(); type != types->End(); ++type)
				{
					ShaderIRTypeRecord record;
					if (!ParseType(*type, record, error))
					{
						return false;
					}
					variant.Layout.Types.Add(record);
				}
				for (auto block = blocks->Begin(); block != blocks->End(); ++block)
				{
					ShaderIRParameterBlock record;
					if (!ParseBlock(*block, record, error))
					{
						return false;
					}
					variant.Layout.ParameterBlocks.Add(record);
				}
				if (!ValidateLayout(variant.Layout, error))
				{
					return false;
				}
				for (auto stage = stages->Begin(); stage != stages->End(); ++stage)
				{
					SLC2StageRecord record;
					if (!ParseStage(*stage, record, error))
					{
						return false;
					}
					for (int32 existing = 0; existing < variant.Stages.Count(); existing++)
					{
						if (variant.Stages[existing].Stage == record.Stage)
						{
							error = SE_TEXT("SLC2 variant has duplicate stages.");
							return false;
						}
					}
					variant.Stages.Add(record);
				}
				bool hasCompute = false;
				bool hasPixel = false;
				for (int32 stageIndex = 0; stageIndex < variant.Stages.Count(); stageIndex++)
				{
                    hasCompute |= variant.Stages[stageIndex].Stage == ShaderStage::Compute;
                    hasPixel |= variant.Stages[stageIndex].Stage == ShaderStage::Pixel;
				}
				if ((hasCompute && variant.Stages.Count() != 1) || (!hasCompute && !hasPixel))
				{
					error = SE_TEXT("SLC2 variant stage contract is invalid.");
					return false;
				}
				for (int32 existing = 0; existing < output.Variants.Count(); existing++)
				{
					if (output.Variants[existing].Variant == variant.Variant)
					{
						error = SE_TEXT("SLC2 target has duplicate variants.");
						return false;
					}
				}
				output.Variants.Add(variant);
			}
			return true;
		}
	}

	bool SLC2Reader::Read(const byte* data, const int32 length, SLC2Artifact& artifact, String& error)
	{
		artifact = SLC2Artifact();
		error = String::Empty;
		if (data == nullptr || length <= 0)
		{
			error = SE_TEXT("SLC2 data is empty.");
			return false;
		}
		Json::Document document;
		document.Parse(reinterpret_cast<const char*>(data), length);
		if (document.HasParseError() || !document.IsObject())
		{
			error = SE_TEXT("SLC2 data is not valid JSON.");
			return false;
		}
		if (!ReadString(document, "format", artifact.Format, error) || !ReadUint(document, "version", artifact.Version, error) || !ReadString(document, "compilerBuildTag", artifact.CompilerBuildTag, error) || artifact.Format != SE_TEXT("SLC2") || artifact.Version != 2)
		{
			if (error.IsEmpty())
			{
				error = SE_TEXT("SLC2 header is invalid.");
			}
			return false;
		}
		const Json::Value* programs = nullptr;
		if (!ReadArray(document, "programs", programs, error) || programs->Empty())
		{
			return false;
		}
		for (auto it = programs->Begin(); it != programs->End(); ++it)
		{
			SLC2ProgramRecord program;
			const Json::Value* groups = nullptr;
			const Json::Value* targets = nullptr;
			if (!ReadString(*it, "programId", program.ProgramId, error) || !ReadArray(*it, "variantGroups", groups, error) || !ReadArray(*it, "targets", targets, error) || program.ProgramId.IsEmpty() || targets->Empty())
			{
				return false;
			}
			for (auto group = groups->Begin(); group != groups->End(); ++group)
			{
				ShaderVariantGroup record;
				const Json::Value* members = nullptr;
				if (!ReadArray(*group, "members", members, error) || !ReadString(*group, "default", record.DefaultMember, error) || members->Empty())
				{
					return false;
				}
				for (auto member = members->Begin(); member != members->End(); ++member)
				{
					if (!member->IsString())
					{
						error = SE_TEXT("SLC2 variant group member is invalid.");
						return false;
					}
					record.Members.Add(StringAnsi(member->GetString(), static_cast<int32>(member->GetStringLength())).ToString());
				}
				program.VariantGroups.Add(record);
			}
			for (auto target = targets->Begin(); target != targets->End(); ++target)
			{
				SLC2TargetRecord record;
				if (!ParseTarget(*target, record, error))
				{
					return false;
				}
				for (int32 existing = 0; existing < program.Targets.Count(); existing++)
				{
					if (program.Targets[existing].TargetKey == record.TargetKey)
					{
						error = SE_TEXT("SLC2 program has duplicate targets.");
						return false;
					}
				}
				program.Targets.Add(record);
			}
			for (int32 existing = 0; existing < artifact.Programs.Count(); existing++)
			{
				if (artifact.Programs[existing].ProgramId == program.ProgramId)
				{
					error = SE_TEXT("SLC2 has duplicate ProgramId values.");
					return false;
				}
			}
			artifact.Programs.Add(program);
		}
		return true;
	}

	bool SLC2Reader::Read(const List<byte>& data, SLC2Artifact& artifact, String& error)
	{
		return Read(data.Get(), data.Count(), artifact, error);
	}

	bool SLC2Reader::ReadAndValidate(const byte* data, const int32 length, String& error)
	{
		SLC2Artifact artifact;
		return Read(data, length, artifact, error);
	}

	bool SLC2Reader::ReadAndValidate(const List<byte>& data, String& error)
	{
		return ReadAndValidate(data.Get(), data.Count(), error);
	}
}
