using System;
using System.Collections.Generic;
using SE.Editor.SceneGraph;

namespace SE.Editor
{
    /// <summary>
    /// Owns loaded-scene editor state, scene-graph synchronization, and selection.
    /// </summary>
    public sealed class SceneModule : EditorModule
    {
        /// <summary>
        /// Root node for all loaded scenes and their actor hierarchies.
        /// </summary>
        public sealed class ScenesRootNode : RootGraphNode
        {
            private readonly SceneModule m_Module;

            internal ScenesRootNode(SceneModule module)
                : base(module.SceneGraph)
            {
                m_Module = module;
            }

            public override IReadOnlyList<ScenesGraphNode> Selection => m_Module.SelectedNodes;

            public override void Spawn(SE.Actor actor, SE.Actor? parent)
            {
                // The native root hook is a Flax placeholder whose forwarding
                // call is commented out. Keep this override intentionally inert.
                _ = actor;
                _ = parent;
            }
        }

        private readonly List<SceneEntry> m_Scenes = new();
        private readonly List<int> m_SceneSelection = new();
        private readonly List<ScenesGraphNode> m_NodeSelection = new();

        public SceneModule(Editor editor)
            : base(editor)
        {
            SceneGraph = new SceneGraphFactory();
            Root = new ScenesRootNode(this);
            SceneGraph.SetRoot(Root);
            Root.LinkTreeNode();
        }

        public event Action? ScenesChanged;
        public event Action? SelectionChanged;
        public event Action? HierarchyChanged;

        public SceneGraphFactory SceneGraph { get; }
        public ScenesRootNode Root { get; }
        public IReadOnlyList<SceneEntry> Scenes => m_Scenes;
        public IReadOnlyList<ScenesGraphNode> SelectedNodes => m_NodeSelection;
        public IReadOnlyList<int> Selection => m_SceneSelection;
        public int SelectionCount => m_NodeSelection.Count != 0 ? m_NodeSelection.Count : m_SceneSelection.Count;
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
            if (!additive && (m_SceneSelection.Count != 1 || m_SceneSelection[0] != sceneIndex))
            {
                m_SceneSelection.Clear();
                changed = true;
            }
            if (!m_SceneSelection.Contains(sceneIndex))
            {
                m_SceneSelection.Add(sceneIndex);
                changed = true;
            }

            if (changed)
                SelectionChanged?.Invoke();
        }

        public void Select(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            if (node != null)
                Select(node);
        }

        public void Select(ScenesGraphNode node, bool additive = false)
        {
            ArgumentNullException.ThrowIfNull(node);
            if (node.IsDisposed)
                return;
            Select(new[] { node }, additive);
        }

