# 运行时与 Vulkan

目标：从已存在且已通过编译侧 self-read 的 SLC2 开始，构造运行时反射、名称绑定状态和 Vulkan Program 级 Pipeline Layout。

## 运行时边界

运行时只消费：

- `ShaderProgramArtifact`
- `ShaderReflectionIR`
- Stage code

运行时不得：

- 调用 Slang。
- 从 SPIR-V 二次反射。
- 参与 Program 发现。
- 参与 Variant 规划。
- 参与 Target codegen。
- 参与 SLC2 写入。

## 新增对象

| 文件 | 类型 | 职责 |
| --- | --- | --- |
| `ShaderProgramArtifact.h/.cpp` | `ShaderProgramArtifact` | 不可变 SLC2 Program/Target/Variant/Stage 数据 |
| `ShaderProgramReflection.h/.cpp` | `ShaderProgramReflection` | type/block 索引和物理布局描述 |
| `ShaderVar.h/.cpp` | `ShaderVar` | 成员和固定数组寻址 |
| `ShaderNameResolver.h/.cpp` | `ShaderNameResolver` | 解析 `a.b[2].c` |
| `ShaderParameterBlockState.h/.cpp` | `ShaderParameterBlockState` | uniform bytes 和逻辑资源槽 |
| `ShaderProgramInstance.h/.cpp` | `ShaderProgramInstance` | 完整 Program 实例 |
| `ShaderProgramLayoutVulkan.h/.cpp` | `ShaderProgramLayoutVulkan` | Vulkan layout 创建/复用 |
| `ShaderDescriptorWriterVulkan.h/.cpp` | `ShaderDescriptorWriterVulkan` | descriptor write |

## 过渡策略

- 旧 `GPUShaderProgram` 仍表示单 Stage。
- 新增完整 `ShaderProgramInstance` 表示 graphics/compute Program。
- 新 Slang Shader 资产显式加载 SLC2。
- 不通过文件内容嗅探在旧 cache 和 SLC2 之间分流。
- 新路径可复用旧 Vulkan stage module 所有者，但不读取旧 `ShaderBindings` 或 `SpirvShaderDescriptorInfo`。

## Program / Target / Variant 选择

```cpp
struct ShaderProgramSelection
{
    StringAnsi ProgramId;
    ShaderCompileTarget Target;
    Array<StringAnsi> Defines;
};
```

选择规则：

- ProgramId 精确、区分大小写匹配。
- Target 使用 `platform/backend/shaderModel` 精确匹配。
- 不选择最接近 ShaderModel。
- Defines 输入无序。
- 使用 SLC2 中 `variantGroups` 执行与编译期相同的 `NormalizeVariant`。
- 未知宏、同组多选、带值宏或 Variant 不存在均失败。
- 不做运行时 specialization。

```text
Select(programId, target, defines):
  program = programsById.FindExact(programId)
  canonicalTarget = CanonicalizeTarget(target)
  target = program.TargetsByKey.Find(canonicalTarget.Key)
  variantText = NormalizeVariant(program.VariantGroups, defines)
  variant = target.VariantsByCanonicalText.Find(variantText)
  validate stage contract
  return ProgramSelection(program, target, variant)
```

## ShaderVarLocation

```cpp
struct ShaderVarLocation
{
    uint32 BlockId;
    Optional<uint32> UniformByteOffset;
    Optional<uint32> ResourceRangeIndex;
    Optional<uint32> ResourceIndex;
    uint32 TypeId;
};
```

规则：

- Struct 成员 offset 相对直接父类型。
- 普通数组：`uniform += index * elementByteStride`。
- 资源数组：`resourceIndex = resourceIndex * elementCount + index`。
- CB/PB 成员通过父 block range 切换到 `SubBlockId`。
- 切换子块后从子块根重新累计。
- Location 不包含 blockIndex，因为块数组被禁止。

## 路径语法

```text
identifier ("." identifier | "[" unsignedInteger "]")*
```

允许：

```text
inputTexture
material.baseColorTexture
textures[3]
materials[2].baseColorTexture
```

拒绝：

```text
textures[-1]
textures[i]
textures[]
.material
material..texture
```

解析失败必须包含已解析前缀。

## 名称解析流程

```text
Resolve(path):
  tokens = ParsePathGrammar(path)
  cursor = root block + root element type + zero offsets

  for token in tokens:
    if Member:
      require current type is Struct
      find exact member
      add uniform/resource offsets
      switch typeId

      if member type is ParameterBlock:
        mapping = currentBlock.RangeBindings[cursor.ResourceRange]
        require mapping is block flavor
        cursor.BlockId = mapping.SubBlockId
        cursor.TypeId = subBlock.ElementTypeId
        reset offsets to zero

    if Index:
      require current type is Array
      check index < elementCount
      uniform += index * elementByteStride
      resourceIndex = resourceIndex * elementCount + index
      typeId = elementTypeId

  return FinalizeLeafLocation(cursor)
```

