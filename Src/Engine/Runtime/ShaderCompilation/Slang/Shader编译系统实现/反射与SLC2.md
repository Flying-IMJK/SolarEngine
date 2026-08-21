# 反射 IR 与 SLC2

目标：编译期从 Slang `ProgramLayout` 生成引擎自有 `ShaderReflectionIR`，再写入 SLC2。运行时只读 SLC2，不依赖 Slang 或 SPIR-V 二次反射。

## 组件边界

```text
slang::ProgramLayout
  -> SlangReflectionBuilder
  -> ShaderReflectionIR
  -> ShaderReflectionValidator
  -> SLC2Writer
  -> SLC2Reader self-read
```

```text
SLC2 Text
  -> SLC2Reader
  -> ShaderProgramArtifact
  -> Runtime Reflection
```

只有 `SlangReflectionBuilder` 可 include `slang.h`。共享类型、Validator、Writer、Reader 都不得依赖 Slang。

## ShaderReflectionIR

```cpp
struct ShaderReflectionIR
{
    uint32 Schema = 2;
    StringAnsi PipelineLayoutFingerprint;
    uint32 RootBlockId;
    Array<ShaderTypeRecord> Types;
    Array<ShaderParameterBlockRecord> ParameterBlocks;
};

struct ShaderParameterBlockRecord
{
    uint32 Id;
    StringAnsi Name;
    uint32 ElementTypeId;
    uint32 UniformByteSize;
    Optional<ShaderDescriptorRange> DefaultUniformBuffer;
    Array<ShaderRangeBinding> RangeBindings;
};
```

规则：

- 每个 `ProgramId x TargetKey x Variant` 一份 IR。
- 全部 Stage 共享同一 IR。
- 只有一个 `RootBlockId`。
- 没有 EntryPoint block。
- `Types` 是可去重逻辑类型图。
- `ParameterBlocks` 是具体出现位置，不能因 `elementTypeId` 相同而合并。
- ID 连续，从 0 开始。

## Type Record

五种稳定 kind：

- `Basic`
- `Struct`
- `Array`
- `Resource`
- `ParameterBlock`

公共字段：

- `id`
- `kind`
- `uniformByteSize`
- `resourceRanges[]`

关键分支：

```text
Struct:
  name?
  members[]:
    name
    typeId
    offset.uniformByte?
    offset.resourceRange?
    offset.resourceIndex?

Array:
  elementTypeId
  elementCount
  elementByteStride

Resource:
  resourceKind
  shape
  access
  elementTypeId?
  imageFormat?
  structuredKind?
  auxiliaryRanges[]

ParameterBlock:
  containerKind = ConstantBuffer | ParameterBlock
  elementTypeId
```

成员偏移永远相对直接父类型。运行时逐层累加。

## Logical Range 与 Physical Mapping

```cpp
struct ShaderResourceRange
{
    ShaderLogicalResourceKind ResourceKind;
    uint32 Count;
    uint32 BaseIndex;
    Optional<ShaderInternalRole> InternalRole;
    Optional<uint32> OwnerRangeOffset;
};

struct ShaderRangeBinding
{
    uint32 RangeIndex;
    ShaderRangeFlavor Flavor;
    Array<ShaderDescriptorRange> DescriptorRanges;
    Optional<uint32> SubBlockId;
};

struct ShaderDescriptorRange
{
    ShaderDescriptorRole Role;
    uint32 Set;
    uint32 Binding;
    uint32 ArrayElementBase;
    uint32 LogicalElementStride;
    ShaderDescriptorType DescriptorType;
    uint32 DescriptorCount;
    ShaderStageMask StageMask;
    ShaderDescriptorFlags Flags;
};
```

规则：

