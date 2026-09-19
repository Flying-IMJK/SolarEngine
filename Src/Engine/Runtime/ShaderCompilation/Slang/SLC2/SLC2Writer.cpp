#include "SLC2Writer.h"

#include "Runtime/Core/Serialization/JsonWriters.hpp"

namespace SE
{
	namespace
	{
		void WriteStringArray(JsonWriter& writer, const List<String>& values)
		{
			writer.StartArray();
			for (int32 i = 0; i < values.Count(); i++)
			{
				writer.String(values[i]);
			}
			writer.EndArray(values.Count());
		}

		void WriteVariantGroups(JsonWriter& writer, const List<ShaderVariantGroup>& groups)
		{
			writer.StartArray();
			for (int32 groupIndex = 0; groupIndex < groups.Count(); groupIndex++)
			{
				const ShaderVariantGroup& group = groups[groupIndex];
				writer.StartObject();
				writer.Key(SE_TEXT("members"));
				WriteStringArray(writer, group.Members);
				writer.Key(SE_TEXT("default"));
				if (group.DefaultMember.IsEmpty())
				{
					writer.String(SE_TEXT(""));
				}
				else
				{
					writer.String(group.DefaultMember);
				}
				writer.EndObject();
			}
			writer.EndArray(groups.Count());
		}

		// Writer 只接受当前票据支持的受控枚举，避免把未知内存值静默写成合法缓存文本。
		namespace SLC2VertexInputWriter
		{
			const char* ShaderTypeName(const SLC2VertexInputType type)
			{
				switch (type)
				{
				case SLC2VertexInputType::Float: return "float";
				case SLC2VertexInputType::Float2: return "float2";
				case SLC2VertexInputType::Float3: return "float3";
				case SLC2VertexInputType::Float4: return "float4";
				case SLC2VertexInputType::Int: return "int";
				case SLC2VertexInputType::Int2: return "int2";
				case SLC2VertexInputType::Int3: return "int3";
				case SLC2VertexInputType::Int4: return "int4";
				case SLC2VertexInputType::UInt: return "uint";
				case SLC2VertexInputType::UInt2: return "uint2";
				case SLC2VertexInputType::UInt3: return "uint3";
				case SLC2VertexInputType::UInt4: return "uint4";
				default: return nullptr;
			}
			}

			bool WriteVertexInputSignature(JsonWriter& writer,
										   const List<SLC2VertexInputSignatureElement>& signature,
										   String& error)
			{
				writer.StartArray();
				for (int32 elementIndex = 0; elementIndex < signature.Count(); elementIndex++)
				{
					const SLC2VertexInputSignatureElement& element = signature[elementIndex];
					const char* shaderType = ShaderTypeName(element.ShaderType);
					if (element.Semantic.IsEmpty() || element.Semantic.StartsWith(SE_TEXT("SV_")) || shaderType == nullptr)
					{
						error = SE_TEXT("SLC2 vertex input signature contains an invalid semantic or ShaderType.");
						return false;
					}
					for (int32 otherIndex = 0; otherIndex < elementIndex; otherIndex++)
					{
						const SLC2VertexInputSignatureElement& other = signature[otherIndex];
						if ((other.Semantic == element.Semantic && other.SemanticIndex == element.SemanticIndex) ||
							other.Location == element.Location)
						{
							error = SE_TEXT("SLC2 vertex input signature contains a duplicated semantic or location.");
							return false;
						}
					}
					writer.StartObject();
					writer.Key(SE_TEXT("semantic"));
					writer.String(element.Semantic);
					writer.Key(SE_TEXT("semanticIndex"));
					writer.Uint(element.SemanticIndex);
					writer.Key(SE_TEXT("location"));
					writer.Uint(element.Location);
					writer.Key(SE_TEXT("shaderType"));
					writer.String(StringAnsi(shaderType).ToString());
					writer.EndObject();
				}
				writer.EndArray(signature.Count());
				return true;
			}
		} // namespace SLC2VertexInputWriter

