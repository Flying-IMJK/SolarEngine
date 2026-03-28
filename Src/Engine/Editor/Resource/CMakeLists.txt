
add_subdirectory(Import)
add_subdirectory(Opreate)
add_subdirectory(Items)
add_subdirectory(Tree)
add_subdirectory(Thumbnails)

# PARENT_SCOPE only one level
# this command is to pass the variable to parent
EDITOR_ADD(
        ContentView.h
        ContentView.cpp
        ContentNavigationButton.h
        ContentNavigationButton.cpp
)