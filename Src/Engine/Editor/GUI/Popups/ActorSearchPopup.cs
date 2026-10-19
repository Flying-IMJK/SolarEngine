// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Popup that shows the list of actors to pick. Supports searching and basic type filtering.
    /// </summary>
    /// <seealso cref="SE.Editor.GUI.ItemsListContextMenu" />
    public class ActorSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The actor item.
        /// </summary>
        /// <seealso cref="SE.Editor.GUI.ItemsListContextMenu.Item" />
        public class ActorItemView : Item
        {
            private Actor m_Actor;

            /// <summary>
            /// Gets the actor.
            /// </summary>
            public Actor Actor => m_Actor;

            /// <summary>
            /// Initializes a new instance of the <see cref="ActorItemView"/> class.
            /// </summary>
            /// <param name="actor">The actor.</param>
            public ActorItemView(Actor actor)
            {
                m_Actor = actor;
                Name = actor.Name;
                TooltipText = actor.ToString();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                m_Actor = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Validates if the given actor item can be used to pick it.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(Actor actor);

        private IsValidDelegate m_IsValid;
        private Action<Actor> m_Selected;

        private ActorSearchPopup(IsValidDelegate isValid, Action<Actor> selected)
        {
            m_IsValid = isValid;
            m_Selected = selected;

            ItemClicked += OnItemClicked;

            // SolarEngine currently exposes loaded scenes without a prefab presenter context.
            for (int i = 0; i < Level.ScenesCount; i++)
            {
                Find(Level.GetScene(i));
            }

            SortItems();
        }

        private void OnItemClicked(Item item)
        {
            m_Selected(((ActorItemView)item).Actor);
        }

        private void Find(Actor actor)
        {
            if (!actor)
            {
                return;
            }

            if (m_IsValid(actor))
            {
                AddItem(new ActorItemView(actor));
            }

            for (int i = 0; i < actor.ChildrenCount; i++)
            {
                Find(actor.GetChild(i));
            }
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="isValid">Event called to check if a given actor item is valid to be used.</param>
        /// <param name="selected">Event called on actor item pick.</param>
        /// <returns>The dialog.</returns>
        public static ActorSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid,
            Action<Actor> selected)
        {
            ActorSearchPopup popup = new ActorSearchPopup(isValid, selected);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            m_IsValid = null;
            m_Selected = null;

            base.OnDestroy();
        }
    }
}
