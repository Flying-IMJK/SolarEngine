#pragma once

#include "EditorModule.h"
#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Types/Collections/HashSet.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Editor/Resource/Opreate/ContentOperate.h"
#include "Editor/Resource/Tree/MainContentTreeNode.h"

namespace SE
{
	struct FileWatcherEvent;
	struct AssetInfo;
    class ProjectInfo;
    class Asset;
}

namespace SE::Editor
{
	class AssetItem;;
    class AssetOperate;

	class AssetDatabaseModule : public EditorModule
	{
	private:
		bool _enableEvents;
		bool _isDuringFastSetup;
		bool _rebuildFlag;
		int _itemsCreated;
		int _itemsDeleted;
		HashSet<MainContentTreeNode*> _dirtyNodes = HashSet<MainContentTreeNode*>();
		CriticalSection* _CriticalSection;

	    void OnContentAssetDisposing(Asset* asset);

	public:
		ProjectTreeNode* ProjectNode;
		// ProjectTreeNode* GameNode;

		/// <summary>
		/// The list of all projects workspace directories (including game, engine and plugins projects).
		/// </summary>
		List<ProjectTreeNode*> Projects = List<ProjectTreeNode*>();

		/// <summary>
		/// The list with all content items proxy objects. Use <see cref="AddProxy"/> and <see cref="RemoveProxy"/> to modify this or <see cref="Rebuild"/> to refresh database when adding new item proxy types.
		/// </summary>
		List<ContentOperate*> Operate = List<ContentOperate*>(128);

		/// <summary>
		/// Occurs when new items is added to the workspace content database.
		/// </summary>
		Delegate<ContentItem*> ItemAdded;

		/// <summary>
		/// Occurs when new items is removed from the workspace content database.
		/// </summary>
		Delegate<ContentItem*> ItemRemoved;

		/// <summary>
		/// Occurs when workspace has been modified.
		/// </summary>
		Action WorkspaceModified;

		/// <summary>
		/// Occurs when workspace has will be rebuilt.
		/// </summary>
		Action WorkspaceRebuilding;

		/// <summary>
		/// Occurs when workspace has been rebuilt.
		/// </summary>
		Action WorkspaceRebuilt;

		/// <summary>
		/// Gets the amount of created items.
		/// </summary>
		int GetItemsCreated() { return _itemsCreated; }

		/// <summary>
		/// Gets the amount of deleted items.
		/// </summary>
		int GetItemsDeleted() { return _itemsDeleted; }

		explicit AssetDatabaseModule(EditorApp* editor);

		int InitOrder() override;

        /// <summary>
        /// Gets the project workspace used by the given project.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <returns>The project workspace or null if not loaded into database.</returns>
	    ProjectTreeNode* GetProjectWorkspace(ProjectInfo* project);

        /// <summary>
        /// Gets the proxy object for the given content item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>Content proxy for that item or null if cannot find.</returns>
	    ContentOperate* GetProxy(ContentItem* item);

        /// <summary>
        /// Gets the proxy object for the given asset type.
        /// </summary>
        /// <returns>Content proxy for that asset type or null if cannot find.</returns>
        template<typename T, typename = typename TIsBaseOf<Asset, T>::Value>
	    ContentOperate* GetProxy()
        {
            for (int i = 0; i < Operate.Count(); i++)
            {
                if (Operate[i]->IsProxyFor(Typeof<T>()))
                {
                    return Operate[i];
                }
            }

            return nullptr;
        }

        /// <summary>
        /// Gets the proxy object for the given file extension. Warning! Different asset types may share the same file extension.
        /// </summary>
        /// <param name="extension">The file extension.</param>
        /// <returns>Content proxy for that item or null if cannot find.</returns>
	    ContentOperate* GetProxy(StringView extension);

        /// <summary>
        /// Gets the proxy object for the given asset type id.
        /// </summary>
        /// <param name="typeID">The asset type id.</param>
        /// <param name="path">The asset path.</param>
        /// <returns>Asset proxy or null if cannot find.</returns>
	    AssetOperate* GetAssetOperate(TypeID typeID, StringView path);

