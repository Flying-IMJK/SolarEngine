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

            public override void Spawn(SE.Actor actor, SE.Actor? parent)
            {
                // The native root hook is a Flax placeholder whose forwarding
                // call is commented out. Keep this override intentionally inert.
                _ = actor;
                _ = parent;
            }
        }

        private Dictionary<ContainerControl, Float2>? m_UiRootSizes;

        public SceneModule(Editor editor)
            : base(editor)
        {
        }

        public event Action? SelectionChanged;

        public SceneGraphFactory SceneGraph { get; private set; } = null!;
        public ScenesRootNode Root { get; private set; } = null!;
        public List<ScenesGraphNode> Selection { get; } = new(64);
        public int SelectionCount => Selection.Count;
        public bool HasSthSelected => Selection.Count > 0;

        internal override int Order => -91;

        public void Select(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            if (node != null)
                Select(node);
        }

        public void Select(ScenesGraphNode node, bool additive = false)
        {
            if (node == null)
                return;

            if (!additive && Selection.Count == 1 && Selection[0] == node)
                return;
            if (additive && Selection.Contains(node))
                return;

            if (!additive)
                Selection.Clear();
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
                return;

            if (!additive)
                Selection.Clear();
            Selection.AddRange(selection);
            SelectionChanged?.Invoke();
        }

        public void Deselect()
        {
            if (Selection.Count == 0)
                return;
            Selection.Clear();
            SelectionChanged?.Invoke();
        }

        public void SaveScene(SE.Scene scene)
        {
            if (GetActorNode(scene) is SceneGraphNode sceneNode)
                SaveScene(sceneNode);
        }

        public void SaveScene(SceneGraphNode sceneNode)
        {
            if (!sceneNode.IsEdited)
                return;

            sceneNode.IsEdited = false;
            SE.Level.SaveSceneAsync(sceneNode.Scene);
        }

        public void CreateSceneFile(string path)
        {
            // Create a sample scene
            var scene = new Scene
            {
                /*StaticFlags = StaticFlags.FullyStatic*/
            };

            //
            /*            var sun = scene.AddChild<DirectionalLight>();
                        sun.Name = "Sun";
                        sun.LocalPosition = new Vector3(40, 160, 0);
                        sun.LocalEulerAngles = new Vector3(45, 0, 0);
                        sun.StaticFlags = StaticFlags.FullyStatic;
                        //
                        var sky = scene.AddChild<Sky>();
                        sky.Name = "Sky";
                        sky.LocalPosition = new Vector3(40, 150, 0);
                        sky.SunLight = sun;
                        sky.StaticFlags = StaticFlags.FullyStatic;
                        //
                        var skyLight = scene.AddChild<SkyLight>();
                        skyLight.Mode = SkyLight.Modes.CustomTexture;
                        skyLight.Brightness = 2.5f;
                        skyLight.CustomTexture = FlaxEngine.Content.LoadAsyncInternal<CubeTexture>(EditorAssets.DefaultSkyCubeTexture);
                        skyLight.StaticFlags = StaticFlags.FullyStatic;
                        //
                        var floor = scene.AddChild<StaticModel>();
                        floor.Name = "Floor";
                        floor.Scale = new Float3(4, 0.5f, 4);
                        floor.Model = FlaxEngine.Content.LoadAsync<Model>(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Primitives/Cube.flax"));
                        if (floor.Model)
                        {
                            floor.Model.WaitForLoaded();
                            floor.SetMaterial(0, FlaxEngine.Content.LoadAsync<MaterialBase>(StringUtils.CombinePaths(Globals.EngineContentFolder, "Engine/WhiteMaterial.flax")));
                        }
                        floor.StaticFlags = StaticFlags.FullyStatic;
                        //
                        var cam = scene.AddChild<Camera>();
                        cam.Name = "Camera";
                        cam.Position = new Vector3(0, 150, -300);
                        //
                        var audioListener = cam.AddChild<AudioListener>();
                        audioListener.Name = "Audio Listener";*/

            // Serialize
            Level.SaveSceneToBytes(scene, out byte[] bytes);

            // Cleanup
            Object.Destroy(ref scene);

            if (bytes == null || bytes.Length == 0)
            {
                throw new Exception("Failed to serialize scene.");
            }

            // Write to file
            using (var fileStream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                fileStream.Write(bytes, 0, bytes.Length);
            }
        }

        public void OpenScene(Guid sceneId, bool additive = false)
        {
            _ = additive;
            SE.Level.LoadSceneAsync(sceneId);
        }

        public void Duplicate()
        {
        }

        public void Delete()
        {
        }

        public void Copy()
        {
        }

        public void Paste()
        {
            PasteActor(null);
        }

        public void PasteActor(SE.Actor? pasteTargetActor)
        {
            _ = pasteTargetActor;
        }

        public void Cut()
        {
            Copy();
            Delete();
        }

        public void Spawn(SE.Actor actor, SE.Actor? parent = null, bool autoSelect = true)
        {
            if (!SE.Level.IsAnySceneLoaded)
            {
                SE.Debug.LogError("Cannot spawn actor when no scene is loaded.");
                return;
            }

            if (parent == null)
                SE.Level.SpawnActor(actor);
            else
                SE.Level.SpawnActor(actor, parent);

            SE.Debug.Assert(GetActorNode(actor) != null, "Failed to create scene node for the spawned actor.");
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
            ContainerControl? uiRoot = RootControl.GetGameRoot();
            if (uiRoot != null)
                OnSaveStart(uiRoot);
        }

        private void OnSceneSaved(SE.Scene scene, Guid sceneId)
        {
            ContainerControl? uiRoot = RootControl.GetGameRoot();
            if (uiRoot != null)
                OnSaveEnd(uiRoot);
        }

        private void OnSceneSaveError(SE.Scene scene, Guid sceneId)
        {
            ContainerControl? uiRoot = RootControl.GetGameRoot();
            if (uiRoot != null)
                OnSaveEnd(uiRoot);
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
            ActorGraphNode? node = Root.FindChildActor(scene);
            if (node == null)
                return;

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
                        newSelection.Add(selected);
                }
                Select(newSelection);
            }
            node.Dispose();
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

            ActorGraphNode node = SceneGraph.BuildActorNode(actor);
            if (node != null)
                node.ParentNode = parentNode;
        }

        private void OnActorDeleted(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            if (node == null)
                return;

            OnActorDeleted(node);
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
            ActorGraphNode? node = null;
            ActorGraphNode? parentNode = GetActorNode(actor.Parent);
            ActorGraphNode? previousParentNode = GetActorNode(previousParent);

            if (previousParentNode != null)
                node = previousParentNode.FindChildActor(actor) ?? GetActorNode(actor);
            else if (parentNode != null)
                node = SceneGraph.BuildActorNode(actor);

            if (node == null)
                return;

            node.ParentNode = parentNode;
            if (parentNode == null)
            {
                if (Selection.Contains(node))
                    Deselect();
                node.Dispose();
            }
        }

        private void OnActorOrderInParentChanged(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            node?.TreeNode.OnOrderInParentChanged();
        }

        private void OnActorNameChanged(SE.Actor actor)
        {
            ActorGraphNode? node = GetActorNode(actor);
            node?.TreeNode.UpdateText();
        }

        private void OnActorActiveChanged(SE.Actor actor)
        {
            if (GetActorNode(actor) != null)
                SE.Debug.Assert(false, "Unreachable code encountered!");
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