		void WriteLayout(JsonWriter& writer, const ShaderReflectionIR& layout)
		{
			// SLC2 只序列化引擎自定义 ShaderReflectionIR，不保存 Slang 对象指针或原生枚举值。
			writer.StartObject();
			writer.Key(SE_TEXT("schema"));
			writer.Uint(layout.Schema);
			writer.Key(SE_TEXT("rootBlockId"));
			writer.Uint(layout.RootBlockId);
			writer.Key(SE_TEXT("pipelineLayoutFingerprint"));
			writer.String(layout.PipelineLayoutFingerprint);

			writer.Key(SE_TEXT("types"));
			writer.StartArray();
			for (int32 typeIndex = 0; typeIndex < layout.Types.Count(); typeIndex++)
			{
				const ShaderIRTypeRecord& type = layout.Types[typeIndex];
				writer.StartObject();
				writer.Key(SE_TEXT("id"));
				writer.Uint(type.Id);
				writer.Key(SE_TEXT("kind"));
				writer.String(type.Kind);
				writer.Key(SE_TEXT("name"));
				writer.String(type.Name);
				writer.Key(SE_TEXT("uniformByteSize"));
				writer.Uint(type.UniformByteSize);
				writer.Key(SE_TEXT("elementTypeId"));
				writer.Uint(type.ElementTypeId);
				writer.Key(SE_TEXT("elementCount"));
				writer.Uint(type.ElementCount);
				writer.Key(SE_TEXT("elementByteStride"));
				writer.Uint(type.ElementByteStride);
				writer.Key(SE_TEXT("resourceKind"));
				writer.String(type.ResourceKind);
				writer.Key(SE_TEXT("shape"));
				writer.String(type.Shape);
				writer.Key(SE_TEXT("access"));
				writer.String(type.Access);
				writer.Key(SE_TEXT("members"));
				writer.StartArray();
				for (int32 memberIndex = 0; memberIndex < type.Members.Count(); memberIndex++)
				{
					const ShaderIRMember& member = type.Members[memberIndex];
					writer.StartObject();
					writer.Key(SE_TEXT("name"));
					writer.String(member.Name);
					writer.Key(SE_TEXT("typeId"));
					writer.Uint(member.TypeId);
					writer.Key(SE_TEXT("uniformOffset"));
					writer.Uint(member.UniformOffset);
					writer.Key(SE_TEXT("resourceRangeOffset"));
					writer.Uint(member.ResourceRangeOffset);
					writer.Key(SE_TEXT("resourceIndexOffset"));
					writer.Uint(member.ResourceIndexOffset);
					writer.EndObject();
				}
				writer.EndArray(type.Members.Count());
				writer.Key(SE_TEXT("resourceRanges"));
				writer.StartArray();
				for (int32 rangeIndex = 0; rangeIndex < type.ResourceRanges.Count(); rangeIndex++)
				{
					const ShaderIRResourceRange& range = type.ResourceRanges[rangeIndex];
					writer.StartObject();
					writer.Key(SE_TEXT("resourceKind"));
					writer.String(range.ResourceKind);
					writer.Key(SE_TEXT("count"));
					writer.Uint(range.Count);
					writer.Key(SE_TEXT("baseIndex"));
					writer.Uint(range.BaseIndex);
					writer.Key(SE_TEXT("internalRole"));
					writer.String(range.InternalRole);
					writer.Key(SE_TEXT("ownerRangeOffset"));
					writer.Uint(range.OwnerRangeOffset);
					writer.EndObject();
				}
				writer.EndArray(type.ResourceRanges.Count());
				writer.EndObject();
			}
			writer.EndArray(layout.Types.Count());

			writer.Key(SE_TEXT("parameterBlocks"));
			writer.StartArray();
			for (int32 blockIndex = 0; blockIndex < layout.ParameterBlocks.Count(); blockIndex++)
			{
				const ShaderIRParameterBlock& block = layout.ParameterBlocks[blockIndex];
				writer.StartObject();
				writer.Key(SE_TEXT("id"));
				writer.Uint(block.Id);
				writer.Key(SE_TEXT("name"));
				writer.String(block.Name);
				writer.Key(SE_TEXT("elementTypeId"));
				writer.Uint(block.ElementTypeId);
				writer.Key(SE_TEXT("uniformByteSize"));
				writer.Uint(block.UniformByteSize);
				writer.Key(SE_TEXT("defaultUniformBuffer"));
				if (block.HasDefaultUniformBuffer)
				{
					writer.StartObject();
					const ShaderIRDescriptorRange& range = block.DefaultUniformBuffer;
					writer.Key(SE_TEXT("role"));
					writer.String(range.Role);
					writer.Key(SE_TEXT("set"));
					writer.Uint(range.Set);
					writer.Key(SE_TEXT("binding"));
					writer.Uint(range.Binding);
					writer.Key(SE_TEXT("arrayElementBase"));
					writer.Uint(range.ArrayElementBase);
					writer.Key(SE_TEXT("logicalElementStride"));
					writer.Uint(range.LogicalElementStride);
					writer.Key(SE_TEXT("descriptorCount"));
					writer.Uint(range.DescriptorCount);
					writer.Key(SE_TEXT("descriptorType"));
					writer.String(range.DescriptorType);
					writer.Key(SE_TEXT("stageMask"));
					writer.String(range.StageMask);
					writer.Key(SE_TEXT("flags"));
					WriteStringArray(writer, range.Flags);
					writer.EndObject();
				}
				else
				{
					writer.RawValue("null", 4);
				}

				writer.Key(SE_TEXT("rangeBindings"));
				writer.StartArray();
				for (int32 bindingIndex = 0; bindingIndex < block.RangeBindings.Count(); bindingIndex++)
				{
					const ShaderIRRangeBinding& binding = block.RangeBindings[bindingIndex];
					writer.StartObject();
					writer.Key(SE_TEXT("rangeIndex"));
					writer.Uint(binding.RangeIndex);
					writer.Key(SE_TEXT("flavor"));
					writer.String(binding.Flavor);
					writer.Key(SE_TEXT("subBlockId"));
					if (binding.SubBlockId >= 0)
					{
						writer.Uint(static_cast<uint32>(binding.SubBlockId));
					}
					else
					{
						writer.RawValue("null", 4);
					}
					writer.Key(SE_TEXT("descriptorRanges"));
					writer.StartArray();
					for (int32 descriptorIndex = 0; descriptorIndex < binding.DescriptorRanges.Count(); descriptorIndex++)
					{
						const ShaderIRDescriptorRange& range = binding.DescriptorRanges[descriptorIndex];
						writer.StartObject();
						writer.Key(SE_TEXT("role"));
						writer.String(range.Role);
						writer.Key(SE_TEXT("set"));
						writer.Uint(range.Set);
						writer.Key(SE_TEXT("binding"));
						writer.Uint(range.Binding);
						writer.Key(SE_TEXT("arrayElementBase"));
						writer.Uint(range.ArrayElementBase);
						writer.Key(SE_TEXT("logicalElementStride"));
						writer.Uint(range.LogicalElementStride);
						writer.Key(SE_TEXT("descriptorCount"));
						writer.Uint(range.DescriptorCount);
						writer.Key(SE_TEXT("descriptorType"));
						writer.String(range.DescriptorType);
						writer.Key(SE_TEXT("stageMask"));
						writer.String(range.StageMask);
						writer.Key(SE_TEXT("flags"));
						WriteStringArray(writer, range.Flags);
						writer.EndObject();
					}
					writer.EndArray(binding.DescriptorRanges.Count());
					writer.EndObject();
				}
				writer.EndArray(block.RangeBindings.Count());
				writer.EndObject();
			}
			writer.EndArray(layout.ParameterBlocks.Count());
			writer.EndObject();
		}
	}

