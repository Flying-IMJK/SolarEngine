using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using System.Xml;

using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>Common model editing pages and their asset lifecycle, following Flax ModelBaseWindow.</summary>
    public abstract partial class ModelBaseWindow<TAsset, TWindow> : AssetEditorWindowBase<TAsset>
        where TAsset : ModelBase
        where TWindow : ModelBaseWindow<TAsset, TWindow>
    {
        protected abstract class PropertiesProxyBase
        {
            [HideInEditor]
            public TWindow Window;

            [HideInEditor]
            public TAsset Asset;

            public virtual void OnLoad(TWindow window)
            {
                Window = window;
                Asset = window.Asset;
            }

            public virtual void OnSave()
            {
            }

            public virtual void OnClean()
            {
                Window = null;
                Asset = null;
            }
        }

        protected abstract class ProxyEditorBase : GenericEditor
        {
            internal override void RefreshInternal()
            {
                PropertiesProxyBase proxy = (PropertiesProxyBase)Values[0];
                if (proxy.Asset == null || !proxy.Asset.IsLoaded() || proxy.Window.m_IsImporting)
                {
                    return;
                }
                base.RefreshInternal();
            }

            protected bool OnAssetProperties(LayoutElementsContainer layout)
            {
                PropertiesProxyBase proxy = (PropertiesProxyBase)Values[0];
                if (proxy.Asset != null && proxy.Asset.IsLoaded() && !proxy.Window.m_IsImporting)
                {
                    return false;
                }
                layout.Label(proxy.Asset != null && proxy.Asset.LastLoadFailed() ? "Failed to load model" : "Loading model...");
                return true;
            }
        }

        protected class Tab : GUI.Tab
        {
            public CustomEditorPresenter Presenter;
            public PropertiesProxyBase Proxy;

            public Tab(string text, TWindow window, bool modifiesAsset = true) : base(text)
            {
                Panel scrollPanel = new Panel(ScrollBars.Vertical)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = this,
                };
                Presenter = new CustomEditorPresenter();
                Presenter.Panel.Parent = scrollPanel;
                if (modifiesAsset)
                {
                    Presenter.Modified += window.MarkAsEdited;
                }
            }

            public override void OnDestroy()
            {
                if (IsDisposing)
                {
                    return;
                }
                Presenter.Deselect();
                Proxy.OnClean();
                base.OnDestroy();
                Presenter = null;
                Proxy = null;
            }
        }

        protected class MeshesPropertiesProxyBase : PropertiesProxyBase
        {
            private readonly List<ComboBox> m_MaterialSlotComboBoxes = new List<ComboBox>();
            private readonly List<CheckBox> m_IsolateCheckBoxes = new List<CheckBox>();
            private readonly List<CheckBox> m_HighlightCheckBoxes = new List<CheckBox>();

            public override void OnLoad(TWindow window)
            {
                base.OnLoad(window);
                Window.m_IsolateIndex = -1;
                Window.m_HighlightIndex = -1;
                Window.m_MeshProxy = this;
            }

            public override void OnClean()
            {
                if (Window != null)
                {
                    Window.m_IsolateIndex = -1;
                    Window.m_HighlightIndex = -1;
                    Window.m_MeshProxy = null;
                }
                m_MaterialSlotComboBoxes.Clear();
                m_IsolateCheckBoxes.Clear();
                m_HighlightCheckBoxes.Clear();
                base.OnClean();
            }

            public void UpdateEffectsOnUI()
            {
                Window.m_SkipEffectsGuiEvents = true;
                foreach (CheckBox box in m_IsolateCheckBoxes)
                {
                    box.Checked = Window.m_IsolateIndex == ((MeshBase)box.Tag).MaterialSlotIndex;
                }
                foreach (CheckBox box in m_HighlightCheckBoxes)
                {
                    box.Checked = Window.m_HighlightIndex == ((MeshBase)box.Tag).MaterialSlotIndex;
                }
                Window.m_SkipEffectsGuiEvents = false;
            }

            public void UpdateMaterialSlotsUI()
            {
                Window.m_SkipEffectsGuiEvents = true;
                MaterialSlot[] slots = Asset.MaterialSlots;
                List<string> labels = new List<string>(slots.Length);
                for (int i = 0; i < slots.Length; i++)
                {
                    labels.Add($"[{i}] {slots[i].Name}");
                }
                foreach (ComboBox combo in m_MaterialSlotComboBoxes)
                {
                    combo.SetItems(labels);
                    combo.SelectedIndex = ((MeshBase)combo.Tag).MaterialSlotIndex;
                    combo.Enabled = labels.Count != 0;
                }
                Window.m_SkipEffectsGuiEvents = false;
                UpdateEffectsOnUI();
            }

            public void SetMaterialSlot(MeshBase mesh, int index)
            {
                if (Window.m_SkipEffectsGuiEvents)
                {
                    return;
                }
                if (index < 0 || index >= Asset.GetMaterialSlotsCount())
                {
                    return;
                }
                mesh.MaterialSlotIndex = index;
                Window.UpdateEffectsOnAsset();
                Window.InvalidateUVPreview();
                UpdateEffectsOnUI();
                Window.MarkAsEdited();
            }

            public void SetIsolate(MeshBase mesh)
            {
                if (Window.m_SkipEffectsGuiEvents)
                {
                    return;
                }
                Window.m_IsolateIndex = mesh != null ? mesh.MaterialSlotIndex : -1;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
            }

            public void SetHighlight(MeshBase mesh)
            {
                if (Window.m_SkipEffectsGuiEvents)
                {
                    return;
                }
                Window.m_HighlightIndex = mesh != null ? mesh.MaterialSlotIndex : -1;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
            }

            protected virtual void OnGeneral(LayoutElementsContainer layout)
            {
            }

            protected class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    if (OnAssetProperties(layout))
                    {
                        return;
                    }
                    MeshesPropertiesProxyBase proxy = (MeshesPropertiesProxyBase)Values[0];
                    proxy.m_MaterialSlotComboBoxes.Clear();
                    proxy.m_IsolateCheckBoxes.Clear();
                    proxy.m_HighlightCheckBoxes.Clear();
                    TAsset asset = proxy.Asset;
                    int countLODs = asset.GetLODsCount();
                    int loadedLODs = asset.LoadedLODs;

                    GroupElement general = layout.Group("General");
                    FloatValueBox minSize = general.AddPropertyItem("Min Screen Size").Custom<FloatValueBox>().CustomControl;
                    minSize.SetLimits(0.0f, 1.0f);
                    minSize.SetSpeed(0.005f);
                    minSize.SetValue(asset.MinScreenSize);
                    minSize.ValueChanged += () =>
                    {
                        asset.MinScreenSize = minSize.GetValue();
                        proxy.Window.MarkAsEdited();
                    };
                    proxy.OnGeneral(layout);

                    for (int lodIndex = 0; lodIndex < countLODs; lodIndex++)
                    {
                        GroupElement group = layout.Group("LOD " + lodIndex);
                        if (lodIndex < countLODs - loadedLODs)
                        {
                            group.Label("Loading LOD...");
                            continue;
                        }
                        ModelLODBase lod = asset.GetLOD(lodIndex);
                        asset.GetMeshes(out MeshBase[] meshes, lodIndex);
                        int triangles = 0;
                        int vertices = 0;
                        foreach (MeshBase mesh in meshes)
                        {
                            triangles += mesh.TriangleCount;
                            vertices += mesh.VertexCount;
                        }
                        group.Label($"Triangles: {triangles:N0}    Vertices: {vertices:N0}");
                        group.Label("Size: " + lod.GetBox().Size);
                        FloatValueBox screenSize = group.AddPropertyItem("Screen Size").Custom<FloatValueBox>().CustomControl;
                        screenSize.SetLimits(0.0f, 10.0f);
                        screenSize.SetValue(lod.ScreenSize);
                        screenSize.ValueChanged += () =>
                        {
                            lod.ScreenSize = screenSize.GetValue();
                            proxy.Window.MarkAsEdited();
                        };
                        for (int meshIndex = 0; meshIndex < meshes.Length; meshIndex++)
                        {
                            MeshBase mesh = meshes[meshIndex];
                            group.Label($"Mesh {meshIndex} (tris: {mesh.TriangleCount:N0}, vert: {mesh.VertexCount:N0})");
                            ComboBox slot = group.AddPropertyItem("Material Slot").Custom<ComboBox>().CustomControl;
                            slot.Tag = mesh;
                            slot.SelectedIndexChanged += combo => proxy.SetMaterialSlot((MeshBase)combo.Tag, combo.SelectedIndex);
                            proxy.m_MaterialSlotComboBoxes.Add(slot);
                            CheckBox isolate = group.AddPropertyItem("Isolate").Checkbox().CheckBox;
                            isolate.Tag = mesh;
                            isolate.StateChanged += box => proxy.SetIsolate(box.Checked ? (MeshBase)box.Tag : null);
                            proxy.m_IsolateCheckBoxes.Add(isolate);
                            CheckBox highlight = group.AddPropertyItem("Highlight").Checkbox().CheckBox;
                            highlight.Tag = mesh;
                            highlight.StateChanged += box => proxy.SetHighlight(box.Checked ? (MeshBase)box.Tag : null);
                            proxy.m_HighlightCheckBoxes.Add(highlight);
                        }
                    }
                    proxy.UpdateMaterialSlotsUI();
                }
            }
        }

        protected class MaterialsPropertiesProxyBase : PropertiesProxyBase
        {
            [Collection(CanReorderItems = true, NotNullItems = true, MinCount = 1, MaxCount = 4095,
                OverrideEditorTypeName = "SE.Editor.GenericEditor", Spacing = 10)]
            [EditorOrder(10), EditorDisplay("Materials", EditorDisplayAttribute.InlineStyle)]
            public MaterialSlot[] MaterialSlots
            {
                get => Asset != null ? Asset.MaterialSlots : null;
                set
                {
                    if (Asset == null)
                    {
                        return;
                    }
                    // Slot objects belong to the native collection. Snapshot all fields before resizing or reordering.
                    MaterialBase[] materials = new MaterialBase[value.Length];
                    string[] names = new string[value.Length];
                    ShadowsCastingMode[] shadowsModes = new ShadowsCastingMode[value.Length];
                    for (int i = 0; i < value.Length; i++)
                    {
                        if (value[i] != null)
                        {
                            materials[i] = value[i].Material;
                            names[i] = value[i].Name;
                            shadowsModes[i] = value[i].ShadowsMode;
                        }
                        else
                        {
                            names[i] = "Material " + i;
                            shadowsModes[i] = ShadowsCastingMode.All;
                        }
                    }

                    bool resized = Asset.GetMaterialSlotsCount() != value.Length;
                    if (resized)
                    {
                        Asset.SetupMaterialSlots(value.Length);
                    }
                    MaterialSlot[] slots = Asset.MaterialSlots;
                    for (int i = 0; i < slots.Length; i++)
                    {
                        slots[i].Material = materials[i];
                        slots[i].Name = names[i];
                        slots[i].ShadowsMode = shadowsModes[i];
                    }
                    if (resized)
                    {
                        // Discard editors holding old native slot references after the current refresh finishes.
                        foreach (Control child in Window.m_Tabs.Children)
                        {
                            if (child is Tab tab && tab.Proxy == this)
                            {
                                tab.Presenter.BuildLayoutOnUpdate();
                            }
                        }
                    }
                    if (Window.m_IsolateIndex >= value.Length)
                    {
                        Window.m_IsolateIndex = -1;
                    }
                    if (Window.m_HighlightIndex >= value.Length)
                    {
                        Window.m_HighlightIndex = -1;
                    }
                    Window.m_MeshProxy?.UpdateMaterialSlotsUI();
                    Window.UpdateEffectsOnAsset();
                    Window.InvalidateUVPreview();
                    Window.MarkAsEdited();
                }
            }

            protected class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    if (!OnAssetProperties(layout))
                    {
                        base.Initialize(layout);
                    }
                }
            }
        }

        protected class ImportPropertiesProxyBase : PropertiesProxyBase
        {
            private ModelImportSettings m_ImportSettings;

            public override void OnLoad(TWindow window)
            {
                base.OnLoad(window);
                m_ImportSettings = window.m_ImportSettings;
            }

            public override void OnClean()
            {
                m_ImportSettings = null;
                base.OnClean();
            }

            public void Reimport() => Window.Reimport();

            protected class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    if (OnAssetProperties(layout))
                    {
                        return;
                    }

                    ImportPropertiesProxyBase proxy = (ImportPropertiesProxyBase)Values[0];

                    GroupElement group = layout.Group("Import Settings");

                    FieldInfo field = typeof(ImportPropertiesProxyBase).GetField(nameof(m_ImportSettings), BindingFlags.NonPublic | BindingFlags.Instance);

                    group.Object(new ValueContainer(field) { proxy.m_ImportSettings });

                    GroupElement path = layout.Group("Import Path");

                    string source = proxy.Asset.GetImportPath();

                    if (string.IsNullOrEmpty(source))
                    {
                        path.Label("No source path in asset metadata");
                    }
                    else
                    {
                        TextBox textBox = path.TextBox().TextBox;
                        textBox.IsReadOnly = true;
                        textBox.Text = source;
                        path.Button("Show in Explorer").Button.Clicked += () => FileSystem.ShowFileExplorer(System.IO.Path.GetDirectoryName(source));
                    }
                    layout.Button("Reimport").Button.Clicked += proxy.Reimport;
                }
            }
        }

        protected readonly SplitPanel m_Split;
        protected readonly Tabs m_Tabs;
        protected readonly ToolStripButton m_SaveButton;
        protected ModelImportSettings m_ImportSettings = new ModelImportSettings();
        protected bool m_RefreshOnLODsLoaded;
        protected bool m_SkipEffectsGuiEvents;
        protected int m_IsolateIndex = -1;
        protected int m_HighlightIndex = -1;
        private MeshesPropertiesProxyBase m_MeshProxy;

        protected ModelBaseWindow(Editor editor, AssetItem item) : base(editor, item)
        {
            m_SaveButton = m_Toolstrip.AddButton("Save", Save);
            m_SaveButton.Enabled = false;
            m_Split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, m_Toolstrip.Bottom, 0),
                SplitterValue = 0.59f,
                Parent = this,
            };
            m_Tabs = new Tabs
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(60, 20),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                UseScroll = true,
                Parent = m_Split.Panel2,
            };
        }

        protected abstract void UpdateEffectsOnAsset();

        protected void UpdatePresenters()
        {
            foreach (Control child in m_Tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Presenter.Update();
                }
            }
        }

        protected override bool OnClosing(ClosingReason reason)
        {
            UpdatePresenters();
            return base.OnClosing(reason);
        }

        protected override void UpdateToolstrip()
        {
            m_SaveButton.Enabled = IsEdited && !m_IsImporting;
            base.UpdateToolstrip();
        }

        protected override void OnAssetLoaded()
        {
            m_RefreshOnLODsLoaded = true;
            AssetsImporting.TryRestoreImportOptions(ref m_ImportSettings.Settings, Item.Path);
            foreach (Control child in m_Tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnLoad((TWindow)this);
                    tab.Presenter.BuildLayout();
                }
            }
            UpdateEffectsOnAsset();
            ClearEditedFlag();
            UpdateToolstrip();
            base.OnAssetLoaded();
        }

        protected override void OnAssetReloading()
        {
            m_MeshData?.Dispose();
            m_MeshData = null;
            foreach (Control child in m_Tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnClean();
                    tab.Presenter.BuildLayout();
                }
            }
            ClearEditedFlag();
            m_RefreshOnLODsLoaded = true;
            base.OnAssetReloading();
        }

        public override void OnItemReimported(ContentItem item)
        {
            OnAssetReloading();
            base.OnItemReimported(item);
        }

        private void Reimport()
        {
            UpdatePresenters();
            if (m_IsImporting || m_Item is not BinaryAssetItem binary)
            {
                return;
            }
            if (IsEdited)
            {
                DialogResult result = MessageBox.Show("Save model changes before reimporting?", "Reimport Model", MessageBoxButtons.YesNoCancel);
                if (result == DialogResult.Cancel || result == DialogResult.Abort)
                {
                    return;
                }
                if (result == DialogResult.Yes || result == DialogResult.OK)
                {
                    Save();
                    if (IsEdited)
                    {
                        return;
                    }
                }
            }
            if (binary.GetImportPath(out string path) || !System.IO.File.Exists(path))
            {
                MessageBox.Show("The model source file is missing. Restore it before reimporting.", "Reimport Model");
                return;
            }
            m_IsImporting = true;
            m_LastImportFailed = false;
            OnAssetReloading();
            UpdateToolstrip();
            Editor.Importing.Reimport(binary, m_ImportSettings, true);
        }

        protected override void UnlinkItem()
        {
            m_MeshData?.Dispose();
            m_MeshData = null;
            foreach (Control child in m_Tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnClean();
                    tab.Presenter.Deselect();
                }
            }
            base.UnlinkItem();
        }

        public override bool UseLayoutData => true;

        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteElementString("Split", m_Split.SplitterValue.ToString(CultureInfo.InvariantCulture));
        }

        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node["Split"]?.InnerText, NumberStyles.Float, CultureInfo.InvariantCulture, out float split) &&
                float.IsFinite(split) && split > 0.0f && split < 1.0f)
            {
                m_Split.SplitterValue = split;
            }
        }

        public override void OnLayoutDeserialize() => m_Split.SplitterValue = 0.59f;
    }
}
