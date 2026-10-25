#include "ShaderNameResolver.h"

#include "ShaderProgramReflection.h"

namespace SE
{
	namespace
	{
		bool IsResourceLeafType(const ShaderIRTypeRecord& type)
		{
			return type.Kind == SE_TEXT("Resource") || type.Kind == SE_TEXT("SamplerState") ||
				type.Kind == SE_TEXT("TextureBuffer") || type.Kind == SE_TEXT("ShaderStorageBuffer");
		}

		bool ResolvePrecomputedAddress(const ShaderProgramReflection& reflection, ShaderVarLocation& location, String& msg)
		{
			const ShaderIRTypeRecord* type = reflection.GetType(location.TypeId);
			if (type == nullptr)
			{
				msg = SE_TEXT("Shader variable type was not found while resolving its address.");
				return false;
			}
			location.LogicalResourceSlotIndex = -1;
			location.UniformByteSize = 0;

			if (IsResourceLeafType(*type))
			{
				if (location.ResourceRangeIndex < 0 || location.ResourceIndex < 0)
				{
					msg = SE_TEXT("Shader resource leaf has no logical resource coordinates.");
					return false;
				}
				ShaderResourceKey key;
				key.BlockId = location.BlockId;
				key.RangeIndex = static_cast<uint32>(location.ResourceRangeIndex);
				key.ResourceIndex = static_cast<uint32>(location.ResourceIndex);
				uint32 slotIndex;
				if (!reflection.FindLogicalResourceSlotIndex(key, slotIndex))
				{
					msg = SE_TEXT("Shader resource NamePath does not map to a user-visible logical slot.");
					return false;
				}
				location.Kind = ShaderParameterAddressKind::Resource;
				location.LogicalResourceSlotIndex = static_cast<int32>(slotIndex);
				return true;
			}

			if (type->Kind == SE_TEXT("ParameterBlock") || type->Kind == SE_TEXT("ConstantBuffer"))
			{
				// 容器类型会在 ResolveMember 中切换到对应 sub-block。
				return true;
			}
			if (type->Kind == SE_TEXT("Struct") || type->Kind == SE_TEXT("Array"))
			{
				// 结构体和数组只负责继续导航；整块写入必须显式解析到 UniformBlock。
				return true;
			}
			if (location.UniformBlockIndex >= 0 && location.UniformByteOffset >= 0 && type->UniformByteSize != 0)
			{
				location.Kind = ShaderParameterAddressKind::UniformValue;
				location.UniformByteSize = type->UniformByteSize;
			}
			else
			{
				location.Kind = ShaderParameterAddressKind::Invalid;
			}
			return true;
		}

