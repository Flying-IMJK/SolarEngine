#pragma once
#include "ContentTreeNode.h"

namespace SE
{
	class ProjectInfo;
}

namespace SE::Editor
{
	class MainContentTreeNode;

	/// <summary>
	/// Root tree node for the project workspace.
	/// </summary>
	class ProjectTreeNode : public ContentTreeNode
	{
		SE_CLASS_DEFAULT(ProjectTreeNode, ContentTreeNode)
	public:
		/// <summary>
		/// The project/
		/// </summary>
		ProjectInfo* Project;

		/// <summary>
		/// The project content directory.
		/// </summary>
		MainContentTreeNode* Content;

		/// <summary>
		/// The project source code directory.
		/// </summary>
		MainContentTreeNode* Source;

		/// <summary>
		/// Initializes a new instance of the <see cref="ProjectTreeNode"/> class.
		/// </summary>
		/// <param name="project">The project.</param>
		ProjectTreeNode(ProjectInfo* project);

		/// <inheritdoc />
		StringView GetNavButtonLabel() override;

		/// <inheritdoc />
		int Compare(const Control* other) const override;

	protected:
		/// <inheritdoc />
		 void BeginDragDrop() override
		{
			// No drag for root nodes
		}
	};

} // SE