        public void Select(IEnumerable<ScenesGraphNode> nodes, bool additive = false)
        {
            ArgumentNullException.ThrowIfNull(nodes);
            List<ScenesGraphNode> requested = new();
            foreach (ScenesGraphNode node in nodes)
            {
                if (node != null && !node.IsDisposed && !requested.Contains(node))
                    requested.Add(node);
            }

            if (requested.Count == 0)
            {
                Deselect();
                return;
            }

            if (!additive && SequenceEqual(m_NodeSelection, requested) && m_SceneSelection.Count == 0)
                return;

            bool changed = false;
            if (!additive)
            {
                changed = m_SceneSelection.Count != 0 || !SequenceEqual(m_NodeSelection, requested);
                m_SceneSelection.Clear();
                m_NodeSelection.Clear();
            }
            foreach (ScenesGraphNode node in requested)
            {
                if (!m_NodeSelection.Contains(node))
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
            if (m_SceneSelection.Count == 0 && m_NodeSelection.Count == 0)
                return;

            m_SceneSelection.Clear();
            m_NodeSelection.Clear();
            SelectionChanged?.Invoke();
        }

        public bool SaveScene(int sceneIndex)
        {
            return sceneIndex >= 0 && sceneIndex < m_Scenes.Count && SE.Level.SaveSceneAt(sceneIndex);
        }

        public void SaveScene(SE.Scene scene)
        {
            if (GetActorNode(scene) is SceneGraphNode sceneNode)
                SaveScene(sceneNode);
        }

        public void SaveScene(SceneGraphNode sceneNode)
        {
            ArgumentNullException.ThrowIfNull(sceneNode);
            if (!sceneNode.IsEdited)
                return;

            sceneNode.IsEdited = false;
            SE.Level.SaveSceneAsync(sceneNode.Scene);
        }

        public void Spawn(SE.Actor actor, SE.Actor? parent = null, bool autoSelect = true)
        {
            ArgumentNullException.ThrowIfNull(actor);
            if (!SE.Level.IsAnySceneLoaded)
            {
                SE.Debug.LogError("Cannot spawn actor when no scene is loaded.");
                return;
            }

            bool failed = parent == null ? SE.Level.SpawnActor(actor) : SE.Level.SpawnActor(actor, parent);
            if (failed)
            {
                SE.Debug.LogError($"Failed to spawn actor '{actor.Name}'.");
                return;
            }

            // Native automatic selection and undo are still commented out.
            _ = autoSelect;
        }

        public ActorGraphNode? GetActorNode(SE.Actor? actor)
        {
            return actor == null ? null : GetActorNode(actor.SceneObjectId);
        }

        public ActorGraphNode? GetActorNode(Guid actorId)
        {
            return SceneGraph.FindNode(actorId) as ActorGraphNode;
        }

        public override void OnInit()
        {
            SE.Level.SceneLoaded += OnSceneLoaded;
            SE.Level.SceneUnloading += OnSceneUnloading;
            SE.Level.SceneUnloaded += OnSceneUnloaded;
            SE.Level.ActorSpawned += OnActorSpawned;
            SE.Level.ActorDeleted += OnActorDeleted;
            SE.Level.ActorParentChanged += OnActorParentChanged;
            SE.Level.ActorOrderInParentChanged += OnActorOrderInParentChanged;
            SE.Level.ActorNameChanged += OnActorNameChanged;
            SE.Level.ActorActiveChanged += OnActorActiveChanged;

            BuildLoadedScenes();
            RefreshSceneEntries();
        }

        public override void OnDispose()
        {
            SE.Level.SceneLoaded -= OnSceneLoaded;
            SE.Level.SceneUnloading -= OnSceneUnloading;
            SE.Level.SceneUnloaded -= OnSceneUnloaded;
            SE.Level.ActorSpawned -= OnActorSpawned;
            SE.Level.ActorDeleted -= OnActorDeleted;
            SE.Level.ActorParentChanged -= OnActorParentChanged;
            SE.Level.ActorOrderInParentChanged -= OnActorOrderInParentChanged;
            SE.Level.ActorNameChanged -= OnActorNameChanged;
            SE.Level.ActorActiveChanged -= OnActorActiveChanged;

            SceneGraph.Dispose();
            m_Scenes.Clear();
            m_SceneSelection.Clear();
            m_NodeSelection.Clear();
            ScenesChanged = null;
            SelectionChanged = null;
            HierarchyChanged = null;
        }

        private void BuildLoadedScenes()
        {
            for (int index = 0; index < SE.Level.ScenesCount; index++)
            {
                SE.Scene scene = SE.Level.GetScene(index);
                if (scene != null && SceneGraph.FindNode(scene.SceneObjectId) == null)
                    AddSceneTree(scene);
            }
        }

        private void AddSceneTree(SE.Scene scene)
        {
            SceneGraphNode sceneNode = SceneGraph.BuildSceneTree(scene);
            sceneNode.ParentNode = Root;
            sceneNode.TreeNode.Expand(true);
        }

        private void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            if (scene != null && SceneGraph.FindNode(sceneId) == null)
                AddSceneTree(scene);

            RefreshSceneEntries();
            HierarchyChanged?.Invoke();
        }

        private void OnSceneUnloading(SE.Scene scene, Guid sceneId)
        {
            ScenesGraphNode? node = SceneGraph.FindNode(sceneId);
            if (node == null && scene != null)
                node = SceneGraph.FindNode(scene.SceneObjectId);
            if (node == null)
                return;

            bool selectionChanged = RemoveSelectionForTree(node);
            node.Dispose();
            if (selectionChanged)
                SelectionChanged?.Invoke();
            HierarchyChanged?.Invoke();
        }