		bool ReadIdentifier(const String& path, int32& cursor, String& identifier)
		{
			const int32 start = cursor;
			while (cursor < path.Length())
			{
				const Char value = path[cursor];
				if (!((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9') || value == '_'))
				{
					break;
				}
				cursor++;
			}
			if (cursor == start)
			{
				return false;
			}
			identifier = String(*path + start, cursor - start);
			return true;
		}

		bool ReadIndex(const String& path, int32& cursor, uint32& index)
		{
			if (cursor >= path.Length() || path[cursor] != '[')
			{
				return false;
			}
			cursor++;
			const int32 digitsStart = cursor;
			index = 0;
			while (cursor < path.Length() && path[cursor] >= '0' && path[cursor] <= '9')
			{
				const uint32 digit = static_cast<uint32>(path[cursor] - '0');
				if (index > (Max_uint32 - digit) / 10)
				{
					return false;
				}
				index = index * 10 + digit;
				cursor++;
			}
			if (cursor == digitsStart || cursor >= path.Length() || path[cursor] != ']')
			{
				return false;
			}
			cursor++;
			return true;
		}

		ShaderVarLocation MakeRootLocation(const ShaderProgramReflection& reflection)
		{
			const ShaderReflectionIR& layout = reflection.GetLayout();
			const ShaderIRParameterBlock* root = reflection.GetBlock(layout.RootBlockId);
			ShaderVarLocation location;
			location.BlockId = root->Id;
			location.TypeId = root->ElementTypeId;
			uint32 uniformBlockIndex;
			if (reflection.FindUniformBlockIndex(root->Id, uniformBlockIndex))
			{
				location.Kind = ShaderParameterAddressKind::UniformBlock;
				location.UniformBlockIndex = static_cast<int32>(uniformBlockIndex);
				location.UniformByteOffset = 0;
				location.UniformByteSize = root->UniformByteSize;
			}
			location.ResourceRangeIndex = -1;
			location.ResourceIndex = -1;
			return location;
		}
	}

	bool ShaderNameResolver::ResolveMember(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, const String& name, ShaderVarLocation& location, String& msg)
	{
		const ShaderIRTypeRecord* parentType = reflection.GetType(source.TypeId);
		if (parentType == nullptr || parentType->Kind != SE_TEXT("Struct"))
		{
			msg = SE_TEXT("Shader variable member access requires a struct.");
			return false;
		}
		const ShaderIRMember* member = reflection.FindMember(source.TypeId, name);
		if (member == nullptr)
		{
			msg = String::Format(SE_TEXT("Shader variable member was not found: {0}"), name);
			return false;
		}
		const ShaderIRTypeRecord* memberType = reflection.GetType(member->TypeId);
		if (memberType == nullptr)
		{
			msg = SE_TEXT("Shader variable member type was not found.");
			return false;
		}

		location = source;
		location.TypeId = member->TypeId;
		location.Kind = ShaderParameterAddressKind::Invalid;
		location.UniformByteSize = 0;
		location.LogicalResourceSlotIndex = -1;
		if (location.UniformByteOffset >= 0)
		{
			location.UniformByteOffset += static_cast<int32>(member->UniformOffset);
		}

		if (memberType->ResourceRanges.HasItems())
		{
			location.ResourceRangeIndex = (source.ResourceRangeIndex < 0 ? 0 : source.ResourceRangeIndex) + static_cast<int32>(member->ResourceRangeOffset);
			location.ResourceIndex = (source.ResourceIndex < 0 ? 0 : source.ResourceIndex) + static_cast<int32>(member->ResourceIndexOffset);
		}
		else
		{
			location.ResourceRangeIndex = -1;
			location.ResourceIndex = -1;
		}

		if (memberType->Kind != SE_TEXT("ParameterBlock") && memberType->Kind != SE_TEXT("ConstantBuffer"))
		{
			return ResolvePrecomputedAddress(reflection, location, msg);
		}

		if (location.ResourceRangeIndex < 0)
		{
			msg = SE_TEXT("Shader parameter block has no logical range.");
			return false;
		}

		const ShaderIRRangeBinding* binding = reflection.FindRangeBinding(location.BlockId, static_cast<uint32>(location.ResourceRangeIndex));
		if (binding == nullptr || binding->SubBlockId < 0)
		{
			msg = SE_TEXT("Shader parameter block has no sub-block mapping.");
			return false;
		}

		const ShaderIRParameterBlock* subBlock = reflection.GetBlock(static_cast<uint32>(binding->SubBlockId));
		if (subBlock == nullptr)
		{
			msg = SE_TEXT("Shader parameter block mapping is invalid.");
			return false;
		}

		location.BlockId = subBlock->Id;
		location.TypeId = subBlock->ElementTypeId;
		location.Kind = ShaderParameterAddressKind::Invalid;
		location.UniformBlockIndex = -1;
		location.UniformByteOffset = -1;
		uint32 uniformBlockIndex;

		if (reflection.FindUniformBlockIndex(subBlock->Id, uniformBlockIndex))
		{
			location.Kind = ShaderParameterAddressKind::UniformBlock;
			location.UniformBlockIndex = static_cast<int32>(uniformBlockIndex);
			location.UniformByteOffset = 0;
			location.UniformByteSize = subBlock->UniformByteSize;
		}
		location.LogicalResourceSlotIndex = -1;
		location.ResourceRangeIndex = -1;
		location.ResourceIndex = -1;
		return true;
	}

	bool ShaderNameResolver::ResolveIndex(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, const uint32 index, ShaderVarLocation& location, String& msg)
	{
		const ShaderIRTypeRecord* type = reflection.GetType(source.TypeId);
		if (type == nullptr || type->Kind != SE_TEXT("Array") || index >= type->ElementCount)
		{
			msg = SE_TEXT("Shader variable array index is out of range.");
			return false;
		}
		location = source;
		location.Kind = ShaderParameterAddressKind::Invalid;
		location.UniformByteSize = 0;
		location.LogicalResourceSlotIndex = -1;
		if (location.UniformByteOffset >= 0)
		{
			location.UniformByteOffset += static_cast<int32>(index * type->ElementByteStride);
		}
		if (location.ResourceIndex >= 0)
		{
			location.ResourceIndex = location.ResourceIndex * static_cast<int32>(type->ElementCount) + static_cast<int32>(index);
		}
		location.TypeId = type->ElementTypeId;
		return ResolvePrecomputedAddress(reflection, location, msg);
	}

	bool ShaderNameResolver::Resolve(const ShaderProgramReflection& reflection, const String& path, ShaderVarLocation& location, String& msg)
	{
		const ShaderReflectionIR& layout = reflection.GetLayout();
		if (reflection.GetBlock(layout.RootBlockId) == nullptr || path.IsEmpty())
		{
			LOG_ERROR("Graphics", "Shader variable path is empty or reflection root is invalid.");
			return false;
		}

		location = MakeRootLocation(reflection);
		int32 cursor = 0;
		bool expectMember = true;
		while (cursor < path.Length())
		{
			if (expectMember)
			{
				String name;
				if (!ReadIdentifier(path, cursor, name) || !ResolveMember(reflection, location, name, location, msg))
				{
					return false;
				}
				expectMember = false;
			}
			else if (path[cursor] == '.')
			{
				cursor++;
				expectMember = true;
			}
			else if (path[cursor] == '[')
			{
				uint32 index = 0;
				if (!ReadIndex(path, cursor, index) || !ResolveIndex(reflection, location, index, location, msg))
				{
					return false;
				}
			}
			else
			{
				LOG_ERROR("Graphics", "Invalid shader variable path separator.");
				return false;
			}
		}
		return !expectMember;
	}

	bool ShaderNameResolver::ResolveUniformBlock(const ShaderProgramReflection& reflection, const String& path, ShaderVarLocation& location, String& msg)
	{
		if (reflection.GetBlock(reflection.GetLayout().RootBlockId) == nullptr)
		{
			LOG_ERROR("Graphics", "Shader reflection root is invalid.");
			return false;
		}
		if (path.IsEmpty())
		{
			location = MakeRootLocation(reflection);
		}
		else if (!Resolve(reflection, path, location, msg))
		{
			return false;
		}
		if (location.Kind != ShaderParameterAddressKind::UniformBlock || location.UniformBlockIndex < 0)
		{
			LOG_ERROR("Graphics", "Shader NamePath does not identify a uniform block.");
			return false;
		}
		return true;
	}
}
