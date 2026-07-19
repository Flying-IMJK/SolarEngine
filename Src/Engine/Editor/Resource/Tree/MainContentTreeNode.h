#pragma once
#include "ContentTreeNode.h"

namespace SE
{
	struct FileWatcherEvent;
}

namespace SE::Editor
{
	class ProjectTreeNode;

	/// <summary>
	/// Content tree node used for main directories.
	/// </summary>
	SE_CLASS(Reflect)
	class MainContentTreeNode : public ContentTreeNode
	{
		SE_DEFINE_CLASS_DEFAULT(MainContentTreeNode, ContentTreeNode)
	private:
		 FileSystemWatcher* _watcher;

		void OnEvent(FileWatcherEvent& event);

	public:
		/// <inheritdoc />
		bool GetCanDelete() override { return false; }

		/// <inheritdoc />
		bool GetCanDuplicate() override { return false; }

		/// <summary>
		/// Initializes a new instance of the <see cref="MainContentTreeNode"/> class.
		/// </summary>
		/// <param name="parent">The parent project.</param>
		/// <param name="type">The folder type.</param>
		/// <param name="path">The folder path.</param>
		MainContentTreeNode(ProjectTreeNode* parent, ContentFolderType type, StringView path);


		/// <inheritdoc />
		void OnDestroy() override;

	protected:
		/// <inheritdoc />
		void BeginDragDrop() override
		{
			// No drag for root nodes
		}
	};

} // SE