        /// <summary>
        /// Gets the virtual proxy object from given path.
        /// <br></br>use case if the asset u trying to display is not a flax asset but u like to add custom functionality
        /// <br></br>to context menu,or display it the asset
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <returns>Asset proxy or null if cannot find.</returns>
	    AssetOperate* GetAssetVirtualOpreate(StringView path);

        /// <summary>
        /// Refreshes the given item folder. Tries to find new content items and remove not existing ones.
        /// </summary>
        /// <param name="item">Folder to refresh</param>
        /// <param name="checkSubDirs">True if search for changes inside a subdirectories, otherwise only top-most folder will be updated</param>
	    void RefreshFolder(ContentItem* item, bool checkSubDirs);

        /// <summary>
        /// Tries to find item at the specified path.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>Found item or null if cannot find it.</returns>
        ContentItem* Find(StringView path);

        /// <summary>
        /// Tries to find item with the specified ID.
        /// </summary>
        /// <param name="id">The item ID.</param>
        /// <returns>Found item or null if cannot find it.</returns>
	    ContentItem* Find(UID id);

        /// <summary>
        /// Tries to find asset with the specified ID.
        /// </summary>
        /// <param name="id">The asset ID.</param>
        /// <returns>Found asset item or null if cannot find it.</returns>
	    AssetItem* FindAsset(UID id);

        /// <summary>
        /// Moves the specified items to the different location. Handles moving whole directories and single assets.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <param name="newParent">The new parent.</param>
	    void Move(List<ContentItem*> items, ContentFolder* newParent);

        /// <summary>
        /// Moves the specified item to the different location. Handles moving whole directories and single assets.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="newParent">The new parent.</param>
	    void Move(ContentItem* item, ContentFolder* newParent);

        /// <summary>
        /// Moves the specified item to the different location. Handles moving whole directories and single assets.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="newPath">The new path.</param>
	    void Move(ContentItem* item, StringView newPath);

        /// <summary>
        /// Copies the specified item to the target location. Handles copying whole directories and single assets.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="targetPath">The target item path.</param>
	    void Copy(ContentItem* item, StringView targetPath);

        /// <summary>
        /// Deletes the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="deletedByUser">If the file was deleted by the user and not outside the editor.</param>
	    void Delete(ContentItem* item, bool deletedByUser = false);

        /// <summary>
        /// Adds the proxy.
        /// </summary>
        /// <param name="proxy">The proxy type.</param>
        /// <param name="rebuild">Should rebuild entire database after addition.</param>
	    void AddProxy(ContentOperate* proxy, bool rebuild = false);

        /// <summary>
        /// Removes the proxy.
        /// </summary>
        /// <param name="proxy">The proxy type.</param>
        /// <param name="rebuild">Should rebuild entire database after removal.</param>
	    void RemoveProxy(ContentOperate* proxy, bool rebuild = false);

        /// <summary>
        /// Rebuilds the whole database (eg. when adding custom item types from plugin).
        /// </summary>
        /// <param name="immediate">True if rebuild now, otherwise will be scheduled for the next editor update (eg. to batch multiple rebuilds within a frame).</param>
	    void Rebuild(bool immediate = false);

        /// <inheritdoc />
        void OnInit() override;

		void OnDirectoryEvent(MainContentTreeNode* node, FileWatcherEvent* event);

        /// <inheritdoc />
	    void OnUpdate() override;

        /// <inheritdoc />
	    void OnExit() override;

	private:

        /// <summary>
        /// Renames a content item
        /// </summary>
        /// <param name="el">Content item</param>
        /// <param name="newPath">New path</param>
        /// <returns>True if failed, otherwise false</returns>
        static bool RenameAsset(ContentItem* el, StringView newPath);

        static void UpdateAssetNewNameTree(ContentItem* el);

        void OnImportFileDone(StringView path);

        void OnAssetTypeInfoChanged(AssetItem* assetItem, AssetInfo &assetInfo);

	    void RebuildInternal();

        void Dispose(ContentItem* item);

        void LoadFolder(ContentTreeNode* node, bool checkSubDirs);

        /*void LoadScripts(ContentTreeNode* parent, String files[]);*/

        void LoadAssets(ContentTreeNode* parent, List<String> &files);

        void LoadProjects(ProjectInfo* project);

    };

} // SE

