using System;
using System.Collections.Generic;
using System.Globalization;
using SE.Editor.Gizmo;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    public partial class EditorGizmoViewport
    {
        private ToolStrip m_ToolStrip;
        private readonly List<ContextMenu> m_ToolStripMenus = new();
        private ViewportWidgetButton m_MoveButton;
        private ViewportWidgetButton m_RotateButton;
        private ViewportWidgetButton m_ScaleButton;
        private ViewportWidgetButton m_SpaceButton;
        private ViewportWidgetButton m_PivotButton;
        private ViewportWidgetButton m_TranslationSnapButton;
        private ViewportWidgetButton m_RotationSnapButton;
        private ViewportWidgetButton m_ScaleSnapButton;
        private ViewportWidgetButton m_TranslationStepButton;
        private ViewportWidgetButton m_RotationStepButton;
        private ViewportWidgetButton m_ScaleStepButton;

        private void BuildToolStrip(ToolStrip toolStrip)
        {
            ArgumentNullException.ThrowIfNull(toolStrip);
            m_ToolStrip = toolStrip;

            // Keep the existing camera settings menu and its bindings, with the camera group first.
            _cameraButton.Parent = toolStrip;
            _orthographicModeButton.Parent = toolStrip;
            _orthographicModeButton.Tag = this;
            _orthographicModeButton.Toggled += button => { UpdateToolStrip(); Focus(); };
            m_ToolStripMenus.Add(_cameraButton.ContextMenu);
            _cameraWidget.Parent = null;
            _cameraWidget.Dispose();
            _cameraWidget = null;
            toolStrip.AddSeparator();

            m_MoveButton = new ViewportWidgetButton(string.Empty, EditorIcons.Translate32)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Translate (W)",
            };
            m_RotateButton = new ViewportWidgetButton(string.Empty, EditorIcons.Rotate32)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Rotate (E)",
            };
            m_ScaleButton = new ViewportWidgetButton(string.Empty, EditorIcons.Scale32)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Scale (R)",
            };
            m_MoveButton.Clicked += button => { TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate; Focus(); };
            m_RotateButton.Clicked += button => { TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate; Focus(); };
            m_ScaleButton.Clicked += button => { TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale; Focus(); };
            toolStrip.AddSeparator();

            ContextMenu spaceMenu = new ContextMenu();
            ContextMenuButton world = spaceMenu.AddButton("World", () => TransformGizmo.ActiveTransformSpace = TransformGizmoBase.TransformSpace.World);
            world.Icon = EditorIcons.Globe32;
            ContextMenuButton local = spaceMenu.AddButton("Local", () => TransformGizmo.ActiveTransformSpace = TransformGizmoBase.TransformSpace.Local);
            local.Icon = EditorIcons.LocalAxes32;
            spaceMenu.VisibleChanged += control =>
            {
                world.Checked = TransformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.World;
                local.Checked = TransformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.Local;
            };
            m_ToolStripMenus.Add(spaceMenu);
            m_SpaceButton = new ViewportWidgetButton(string.Empty, EditorIcons.Globe32, spaceMenu)
            {
                Parent = toolStrip, Tag = this,
            };

            ContextMenu pivotMenu = new ContextMenu();
            ContextMenuButton selectionPivot = pivotMenu.AddButton("Selection center",
                () => TransformGizmo.ActivePivot = TransformGizmoBase.PivotType.SelectionCenter);
            selectionPivot.Icon = EditorIcons.PivotSelection32;
            ContextMenuButton objectPivot = pivotMenu.AddButton("Object center",
                () => TransformGizmo.ActivePivot = TransformGizmoBase.PivotType.ObjectCenter);
            objectPivot.Icon = EditorIcons.PivotObject32;
            ContextMenuButton worldPivot = pivotMenu.AddButton("World origin",
                () => TransformGizmo.ActivePivot = TransformGizmoBase.PivotType.WorldOrigin);
            worldPivot.Icon = EditorIcons.PivotWorldOrigin32;
            pivotMenu.VisibleChanged += control =>
            {
                selectionPivot.Checked = TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.SelectionCenter;
                objectPivot.Checked = TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.ObjectCenter;
                worldPivot.Checked = TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.WorldOrigin;
            };
            m_ToolStripMenus.Add(pivotMenu);
            m_PivotButton = new ViewportWidgetButton(string.Empty, EditorIcons.PivotSelection32, pivotMenu)
            {
                Parent = toolStrip, Tag = this,
            };
            toolStrip.AddSeparator();

            m_TranslationSnapButton = new ViewportWidgetButton(string.Empty, EditorIcons.Grid32, autoCheck: true)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Toggle translation snapping. Ctrl temporarily enables snapping.",
            };
            m_TranslationSnapButton.Toggled += button =>
            {
                TransformGizmo.TranslationSnapEnable = button.Checked;
                Focus();
            };
            ContextMenu translationMenu = new ContextMenu();
            foreach (float step in new float[] { 1, 5, 10, 25, 50, 100 })
            {
                ContextMenuButton item = translationMenu.AddButton(step.ToString(CultureInfo.InvariantCulture),
                    () => { TransformGizmo.TranslationSnapValue = step; UpdateToolStrip(); });
                item.Tag = step;
            }
            translationMenu.VisibleChanged += control =>
            {
                foreach (ContextMenuItem item in translationMenu.Items)
                {
                    if (item is ContextMenuButton button && button.Tag is float step)
                    {
                        button.Checked = Mathf.NearEqual(step, TransformGizmo.TranslationSnapValue);
                    }
                }
            };
            m_ToolStripMenus.Add(translationMenu);
            m_TranslationStepButton = new ViewportWidgetButton(string.Empty, SpriteHandle.Invalid, translationMenu)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Translation snap step in world units",
            };

            m_RotationSnapButton = new ViewportWidgetButton(string.Empty, EditorIcons.RotateSnap32, autoCheck: true)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Toggle rotation snapping. Ctrl temporarily enables snapping.",
            };
            m_RotationSnapButton.Toggled += button =>
            {
                TransformGizmo.RotationSnapEnabled = button.Checked;
                Focus();
            };
            ContextMenu rotationMenu = new ContextMenu();
            foreach (float step in new float[] { 1, 5, 10, 15, 30, 45, 90 })
            {
                ContextMenuButton item = rotationMenu.AddButton(step.ToString(CultureInfo.InvariantCulture) + "°",
                    () => { TransformGizmo.RotationSnapValue = step; UpdateToolStrip(); });
                item.Tag = step;
            }
            rotationMenu.VisibleChanged += control =>
            {
                foreach (ContextMenuItem item in rotationMenu.Items)
                {
                    if (item is ContextMenuButton button && button.Tag is float step)
                    {
                        button.Checked = Mathf.NearEqual(step, TransformGizmo.RotationSnapValue);
                    }
                }
            };
            m_ToolStripMenus.Add(rotationMenu);
            m_RotationStepButton = new ViewportWidgetButton(string.Empty, SpriteHandle.Invalid, rotationMenu)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Rotation snap step in degrees",
            };

            m_ScaleSnapButton = new ViewportWidgetButton(string.Empty, EditorIcons.ScaleSnap32, autoCheck: true)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Toggle scale snapping. Ctrl temporarily enables snapping.",
            };
            m_ScaleSnapButton.Toggled += button =>
            {
                TransformGizmo.ScaleSnapEnabled = button.Checked;
                Focus();
            };
            ContextMenu scaleMenu = new ContextMenu();
            foreach (float step in new float[] { 0.01f, 0.05f, 0.1f, 0.25f, 0.5f, 1.0f })
            {
                ContextMenuButton item = scaleMenu.AddButton(step.ToString(CultureInfo.InvariantCulture),
                    () => { TransformGizmo.ScaleSnapValue = step; UpdateToolStrip(); });
                item.Tag = step;
            }
            scaleMenu.VisibleChanged += control =>
            {
                foreach (ContextMenuItem item in scaleMenu.Items)
                {
                    if (item is ContextMenuButton button && button.Tag is float step)
                    {
                        button.Checked = Mathf.NearEqual(step, TransformGizmo.ScaleSnapValue);
                    }
                }
            };
            m_ToolStripMenus.Add(scaleMenu);
            m_ScaleStepButton = new ViewportWidgetButton(string.Empty, SpriteHandle.Invalid, scaleMenu)
            {
                Parent = toolStrip, Tag = this, TooltipText = "Scale snap step",
            };
            UpdateToolStrip();
        }

        private void UpdateToolStrip()
        {
            // Projection and snap settings are existing public fields; reflect their current values too.
            string speedText = MovementSpeed.ToString("0.##", CultureInfo.InvariantCulture);
            if (_cameraButton.Text != speedText)
            {
                _cameraButton.Text = speedText;
            }
            _orthographicModeButton.Checked = !UseOrthographicProjection;
            _orthographicModeButton.Icon = UseOrthographicProjection ? EditorIcons.Orthographic32 : EditorIcons.Perspective32;
            _orthographicModeButton.TooltipText = UseOrthographicProjection ? "Orthographic — switch to perspective" : "Perspective — switch to orthographic";

            m_MoveButton.Checked = TransformGizmo.ActiveMode == TransformGizmoBase.Mode.Translate;
            m_RotateButton.Checked = TransformGizmo.ActiveMode == TransformGizmoBase.Mode.Rotate;
            m_ScaleButton.Checked = TransformGizmo.ActiveMode == TransformGizmoBase.Mode.Scale;
            bool local = TransformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.Local;
            m_SpaceButton.Icon = local ? EditorIcons.LocalAxes32 : EditorIcons.Globe32;
            m_SpaceButton.TooltipText = local ? "Local coordinates (X)" : "World coordinates (X)";
            switch (TransformGizmo.ActivePivot)
            {
                case TransformGizmoBase.PivotType.ObjectCenter:
                    m_PivotButton.Icon = EditorIcons.PivotObject32;
                    m_PivotButton.TooltipText = "Pivot: object center (P)";
                    break;
                case TransformGizmoBase.PivotType.SelectionCenter:
                    m_PivotButton.Icon = EditorIcons.PivotSelection32;
                    m_PivotButton.TooltipText = "Pivot: selection center (P)";
                    break;
                case TransformGizmoBase.PivotType.WorldOrigin:
                    m_PivotButton.Icon = EditorIcons.PivotWorldOrigin32;
                    m_PivotButton.TooltipText = "Pivot: world origin (P)";
                    break;
            }
            m_TranslationSnapButton.Checked = TransformGizmo.TranslationSnapEnable;
            m_RotationSnapButton.Checked = TransformGizmo.RotationSnapEnabled;
            m_ScaleSnapButton.Checked = TransformGizmo.ScaleSnapEnabled;
            string translation = TransformGizmo.TranslationSnapValue.ToString("0.##", CultureInfo.InvariantCulture);
            string rotation = TransformGizmo.RotationSnapValue.ToString("0.##", CultureInfo.InvariantCulture) + "°";
            string scale = TransformGizmo.ScaleSnapValue.ToString("0.##", CultureInfo.InvariantCulture);
            if (m_TranslationStepButton.Text != translation)
            {
                m_TranslationStepButton.Text = translation;
            }
            if (m_RotationStepButton.Text != rotation)
            {
                m_RotationStepButton.Text = rotation;
            }
            if (m_ScaleStepButton.Text != scale)
            {
                m_ScaleStepButton.Text = scale;
            }
        }

        private void DestroyToolStripItems()
        {
            // The window owns the strip; the viewport owns its buttons, bindings, and popup menus.
            while (m_ToolStrip.ChildrenCount != 0)
            {
                Control item = m_ToolStrip.GetChild(0);
                item.Parent = null;
                item.Dispose();
            }
            foreach (ContextMenu menu in m_ToolStripMenus)
            {
                menu.Dispose();
            }
            m_ToolStripMenus.Clear();
        }
    }
}