路径成功后可缓存规范路径到 `ShaderVarLocation`。缓存不写入 SLC2。

## 资源与 Uniform API

```cpp
bool SetTexture(StringView path, GPUTextureView* value, String& error);
bool SetBuffer(StringView path, GPUBuffer* value, String& error);
bool SetSampler(StringView path, GPUSampler* value, String& error);
bool SetUniform(StringView path, const void* data, uint32 size, String& error);
```

规则：

- 先解析路径，再校验类型。
- 设置 `null` 等价于未绑定。
- 没有 required/optional 策略。
- 没有默认资源库。
- 主资源绑定自动覆盖其 `auxiliaryRanges`。
- 用户不能按 internal role 名直接绑定。
- 固定资源数组支持逐元素绑定。
- Uniform 写入只复制到对应 block 的 CPU byte array。

## 绑定状态

```text
ShaderParameterBlockState
  uniformData[uniformByteSize]
  resources[logicalRange][logicalElement]
  subBlocks[]
```

状态按逻辑位置保存，不按 Vulkan binding 保存。后端提交时才映射到 descriptor。

## 提交前完整性

```text
ValidateAllBindings:
  pass 1:
    walk reachable blocks
    for each simple logical range
      skip internalRole ranges
      require every resource element is non-null
      require object still alive and compatible
      add to snapshot

  pass 2:
    for each block with defaultUniformBuffer
      materialize uniform GPU buffer
      add to snapshot

  return frozen snapshot
```

任何缺失资源都必须在构造 `VkWriteDescriptorSet` 前失败。

## Vulkan Pipeline Layout

旧路径问题：

- 当前 Vulkan 以 Stage 为单位调用 `AddBindingsForStage`。
- 新契约要求完整 Program 的共享 Layout。
- 新路径不能从每 Stage descriptor info 合成布局。

新流程：

1. 从 `ShaderProgramReflection` 收集规范物理 descriptor。
2. 重算 fingerprint。
3. 用 fingerprint 查 layout cache。
4. Hash 命中后深比较规范物理记录。
5. 按 set 分组生成 `VkDescriptorSetLayoutBinding`。
6. 空 set 也创建空 DescriptorSetLayout。
7. 创建 `VkPipelineLayout`，push constant count 为 0。

禁止：

- `binding = descriptorIndex`
- Stage index 当 descriptor set
- SPIR-V 二次反射覆盖 SLC2

## Descriptor 写入

公式：

```text
dstSet = descriptorSets[mapping.set]
dstBinding = mapping.binding
dstArrayElement =
  mapping.arrayElementBase
  + logicalResourceIndex * mapping.logicalElementStride
```

规则：

- 资源数组只改变 `dstArrayElement`。
- 不把数组下标加到 binding。
- 一条逻辑 binding 的所有物理 mapping 必须同一提交更新。
- descriptorType 决定 image/buffer/texel payload。
- Stage mask 只用于 layout 创建，不影响 write。
- payload 存储必须活到 `vkUpdateDescriptorSets` 返回。

## Pipeline 接入

Compute：

- Program Variant 只能包含 CS。
- 从 Stage code 创建 VkShaderModule。
- 使用 SLC2 `entryPoint`。
- 从共享 Layout 创建 compute pipeline。
- Dispatch 前执行绑定完整性、descriptor 写入和 set bind。

Graphics：

- Program Variant 包含 VS/PS，可选 HS/DS/GS。
- 所有 Stage 共享同一 VkPipelineLayout。
- HS/DS 成对和 PS 必须存在已由编译/Reader 验证。
- PipelineState 持有固定功能状态。
- Program 可被多个兼容 PipelineState 复用。

提交：

```text
PrepareAndBind:
  require pipeline fingerprint == program layout fingerprint
  snapshot = rootParameters.ValidateAllBindings()
  sets = AllocateDescriptorSets(layout)
  DescriptorWriter.BuildAndCommit(snapshot, sets)
  BindPipeline
  BindDescriptorSets
```

## 错误分层

| 阶段 | 示例 | 行为 |
| --- | --- | --- |
| Artifact 加载 | JSON/schema/hash/fingerprint 无效 | 整份失败 |
| 选择 | Program/Target/Variant 不存在 | 不创建实例 |
| 名称解析 | 成员不存在、数组越界、类型不符 | Set 失败，旧值不变 |
| 提交 | 资源未绑定/null | 中止 Draw/Dispatch |
| Vulkan 创建 | 设备上限、descriptor 类型不支持 | Program 创建失败 |

## 当前不展开

- Cook/发布。
- DependencyIndex。
- 默认资源库。
- descriptor 生命周期优化。
- 热重载交换。
- 动态 specialization。
- Push Constant。
- 无界 descriptor。

