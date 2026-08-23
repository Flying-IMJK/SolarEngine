# Flax Editor GUI 基础组件迁移范围

## 目的

本文档定义从 Flax `Source/Editor/GUI` 向 SolarEngine `Src/Engine/Editor/GUI` 迁移时的准入边界。它只覆盖基础 GUI 功能和通用 GUI 组件，不覆盖依赖上层 Editor 业务模块的 GUI 控件。

Flax 参考基线固定为提交 `97314377172f294753ded61e9e47f37f4a6f7584`。当前 Flax 目录有 120 个 C# 文件；SolarEngine 当前 `Editor/GUI` 有 41 个 C# 文件。文件同名或类型已存在不表示行为已经与 Flax 对齐，仍须逐项比较默认值、状态、事件顺序、布局、绘制和输入语义。

## 准入规则

1. 只有可以完全建立在 `SE.GUI`、现有生成绑定和既有基础 API 上的 Flax GUI 组件可直接迁移。
2. 已有同名或同类型实现必须以 Flax 为功能基线比对；发现行为差异后，仅在不触及 GUI 外模块时修正。
3. 若完整复现 Flax 行为需要新增或修改 GUI 外的窗口、输入、渲染、资源、脚本、反射绑定、选项、资产、场景或编辑器业务 API，必须先提交确认包，未确认前不写桩实现、兼容层或替代数据结构。
4. 直接依赖资产浏览器、场景树、属性编辑器、编辑器窗口、项目配置、时间轴或预览系统的控件不属于本次迁移范围。
5. 迁移以类型及其不可拆分的协作类型组为单位；不得仅迁移可编译的子集而改变 Flax 的可观察行为。

## 范围统计

| 分类 | Flax 文件数 | 处理方式 |
|---|---:|---|
| 可直接审查的基础组件 | 37 | 按 Flax 与当前 Runtime/Editor 实现逐项比较后迁移或修正。 |
| 基础组件但依赖需确认 | 14 | 先完成 GUI 内部比较；若完整行为需要 GUI 外改动，提交确认包。 |
| 上层模块 GUI | 69 | 本次不迁移，也不为其补充依赖。 |

## 可直接审查的基础组件（37）

这些文件仅应使用当前 `SE.GUI` 与已有基础能力。开始代码改动前仍需检查其对 Runtime GUI 的依赖是否已经完整存在。

| 功能组 | Flax 文件 |
|---|---|
| 根目录通用控件 | `ClickableLabel.cs`、`ColumnDefinition.cs`、`MainMenu.cs`、`MainMenuButton.cs`、`NavigationBar.cs`、`NavigationButton.cs`、`Row.cs`、`StatusBar.cs`、`Table.cs` |
| ContextMenu 基础类型 | `ContextMenu/ContextMenuBase.cs`、`ContextMenu/ContextMenuButton.cs`、`ContextMenu/ContextMenuChildMenu.cs`、`ContextMenu/ContextMenuItem.cs`、`ContextMenu/ContextMenuSeparator.cs`、`ContextMenu/ContextMenuSingleSelectGroup.cs` |
| Docking 基础类型 | `Docking/DockHintWindow.cs`、`Docking/DockPanel.cs`、`Docking/DockPanelProxy.cs`、`Docking/FloatWindowDockPanel.cs`、`Docking/MasterDockPanel.cs` |
| 通用拖放 | `Drag/DragEventArgs.cs`、`Drag/DragHandlers.cs`、`Drag/DragHelper.cs`、`Drag/DragNames.cs` |
| 输入组件 | `Input/ColorValueBox.cs`、`Input/DoubleValueBox.cs`、`Input/FloatValueBox.cs`、`Input/IntValueBox.cs`、`Input/LongValueBox.cs`、`Input/SearchBox.cs`、`Input/SliderControl.cs`、`Input/UIntValueBox.cs`、`Input/ULongValueBox.cs`、`Input/ValueBox.cs` |
| 通用弹出层 | `Popups/RenamePopup.cs` |
| 工具栏项 | `ToolStripButton.cs`、`ToolStripSeparator.cs` |

## 基础组件但需要确认的依赖（14）

