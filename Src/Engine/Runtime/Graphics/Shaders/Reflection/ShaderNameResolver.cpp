#include "ShaderNameResolver.h"

#include "ShaderProgramReflection.h"

namespace SE
{
	namespace
	{
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
			location.UniformByteOffset = 0;
			location.ResourceRangeIndex = -1;
			location.ResourceIndex = -1;
			return location;
		}
	}

	bool ShaderNameResolver::ResolveMember(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, const String& name, ShaderVarLocation& location)
	{
		const ShaderIRTypeRecord* parentType = reflection.GetType(source.TypeId);
		if (parentType == nullptr || parentType->Kind != SE_TEXT("Struct"))
		{
			LOG_ERROR("Graphics", "Shader variable member access requires a struct.");
			return false;
		}
		const ShaderIRMember* member = reflection.FindMember(source.TypeId, name);
		if (member == nullptr)
		{
			LOG_ERROR("Graphics", "Shader variable member was not found: {0}", name);
			return false;
		}
		const ShaderIRTypeRecord* memberType = reflection.GetType(member->TypeId);
		if (memberType == nullptr)
		{
			LOG_ERROR("Graphics", "Shader variable member type was not found.");
			return false;
		}

		location = source;
		location.TypeId = member->TypeId;
		if (location.UniformByteOffset >= 0)
		{
			location.UniformByteOffset += static_cast<int32>(member->UniformOffset);
		}
		if (memberType->ResourceRanges.HasItems())
		{
			location.ResourceRangeIndex = (source.ResourceRangeIndex < 0 ? 0 : source.ResourceRangeIndex) + static_cast<int32>(member->ResourceRangeOffset);
			location.ResourceIndex = (source.ResourceIndex < 0 ? 0 : source.ResourceIndex) + static_cast<int32>(member->ResourceIndexOffset);
		}
		if (memberType->Kind != SE_TEXT("ParameterBlock") && memberType->Kind != SE_TEXT("ConstantBuffer"))
		{
			return true;
		}

		if (location.ResourceRangeIndex < 0)
		{
			LOG_ERROR("Graphics", "Shader parameter block has no logical range.");
			return false;
		}
		const ShaderIRRangeBinding* binding = reflection.FindRangeBinding(location.BlockId, static_cast<uint32>(location.ResourceRangeIndex));
		if (binding == nullptr || binding->SubBlockId < 0)
		{
			LOG_ERROR("Graphics", "Shader parameter block has no sub-block mapping.");
			return false;
		}
		const ShaderIRParameterBlock* subBlock = reflection.GetBlock(static_cast<uint32>(binding->SubBlockId));
		if (subBlock == nullptr)
		{
			LOG_ERROR("Graphics", "Shader parameter block mapping is invalid.");
			return false;
		}
		location.BlockId = subBlock->Id;
		location.TypeId = subBlock->ElementTypeId;
		location.UniformByteOffset = 0;
		location.ResourceRangeIndex = -1;
		location.ResourceIndex = -1;
		return true;
	}

	bool ShaderNameResolver::ResolveIndex(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, const uint32 index, ShaderVarLocation& location)
	{
		const ShaderIRTypeRecord* type = reflection.GetType(source.TypeId);
		if (type == nullptr || type->Kind != SE_TEXT("Array") || index >= type->ElementCount)
		{
			LOG_ERROR("Graphics", "Shader variable array index is out of range.");
			return false;
		}
		location = source;
		if (location.UniformByteOffset >= 0)
		{
			location.UniformByteOffset += static_cast<int32>(index * type->ElementByteStride);
		}
		if (location.ResourceIndex >= 0)
		{
			location.ResourceIndex = location.ResourceIndex * static_cast<int32>(type->ElementCount) + static_cast<int32>(index);
		}
		location.TypeId = type->ElementTypeId;
		return true;
	}

	bool ShaderNameResolver::Resolve(const ShaderProgramReflection& reflection, const String& path, ShaderVarLocation& location)
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
				if (!ReadIdentifier(path, cursor, name) || !ResolveMember(reflection, location, name, location))
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
				if (!ReadIndex(path, cursor, index) || !ResolveIndex(reflection, location, index, location))
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
}
