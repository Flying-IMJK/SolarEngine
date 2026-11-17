using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>Views and edits a static model using the common model asset pages.</summary>
    public sealed class ModelWindow : ModelBaseWindow<Model, ModelWindow>
    {
        private sealed class Preview : ModelPreview
        {
            private readonly ModelWindow m_Window;

            public Preview(ModelWindow window) : base(true)
            {
                m_Window = window;
            }

            public override void Draw()
            {
                base.Draw();
                Model asset = m_Window.Asset;
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                string message = null;
                if (m_Window.m_IsImporting)
                {
                    message = "Reimporting model...";
                }
                else if (asset == null || !asset.IsLoaded())
                {
                    message = asset != null && asset.LastLoadFailed() ? "Failed to load model" : "Loading model...";
                }
                if (message != null)
                {
                    Render2D.DrawText(Style.Current.FontLarge, message, bounds, Style.Current.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
                else if (m_Window.m_LastImportFailed)
                {
                    Render2D.DrawText(Style.Current.FontMedium, "Reimport failed. See the log for details.", bounds, Color.Yellow);
                }
                else if (asset.LoadedLODs != asset.GetLODsCount())
                {
                    Render2D.DrawText(Style.Current.FontMedium, $"Loading LODs: {asset.LoadedLODs}/{asset.GetLODsCount()}", bounds, Color.White);
                }
            }
        }

        [CustomEditor(typeof(MeshesPropertiesProxy.ProxyEditor))]
        private sealed class MeshesPropertiesProxy : MeshesPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(MaterialsPropertiesProxy.ProxyEditor))]
        private sealed class MaterialsPropertiesProxy : MaterialsPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(UVsPropertiesProxy.ProxyEditor))]
        private sealed class UVsPropertiesProxy : UVsPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(ImportPropertiesProxy.ProxyEditor))]
        private sealed class ImportPropertiesProxy : ImportPropertiesProxyBase
        {
        }

        private sealed class MeshesTab : Tab
        {
            public MeshesTab(ModelWindow window) : base("Meshes", window)
            {
                Proxy = new MeshesPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private sealed class MaterialsTab : Tab
        {
            public MaterialsTab(ModelWindow window) : base("Materials", window)
            {
                Proxy = new MaterialsPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private sealed class UVsTab : Tab
        {
            public UVsTab(ModelWindow window) : base("UVs", window, false)
            {
                Proxy = new UVsPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private sealed class ImportTab : Tab
        {
            public ImportTab(ModelWindow window) : base("Import", window, false)
            {
                Proxy = new ImportPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private readonly ModelPreview m_Preview;
        private StaticModel m_HighlightActor;
        private MaterialBase m_HighlightMaterial;
        private readonly ComboBox m_LOD;
        private readonly ToolStripButton m_ShowCurrentLODButton;

        public ModelWindow(Editor editor, AssetItem item) : base(editor, item)
        {
            m_Toolstrip.AddSeparator();
            m_ShowCurrentLODButton = m_Toolstrip.AddButton("LOD Statistics", () => m_Preview.ShowCurrentLOD = !m_Preview.ShowCurrentLOD);
            m_Toolstrip.AddButton("Frame Model", () => m_Preview.ResetCamera());
            // ToolStrip stretches its children; preserve the default combobox height to keep its text scale at 1.
            ContainerControl lodContainer = new ContainerControl
            {
                Width = 120.0f,
                Height = m_Toolstrip.ItemsHeight,
                Parent = m_Toolstrip,
            };
            m_LOD = new ComboBox
            {
                Width = lodContainer.Width,
                Height = ComboBox.DefaultHeight,
                Parent = lodContainer,
                AnchorPreset = AnchorPresets.MiddleLeft,
                TooltipText = "Preview level of detail",
            };
            m_LOD.Items.Add("LOD: Automatic");
            m_LOD.SelectedIndex = 0;
            m_LOD.SelectedIndexChanged += OnLODChanged;

            m_Preview = new Preview(this)
            {
                ScaleToFit = false,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = m_Split.Panel1,
            };
            m_Tabs.AddTab(new MeshesTab(this));
            m_Tabs.AddTab(new MaterialsTab(this));
            m_Tabs.AddTab(new UVsTab(this));
            m_Tabs.AddTab(new ImportTab(this));
            m_HighlightActor = Object.New<StaticModel>();
            m_HighlightActor.IsActive = false;
            m_Preview.GetTask().AddCustomActor(m_HighlightActor);
            m_Preview.GetTask().Begin += OnPreviewBegin;
        }

        private void OnLODChanged(ComboBox combo)
        {
            m_Preview.PreviewModel.ForcedLOD = combo.SelectedIndex - 1;
            m_HighlightActor.ForcedLOD = combo.SelectedIndex - 1;
        }

        protected override void UpdateEffectsOnAsset()
        {
            if (m_Asset == null || !m_Asset.IsLoaded() || m_IsImporting)
            {
                return;
            }
            if (m_HighlightIndex != -1 && m_HighlightMaterial == null)
            {
                m_HighlightMaterial = SE.Editor.Editor.GetModelHighlightMaterial();
            }
            bool highlight = m_HighlightIndex != -1 && m_HighlightMaterial != null && m_HighlightMaterial.IsLoaded() && m_HighlightMaterial.IsReady();
            m_HighlightActor.IsActive = highlight;
            int slotsCount = m_Asset.GetMaterialSlotsCount();
            ModelInstanceEntry[] previewEntries = m_Preview.PreviewModel.Entries;
            ModelInstanceEntry[] highlightEntries = m_HighlightActor.Entries;
            int previewCount = previewEntries.Length;
            int highlightCount = highlightEntries.Length;
            bool previewChanged = previewCount != slotsCount;
            bool highlightChanged = highlightCount != slotsCount;
            System.Array.Resize(ref previewEntries, slotsCount);
            System.Array.Resize(ref highlightEntries, slotsCount);
            for (int i = 0; i < slotsCount; i++)
            {
                if (i >= previewCount)
                {
                    previewEntries[i] = new ModelInstanceEntry { ShadowsMode = ShadowsCastingMode.All, ReceiveDecals = true };
                }
                if (i >= highlightCount)
                {
                    highlightEntries[i] = new ModelInstanceEntry { ShadowsMode = ShadowsCastingMode.All, ReceiveDecals = true };
                }
                bool visible = m_IsolateIndex == -1 || m_IsolateIndex == i;
                previewChanged |= previewEntries[i].Visible != visible;
                previewEntries[i].Visible = visible;
                visible = highlight && m_HighlightIndex == i;
                highlightChanged |= highlightEntries[i].Visible != visible;
                highlightEntries[i].Visible = visible;
                if (highlight)
                {
                    highlightChanged |= highlightEntries[i].Material != m_HighlightMaterial;
                    highlightEntries[i].Material = m_HighlightMaterial;
                }
            }
            // Entries are copied across the binding boundary; submit changed arrays to the native actors.
            if (previewChanged)
            {
                m_Preview.PreviewModel.SetEntries(previewEntries);
            }
            if (highlightChanged)
            {
                m_HighlightActor.SetEntries(highlightEntries);
            }
        }

        private void OnPreviewBegin(RenderTask task, GPUContext context)
        {
            m_HighlightActor.Transform = m_Preview.PreviewModel.Transform;
            UpdateEffectsOnAsset();
        }

        public override void OnUpdate()
        {
            base.OnUpdate();
            m_ShowCurrentLODButton.Checked = m_Preview.ShowCurrentLOD;
            if (m_RefreshOnLODsLoaded && m_Asset != null && m_Asset.IsLoaded() && !m_IsImporting && m_Asset.LoadedLODs == m_Asset.GetLODsCount())
            {
                m_RefreshOnLODsLoaded = false;
                m_MeshData?.Dispose();
                m_MeshData = null;
                foreach (Control child in m_Tabs.Children)
                {
                    if (child is Tab tab)
                    {
                        tab.Presenter.BuildLayout();
                    }
                }
            }
        }

        public override void Save()
        {
            // Flush all presenters, including hidden tabs, before checking the edited flag.
            UpdatePresenters();
            if (!IsEdited || m_IsImporting || m_Asset == null || !m_Asset.WaitForLoaded())
            {
                return;
            }
            foreach (Control child in m_Tabs.Children)
            {
                if (child is Tab tab && tab.Proxy.Window != null)
                {
                    tab.Proxy.OnSave();
                }
            }
            if (!SE.Editor.Editor.SaveModelAsset(m_Asset))
            {
                MessageBox.Show("Cannot save the model. See the log for details.", "Save Model", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            ClearEditedFlag();
            Item.RefreshThumbnail();
        }

        protected override void OnAssetLinked()
        {
            m_Preview.Model = m_Asset;
            m_HighlightActor.Model = m_Asset;
            base.OnAssetLinked();
        }

        protected override void OnAssetLoaded()
        {
            m_Preview.Model = m_Asset;
            m_HighlightActor.Model = m_Asset;
            m_LOD.Items.Clear();
            m_LOD.Items.Add("LOD: Automatic");
            for (int i = 0; i < m_Asset.GetLODsCount(); i++)
            {
                m_LOD.Items.Add("LOD " + i);
            }
            m_LOD.SelectedIndex = 0;
            m_Preview.PreviewModel.ForcedLOD = -1;
            m_HighlightActor.ForcedLOD = -1;
            m_Preview.ResetCamera();
            base.OnAssetLoaded();
        }

        protected override void OnAssetReloading()
        {
            m_Preview.Model = null;
            m_HighlightActor.Model = null;
            m_HighlightActor.IsActive = false;
            base.OnAssetReloading();
        }

        protected override void UnlinkItem()
        {
            m_Preview.Model = null;
            m_HighlightActor.Model = null;
            base.UnlinkItem();
        }

        public override void OnDestroy()
        {
            if (IsDisposing)
            {
                return;
            }
            m_Preview.GetTask().Enabled = false;
            m_Preview.GetTask().Begin -= OnPreviewBegin;
            // The asset base unlinks first; the preview then destroys its task before the highlight actor.
            base.OnDestroy();
            Object.DestroyNow(m_HighlightActor);
            m_HighlightActor = null;
            m_HighlightMaterial = null;
        }
    }
}