这些仍是 GUI 框架或通用组件，但其 Flax 完整行为涉及 GUI 外服务。只有依赖已在 SolarEngine 中以语义等价方式可用时才能迁移；否则应提交确认包。

| Flax 文件 | GUI 外依赖或待核对行为 |
|---|---|
| `ContextMenu/ContextMenu.cs` | 允许迁移；`InputBinding` 重载暂以注释保留，不创建替代绑定类型。 |
| `Docking/DockWindow.cs` | 允许迁移；Editor 默认停靠位置、`InputActions` 快捷键以及窗口布局序列化均暂以 Flax 原型注释保留，不创建替代绑定、布局格式或持久化实现。 |
| `Tree/Tree.cs` | 允许迁移；SelectAll、反选、取消选择等 Editor 输入选项快捷键暂以注释保留。 |
| `Tree/TreeNode.cs`、`Tree/TreeNodeWithAddons.cs` | Flax 的长按、拖放后鼠标抬起抑制和 Shift/Ctrl/Alt 选择，分别依赖未公开的无缩放时间、帧序号和修饰键状态查询。不得用局部计时器或自定义按键状态替代。 |
| `ToolStrip.cs` | 允许迁移；图标缩放选项与 `EditorWindow.InputActions` 路由暂以注释保留。 |
| `Dialogs/Dialog.cs` | Editor 主窗口默认宿主与托管窗口创建。 |
| `Dialogs/ColorPickerDialog.cs` | 项目缓存、编辑器图标、窗口焦点与颜色编辑会话。 |
| `Dialogs/ColorSelector.cs` | 编辑器图标和 Sprite 绘制能力。 |
| `ItemsListContextMenu.cs` | 编辑器图标与 Sprite 绘制能力。 |
| `Drag/DragScripts.cs` | `Script` 托管类型、对象查找与反射/绑定支持。 |
| `Tabs/Tab.cs`、`Tabs/Tabs.cs` | `SpriteHandle` 只有空的托管结构，无法判断 `IsValid` 或表示 Flax 的 `Invalid`；标签图标无法按 Flax 语义绘制。`Style` 也缺少 `LightBackground` 和 `ForegroundGrey`。 |
| `ComboBox.cs` | 需要 `FontReference`、`SpriteBrush`、可用的 `SpriteHandle`，以及 `Style.ForegroundGrey` 等当前 Runtime GUI 未提供的视觉语义。 |

确认包必须列出 Flax 调用点、SolarEngine 现有能力、缺口、拟改 API/文件、影响范围和最小验证方案。未确认时不得通过移除该行为或临时模型让组件“看起来可用”。

## 当前验证基线

- 已完成 `ClickableLabel.cs` 的首次 Flax 对齐：公开回调恢复为 `DoubleClick`、`LeftClick`、`RightClick`，并保持 Flax 的鼠标事件顺序。
- 已完成 `ColumnDefinition.cs` 的首次 Flax 对齐：恢复可继承类型、公开字段、`Color.Brown` 默认标题背景和 Flax 的列宽钳制语义。
- 已将 `GuiRect`、`GuiPoint`、`GuiSize`、`GuiMargin` 全部映射至已有的 `Rectangle`、`Float2` 与 `Margin`，未重新引入临时结构；拖放已连接到现有 `SE.GUI.DragData` 与 `SE.DragDropEffect`。
- `DockWindow` 的 Flax 窗口布局序列化原型已按约定注释保留；未引入替代布局格式，也未启用 XML 持久化路径。
- `dotnet build Src/Engine/Editor/EditorCSharp.csproj --no-restore` 当前仍因 `TreeNode` 所需的运行时状态接口，以及不属于本次范围的 `ManagedEditor.cs`、`Resource/ContentView.cs` 失败。
- 后续审查已确认，`ComboBox`、`Tabs` 以及所有图标型控件共享运行时视觉依赖：当前生成的 `SE.SpriteHandle` 是空结构，`SE.GUI` 也没有 `SpriteBrush`；Flax 所用的 `Style.LightBackground`、`Style.ForegroundGrey` 和 `Style.DragWindow` 尚未存在。涉及这些能力的迁移将在得到确认后继续。

## 本次排除的上层模块 GUI（69）

### 资产、场景、属性编辑和项目业务（22）

