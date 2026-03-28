
add_subdirectory(Editors)
add_subdirectory(Element)
add_subdirectory(Values)

EDITOR_ADD(
    LayoutElement.h
    LayoutElementsContainer.h
    LayoutElementsContainer.cpp
    CustomEditor.h
    CustomEditor.cpp
    CustomEditorPresenter.h
    CustomEditorPresenter.cpp
    SyncPointEditor.h
    SyncPointEditor.cpp
    CustomEditorFactory.h
    CustomEditorFactory.cpp
)