using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Edits actor static flags and offers to apply changes to child actors.
    /// </summary>
    public sealed class ActorStaticFlagsEditor : EnumEditor
    {
        protected override void OnValueChanged()
        {
            StaticMask value = (StaticMask)element.EnumTypeValue;

            // Keep the normal property edit path unless a single actor has children.
            if (Values.IsSingleObject && (StaticMask)Values[0] != value &&
                ParentEditor.Values[0] is Actor actor && actor.ChildrenCount > 0)
            {
                DialogResult answer = MessageBox.Show(
                    "Do you want to set static flags to all child actors as well?",
                    "Change actor static flags", MessageBoxButtons.YesNoCancel);
                if (answer == DialogResult.Cancel)
                {
                    return;
                }
                if (answer == DialogResult.Yes)
                {
                    List<Actor> actors = new List<Actor> { actor };
                    for (int i = 0; i < actors.Count; i++)
                    {
                        Actor parent = actors[i];
                        for (int childIndex = 0; childIndex < parent.ChildrenCount; childIndex++)
                        {
                            actors.Add(parent.GetChild(childIndex));
                        }
                    }

                    // SolarEngine currently has no editor undo block; update the whole tree through the Actor property.
                    foreach (Actor child in actors)
                    {
                        child.StaticFlags = value;
                    }
                    Values.Refresh(ParentEditor.Values);
                    return;
                }
            }

            base.OnValueChanged();
        }
    }
}
