
using System;
using SE;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Represents control which contains collection of <see cref="Tab"/>.
    /// </summary>
    /// <seealso cref="SE.GUI.ContainerControl" />
    [HideInEditor]
    public class Tabs : ContainerControl
    {
        /// <summary>
        /// Tab header control. Draw the tab title and handles selecting it.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        public class TabHeader : Control
        {
            /// <summary>
            /// The tabs control.
            /// </summary>
            protected Tabs m_Tabs;

            /// <summary>
            /// The tab.
            /// </summary>
            protected Tab m_Tab;

            /// <summary>
            /// Initializes a new instance of the <see cref="TabHeader"/> class.
            /// </summary>
            /// <param name="tabs">The tabs.</param>
            /// <param name="tab">The tab.</param>
            public TabHeader(Tabs tabs, Tab tab)
            : base(Float2.Zero, tabs.m_TabsSize)
            {
                m_Tabs = tabs;
                m_Tab = tab;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (EnabledInHierarchy && m_Tab.Enabled)
                {
                    m_Tabs.SelectedTab = m_Tab;
                    m_Tab.PerformLayout(true);
                    m_Tabs.Focus();
                }
                return true;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                Style style = Style.Current;
                bool enabled = EnabledInHierarchy && m_Tab.EnabledInHierarchy;
                Rectangle tabRect = new Rectangle(Float2.Zero, Size);
                int textOffset = m_Tabs.m_Orientation == Orientation.Horizontal ? 0 : 8;

                // Draw bar
                if (m_Tabs.SelectedTab == m_Tab)
                {
                    Color color = style.BackgroundSelected;
                    if (!enabled)
                    {
                        color *= 0.6f;
                    }
                    if (m_Tabs.m_Orientation == Orientation.Horizontal)
                    {
                        Render2D.FillRectangle(tabRect, color);
                    }
                    else
                    {
                        const float lefEdgeWidth = 4;
                        Rectangle leftEdgeRect = tabRect;
                        leftEdgeRect.Size.X = lefEdgeWidth;
                        Rectangle fillRect = tabRect;
                        fillRect.Size.X -= lefEdgeWidth;
                        fillRect.Location.X += lefEdgeWidth;
                        Render2D.FillRectangle(fillRect, style.Background);
                        Render2D.FillRectangle(leftEdgeRect, color);
                    }
                }
                else if (IsMouseOver && enabled)
                {
                    Render2D.FillRectangle(tabRect, style.BackgroundHighlighted);
                }

                // Draw icon
                if (m_Tab.Icon.IsValid)
                {
                    Render2D.DrawSprite(m_Tab.Icon, tabRect.MakeExpanded(-8), style.Foreground);
                }

                // Draw text
                if (!string.IsNullOrEmpty(m_Tab.Text))
                {
                    Rectangle textRect = new Rectangle(tabRect.X + textOffset, tabRect.Y, tabRect.Width - textOffset, tabRect.Height);
                    Render2D.DrawText(style.FontMedium, m_Tab.Text, textRect, style.Foreground,
                        m_Tabs.TabsTextHorizontalAlignment, m_Tabs.TabsTextVerticalAlignment);
                }
            }
        }

        /// <summary>
        /// The tabs headers container control. Arranges the tabs headers and support scrolling them.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Panel" />
        public class TabsHeader : Panel
        {
            /// <summary>
            /// The tabs control.
            /// </summary>
            protected Tabs m_Tabs;

            /// <summary>
            /// Initializes a new instance of the <see cref="TabsHeader"/> class.
            /// </summary>
            /// <param name="tabs">The tabs.</param>
            public TabsHeader(Tabs tabs)
            {
                m_Tabs = tabs;
            }

            /// <inheritdoc />
            protected override void PerformLayoutBeforeChildren()
            {
                base.PerformLayoutBeforeChildren();

                // Arrange tab header controls
                Float2 pos = Float2.Zero;
                Float2 sizeMask = m_Tabs.m_Orientation == Orientation.Horizontal ? Float2.UnitX : Float2.UnitY;
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is TabHeader tabHeader)
                    {
                        tabHeader.Location = pos;
                        pos += tabHeader.Size * sizeMask;
                    }
                }
            }
        }

        /// <summary>
        /// The selected tab index.
        /// </summary>
        protected int m_SelectedIndex;

        /// <summary>
        /// The tabs size.
        /// </summary>
        protected Float2 m_TabsSize;

        /// <summary>
        /// Automatic tab size based on the fill axis.
        /// </summary>
        protected bool m_AutoTabsSizeAuto;

        /// <summary>
        /// The orientation.
        /// </summary>
        protected Orientation m_Orientation;

        /// <summary>
        /// Gets the size of the tabs.
        /// </summary>
        public Float2 TabsSize
        {
            get => m_TabsSize;
            set
            {
                m_TabsSize = value;
                if (!m_AutoTabsSizeAuto)
                {
                    for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                    {
                        if (TabsPanel.Children[i] is TabHeader tabHeader)
                        {
                            tabHeader.Size = value;
                        }
                    }
                }
                PerformLayout();
            }
        }

        /// <summary>
        /// Enables automatic tabs size to fill the space.
        /// </summary>
        public bool AutoTabsSize
        {
            get => m_AutoTabsSizeAuto;
            set
            {
                if (m_AutoTabsSizeAuto == value)
                {
                    return;
                }
                m_AutoTabsSizeAuto = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the orientation.
        /// </summary>
        public Orientation Orientation
        {
            get => m_Orientation;
            set
            {
                if (m_Orientation == value)
                {
                    return;
                }
                m_Orientation = value;
                if (UseScroll)
                {
                    TabsPanel.SetScrollBars(m_Orientation == Orientation.Horizontal ? ScrollBars.Horizontal : ScrollBars.Vertical);
                }
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the color of the tab strip background.
        /// </summary>
        public Color TabStripColor
        {
            get => TabsPanel.BackgroundColor;
            set => TabsPanel.BackgroundColor = value;
        }

        /// <summary>
        /// Occurs when selected tab gets changed.
        /// </summary>
        public event Action<Tabs> SelectedTabChanged;

        /// <summary>
        /// The tabs panel control.
        /// </summary>
        public readonly TabsHeader TabsPanel;

        /// <summary>
        /// Gets or sets the selected tab.
        /// </summary>
        public Tab SelectedTab
        {
            get => m_SelectedIndex < 0 || Children.Count <= (m_SelectedIndex+1) ? null : Children[m_SelectedIndex + 1] as Tab;
            set => SelectedTabIndex = value != null ? Children.IndexOf(value) - 1 : -1;
        }

        /// <summary>
        /// Gets or sets the selected tab index.
        /// </summary>
        public int SelectedTabIndex
        {
            get => m_SelectedIndex;
            set
            {
                int index = value;
                int tabsCount = Children.Count - 1;

                // Clamp index
                if (index < -1)
                {
                    index = -1;
                }
                else if (index >= tabsCount)
                {
                    index = tabsCount - 1;
                }

                // Check if index will change
                if (m_SelectedIndex != index)
                {
                    Tab prev = SelectedTab;
                    if (prev != null)
                    {
                        prev.m_SelectedInTabs = null;
                        prev.OnDeselected();
                    }
                    m_SelectedIndex = index;
                    PerformLayout();
                    OnSelectedTabChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether use scroll bars for the tabs.
        /// </summary>
        public bool UseScroll
        {
            get => TabsPanel.GetScrollBars() != ScrollBars.None;
            set
            {
                if (value)
                {
                    TabsPanel.SetScrollBars(m_Orientation == Orientation.Horizontal ? ScrollBars.Horizontal : ScrollBars.Vertical);
                }
                else
                {
                    TabsPanel.SetScrollBars(ScrollBars.None);
                }
            }
        }

        /// <summary>
        /// Gets or sets the horizontal tabs text alignment within the tab titles bounds.
        /// </summary>
        public TextAlignment TabsTextHorizontalAlignment { get; set; } = TextAlignment.Near;

        /// <summary>
        /// Gets or sets the vertical tabs text alignment within the tab titles bounds.
        /// </summary>
        public TextAlignment TabsTextVerticalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Initializes a new instance of the <see cref="Tabs"/> class.
        /// </summary>
        public Tabs()
        {
            AutoFocus = false;
            BackgroundColor = Style.Current.Background;

            m_SelectedIndex = -1;
            m_TabsSize = new Float2(70, 16);
            m_Orientation = Orientation.Horizontal;

            TabsPanel = new TabsHeader(this);

            TabStripColor = Style.Current.LightBackground;

            TabsPanel.Parent = this;
        }

        /// <summary>
        /// Adds the tab.
        /// </summary>
        /// <typeparam name="T">Tab control type.</typeparam>
        /// <param name="tab">The tab.</param>
        /// <returns>The tab.</returns>
        public T AddTab<T>(T tab) where T : Tab
        {
            if (tab == null)
            {
                throw new ArgumentNullException();
            }
            Debug.Assert(!Children.Contains(tab), "Tab already belongs to this container.");

            // Add
            tab.Parent = this;

            // Check if has no selected tab
            if (m_SelectedIndex == -1)
            {
                SelectedTab = tab;
            }

            return tab;
        }

        /// <summary>
        /// Called when selected tab gets changed.
        /// </summary>
        protected virtual void OnSelectedTabChanged()
        {
            Tab selectedTab = SelectedTab;
            SelectedTabChanged?.Invoke(this);
            if (selectedTab != null)
            {
                selectedTab.m_SelectedInTabs = this;
                selectedTab.OnSelected();
            }
        }

        /// <inheritdoc />
        public override void OnChildrenChanged()
        {
            bool wasLocked = TabsPanel.IsLayoutLocked;
            TabsPanel.IsLayoutLocked = true;

            // Update tabs headers
            TabsPanel.DisposeChildren();
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Tab tab)
                {
                    TabsPanel.AddChild(tab.CreateHeader());
                }
            }

            TabsPanel.IsLayoutLocked = wasLocked;

            base.OnChildrenChanged();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Update all controls except not selected tabs
            Tab selectedTab = SelectedTab;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i].Enabled && (!(m_Children[i] is Tab) || m_Children[i] == selectedTab))
                {
                    m_Children[i].Update(deltaTime);
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            Float2 tabsSize = m_TabsSize;
            if (m_AutoTabsSizeAuto)
            {
                // Horizontal is default for Toolbox so tabs go to the right
                int tabsCount = 0;
                for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                {
                    if (TabsPanel.Children[i] is TabHeader)
                    {
                        tabsCount++;
                    }
                }
                if (tabsCount == 0)
                {
                    tabsCount = 1;
                }
                if (m_Orientation == Orientation.Horizontal)
                {
                    tabsSize.X = Width / tabsCount;
                }
                else
                {
                    tabsSize.Y = Height / tabsCount;
                }
                for (int i = 0; i < TabsPanel.ChildrenCount; i++)
                {
                    if (TabsPanel.Children[i] is TabHeader tabHeader)
                    {
                        tabHeader.Size = tabsSize;
                    }
                }
            }
            else if (UseScroll)
            {
                // If scroll bar is visible it covers part of the tab header so include this in tab size to improve usability
                if (m_Orientation == Orientation.Horizontal && TabsPanel.hScrollBar.Visible)
                {
                    tabsSize.Y += TabsPanel.hScrollBar.Height;
                    Style style = Style.Current;
                    TabsPanel.hScrollBar.TrackColor = style.Background;
                    TabsPanel.hScrollBar.ThumbColor = style.ForegroundGrey;
                }
                else if (m_Orientation == Orientation.Vertical && TabsPanel.vScrollBar.Visible)
                {
                    tabsSize.X += TabsPanel.vScrollBar.Width;
                    Style style = Style.Current;
                    TabsPanel.vScrollBar.TrackColor = style.Background;
                    TabsPanel.vScrollBar.ThumbColor = style.ForegroundGrey;
                }
            }

            // Fit the tabs panel
            TabsPanel.Size = m_Orientation == Orientation.Horizontal
                             ? new Float2(Width, tabsSize.Y)
                             : new Float2(tabsSize.X, Height);

            // Hide all pages except selected one
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is Tab tab)
                {
                    if (i - 1 == m_SelectedIndex)
                    {
                        // Show and fit size
                        tab.Visible = true;
                        tab.Bounds = m_Orientation == Orientation.Horizontal
                                     ? new Rectangle(0, tabsSize.Y, Width, Height - tabsSize.Y)
                                     : new Rectangle(tabsSize.X, 0, Width - tabsSize.X, Height);
                    }
                    else
                    {
                        // Hide
                        tab.Visible = false;
                    }
                }
            }
        }
    }
}
