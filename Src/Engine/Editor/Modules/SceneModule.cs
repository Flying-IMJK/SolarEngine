using SE.Editor.GUI;
using SE.Editor.SceneGraph;
using SE.GUI;

using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;

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

            public override IReadOnlyList<ScenesGraphNode> Selection => m_Module.Selection;

            public override void Spawn(SE.Actor actor, SE.Actor parent)
            {
                // The native root hook is a Flax placeholder whose forwarding
                // call is commented out. Keep this override intentionally inert.
                _ = actor;
                _ = parent;
            }
        }

        private Dictionary<ContainerControl, Float2> m_UiRootSizes;

        public SceneModule(Editor editor)
            : base(editor)
        {
        }

        public event Action SelectionChanged;

        public SceneGraphFactory SceneGraph { get; private set; } = null!;
        public ScenesRootNode Root { get; private set; } = null!;
        public List<ScenesGraphNode> Selection { get; } = new(64);
        public int SelectionCount => Selection.Count;
        public bool HasSthSelected => Selection.Count > 0;

        internal override int Order => -91;

        /// <summary>
        /// Marks a loaded scene as modified. Scenes without a graph node are ignored.
        /// </summary>
        public void MarkSceneEdited(SE.Scene scene)
        {
            MarkSceneEdited(GetActorNode(scene) as SceneGraphNode);
        }

        /// <summary>
        /// Marks a scene node as modified. Null nodes are ignored.
        /// </summary>
        public void MarkSceneEdited(SceneGraphNode scene)
        {
            if (scene != null)
            {
                scene.IsEdited = true;
            }
        }

        /// <summary>
        /// Marks the specified loaded scenes as modified.
        /// </summary>
        public void MarkSceneEdited(IEnumerable<SE.Scene> scenes)
        {
            foreach (SE.Scene scene in scenes)
            {
                MarkSceneEdited(scene);
            }
        }

        /// <summary>
        /// Marks all loaded scenes as modified.
        /// </summary>
        public void MarkAllScenesEdited()
        {
            foreach (ScenesGraphNode scene in Root.ChildNodes)
            {
                if (scene is SceneGraphNode node)
                {
                    MarkSceneEdited(node);
                }
            }
        }

        /// <summary>
        /// Returns whether a loaded scene is modified, or false if its graph node is missing.
        /// </summary>
        public bool IsEdited(SE.Scene scene)
        {
            SceneGraphNode node = GetActorNode(scene) as SceneGraphNode;
            return node?.IsEdited ?? false;
        }

        /// <summary>
        /// Returns whether any loaded scene is modified. Returns false when no scenes are loaded.
        /// </summary>
        public bool IsEdited()
        {
            foreach (ScenesGraphNode scene in Root.ChildNodes)
            {
                if (scene is SceneGraphNode node && node.IsEdited)
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Returns whether every loaded scene is modified. Returns true when no scenes are loaded.
        /// </summary>
        public bool IsEverySceneEdited()
        {
            foreach (ScenesGraphNode scene in Root.ChildNodes)
            {
                if (scene is SceneGraphNode node && !node.IsEdited)
                {
                    return false;
                }
            }
            return true;
        }

        public void Select(SE.Actor actor)
        {
            ActorGraphNode node = GetActorNode(actor);
            if (node != null)
            {
                Select(node);
            }
        }

        public void Select(ScenesGraphNode node, bool additive = false)
        {
            if (node == null)
            {
                return;
            }

            if (!additive && Selection.Count == 1 && Selection[0] == node)
            {
                return;
            }
            if (additive && Selection.Contains(node))
            {
                return;
            }

            if (!additive)
            {
                Selection.Clear();
            }
            Selection.Add(node);
            SelectionChanged?.Invoke();
        }

        public void Select(List<ScenesGraphNode> selection, bool additive = false)
        {
            if (selection.Count == 0)
            {
                Deselect();
                return;
            }

            selection.RemoveAll(node => node == null);
            if (!additive && SequenceEqual(Selection, selection))
            {
                return;
            }

            if (!additive)
            {
                Selection.Clear();
            }
            Selection.AddRange(selection);
            SelectionChanged?.Invoke();
        }

        public void Deselect()
        {
            if (Selection.Count == 0)
            {
                return;
            }
            Selection.Clear();
            SelectionChanged?.Invoke();
        }

        public void SaveScene(SE.Scene scene)
        {
            if (GetActorNode(scene) is SceneGraphNode sceneNode)
            {
                SaveScene(sceneNode);
            }
        }

        public void SaveScene(SceneGraphNode sceneNode)
        {
            if (sceneNode == null || sceneNode.IsDisposed || !sceneNode.IsEdited)
            {
                return;
            }

            SE.Level.SaveSceneAsync(sceneNode.Scene);
        }

        /// <summary>
        /// Checks pending edits, then queues unloading the scene. Cancel or save failure leaves it open.
        /// </summary>
        public void CloseScene(SE.Scene scene)
        {
            if (GetActorNode(scene) is not SceneGraphNode sceneNode || sceneNode.IsDisposed)
            {
                return;
            }
            List<SceneGraphNode> scenes = new() { sceneNode };
            if (!CheckSaveBeforeClose(scenes))
            {
                SE.Level.UnloadSceneAsync(sceneNode.Scene);
            }
        }

        /// <summary>
        /// Checks all target scenes before queueing any unload, keeping the specified scene loaded.
        /// </summary>
        public void CloseAllScenesExcept(SE.Scene scene)
        {
            if (GetActorNode(scene) is not SceneGraphNode keptScene || keptScene.IsDisposed)
            {
                return;
            }
            List<SceneGraphNode> scenes = new();
            foreach (ScenesGraphNode child in Root.ChildNodes)
            {
                if (child is SceneGraphNode sceneNode && sceneNode != keptScene && !sceneNode.IsDisposed)
                {
                    scenes.Add(sceneNode);
                }
            }
            // Check every target before queueing any unload, so Cancel keeps all scenes open.
            if (!CheckSaveBeforeClose(scenes))
            {
                foreach (SceneGraphNode sceneNode in scenes)
                {
                    SE.Level.UnloadSceneAsync(sceneNode.Scene);
                }
            }
        }

        private bool CheckSaveBeforeClose(List<SceneGraphNode> scenes)
        {
            // True means unloading was canceled or a requested save failed.
            List<SceneGraphNode> editedScenes = scenes.FindAll(scene => scene.IsEdited);
            if (editedScenes.Count == 0)
            {
                return false;
            }
            string message = editedScenes.Count == 1
                ? $"Scene '{editedScenes[0].Name}' has been edited. Save before closing?"
                : $"{editedScenes.Count} scenes have been edited. Save before closing?";
            DialogResult result = MessageBox.Show(message, "Close scenes", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
            if (result == DialogResult.No)
            {
                return false;
            }
            if (result != DialogResult.Yes)
            {
                return true;
            }
            foreach (SceneGraphNode sceneNode in editedScenes)
            {
                if (sceneNode.IsDisposed || !SE.Level.SaveScene(sceneNode.Scene))
                {
                    SE.Debug.LogError("Failed to save scene. Unloading was canceled.");
                    return true;
                }
            }
            return false;
        }

        public void CreateSceneFile(string path)
        {
            // Create a sample scene
            Scene scene = new Scene
            {
                /*StaticFlags = StaticFlags.FullyStatic*/
            };

            //
            var sun = new DirectionalLight();
            sun.Name = "Sun";
            sun.LocalPosition = new Float3(40, 160, 0);
            sun.LocalQuaternion = Quaternion.Euler(new Float3(45, 0, 0));
            sun.StaticFlags = StaticMask.FullyStatic;

            Level.SpawnActor(sun, scene);

            //
            var sky = new Sky();
            sky.Name = "Sky";
            sky.LocalPosition = new Float3(40, 150, 0);
            sky.SunLight = sun;
            sky.StaticFlags = StaticMask.FullyStatic;

            Level.SpawnActor(sky, scene);
            //
            //var skyLight = new SkyLight();
            //skyLight.Mode = SkyLight.Modes.CustomTexture;
            //skyLight.Brightness = 2.5f;
            //skyLight.CustomTexture = FlaxEngine.Content.LoadAsyncInternal<CubeTexture>(EditorAssets.DefaultSkyCubeTexture);
            //skyLight.StaticFlags = StaticMask.FullyStatic;
            //
            var floor = new StaticModel();
            floor.Name = "Sphere";
            floor.Scale = new Float3(4, 0.5f, 4);
            floor.Model = AssetContent.LoadAsync<Model>(FileSystem.CombinePaths(EngineContext.ProjectCacheFolder, "Assets/Models/Sphere.seg"));
            if (floor.Model)
            {
                floor.Model.WaitForLoaded();
                //floor.SetMaterial(0, AssetContent.LoadAsync<MaterialBase>(FileSystem.CombinePaths(EngineContext.ProjectCacheFolder, "Assets/Models/WhiteMaterial.flax")));
            }
            floor.StaticFlags = StaticMask.FullyStatic;

            Level.SpawnActor(floor, scene);

            //
            var cam = new Camera();
            cam.Name = "Camera";
            cam.Position = new Float3(0, 150, -300);

            Level.SpawnActor(cam, scene);

            // Serialize
            Level.SaveSceneToBytes(scene, out byte[] bytes);

            // Cleanup
            Object.Destroy(ref scene);

            if (bytes == null || bytes.Length == 0)
            {
                throw new Exception("Failed to serialize scene.");
            }

            // Write to file
            using (FileStream fileStream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                fileStream.Write(bytes, 0, bytes.Length);
            }
        }

        public void OpenScene(Guid sceneId, bool additive = false)
        {
            SE.Level.LoadSceneAsync(sceneId);
        }

        public void Duplicate()
        {
            List<ActorGraphNode> roots = GetSelectedActorRoots(node => node.CanDuplicate);
            if (roots.Count != 0)
            {
                PasteActors(SerializeActors(roots, node => node.CanDuplicate), null);
            }
        }

        public void Delete()
        {
            List<ActorGraphNode> roots = GetSelectedActorRoots(node => node.CanDelete);
            foreach (ActorGraphNode node in roots)
            {
                SE.Level.DeleteActor(node.Actor);
            }
        }

        private List<ActorGraphNode> GetSelectedActorRoots(Func<ScenesGraphNode, bool> canEdit)
        {
            List<ActorGraphNode> roots = new();
            foreach (ScenesGraphNode selected in Selection)
            {
                if (selected == null || !canEdit(selected) || selected is not ActorGraphNode node)
                {
                    continue;
                }

                bool hasSelectedParent = false;
                for (ScenesGraphNode parent = node.ParentNode; parent != null; parent = parent.ParentNode)
                {
                    if (canEdit(parent) && Selection.Contains(parent))
                    {
                        hasSelectedParent = true;
                        break;
                    }
                }
                if (!hasSelectedParent && !roots.Contains(node))
                {
                    roots.Add(node);
                }
            }
            return roots;
        }

        public void Copy()
        {
            TryCopy(GetSelectedActorRoots(node => node.CanCopyPaste));
        }

        private static byte[] SerializeActors(List<ActorGraphNode> roots, Func<ScenesGraphNode, bool> canCopy)
        {
            List<ScenesGraphNode> nodes = new(roots);
            for (int index = 0; index < nodes.Count; index++)
            {
                foreach (ScenesGraphNode child in nodes[index].ChildNodes)
                {
                    if (!child.IsDisposed && !nodes.Contains(child))
                    {
                        nodes.Add(child);
                    }
                }
            }
            List<SE.Actor> actors = new(nodes.Count);
            foreach (ScenesGraphNode node in nodes)
            {
                if (canCopy(node) && node is ActorGraphNode actorNode)
                {
                    actors.Add(actorNode.Actor);
                }
            }
            return SE.Actor.ToBytes(actors.ToArray());
        }

        private static bool TryCopy(List<ActorGraphNode> roots)
        {
            if (roots.Count == 0)
            {
                return false;
            }
            try
            {
                byte[] data = SerializeActors(roots, node => node.CanCopyPaste);
                if (data == null || data.Length == 0)
                {
                    SE.Debug.LogError("Failed to copy actors data.");
                    return false;
                }
                Clipboard.RawData = data;
                return true;
            }
            catch (Exception exception)
            {
                SE.Debug.LogError($"Failed to copy actors: {exception.Message}");
            }
            return false;
        }

        public void Paste()
        {
            PasteActor(null);
        }

        public void PasteActor(SE.Actor pasteTargetActor)
        {
            if (!SE.Level.IsAnySceneLoaded)
            {
                return;
            }
            if (pasteTargetActor == null && Selection.Count == 1 && Selection[0] is ActorGraphNode node && !node.IsDisposed)
            {
                pasteTargetActor = node.Actor;
            }
            PasteActors(Clipboard.RawData, pasteTargetActor);
        }

        private void PasteActors(byte[] data, SE.Actor parent)
        {
            if (data == null || data.Length == 0 || !SE.Level.IsAnySceneLoaded)
            {
                return;
            }
            Guid[] ids = SE.Actor.TryGetSerializedObjectsIds(data);
            if (ids == null || ids.Length == 0)
            {
                SE.Debug.LogError("Clipboard does not contain valid actors data.");
                return;
            }
            Guid[] newIds = new Guid[ids.Length];
            for (int index = 0; index < newIds.Length; index++)
            {
                newIds[index] = Guid.NewGuid();
            }
            SE.Actor[] actors = SE.Actor.FromBytes(data, newIds, parent);
            if (actors == null || actors.Length == 0)
            {
                SE.Debug.LogError("Failed to create actors from clipboard data.");
                return;
            }

            HashSet<Guid> createdIds = new(newIds);
            List<ScenesGraphNode> selection = new();
            foreach (SE.Actor actor in actors)
            {
                if (actor.Parent != null && createdIds.Contains(actor.Parent.SceneObjectId))
                {
                    continue;
                }
                SE.Actor actorParent = actor.Parent;
                string name = actor.Name;
                string prefix = name + " ";
                string ending = name.EndsWith(')') ? ")" : string.Empty;
                int numberEnd = name.Length - ending.Length;
                int numberStart = numberEnd;
                while (numberStart > 0 && char.IsAsciiDigit(name[numberStart - 1]))
                {
                    numberStart--;
                }
                int suffix = 0;
                if (numberStart < numberEnd && int.TryParse(name.AsSpan(numberStart, numberEnd - numberStart), out int number) && number < int.MaxValue)
                {
                    prefix = name[..numberStart];
                    suffix = number + 1;
                }
                else
                {
                    ending = string.Empty;
                }
                bool hasNameCollision;
                do
                {
                    hasNameCollision = false;
                    for (int index = 0; index < actorParent.ChildrenCount; index++)
                    {
                        SE.Actor sibling = actorParent.GetChild(index);
                        if (sibling != actor && sibling.Name == name)
                        {
                            hasNameCollision = true;
                            name = $"{prefix}{suffix++}{ending}";
                            break;
                        }
                    }
                }
                while (hasNameCollision);
                actor.Name = name;

                ActorGraphNode node = GetActorNode(actor);
                MarkSceneEdited(node.ParentScene);
                selection.Add(node);
            }
            Select(selection);
        }

        public void Cut()
        {
            List<ActorGraphNode> roots = GetSelectedActorRoots(node => node.CanCopyPaste && node.CanDelete);
            if (TryCopy(roots))
            {
                foreach (ActorGraphNode node in roots)
                {
                    SE.Level.DeleteActor(node.Actor);
                }
            }
        }

        public void Spawn(SE.Actor actor, SE.Actor parent = null, bool autoSelect = true)
        {
            if (!SE.Level.IsAnySceneLoaded)
            {
                SE.Debug.LogError("Cannot spawn actor when no scene is loaded.");
                return;
            }

            SE.Level.SpawnActor(actor, parent);

            MarkSceneEdited(actor.Scene);
        }

        /// <summary>
        /// Moves actor subtrees into a loaded parent. The order is an insertion boundary in the original child list;
        /// a negative order appends. Returns false for invalid input or an unchanged hierarchy.
        /// </summary>
        public bool ChangeParent(IReadOnlyList<ActorGraphNode> nodes, SE.Actor parent, int orderInParent = -1, bool worldPositionsStays = true)
        {
            ActorGraphNode parentNode = GetActorNode(parent);
            if (nodes == null || nodes.Count == 0 || parentNode == null || parentNode.IsDisposed ||
                parentNode.ParentScene == null || parent.HasPrefabLink)
            {
                return false;
            }

            // Validate the whole operation before any native hierarchy is changed.
            HashSet<ActorGraphNode> requested = new();
            foreach (ActorGraphNode node in nodes)
            {
                if (node == null || !node.CanDrag || node.Root != Root || GetActorNode(node.ID) != node)
                {
                    return false;
                }
                for (SE.Actor ancestor = parent; ancestor != null; ancestor = ancestor.Parent)
                {
                    if (ancestor == node.Actor)
                    {
                        return false;
                    }
                }
                requested.Add(node);
            }

            // Visit in native hierarchy order, skipping descendants of selected roots.
            List<ActorGraphNode> roots = new();
            Stack<ActorGraphNode> pending = new();
            for (int i = Root.ChildNodes.Count - 1; i >= 0; i--)
            {
                if (Root.ChildNodes[i] is SceneGraphNode scene)
                {
                    pending.Push(scene);
                }
            }
            while (pending.Count > 0)
            {
                ActorGraphNode node = pending.Pop();
                if (requested.Contains(node))
                {
                    roots.Add(node);
                    continue;
                }
                for (int i = node.Actor.ChildrenCount - 1; i >= 0; i--)
                {
                    ActorGraphNode child = GetActorNode(node.Actor.GetChild(i));
                    if (child != null)
                    {
                        pending.Push(child);
                    }
                }
            }
            if (roots.Count == 0)
            {
                return false;
            }

            List<SE.Actor> actors = roots.ConvertAll(node => node.Actor);
            HashSet<SE.Actor> moving = new(actors);
            List<SE.Actor> previousChildren = new(parent.ChildrenCount);
            List<SE.Actor> children = new(parent.ChildrenCount + actors.Count);
            int insertion = orderInParent < 0 ? parent.ChildrenCount : Math.Min(orderInParent, parent.ChildrenCount);
            int removedBeforeInsertion = 0;
            for (int i = 0; i < parent.ChildrenCount; i++)
            {
                SE.Actor child = parent.GetChild(i);
                previousChildren.Add(child);
                if (moving.Contains(child))
                {
                    if (i < insertion)
                    {
                        removedBeforeInsertion++;
                    }
                }
                else
                {
                    children.Add(child);
                }
            }
            children.InsertRange(insertion - removedBeforeInsertion, actors);

            bool changed = children.Count != previousChildren.Count;
            for (int i = 0; !changed && i < children.Count; i++)
            {
                changed = children[i] != previousChildren[i];
            }
            if (!changed)
            {
                return false;
            }

            HashSet<SceneGraphNode> editedScenes = new() { parentNode.ParentScene };
            HashSet<SE.Actor> previousParents = new();
            foreach (ActorGraphNode node in roots)
            {
                editedScenes.Add(node.ParentScene);
                SE.Actor actor = node.Actor;
                if (actor.Parent != parent)
                {
                    previousParents.Add(actor.Parent);
                    actor.SetParent(parent, worldPositionsStays, false);
                }
            }
            for (int i = 0; i < children.Count; i++)
            {
                children[i].OrderInParent = i;
            }

            // Removing a child also changes the cached indices of its old siblings.
            foreach (SE.Actor previousParent in previousParents)
            {
                if (previousParent != null && previousParent.ChildrenCount > 0)
                {
                    GetActorNode(previousParent.GetChild(0))?.TreeNode.OnOrderInParentChanged();
                }
            }
            foreach (SceneGraphNode scene in editedScenes)
            {
                MarkSceneEdited(scene);
            }
            parentNode.TreeNode.Expand();
            Select(roots.ConvertAll<ScenesGraphNode>(node => node));
            roots[0].TreeNode.Focus();
            return true;
        }

        public ActorGraphNode GetActorNode(SE.Actor actor)
        {
            return actor == null ? null : GetActorNode(actor.SceneObjectId);
        }

        public ActorGraphNode GetActorNode(Guid actorId)
        {
            return SceneGraph.FindNode(actorId) as ActorGraphNode;
        }

        public override void OnInit()
        {
            SceneGraph = new SceneGraphFactory();
            Root = new ScenesRootNode(this);
            SceneGraph.SetRoot(Root);
            Root.LinkTreeNode();

            SE.Level.SceneSaving += OnSceneSaving;
            SE.Level.SceneSaved += OnSceneSaved;
            SE.Level.SceneSaveError += OnSceneSaveError;
            SE.Level.SceneLoaded += OnSceneLoaded;
            SE.Level.SceneUnloading += OnSceneUnloading;
            SE.Level.ActorSpawned += OnActorSpawned;
            SE.Level.ActorDeleted += OnActorDeleted;
            SE.Level.ActorParentChanged += OnActorParentChanged;
            SE.Level.ActorOrderInParentChanged += OnActorOrderInParentChanged;
            SE.Level.ActorNameChanged += OnActorNameChanged;
            SE.Level.ActorActiveChanged += OnActorActiveChanged;



        }

        public override void OnDispose()
        {
            SE.Level.SceneSaving -= OnSceneSaving;
            SE.Level.SceneSaved -= OnSceneSaved;
            SE.Level.SceneSaveError -= OnSceneSaveError;
            SE.Level.SceneLoaded -= OnSceneLoaded;
            SE.Level.SceneUnloading -= OnSceneUnloading;
            SE.Level.ActorSpawned -= OnActorSpawned;
            SE.Level.ActorDeleted -= OnActorDeleted;
            SE.Level.ActorParentChanged -= OnActorParentChanged;
            SE.Level.ActorOrderInParentChanged -= OnActorOrderInParentChanged;
            SE.Level.ActorNameChanged -= OnActorNameChanged;
            SE.Level.ActorActiveChanged -= OnActorActiveChanged;

            SceneGraph.Dispose();
            Selection.Clear();
            m_UiRootSizes?.Clear();
            SelectionChanged = null;
        }

        private void OnSceneSaving(SE.Scene scene, Guid sceneId)
        {
            ContainerControl uiRoot = RootControl.GetGameRoot();
            if (uiRoot != null)
            {
                OnSaveStart(uiRoot);
            }
        }

        private void OnSceneSaved(SE.Scene scene, Guid sceneId)
        {
            if (GetActorNode(sceneId) is SceneGraphNode sceneNode)
            {
                sceneNode.IsEdited = false;
            }
            ContainerControl uiRoot = RootControl.GetGameRoot();
            if (uiRoot != null)
            {
                OnSaveEnd(uiRoot);
            }
        }

        private void OnSceneSaveError(SE.Scene scene, Guid sceneId)
        {
            ContainerControl uiRoot = RootControl.GetGameRoot();
            if (uiRoot != null)
            {
                OnSaveEnd(uiRoot);
            }
        }

        private void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            SceneGraphNode sceneNode = SceneGraph.BuildSceneTree(scene);
            ActorTreeNode treeNode = sceneNode.TreeNode;
            treeNode.IsLayoutLocked = true;
            treeNode.Expand(true);

            ActorTreeNode rootNode = Root.TreeNode;
            rootNode.IsLayoutLocked = true;
            sceneNode.ParentNode = Root;
            rootNode.SortChildren();
            rootNode.IsLayoutLocked = false;
            rootNode.Parent?.PerformLayout();
        }

        private void OnSceneUnloading(SE.Scene scene, Guid sceneId)
        {
            ActorGraphNode node = Root.FindChildActor(scene);
            if (node == null)
            {
                return;
            }

            bool hasSceneSelection = false;
            foreach (ScenesGraphNode selected in Selection)
            {
                if (selected.ParentScene == node)
                {
                    hasSceneSelection = true;
                    break;
                }
            }
            if (hasSceneSelection)
            {
                List<ScenesGraphNode> newSelection = new();
                foreach (ScenesGraphNode selected in Selection)
                {
                    if (selected.ParentScene != node)
                    {
                        newSelection.Add(selected);
                    }
                }
                Select(newSelection);
            }
            PropertiesWindow properties = Editor.Windows?.Properties;
            if (properties != null && properties.LockSelection)
            {
                foreach (object value in properties.Presenter.Selection)
                {
                    if (value is SE.Actor actor && actor.Scene == scene)
                    {
                        properties.LockSelection = false;
                        break;
                    }
                }
            }
            node.Dispose();
        }

        private void OnActorSpawned(SE.Actor actor)
        {
            if (actor == null || GetActorNode(actor.Scene) == null)
            {
                return;
            }

            SE.Actor parentActor = actor.Parent;
            if (parentActor == null)
            {
                return;
            }

            ActorGraphNode parentNode = GetActorNode(parentActor);
            if (parentNode == null)
            {
                return;
            }

            ActorGraphNode node = SceneGraph.BuildActorNode(actor);
            if (node != null)
            {
                node.ParentNode = parentNode;
            }
        }

        private void OnActorDeleted(SE.Actor actor)
        {
            ActorGraphNode node = GetActorNode(actor);
            if (node == null)
            {
                return;
            }

            MarkSceneEdited(node.ParentScene);
            int removed = Selection.RemoveAll(selected =>
            {
                for (ScenesGraphNode parent = selected; parent != null; parent = parent.ParentNode)
                {
                    if (parent == node)
                    {
                        return true;
                    }
                }
                return false;
            });
            OnActorDeleted(node);
            if (removed != 0)
            {
                SelectionChanged?.Invoke();
            }
        }

        private void OnActorDeleted(ActorGraphNode node)
        {
            for (int index = 0; index < node.ChildNodes.Count; index++)
            {
                if (node.ChildNodes[index] is ActorGraphNode child)
                {
                    index--;
                    OnActorDeleted(child);
                }
            }
            node.Dispose();
        }

        private void OnActorParentChanged(SE.Actor actor, SE.Actor previousParent)
        {
            ActorGraphNode node = null;
            ActorGraphNode parentNode = GetActorNode(actor.Parent);
            ActorGraphNode previousParentNode = GetActorNode(previousParent);

            if (previousParentNode != null)
            {
                node = previousParentNode.FindChildActor(actor) ?? GetActorNode(actor);
            }
            else if (parentNode != null)
            {
                node = SceneGraph.BuildActorNode(actor);
            }

            if (node == null)
            {
                return;
            }

            node.ParentNode = parentNode;
            if (parentNode == null)
            {
                if (Selection.Contains(node))
                {
                    Deselect();
                }
                node.Dispose();
            }
        }

        private void OnActorOrderInParentChanged(SE.Actor actor)
        {
            ActorGraphNode node = GetActorNode(actor);
            node?.TreeNode.OnOrderInParentChanged();
        }

        private void OnActorNameChanged(SE.Actor actor)
        {
            ActorGraphNode node = GetActorNode(actor);
            node?.TreeNode.UpdateText();
        }

        private void OnActorActiveChanged(SE.Actor actor)
        {
            if (GetActorNode(actor) != null)
            {
                SE.Debug.Assert(false, "Unreachable code encountered!");
            }
        }

        public void OnSaveStart(ContainerControl uiRoot)
        {
            m_UiRootSizes ??= new Dictionary<ContainerControl, Float2>();
            m_UiRootSizes[uiRoot] = uiRoot.Size;
            uiRoot.Size = new Float2(1920, 1080);
        }

        public void OnSaveEnd(ContainerControl uiRoot)
        {
            if (m_UiRootSizes != null && m_UiRootSizes.TryGetValue(uiRoot, out Float2 size))
            {
                uiRoot.Size = size;
                m_UiRootSizes.Remove(uiRoot);
            }
        }

        private static bool SequenceEqual(IReadOnlyList<ScenesGraphNode> left, IReadOnlyList<ScenesGraphNode> right)
        {
            if (left.Count != right.Count)
            {
                return false;
            }

            for (int index = 0; index < left.Count; index++)
            {
                if (left[index] != right[index])
                {
                    return false;
                }
            }

            return true;
        }
    }
}
