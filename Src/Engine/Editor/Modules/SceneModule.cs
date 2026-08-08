using System;
using System.Collections.Generic;
using SE.Editor.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Managed scene service. It mirrors the currently loaded native scenes through
    /// the generated <see cref="SE.Level"/> API and owns editor-side selection.
    /// </summary>
    public sealed class SceneModule : EditorModule
    {
        private readonly List<SceneEntry> m_Scenes = new();
        private readonly List<int> m_Selection = new();
        private readonly List<SceneGraphNode> m_NodeSelection = new();
        private int m_RefreshFrames;

        public SceneModule(Editor editor)
            : base(editor)
        {
        }

        public event Action? ScenesChanged;
        public event Action? SelectionChanged;
        public event Action? HierarchyChanged;

        /// <summary>
        /// Owns managed scene hierarchy nodes backed by Reflector-generated
        /// Runtime Actor and Scene bindings.
        /// </summary>
        public SceneGraphFactory SceneGraph { get; } = new();
        public ScenesRootNode Root => SceneGraph.Root;
        public IReadOnlyList<SceneEntry> Scenes => m_Scenes;
        /// <summary>
        /// Selected managed hierarchy nodes. This is the authoritative editor
        /// selection model once Level event bindings are generated.
        /// </summary>
        public IReadOnlyList<SceneGraphNode> SelectedNodes => m_NodeSelection;
        public IReadOnlyList<int> Selection => m_Selection;
        public int SelectionCount => m_NodeSelection.Count != 0 ? m_NodeSelection.Count : m_Selection.Count;
        public bool HasSelection => SelectionCount != 0;

        internal override int Order => -91;

        public void Select(int sceneIndex, bool additive = false)
        {
            if (sceneIndex < 0 || sceneIndex >= m_Scenes.Count)
                return;

            bool changed = false;
            if (!additive && m_NodeSelection.Count != 0)
            {
                m_NodeSelection.Clear();
                changed = true;
            }
            if (!additive && (m_Selection.Count != 1 || m_Selection[0] != sceneIndex))
            {
                m_Selection.Clear();
                changed = true;
            }
            if (!m_Selection.Contains(sceneIndex))
            {
                m_Selection.Add(sceneIndex);
                changed = true;
            }
            if (changed)
                SelectionChanged?.Invoke();
        }

        public void Select(SceneGraphNode node, bool additive = false)
        {
            if (node == null || node.IsDisposed)
                return;

            bool changed = false;
            if (!additive && m_Selection.Count != 0)
            {
                m_Selection.Clear();
                changed = true;
            }
            if (!additive && (m_NodeSelection.Count != 1 || !ReferenceEquals(m_NodeSelection[0], node)))
            {
                m_NodeSelection.Clear();
                changed = true;
            }
            if (!m_NodeSelection.Contains(node))
            {
                m_NodeSelection.Add(node);
                changed = true;
            }
            if (changed)
                SelectionChanged?.Invoke();
        }

        public void Select(IEnumerable<SceneGraphNode> nodes, bool additive = false)
        {
            if (nodes == null)
                throw new ArgumentNullException(nameof(nodes));

            bool changed = false;
            if (!additive && m_Selection.Count != 0)
            {
                m_Selection.Clear();
                changed = true;
            }
            if (!additive && m_NodeSelection.Count != 0)
            {
                m_NodeSelection.Clear();
                changed = true;
            }
            foreach (SceneGraphNode node in nodes)
            {
                if (node != null && !node.IsDisposed && !m_NodeSelection.Contains(node))
                {
                    m_NodeSelection.Add(node);
                    changed = true;
                }
            }
            if (changed)
                SelectionChanged?.Invoke();
        }

        public void Deselect()
        {
            if (m_Selection.Count == 0 && m_NodeSelection.Count == 0)
                return;

            m_Selection.Clear();
            m_NodeSelection.Clear();
            SelectionChanged?.Invoke();
        }

        public bool SaveScene(int sceneIndex)
        {
            return sceneIndex >= 0 && sceneIndex < m_Scenes.Count && SE.Level.SaveSceneAt(sceneIndex);
        }

        public override void OnInit()
        {
            SE.Level.SceneLoaded += OnSceneLoaded;
            SE.Level.SceneUnloaded += OnSceneUnloaded;
            SE.Level.ActorSpawned += OnActorSpawned;
            SE.Level.ActorDeleted += OnActorDeleted;
            SE.Level.ActorParentChanged += OnActorParentChanged;
            SE.Level.ActorOrderInParentChanged += OnActorOrderInParentChanged;
            SE.Level.ActorNameChanged += OnActorChanged;
            SE.Level.ActorActiveChanged += OnActorChanged;
            RefreshScenes();
        }

        public override void OnUpdate()
        {
            if (++m_RefreshFrames < 15)
                return;

            m_RefreshFrames = 0;
            RefreshScenes();
        }

        public override void OnDispose()
        {
            SE.Level.SceneLoaded -= OnSceneLoaded;
            SE.Level.SceneUnloaded -= OnSceneUnloaded;
            SE.Level.ActorSpawned -= OnActorSpawned;
            SE.Level.ActorDeleted -= OnActorDeleted;
            SE.Level.ActorParentChanged -= OnActorParentChanged;
            SE.Level.ActorOrderInParentChanged -= OnActorOrderInParentChanged;
            SE.Level.ActorNameChanged -= OnActorChanged;
            SE.Level.ActorActiveChanged -= OnActorChanged;

            SceneGraph.Dispose();
            m_Scenes.Clear();
            m_Selection.Clear();
            m_NodeSelection.Clear();
            ScenesChanged = null;
            SelectionChanged = null;
            HierarchyChanged = null;
        }

        private void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            RefreshScenes();
            HierarchyChanged?.Invoke();
        }

        private void OnSceneUnloaded(SE.Scene scene, Guid sceneId)
        {
            RemoveActorsForScene(scene);
            RefreshScenes();
            HierarchyChanged?.Invoke();
        }

        private void OnActorSpawned(SE.Actor actor)
        {
            UpsertActor(actor);
            HierarchyChanged?.Invoke();
        }

        private void OnActorDeleted(SE.Actor actor)
        {
            if (actor == null)
                return;

            Guid id = actor.SceneObjectId;
            if (!SceneGraph.Remove(id))
                return;

            m_NodeSelection.RemoveAll(node => node.Id == id || node.IsDisposed);
            SelectionChanged?.Invoke();
            HierarchyChanged?.Invoke();
        }

        private void OnActorParentChanged(SE.Actor actor, SE.Actor previousParent)
        {
            UpsertActor(actor);
            HierarchyChanged?.Invoke();
        }

        private void OnActorOrderInParentChanged(SE.Actor actor)
        {
            UpsertActor(actor);
            HierarchyChanged?.Invoke();
        }

        private void OnActorChanged(SE.Actor actor)
        {
            UpsertActor(actor);
            HierarchyChanged?.Invoke();
        }

        private void UpsertActor(SE.Actor actor)
        {
            if (actor == null)
                return;

            Guid id = actor.SceneObjectId;
            if (id == Guid.Empty)
                return;

            SceneGraphNode parent = ResolveParent(actor);
            if (SceneGraph.FindNode(id) is ActorGraphNode node)
            {
                node.Update(actor);
                node.SetParent(parent);
            }
            else
            {
                node = SceneGraph.Register(new ActorGraphNode(actor), parent);
            }

            ReparentKnownChildren(node);
        }

        private SceneGraphNode ResolveParent(SE.Actor actor)
        {
            SE.Actor parentActor = actor.Parent;
            if (parentActor != null && SceneGraph.FindNode(parentActor.SceneObjectId) is ActorGraphNode parent)
                return parent;

            return SceneGraph.Root;
        }

        private void ReparentKnownChildren(ActorGraphNode parent)
        {
            foreach (SceneGraphNode child in SceneGraph.Nodes.Values)
            {
                if (child is not ActorGraphNode actorNode || ReferenceEquals(actorNode, parent))
                    continue;

                SE.Actor actorParent = actorNode.Actor.Parent;
                if (actorParent != null && actorParent.SceneObjectId == parent.Id)
                    actorNode.SetParent(parent);
            }
        }

        private void SynchronizeSceneGraph()
        {
            HashSet<Guid> seenActors = new();
            int sceneCount = SE.Level.GetScenesCount();
            for (int index = 0; index < sceneCount; index++)
                SynchronizeSceneActors(SE.Level.GetScene(index), seenActors);

            List<Guid> removedActors = new();
            foreach (KeyValuePair<Guid, SceneGraphNode> pair in SceneGraph.Nodes)
            {
                if (!seenActors.Contains(pair.Key))
                    removedActors.Add(pair.Key);
            }

            foreach (Guid id in removedActors)
            {
                SceneGraph.Remove(id);
            }
            if (removedActors.Count != 0)
            {
                m_NodeSelection.RemoveAll(node => node.IsDisposed);
                SelectionChanged?.Invoke();
            }
        }

        private void SynchronizeSceneActors(SE.Scene scene, HashSet<Guid>? seenActors = null)
        {
            if (scene == null)
                return;

            for (int index = 0; index < scene.ChildrenCount; index++)
                SynchronizeActorTree(scene.GetChild(index), seenActors);
        }

        private void SynchronizeActorTree(SE.Actor actor, HashSet<Guid>? seenActors)
        {
            if (actor == null)
                return;

            Guid id = actor.SceneObjectId;
            if (id == Guid.Empty || (seenActors != null && !seenActors.Add(id)))
                return;

            UpsertActor(actor);
            for (int index = 0; index < actor.ChildrenCount; index++)
                SynchronizeActorTree(actor.GetChild(index), seenActors);
        }

        private void RemoveActorsForScene(SE.Scene scene)
        {
            if (scene == null)
                return;

            Guid sceneId = scene.SceneObjectId;
            List<Guid> removedActors = new();
            foreach (KeyValuePair<Guid, SceneGraphNode> pair in SceneGraph.Nodes)
            {
                if (pair.Value is not ActorGraphNode actorNode)
                    continue;

                SE.Scene actorScene = actorNode.Actor.Scene;
                if (actorScene != null && actorScene.SceneObjectId == sceneId)
                    removedActors.Add(pair.Key);
            }

            foreach (Guid id in removedActors)
            {
                SceneGraph.Remove(id);
            }
            if (removedActors.Count != 0)
            {
                m_NodeSelection.RemoveAll(node => node.IsDisposed);
                SelectionChanged?.Invoke();
            }
        }

        private void RefreshScenes()
        {
            int count = SE.Level.GetScenesCount();
            bool changed = count != m_Scenes.Count;
            List<SceneEntry> refreshed = new List<SceneEntry>(count);
            for (int index = 0; index < count; index++)
            {
                SceneEntry entry = new SceneEntry(index, SE.Level.GetSceneName(index));
                refreshed.Add(entry);
                changed |= index >= m_Scenes.Count || !m_Scenes[index].Equals(entry);
            }

            SynchronizeSceneGraph();

            if (!changed)
                return;

            m_Scenes.Clear();
            m_Scenes.AddRange(refreshed);
            m_Selection.RemoveAll(index => index < 0 || index >= m_Scenes.Count);
            ScenesChanged?.Invoke();
            SelectionChanged?.Invoke();
        }
    }

    public readonly struct SceneEntry : IEquatable<SceneEntry>
    {
        public SceneEntry(int index, string name)
        {
            Index = index;
            Name = name ?? string.Empty;
        }

        public int Index { get; }
        public string Name { get; }

        public bool Equals(SceneEntry other)
        {
            return Index == other.Index && string.Equals(Name, other.Name, StringComparison.Ordinal);
        }

        public override bool Equals(object? obj)
        {
            return obj is SceneEntry other && Equals(other);
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(Index, Name);
        }
    }
}