        private void OnSceneUnloaded(SE.Scene scene, Guid sceneId)
        {
            RefreshSceneEntries();
        }

        private void OnActorSpawned(SE.Actor actor)
        {
            if (actor == null || GetActorNode(actor.Scene) == null)
                return;

            SE.Actor parentActor = actor.Parent;
            if (parentActor == null)
                return;

            ActorGraphNode? parentNode = GetActorNode(parentActor);
            if (parentNode == null)
                return;

            // Parent-change and spawn notifications can be adjacent. Reuse a
            // node already created by the first notification.
            ActorGraphNode node = GetActorNode(actor) ?? SceneGraph.BuildActorNode(actor);
            node.ParentNode = parentNode;
            HierarchyChanged?.Invoke();
        }

        private void OnActorDeleted(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            if (node == null)
                return;

            bool selectionChanged = RemoveSelectionForTree(node);
            node.Dispose();
            if (selectionChanged)
                SelectionChanged?.Invoke();
            HierarchyChanged?.Invoke();
        }

        private void OnActorParentChanged(SE.Actor actor, SE.Actor previousParent)
        {
            ActorGraphNode? node = GetActorNode(actor);
            ActorGraphNode? parentNode = GetActorNode(actor.Parent);
            ActorGraphNode? previousParentNode = GetActorNode(previousParent);

            if (previousParentNode != null)
                node = previousParentNode.FindChildActor(actor) ?? GetActorNode(actor);
            else if (node == null && parentNode != null)
                node = SceneGraph.BuildActorNode(actor);

            if (node == null)
                return;

            node.ParentNode = parentNode;
            if (parentNode == null)
            {
                bool selectionChanged = RemoveSelectionForTree(node);
                node.Dispose();
                if (selectionChanged)
                    SelectionChanged?.Invoke();
            }

            HierarchyChanged?.Invoke();
        }

        private void OnActorOrderInParentChanged(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            node?.TreeNode.OnOrderInParentChanged();
            if (node != null)
                HierarchyChanged?.Invoke();
        }

        private void OnActorNameChanged(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            node?.RefreshActor(actor);
            if (node != null)
                HierarchyChanged?.Invoke();
        }

        private void OnActorActiveChanged(SE.Actor actor)
        {
            // The native active-state tree update is still a commented Flax
            // placeholder, so the managed migration intentionally does nothing.
            _ = actor;
        }

        private bool RemoveSelectionForTree(ScenesGraphNode root)
        {
            HashSet<ScenesGraphNode> removed = new();
            CollectTree(root, removed);
            int previousCount = m_NodeSelection.Count;
            m_NodeSelection.RemoveAll(removed.Contains);
            return previousCount != m_NodeSelection.Count;
        }

        private static void CollectTree(ScenesGraphNode node, HashSet<ScenesGraphNode> result)
        {
            if (!result.Add(node))
                return;
            foreach (ScenesGraphNode child in node.ChildNodes)
                CollectTree(child, result);
        }

        private void RefreshSceneEntries()
        {
            int count = SE.Level.ScenesCount;
            bool changed = count != m_Scenes.Count;
            List<SceneEntry> refreshed = new(count);
            for (int index = 0; index < count; index++)
            {
                SceneEntry entry = new(index, SE.Level.GetSceneName(index));
                refreshed.Add(entry);
                changed |= index >= m_Scenes.Count || !m_Scenes[index].Equals(entry);
            }

            if (!changed)
                return;

            m_Scenes.Clear();
            m_Scenes.AddRange(refreshed);
            m_SceneSelection.RemoveAll(index => index < 0 || index >= m_Scenes.Count);
            ScenesChanged?.Invoke();
            SelectionChanged?.Invoke();
        }

        private static bool SequenceEqual(IReadOnlyList<ScenesGraphNode> left, IReadOnlyList<ScenesGraphNode> right)
        {
            if (left.Count != right.Count)
                return false;
            for (int index = 0; index < left.Count; index++)
            {
                if (!ReferenceEquals(left[index], right[index]))
                    return false;
            }

            return true;
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
