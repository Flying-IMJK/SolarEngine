
using System;
using System.IO;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Assets picking control.
    /// </summary>
    /// <seealso cref="Control" />
    /// <seealso cref="IContentItemOwner" />
    [HideInEditor]
    public class AssetPicker : Control
    {
        private const float DefaultIconSize = 64;
        private const float ButtonsOffset = 2;
        private const float ActionIconSize = 12;

        private bool m_IsMouseDown;
        private Float2 m_MouseDownPos;
        private Float2 m_MousePos;
        private DragItems m_DragOverElement;

        /// <summary>
        /// The asset validator. Used to ensure only appropriate items can be picked.
        /// </summary>
        public AssetPickerValidator Validator { get; }

        /// <summary>
        /// Occurs when selected item gets changed.
        /// </summary>
        public event Action SelectedItemChanged;

        /// <summary>
        /// False if changing selected item is disabled.
        /// </summary>
        public bool CanEdit = true;

        /// <summary>
        /// Utility flag used to indicate that there are different values assigned to this reference editor and user
        /// should be informed about it.
        /// </summary>
        public bool DifferentValues;

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPicker"/> class.
        /// </summary>
        public AssetPicker()
        : this(typeof(Asset), Float2.Zero)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPicker"/> class.
        /// </summary>
        /// <param name="assetType">The asset types that this picker accepts.</param>
        /// <param name="location">The control location.</param>
        public AssetPicker(Type assetType, Float2 location)
        : base(location, new Float2(DefaultIconSize + (ButtonsOffset + DefaultIconSize * 2 / 3) * 2, DefaultIconSize))
        {
            Validator = new AssetPickerValidator(assetType);
            Validator.SelectedItemChanged += OnSelectedItemChanged;
            m_MousePos = Float2.Minimum;
        }

        /// <summary>
        /// Called when selected item gets changed.
        /// </summary>
        protected virtual void OnSelectedItemChanged()
        {
            if (IsDisposing)
            {
                return;
            }

            // Update tooltip
            string tooltip;
            if (Validator.SelectedItem is AssetItem assetItem)
            {
                tooltip = assetItem.NamePath;
            }
            else
            {
                tooltip = Validator.SelectedPath;
            }
            TooltipText = tooltip;

            SelectedItemChanged?.Invoke();
        }

        private void DoDrag()
        {
            // Do the drag drop operation if has selected element
            if (new Rectangle(Float2.Zero, Size).Contains(ref m_MouseDownPos))
            {
                if (Validator.SelectedAsset != null)
                {
                    DoDragDrop(DragItems.GetDragData(Validator.SelectedAsset.GetPath()));
                }
                else if (Validator.SelectedItem != null)
                {
                    DoDragDrop(DragItems.GetDragData(Validator.SelectedItem));
                }
            }
        }

        private Rectangle IconRect => new Rectangle(0, 0, Height, Height);

        private float NameFieldHeight => Height / 3;

        private Rectangle NameFieldRect => new Rectangle(Height + ButtonsOffset, 0,
            Mathf.Max(0, Width - Height - ButtonsOffset), NameFieldHeight);

        private Rectangle Button1Rect => new Rectangle(Mathf.Max(Height + ButtonsOffset, Width - ButtonsOffset - NameFieldHeight),
            0, NameFieldHeight, NameFieldHeight);

        private Rectangle Button2Rect
        {
            get
            {
                float size = Height - NameFieldHeight;
                return new Rectangle(Height + ButtonsOffset, NameFieldHeight, size, size);
            }
        }

        private Rectangle Button3Rect
        {
            get
            {
                float size = Height - NameFieldHeight;
                return new Rectangle(Height + ButtonsOffset * 2 + size, NameFieldHeight, size, size);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            Style style = Style.Current;
            Rectangle iconRect = IconRect;
            Rectangle button1Rect = Button1Rect;
            Rectangle button2Rect = Button2Rect;
            Rectangle button3Rect = Button3Rect;

            // Draw fixed-size icons centered inside their button hit areas.
            Rectangle pickerIconRect = new Rectangle(button1Rect.X + (button1Rect.Width - ActionIconSize) * 0.5f,
                button1Rect.Y + (button1Rect.Height - ActionIconSize) * 0.5f, ActionIconSize, ActionIconSize);
            Rectangle locateIconRect = new Rectangle(button2Rect.X + (button2Rect.Width - ActionIconSize) * 0.5f,
                button2Rect.Y + (button2Rect.Height - ActionIconSize) * 0.5f, ActionIconSize, ActionIconSize);
            Rectangle clearIconRect = new Rectangle(button3Rect.X + (button3Rect.Width - ActionIconSize) * 0.5f,
                button3Rect.Y + (button3Rect.Height - ActionIconSize) * 0.5f, ActionIconSize, ActionIconSize);

            // Use the top third for the name and the remaining height for the action buttons.
            Render2D.FillRectangle(NameFieldRect, style.BackgroundNormal);
            Render2D.DrawRectangle(NameFieldRect, style.Background);

            // Draw asset picker button
            if (CanEdit)
            {
                Render2D.DrawSprite(EditorIcons.ArrowDown12, pickerIconRect, button1Rect.Contains(m_MousePos) ? style
                    .Foreground : style.ForegroundGrey);
            }

            // Keep locate visible even when there is no content item to select.
            bool canLocate = !DifferentValues && Validator.SelectedItem != null;
            Color locateColor = canLocate ? style.ForegroundGrey : style.ForegroundDisabled;
            if (canLocate && button2Rect.Contains(m_MousePos))
            {
                locateColor = style.Foreground;
            }
            Render2D.DrawSprite(EditorIcons.Search12, locateIconRect, locateColor);

            if (DifferentValues)
            {
                // No element selected
                Render2D.FillRectangle(iconRect, style.BackgroundNormal);
                Render2D.DrawText(style.FontMedium, "Multiple\nValues", iconRect, style.Foreground, TextAlignment
                    .Center, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, Height / DefaultIconSize);
            }
            else if (Validator.SelectedItem != null)
            {
                // Draw item preview
                Validator.SelectedItem.DrawThumbnail(iconRect);

                // Draw remove button
                if (CanEdit)
                {
                    Render2D.DrawSprite(EditorIcons.Cross16, clearIconRect, button3Rect.Contains(m_MousePos) ? style
                        .Foreground : style.ForegroundGrey);
                }
            }
            // Check if has no item but has an asset (eg. virtual asset)
            else if (Validator.SelectedAsset)
            {
                // Draw remove button
                if (CanEdit)
                {
                    Render2D.DrawSprite(EditorIcons.Cross16, clearIconRect, button3Rect.Contains(m_MousePos) ? style
                        .Foreground : style.ForegroundGrey);
                }
                Render2D.FillRectangle(iconRect, style.BackgroundNormal);
            }
            else
            {
                // No element selected
                Render2D.FillRectangle(iconRect, style.BackgroundNormal);
                Render2D.DrawText(style.FontMedium, "No asset\nselected", iconRect, Color.Orange, TextAlignment
                    .Center, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, Height / DefaultIconSize);
            }

            // Draw the reference name once for both file-backed and virtual assets.
            Rectangle nameRect = new Rectangle(NameFieldRect.X + ButtonsOffset, 0,
                Mathf.Max(0, button1Rect.X - NameFieldRect.X - ButtonsOffset * 2), NameFieldHeight);
            if (nameRect.Width > 0)
            {
                string name = "None";
                if (DifferentValues)
                {
                    name = "Multiple Values";
                }
                else if (Validator.SelectedItem != null)
                {
                    name = Validator.SelectedItem.ShortName;
                }
                else if (Validator.SelectedAsset)
                {
                    name = Validator.SelectedAsset.GetType().Name;
                }
                Render2D.PushClip(nameRect);
                Render2D.DrawText(
                                  style.FontSmall,
                                  name,
                                  nameRect,
                                  style.Foreground,
                                  TextAlignment.Near,
                                  TextAlignment.Center);
                Render2D.PopClip();
            }

            // Check if drag is over
            if (IsDragOver() && m_DragOverElement != null && m_DragOverElement.HasValidDrag)
            {
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder);
            }

            // Navigation focus highlight
            if (IsNavFocused)
            {
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                Render2D.DrawRectangle(bounds, style.BackgroundSelected);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Validator.OnDestroy();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            m_MousePos = Float2.Minimum;

            // Check if start drag drop
            if (m_IsMouseDown)
            {
                m_IsMouseDown = false;
                DoDrag();
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            m_MousePos = location;
            m_MouseDownPos = Float2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            m_MousePos = location;

            // Check if start drag drop
            if (m_IsMouseDown && Float2.Distance(location, m_MouseDownPos) > 10.0f && IconRect.Contains(m_MouseDownPos))
            {
                m_IsMouseDown = false;
                DoDrag();
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_IsMouseDown)
            {
                m_IsMouseDown = false;
                bool canLocate = !DifferentValues && Validator.SelectedItem != null;

                // Buttons logic
                if (!CanEdit)
                {
                    if (Button2Rect.Contains(location) && canLocate)
                    {
                        // Select asset
                        Editor.Instance.Windows.ContentWin.Select(Validator.SelectedItem);
                        Editor.Instance.Windows.ContentWin.ClearItemsSearch();
                    }
                }
                else if (Button1Rect.Contains(location))
                {
                    // Show picker
                    Focus();
                    OnSubmit();
                }
                else if (Validator.SelectedAsset != null || Validator.SelectedItem != null)
                {
                    if (Button2Rect.Contains(location) && canLocate)
                    {
                        // Select asset
                        Editor.Instance.Windows.ContentWin.Select(Validator.SelectedItem);
                        Editor.Instance.Windows.ContentWin.ClearItemsSearch();
                    }
                    else if (Button3Rect.Contains(location))
                    {
                        // Deselect asset
                        Focus();
                        if (Validator.SelectedItem != null)
                        {
                            Validator.SelectedItem = null;
                        }
                        else
                        {
                            // Virtual assets have no content item to clear.
                            Validator.SelectedAsset = null;
                        }
                    }
                }
            }
            if (button == MouseButton.Middle && IconRect.Contains(ref location) && CanEdit)
            {
                // Clear value
                Focus();
                if (Validator.SelectedItem != null)
                {
                    Validator.SelectedItem = null;
                }
                else
                {
                    Validator.SelectedAsset = null;
                }
            }
            if (button == MouseButton.Right && IconRect.Contains(ref location) && CanEdit)
            {
                // Show picker
                Focus();
                OnSubmit();
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_IsMouseDown = true;
                m_MouseDownPos = location;
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Focus();

            if (Validator.SelectedItem != null && IconRect.Contains(location))
            {
                // Open it
                Editor.Instance.Windows.ContentWin.Open(Validator.SelectedItem);
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            // Check if drop asset
            if (m_DragOverElement == null)
            {
                m_DragOverElement = new DragItems(Validator.IsValid, path => Editor.Instance.AssetDatabase.Find(path));
            }
            if (CanEdit && m_DragOverElement.OnDragEnter(data))
            {
            }

            return m_DragOverElement.Effect;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            return m_DragOverElement.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            // Clear cache
            m_DragOverElement.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            base.OnDragDrop(ref location, data);

            if (CanEdit && m_DragOverElement.HasValidDrag)
            {
                // Select element
                Validator.SelectedItem = m_DragOverElement.Objects[0];
            }

            // Clear cache
            m_DragOverElement.OnDragDrop();

            return DragDropEffect.Move;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            if (Validator.AssetType != null)
            {
                // Show asset picker popup
                AssetSearchPopup popup = AssetSearchPopup.Show(this, NameFieldRect.BottomLeft, Validator.IsValid, item =>
                {
                    Validator.SelectedItem = item;
                    RootWindow().Focus();
                    Focus();
                });
                if (Validator.SelectedAsset != null)
                {
                    string selectedAssetName = Path.GetFileNameWithoutExtension(Validator.SelectedAsset.GetPath());
                    popup.ScrollToAndHighlightItemByName(selectedAssetName);
                }
            }
            else
            {
                // Show content item picker popup
                ContentSearchPopup popup = ContentSearchPopup.Show(this, NameFieldRect.BottomLeft, Validator.IsValid,
                    item =>
                {
                    Validator.SelectedItem = item;
                    RootWindow().Focus();
                    Focus();
                });
                if (Validator.SelectedItem != null)
                {
                    popup.ScrollToAndHighlightItemByName(Validator.SelectedItem.ShortName);
                }
            }
        }
    }
}
