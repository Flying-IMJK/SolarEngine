

add_subdirectory(PropertyGrid)

# PARENT_SCOPE only one level
# this command is to pass the variable to parent
EDITOR_ADD(
	UndoStack.h
	UndoStack.cpp

	CommonDialogs.h
	CommonDialogs.cpp
	CategoryTree.h

	EditorEmbeddedResources.inl

	DialogManager.h
	DialogManager.cpp

	QueryFilterHelper.h
	QueryFilterHelper.cpp
)