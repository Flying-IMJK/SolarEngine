using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Actor))]
    internal class ActorEditor : GenericEditor
    {
        protected override List<ItemInfo> GetItemsForType(Type type)
        {
            List<ItemInfo> items = base.GetItemsForType(type);

            items.RemoveAll(item => 
            {
                return item.Name == nameof(Actor.Name) || 
                item.Name == nameof(Actor.IsActive) || 
                item.Name == nameof(Actor.Layer) || 
                item.Name == nameof(Actor.LayerName) || 
                item.Name == nameof(Actor.StaticFlags) ||
                item.Name == nameof(Actor.Position) || 
                item.Name == nameof(Actor.Orientation) || 
                item.Name == nameof(Actor.Scale);
            });
            return items;
        }

        public override void Initialize(LayoutElementsContainer layout)
        {
            if (Values.Count == 0 || Values.HasNull)
            {
                base.Initialize(layout);
                return;
            }

            const float Padding = 4;
            const float RowSpacing = 4;
            float rowHeight = TextBox.DefaultHeight;
            float iconSize = rowHeight * 2 + RowSpacing;
            CustomElementsContainer<ContainerControl> header = layout.CustomContainer<ContainerControl>();
            header.ContainerControl.Height = iconSize + Padding * 2;
            header.ContainerControl.AutoFocus = false;
            // The icon spans both the name row and the layer row.
            Image icon = new Image(Padding, Padding, iconSize, iconSize)
            {
                Parent = header.ContainerControl,
                Brush = new SpriteBrush(EditorIcons.Scene128),
                TooltipText = "Actor",
            };

            CustomElementsContainer<ContainerControl> active = header.CustomContainer<ContainerControl>();
            active.ContainerControl.AutoFocus = false;
            active.ContainerControl.Bounds = new Rectangle(icon.Right + Padding, Padding, 18, rowHeight);
            AddActorProperty(active, nameof(Actor.IsActive), "Is Active", displayStyle: DisplayStyle.InlineIntoParent);
            foreach (Control control in active.ContainerControl.Children)
            {
                control.Bounds = new Rectangle(0, 0, 18, rowHeight);
                control.TooltipText = "Is Active";
            }

            CustomElementsContainer<ContainerControl> name = header.CustomContainer<ContainerControl>();
            name.ContainerControl.AutoFocus = false;
            name.ContainerControl.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            name.ContainerControl.Offsets = new Margin(active.ContainerControl.Right + Padding, Padding, Padding, rowHeight);
            // The basic text editor preserves the user's in-progress text during refresh.
            AddActorProperty(name, nameof(Actor.Name), "Name", new BasicValueEditor(), DisplayStyle.InlineIntoParent);
            foreach (Control control in name.ContainerControl.Children)
            {
                control.AnchorPreset = AnchorPresets.StretchAll;
                control.Offsets = new Margin(0);
                control.TooltipText = "Name";
            }

            // Keep the two actor settings in equal-width slots as the Properties panel resizes.
            CustomElementsContainer<UniformGridPanel> settings = header.CustomContainer<UniformGridPanel>();
            settings.CustomControl.SlotsHorizontally = 2;
            settings.CustomControl.SlotsVertically = 1;
            settings.CustomControl.SlotSpacing = new Float2(RowSpacing, 0);
            settings.CustomControl.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            settings.CustomControl.Offsets = new Margin(icon.Right + Padding, Padding, Padding + rowHeight + RowSpacing,
                rowHeight);

            CustomElementsContainer<VerticalPanel> layer = settings.CustomContainer<VerticalPanel>();
            layer.CustomControl.Margin = new Margin(0);
            AddActorProperty(layer, nameof(Actor.Layer), "Layer");

            CustomElementsContainer<VerticalPanel> staticFlags = settings.CustomContainer<VerticalPanel>();
            staticFlags.CustomControl.Margin = new Margin(0);
            AddActorProperty(staticFlags, nameof(Actor.StaticFlags), "Static", new ActorStaticFlagsEditor());
            layout.Space(4);

            GroupElement transform = OnGroup(layout, "Transform");
            // Keep the existing world-space properties; Rotation is only a UI label.
            AddActorProperty(transform, nameof(Actor.Position), "Position", new Float3Editor { ShowAxisLabels = true });
            AddActorProperty(transform, nameof(Actor.Orientation), "Rotation", new QuaternionEditor { ShowAxisLabels = true });
            AddActorProperty(transform, nameof(Actor.Scale), "Scale", new Float3Editor { ShowAxisLabels = true });

            base.Initialize(layout);
        }

        private void AddActorProperty(LayoutElementsContainer layout, string memberName, string label,
            CustomEditor overrideEditor = null, DisplayStyle? displayStyle = null)
        {
            ItemInfo item = new ItemInfo(typeof(Actor).GetProperty(memberName));
            CustomEditor editor = layout.Property(label, item.GetValues(Values), overrideEditor ?? item.OverrideEditor,
                item.TooltipText, displayStyle);
            editor.SetReadOnly(item.IsReadOnly);
        }
    }
}
