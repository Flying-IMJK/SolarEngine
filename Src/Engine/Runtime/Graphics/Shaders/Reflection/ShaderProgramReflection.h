#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"
#include "Runtime/ShaderCompilation/Slang/ShaderReflectionIR.h"
#include "ShaderVar.h"

namespace SE
{
	enum class ShaderProgramBindingKind : byte
	{
		Uniform,
		Texture,
		Buffer,
		Sampler,
	};

	struct SE_API_RUNTIME ShaderResourceKey
	{
		uint32 BlockId = 0;
		uint32 RangeIndex = 0;
		uint32 ResourceIndex = 0;

		bool operator==(const ShaderResourceKey& other) const
		{
			return BlockId == other.BlockId && RangeIndex == other.RangeIndex && ResourceIndex == other.ResourceIndex;
		}
	};

	inline uint32 GetHash(const ShaderResourceKey& key)
	{
		uint32 hash = GetHash(key.BlockId);
		hash = HashCombine(hash, key.RangeIndex);
		return HashCombine(hash, key.ResourceIndex);
	}

	struct SE_API_RUNTIME ShaderUniformBlockLayout
	{
		uint32 BlockId = 0;
		uint32 TypeId = 0;
		uint32 ByteSize = 0;
	};

	struct SE_API_RUNTIME ShaderLogicalResourceSlot
	{
		ShaderResourceKey Key;
		uint32 TypeId = Max_uint32;
		ShaderProgramBindingKind Kind = ShaderProgramBindingKind::Texture;
	};

	struct SE_API_RUNTIME ShaderProgramBinding
	{
		String Path;
		String TypeName;
        ShaderProgramBindingKind Kind = ShaderProgramBindingKind::Uniform;
        ShaderVarLocation        Location;
        uint32                   UniformByteSize = 0;
        bool                     IsArrayElement  = false;
    };

    class SE_API_RUNTIME ShaderProgramReflection
    {
    public:
        // layout 由所属 SLC2 Program 持有，并且必须在本对象的整个使用期间保持存活且不可变。
        bool Initialize(const ShaderReflectionIR& layout);
        // 仅在 Initialize 成功后调用；按稳定 NamePath 返回可绑定叶子，output 由调用方持有。
        bool                            EnumerateBindings(List<ShaderProgramBinding>& output) const;
        // These indices are stable for the immutable layout supplied to Initialize; calls require successful Initialize.
        bool                            FindUniformBlockIndex(uint32 blockId, uint32& output) const;
        uint32                          GetUniformBlockCount() const;
        // Returned layout pointers remain valid while this Reflection and its source Program layout remain alive.
        const ShaderUniformBlockLayout* GetUniformBlockLayout(uint32 index) const;
        bool FindLogicalResourceSlotIndex(const ShaderResourceKey& key, uint32& output) const;
        uint32 GetLogicalResourceSlotCount() const;
        // Returned slot pointers remain valid for the lifetime of this immutable Reflection.
        const ShaderLogicalResourceSlot* GetLogicalResourceSlot(uint32 index) const;
        // 仅返回指定逻辑槽的分类；slotIndex 来自同一 Reflection 的 NamePath 解析结果。
        bool TryGetLogicalResourceSlotKind(uint32 slotIndex, ShaderProgramBindingKind& output) const;

        const ShaderReflectionIR&     GetLayout() const;
        const ShaderIRTypeRecord* GetType(uint32 typeId) const;
		const ShaderIRParameterBlock* GetBlock(uint32 blockId) const;
		const ShaderIRMember* FindMember(uint32 typeId, const String& name) const;
		const ShaderIRRangeBinding* FindRangeBinding(uint32 blockId, uint32 rangeIndex) const;

	private:
		void Clear();
		bool BuildPrecomputedLayout();
		bool BuildBlockLayout(uint32 blockId, List<byte>& visitState);
		bool ResolveResourceSlotTypes();

		const ShaderReflectionIR* m_Layout = nullptr;
		List<ShaderUniformBlockLayout> m_UniformBlocks;
		List<int32> m_BlockToUniformBlockIndex;
		List<ShaderLogicalResourceSlot> m_LogicalResourceSlots;
		Dictionary<ShaderResourceKey, uint32> m_LogicalResourceSlotLookup;
    };
}