- `resourceRanges[]` 是逻辑范围。
- `rangeBindings[]` 是当前 block 对逻辑 range 的落点。
- 一条 simple range 可映射 1-N 个 descriptor。
- `ConstantBuffer` / `ParameterBlock` range 必须映射一个 `SubBlockId`。
- `defaultUniformBuffer` 使用同一物理字段集合，Role 固定为 `uniformData`。
- Schema 2 固定：
  - `ArrayElementBase = 0`
  - `LogicalElementStride = 1`
  - `DescriptorCount = logical range.Count`

## SlangReflectionBuilder

输入：

- 已 linked 的 `IComponentType`
- 当前 Program/Target/Variant 上下文
- `ProgramLayout`

输出：

- `ShaderReflectionIR`
- 错误文本

Builder 必须：

- 先检查 EntryPoint 参数类别。
- 从 `ProgramLayout::getGlobalParamsVarLayout()` 作为唯一根开始。
- 不用逐参数 API 拼全局作用域。
- 立即复制所有需要的数据。
- 不把 Slang 指针、原生枚举或生命周期泄露到 IR。

## BindingPath

`BindingPath` 是 Builder 内部算法状态，不写入 SLC2。

```cpp
struct BindingPathNode
{
    const slang::VariableLayoutReflection* VarLayout;
    const BindingPathNode* Outer;
};

struct BindingPath
{
    const BindingPathNode* Leaf;
    const BindingPathNode* DeepestConstantBuffer;
    const BindingPathNode* DeepestParameterBlock;
};

struct BlockLayoutContext
{
    const slang::TypeLayoutReflection* ContainerTypeLayout;
    const slang::TypeLayoutReflection* ElementTypeLayout;
    Optional<BindingPath> ContainerPath;
    BindingPath ElementPath;
};
```

规则：

- Node 地址必须稳定，使用 chunked arena。
- `Outer` 指向更外层。
- deepest 指针必须为空或属于同一链。
- 数组下标、range index、descriptor index 不能伪装成 Node。
- 枚举某块元素 TypeLayout 的 descriptor ranges 时，owner path 只到 `ElementPath`，不能再追加 leaf variable。

类别累计：

```text
Resolve(Uniform):
  accumulate value until DeepestConstantBuffer

Resolve(SpaceAware):
  accumulate value/space until DeepestParameterBlock
  then accumulate SubElementRegisterSpace through outer PB chain
```

Vulkan 只接受 `DescriptorTableSlot` 作为 descriptor range category。

## Type Graph 构建

两阶段：

1. 复制 Slang 信息并验证，生成 raw type graph。
2. 计算结构 key，按 root-first 首次可达顺序分配连续 ID。

拒绝：

- unknown/unbounded size
- unsupported type kind
- pointer
- unresolved interface/generic
- recursive illegal graph
- block array
- array element closure contains `ParameterBlock`

## Logical Resource Range 归一化

按类型生成 normalized ranges：

- Basic：空。
- Resource：读取直接 leaf ranges。
- ParameterBlock：一条 count=1 容器 range。
- Struct：按声明顺序拼接字段 ranges，并用 Slang field binding offset 复核。
- Array：复制元素 ranges，`Count *= elementCount`。

然后：

- `BaseIndex` 是 count 前缀和。
- reflected binding range count 必须等于 normalized count。
- 每个 reflected count 和 logical kind 必须兼容。

Counter：

- Counter range 的 `InternalRole = Counter`。
- `OwnerRangeOffset` 指向当前 Type Record 内主 range 的绝对局部索引。
- Resource 的 `AuxiliaryRanges.RangeOffset` 是从主 range 到 counter range 的正向偏移。

## ParameterBlock 构建

规则：

- root block ID 为 0。
- 每个具体 CB/PB 出现位置创建独立 block。
- `uniformByteSize > 0` 时必须有 `defaultUniformBuffer`。
- `uniformByteSize == 0` 时 `defaultUniformBuffer = null`。
- `rangeBindings[]` 数量必须等于 element type 的 `resourceRanges[]` 数量。
- block flavor 必须有一个 `SubBlockId` 且没有 descriptor ranges。
- simple flavor 必须有 descriptor ranges 且没有 `SubBlockId`。
- 块图无环；非根 block 单父。

