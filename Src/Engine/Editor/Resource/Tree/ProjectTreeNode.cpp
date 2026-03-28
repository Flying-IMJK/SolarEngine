

#include "ProjectTreeNode.h"

#include "Editor/EditorApp.h"
#include "Editor/Resource/Items/ContentFolder.h"
#include "Runtime/Project/ProjectInfo.h"

namespace SE::Editor
{
	ProjectTreeNode::ProjectTreeNode(ProjectInfo* project) : ContentTreeNode(nullptr, project->ProjectFolderPath)
	{
		Project = project;
		GetFolder()->FileName = GetFolder()->ShortName = Text = project->Name;
	}

	StringView ProjectTreeNode::GetNavButtonLabel()
	{
		return Project->Name;
	}

	int ProjectTreeNode::Compare(const Control* other) const
	{
		// Move the main game project to the top
		if (Project->Name == EditorApp::Ins().Project->Name)
			return -1;
		return ContentTreeNode::Compare(other);
	}
} // SE