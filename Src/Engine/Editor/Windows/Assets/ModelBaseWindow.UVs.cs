using SE.GUI;

namespace SE.Editor
{
    public abstract partial class ModelBaseWindow<TAsset, TWindow>
        where TAsset : ModelBase
        where TWindow : ModelBaseWindow<TAsset, TWindow>
    {
        private protected MeshDataCache m_MeshData;

        private void InvalidateUVPreview()
        {
            foreach (Control child in m_Tabs.Children)
            {
                if (child is Tab tab && tab.Proxy is UVsPropertiesProxyBase proxy)
                {
                    proxy.InvalidatePreview();
                }
            }
        }

        protected class UVsPropertiesProxyBase : PropertiesProxyBase
        {
            private UVsLayoutPreviewControl m_Preview;

            public void InvalidatePreview() => m_Preview?.Invalidate();

            public enum UVChannel
            {
                None,
                TexCoord0,
                TexCoord1,
                TexCoord2,
                TexCoord3,
                LightmapUVs,
            }

            [EditorOrder(0), EditorDisplay(null, "Preview UV Channel")]
            public UVChannel Channel;

            [EditorOrder(1), Limit(0, 5), VisibleIf(nameof(ShowUVs))]
            public int LOD;

            [EditorOrder(2), Limit(-1, 1000000), VisibleIf(nameof(ShowUVs))]
            public int Mesh = -1;

            private bool ShowUVs => Channel != UVChannel.None;

            public override void OnClean()
            {
                Channel = UVChannel.None;
                LOD = 0;
                Mesh = -1;
                base.OnClean();
            }

            protected class ProxyEditor : ProxyEditorBase
            {
                private UVsLayoutPreviewControl m_UVsPreview;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    if (OnAssetProperties(layout))
                    {
                        return;
                    }
                    base.Initialize(layout);
                    UVsPropertiesProxyBase proxy = (UVsPropertiesProxyBase)Values[0];
                    m_UVsPreview = layout.Custom<UVsLayoutPreviewControl>().CustomControl;
                    m_UVsPreview.Window = proxy.Window;
                    proxy.m_Preview = m_UVsPreview;
                }

                public override void Refresh()
                {
                    base.Refresh();
                    if (m_UVsPreview != null)
                    {
                        UVsPropertiesProxyBase proxy = (UVsPropertiesProxyBase)Values[0];
                        m_UVsPreview.Channel = (int)proxy.Channel - 1;
                        m_UVsPreview.LOD = proxy.LOD;
                        m_UVsPreview.Mesh = proxy.Mesh;
                        m_UVsPreview.HighlightIndex = proxy.Window.m_HighlightIndex;
                        m_UVsPreview.IsolateIndex = proxy.Window.m_IsolateIndex;
                    }
                }

                protected override void Deinitialize()
                {
                    m_UVsPreview = null;
                    base.Deinitialize();
                }
            }
        }

        protected sealed class UVsLayoutPreviewControl : RenderToTextureControl
        {
            public ModelBaseWindow<TAsset, TWindow> Window;
            private int m_Channel = -1;
            private int m_LOD;
            private int m_Mesh = -1;
            private int m_HighlightIndex = -1;
            private int m_IsolateIndex = -1;

            public UVsLayoutPreviewControl()
            {
                AutomaticInvalidate = false;
                BackgroundColor = Color.Black;
                Visible = false;
            }

            public int Channel
            {
                set
                {
                    if (m_Channel != value)
                    {
                        m_Channel = value;
                        Visible = value != -1;
                        Invalidate();
                    }
                }
            }

            public int LOD
            {
                set
                {
                    if (m_LOD != value)
                    {
                        m_LOD = value;
                        Invalidate();
                    }
                }
            }

            public int Mesh
            {
                set
                {
                    if (m_Mesh != value)
                    {
                        m_Mesh = value;
                        Invalidate();
                    }
                }
            }

            public int HighlightIndex
            {
                set
                {
                    if (m_HighlightIndex != value)
                    {
                        m_HighlightIndex = value;
                        Invalidate();
                    }
                }
            }