	bool SLC2Writer::WriteDeterministic(const SLC2Artifact& artifact, List<byte>& output, String& error)
	{
		output.Clear();

		if (artifact.Format != SE_TEXT("SLC2") || artifact.Version != 4)
		{
			error = SE_TEXT("Invalid SLC2 artifact header.");
			return false;
		}

		Json::StringBuffer buffer;
        JsonWriter& writer = PrettyJsonWriter(buffer);

		writer.StartObject();
		writer.Key(SE_TEXT("format"));
		writer.String(artifact.Format);
		writer.Key(SE_TEXT("version"));
		writer.Uint(artifact.Version);
		writer.Key(SE_TEXT("compilerBuildTag"));
		writer.String(artifact.CompilerBuildTag);

		writer.Key(SE_TEXT("programs"));
		writer.StartArray();
		for (int32 programIndex = 0; programIndex < artifact.Programs.Count(); programIndex++)
		{
			const SLC2ProgramRecord& program = artifact.Programs[programIndex];
			writer.StartObject();
			writer.Key(SE_TEXT("programId"));
			writer.String(program.ProgramId);
			writer.Key(SE_TEXT("variantGroups"));
			WriteVariantGroups(writer, program.VariantGroups);

			writer.Key(SE_TEXT("targets"));
			writer.StartArray();
			for (int32 targetIndex = 0; targetIndex < program.Targets.Count(); targetIndex++)
			{
				const SLC2TargetRecord& target = program.Targets[targetIndex];
				writer.StartObject();
				writer.Key(SE_TEXT("targetKey"));
				writer.String(target.TargetKey);
				writer.Key(SE_TEXT("platform"));
				writer.String(ToString(target.Target.Platform));
				writer.Key(SE_TEXT("shaderProfile"));
				writer.String(ToString(target.Target.Profile));
				writer.Key(SE_TEXT("featureLevel"));
				writer.String(ToString(target.Target.Feature));

				writer.Key(SE_TEXT("variants"));
				writer.StartArray();
				for (int32 variantIndex = 0; variantIndex < target.Variants.Count(); variantIndex++)
				{
					const SLC2VariantRecord& variant = target.Variants[variantIndex];
					writer.StartObject();
					writer.Key(SE_TEXT("variant"));
					writer.String(variant.Variant);
					writer.Key(SE_TEXT("layout"));
					WriteLayout(writer, variant.Layout);

					writer.Key(SE_TEXT("stages"));
					writer.StartArray();
					for (int32 stageIndex = 0; stageIndex < variant.Stages.Count(); stageIndex++)
					{
						const SLC2StageRecord& stage = variant.Stages[stageIndex];
						if (stage.Stage != ShaderStage::Vertex && stage.VertexInputSignature.HasItems())
						{
							error = SE_TEXT("SLC2 non-vertex stage cannot contain a vertex input signature.");
							output.Clear();
							return false;
						}
						writer.StartObject();
						writer.Key(SE_TEXT("stage"));
						writer.String(ToString(stage.Stage));
						writer.Key(SE_TEXT("entryPoint"));
						writer.String(stage.EntryPoint);
						if (stage.Stage == ShaderStage::Vertex)
						{
							writer.Key(SE_TEXT("vertexInputSignature"));
							if (!SLC2VertexInputWriter::WriteVertexInputSignature(
									writer, stage.VertexInputSignature, error))
							{
								output.Clear();
								return false;
							}
						}
						writer.Key(SE_TEXT("outputControlPoints"));
						writer.Uint(static_cast<uint32>(stage.OutputControlPoints));
						writer.Key(SE_TEXT("code"));
						if (stage.Code.Count() > 0)
						{
							writer.Blob(stage.Code.Get(), stage.Code.Count());
						}
						else
						{
							writer.String(SE_TEXT(""));
						}
						writer.Key(SE_TEXT("codeHash"));
						writer.String(ComputeSHA256Hex(stage.Code.Get(), stage.Code.Count()));
						writer.EndObject();
					}
					writer.EndArray(variant.Stages.Count());
					writer.EndObject();
				}
				writer.EndArray(target.Variants.Count());
				writer.EndObject();
			}
			writer.EndArray(program.Targets.Count());
			writer.EndObject();
		}
		writer.EndArray(artifact.Programs.Count());
		writer.EndObject();

		output.Set((const byte*)buffer.GetString(), (int32)buffer.GetSize());
		return true;
	}
}