- `AssetPicker.cs`
- `CurveEditor.Access.cs`、`CurveEditor.Base.cs`、`CurveEditor.Contents.cs`、`CurveEditor.cs`
- `EnumComboBox.cs`
- `IKeyframesEditor.cs`、`IKeyframesEditorContext.cs`、`KeyframesEditorUtils.cs`
- `PlatformSelector.cs`
- `PrefabDiffContextMenu.cs`
- `StyleValueEditor.cs`
- `Drag/DragActors.cs`、`Drag/DragActorType.cs`、`Drag/DragAssets.cs`、`Drag/DragControlType.cs`、`Drag/DragItems.cs`、`Drag/DragScriptItems.cs`
- `Popups/ActorSearchPopup.cs`、`Popups/AssetSearchPopup.cs`、`Popups/ScriptSearchPopup.cs`、`Popups/TypeSearchPopup.cs`

### Timeline、轨道与撤销（47）

- `Timeline/AnimationTimeline.cs`、`Timeline/Media.cs`、`Timeline/ParticleSystemTimeline.cs`、`Timeline/SceneAnimationTimeline.cs`、`Timeline/Timeline.cs`、`Timeline/Timeline.Data.cs`、`Timeline/Timeline.UI.cs`、`Timeline/Track.cs`、`Timeline/TrackArchetype.cs`
- `Timeline/GUI/Background.cs`、`Timeline/GUI/BackgroundArea.cs`、`Timeline/GUI/GradientEditor.cs`、`Timeline/GUI/KeyframesEditor.cs`、`Timeline/GUI/PositionHandle.cs`、`Timeline/GUI/TimelineEdge.cs`
- `Timeline/Tracks/ActorTrack.cs`、`Timeline/Tracks/AnimationChannelTrack.cs`、`Timeline/Tracks/AnimationEventTrack.cs`、`Timeline/Tracks/AudioTrack.cs`、`Timeline/Tracks/CameraCutTrack.cs`、`Timeline/Tracks/ConductorTrack.cs`、`Timeline/Tracks/CurvePropertyTrack.cs`、`Timeline/Tracks/EventTrack.cs`、`Timeline/Tracks/FolderTrack.cs`、`Timeline/Tracks/KeyframesPropertyTrack.cs`、`Timeline/Tracks/MemberTrack.cs`、`Timeline/Tracks/NestedAnimationTrack.cs`、`Timeline/Tracks/NestedSceneAnimationTrack.cs`、`Timeline/Tracks/ObjectPropertyTrack.cs`、`Timeline/Tracks/ObjectReferencePropertyTrack.cs`、`Timeline/Tracks/ObjectTrack.cs`、`Timeline/Tracks/ParticleEmitterTrack.cs`、`Timeline/Tracks/PostProcessMaterialTrack.cs`、`Timeline/Tracks/ScreenFadeTrack.cs`、`Timeline/Tracks/ScriptTrack.cs`、`Timeline/Tracks/SingleMediaAssetTrack.cs`、`Timeline/Tracks/SingleMediaTrack.cs`、`Timeline/Tracks/StringPropertyTrack.cs`、`Timeline/Tracks/StructPropertyTrack.cs`
- `Timeline/Undo/AddRemoveTrackAction.cs`、`Timeline/Undo/EditFpsAction.cs`、`Timeline/Undo/EditTimelineAction.cs`、`Timeline/Undo/EditTrackAction.cs`、`Timeline/Undo/RenameTrackAction.cs`、`Timeline/Undo/ReorderTrackAction.cs`、`Timeline/Undo/TimelineUndoBlock.cs`、`Timeline/Undo/TrackUndoBlock.cs`

## 实施顺序

1. 对当前已存在的基础组件完成 Flax 行为差异表，优先 ContextMenu、Docking、输入、Tree 和通用拖放。
2. 在不需要 GUI 外改动的前提下，补齐 Tabs 和其余缺失的基础组件。
3. 遇到“需要确认”的九个组件时，先提交确认包，再决定是否继续。
4. 每个组件完成后执行 C# 编译、无窗口逻辑验证，以及托管窗口输入和布局验证；现有 C++ 编辑器保持原生 GUI 后端不变。