            public int IsolateIndex
            {
                set
                {
                    if (m_IsolateIndex != value)
                    {
                        m_IsolateIndex = value;
                        Invalidate();
                    }
                }
            }

            public override void DrawSelf()
            {
                base.DrawSelf();
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                if (m_Channel < 0 || bounds.Size.MaxValue < 5.0f || Window == null || Window.m_IsImporting || Window.Asset == null || !Window.Asset.IsLoaded())
                {
                    return;
                }
                if (Window.Asset.GetLODsCount() == 0)
                {
                    Render2D.DrawText(Style.Current.FontMedium, "No mesh data", bounds, Color.Yellow);
                    return;
                }
                Window.m_MeshData ??= new MeshDataCache();
                if (!Window.m_MeshData.RequestMeshData(Window.Asset))
                {
                    Render2D.DrawText(Style.Current.FontMedium, Window.m_MeshData.Error ?? "Loading UVs...", bounds, Color.White);
                    if (Window.m_MeshData.Error == null)
                    {
                        Invalidate();
                    }
                    return;
                }
                MeshDataCache.MeshData[][] meshDatas = Window.m_MeshData.MeshDatas;
                if (meshDatas.Length == 0)
                {
                    Render2D.DrawText(Style.Current.FontMedium, "No mesh data", bounds, Color.Yellow);
                    return;
                }
                int lod = Mathf.Clamp(m_LOD, 0, meshDatas.Length - 1);
                MeshDataCache.MeshData[] data = meshDatas[lod];
                Render2D.PushClip(bounds);
                string error = null;
                bool hasGeometry = false;
                int selectedMesh = Mathf.Clamp(m_Mesh, -1, data.Length - 1);
                for (int mesh = 0; mesh < data.Length; mesh++)
                {
                    if (selectedMesh != -1 && selectedMesh != mesh)
                    {
                        continue;
                    }
                    MeshDataCache.MeshData meshData = data[mesh];
                    int slot = meshData.Mesh.MaterialSlotIndex;
                    if (m_IsolateIndex != -1 && m_IsolateIndex != slot)
                    {
                        continue;
                    }
                    // Lightmap selection is resolved per mesh, rather than inferred from another mesh's channel.
                    int channel = m_Channel == 4 ? meshData.LightmapUVsIndex : m_Channel;
                    Float2[] uvs = channel >= 0 && channel < meshData.TexCoords.Length ? meshData.TexCoords[channel] : null;
                    if (uvs == null)
                    {
                        error = m_Channel == 4 ? "Missing Lightmap UVs" : "Missing texcoords channel";
                        continue;
                    }
                    hasGeometry |= meshData.Indices.Length != 0;
                    Color color = m_HighlightIndex == slot ? Style.Current.BackgroundSelected : Color.White;
                    for (int i = 0; i + 2 < meshData.Indices.Length; i += 3)
                    {
                        Float2 a = uvs[meshData.Indices[i]] * Size;
                        Float2 b = uvs[meshData.Indices[i + 1]] * Size;
                        Float2 c = uvs[meshData.Indices[i + 2]] * Size;
                        if (Float2.TriangleArea(ref a, ref b, ref c) > 10.0f)
                        {
                            Render2D.DrawLine(a, b, color);
                            Render2D.DrawLine(b, c, color);
                            Render2D.DrawLine(c, a, color);
                        }
                    }
                }
                if (error != null || !hasGeometry)
                {
                    Render2D.DrawText(Style.Current.FontMedium, error ?? "No mesh data", bounds, Color.Yellow);
                }
                Render2D.PopClip();
            }

            protected override void OnSizeChanged()
            {
                Height = Width;
                base.OnSizeChanged();
            }

            protected override void OnVisibleChanged()
            {
                base.OnVisibleChanged();
                Parent?.PerformLayout();
                Height = Width;
            }

            public override void OnDestroy()
            {
                Window = null;
                base.OnDestroy();
            }
        }
    }
}