Sub-object 是独立索引域：

- 使用 `getSubObjectRangeBindingRangeIndex()` 建立反向索引。
- CB/PB range 必须恰好一个 sub-object。
- `getSubObjectRangeOffset()` 用于进入子块。
- `getSubObjectRangeSpaceOffset()` 只检查 finite，不参与重复累加。

StructuredBuffer：

- 可有一个 Slang shader-object sub-object。
- 只做结构校验。
- 不创建引擎 `SubBlockId`。

## Stage mask

从 linked component 的 `getEntryPointMetadata()` 查询：

- 以绝对 `(set, binding)` 判断每个 Stage 是否使用。
- 同一物理 binding 多 Stage 出现时，descriptor type/count/flags 必须一致。
- Stage mask 取并集。
- metadata 查询失败是编译失败。
- 如果 declared 资源被优化到所有 Stage 都未使用，Stage mask 使用完整 Program Stage 集合作为确定性上界。

## IR Validator

Writer 和 Reader 共用同一个 Validator。

必须验证：

- `schema == 2`
- type/block ID 连续、唯一、范围内
- 全部 type/block 从 root 可达
- root 单根
- 非根 block 单父
- Tagged union 字段严格
- 稳定枚举已知
- size/offset/stride 在 `uint32`
- array count 有限且大于 0
- 无 block array
- 无 Push Constant
- 无 EntryPoint uniform/resource 参数
- `uniformByteSize == 0` 等价于 `defaultUniformBuffer == null`
- rangeIndex 完整覆盖
- `BaseIndex` 为 count 前缀和
- descriptor count 等于 logical count
- physical `(set,binding)` 写入所有者唯一
- fingerprint 重算一致

## SLC2 聚合模型

```text
SLC2Artifact
  format
  programs[]
    programId
    variantGroups[]
    targets[]
      meta { platform, backend, shaderModel }
      variants[]
        variant
        layout
        stages[]
          entryPoint
          stage
          code
```

不写：

- shaderId
- source path
- import path
- DependencyIndex
- diagnostics
- names[]
- 默认资源
- runtime lifecycle 信息
- Slang/Vulkan 原生枚举

## Writer

要求：

- 使用固定字段顺序。
- UTF-8 without BOM。
- LF 换行，最终换行。
- Base64 不换行。
- code 同时写 `byteLength` 和 SHA-256。
- 写入前执行 IR Validator。
- 写入后立即用共享无后端 Reader self-read。
- Writer 不写正式文件。

## Reader

阶段：

1. 严格 JSON parse。
2. 校验 magic/major。
3. 逐字段读取并范围检查。
4. 解码 Stage code 并验证 hash。
5. 执行 IR Validator。
6. 重算 fingerprint。
7. 构造不可变 `ShaderProgramArtifact`。

Reader 不调用 Slang，不反射 SPIR-V，不尝试修复旧 cache。

## PipelineLayoutFingerprint

输入：

- 所有可达 block 的 `defaultUniformBuffer`
- 所有 simple range 的 `descriptorRanges[]`

规范化：

- default UBO 转成普通物理 descriptor 记录。
- 按 `(set,binding)` 合并。
- descriptorType/count/flags/immutable sampler 必须一致。
- Stage mask 取并集。
- set、binding 升序。
- 空 set 也参与 `setCount`。

Hash：

```text
LowerHex(SHA-256(UTF8(CanonicalPipelineLayoutText)))
```

不参与 hash：

- ProgramId
- Variant
- EntryPoint
- code
- member/resource name
- type/block/range ID
- uniform offset
- resourceIndex
- role
- arrayElementBase
- logicalElementStride
- 当前绑定对象

